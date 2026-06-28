#include "File.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "Common/List.h"
#include "Common/Panic.h"
#include "Common/Strings.h"
#include "Files/Error.h"

file_error_t file_init(IndexedFile* file, const char* path) {
  PANIC_IF_NULL(file);
  PANIC_IF_NULL(path);

  /* Check if file exists and is readable */
  if (access(path, R_OK) != 0) {
    if (errno == ENOENT || errno == ENOTDIR) {
      return FERR_INVALID_VALUE;
    }

    return FERR_ACCESS_DENIED;
  }

  /* Get file timestamp */
  struct stat file_stat;
  int res = stat(path, &file_stat);
  if (res != 0) {
    return FERR_ACCESS_DENIED;
  }
  file->real_timestamp = file_stat.st_ctime;
  file->override_timestamp = file->real_timestamp;
  file->path = copy_string(path);
  file->tag_count = 0;
  for (size_t i = 0; i < FILE_MAX_TAGS; ++i) {
    file->tags[i] = NULL;
  }
  list_node_init(&file->as_node);

  return FERR_NONE;
}

void file_cleanup(IndexedFile* file) {
  PANIC_IF_NULL(file);

  if (!list_node_is_null(&file->as_node)) {
    list_take_node(&file->as_node);
  }
  file_clear_tags(file);
  free(file->path);
  file->path = NULL;
}

/**
 * Floor division/modulo (round toward negative infinity), needed because
 * '/' and '%' truncate toward zero in C and would mishandle negative
 * month offsets.
 */
static long long floor_div(long long a, long long b) {
  long long quotient = a / b;
  long long remainder = a % b;
  if (remainder != 0 && ((remainder < 0) != (b < 0))) {
    quotient -= 1;
  }
  return quotient;
}

static long long floor_mod(long long a, long long b) {
  long long remainder = a % b;
  if (remainder != 0 && ((remainder < 0) != (b < 0))) {
    remainder += b;
  }
  return remainder;
}

/**
 * Days since 1970-01-01 for civil date (year, month, day), where `month`
 * must be in [1, 12] but `day` may be any integer (including out of the
 * normal 1-31 range) -- overflow/underflow in `day` carries into the
 * result correctly. Howard Hinnant's well-known "days_from_civil"
 * algorithm, extended to `long long` to avoid the range limits of
 * `time_t`-sized arithmetic on platforms with 32-bit `time_t`.
 */
static long long days_from_civil(long long year, int month, long long day) {
  long long y = year - (month <= 2 ? 1 : 0);
  long long era = (y >= 0 ? y : y - 399) / 400;
  long long year_of_era = y - era * 400;
  long long day_of_year =
    (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  long long day_of_era =
    year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
  return era * 146097 + day_of_era - 719468;
}

/**
 * Converts a possibly out-of-range (year, 0-based month, day) calendar
 * date plus time-of-day into a UTC `time_t`. Avoids `timegm()`, which is
 * not part of ISO C and is gated behind extension feature-test macros on
 * some platforms.
 */
static time_t compute_utc_timestamp(
  long long year,
  long long month0,
  long long day,
  int hour,
  int minute,
  int second
) {
  long long total_months = year * 12 + month0;
  long long normalized_year = floor_div(total_months, 12);
  int normalized_month = (int) floor_mod(total_months, 12) + 1;

  long long days = days_from_civil(normalized_year, normalized_month, day);
  return (time_t) (
    days * 86400LL + hour * 3600LL + minute * 60LL + second
  );
}

void file_apply_timestamp_override(
  IndexedFile* file,
  const TimestampOverride* override
) {
  PANIC_IF_NULL(file);
  PANIC_IF_NULL(override);

  struct tm time = *gmtime(&file->real_timestamp);

  long long year = 1900LL + time.tm_year + override->offset_year;
  long long month0 = time.tm_mon + override->offset_month;
  long long day = time.tm_mday + override->offset_day;

  if (override->year != 0) {
    year = (long long) override->year;
  }
  if (override->month != 0) {
    month0 = (long long) override->month - 1;
  }
  if (override->day != 0) {
    day = (long long) override->day;
  }

  file->override_timestamp = compute_utc_timestamp(
    year, month0, day, time.tm_hour, time.tm_min, time.tm_sec
  );
}

void file_format_date(const IndexedFile* file, char* buf, size_t buf_size) {
  PANIC_IF_NULL(file);
  PANIC_IF_NULL(buf);

  const struct tm* time = gmtime(&file->override_timestamp);
  strftime(buf, buf_size, "%Y-%m-%d", time);
}

time_t file_truncate_to_day(time_t timestamp) {
  struct tm time = *gmtime(&timestamp);
  return compute_utc_timestamp(
    1900LL + time.tm_year, time.tm_mon, time.tm_mday, 0, 0, 0
  );
}

static const char* get_extension(const char* path) {
  const char* last_dot = strrchr(path, '.');
  const char* last_slash = strrchr(path, '/');
  int has_dot = (last_dot != NULL);
  int has_slash = (last_slash != NULL);
  const char* extension = "";
  /*
   * RANT INCOMING 
   *
   * This snippet would be somewhat easier to write if not for the fact that
   * an obvious `NULL < ptr` for any non-null `ptr` isn't true under any of
   * C standards and instead invokes undefined behavior, which means that both
   * pointers have to be checked for equality with NULL before comparison.
   */
  if (has_dot) {
    if (!has_slash || last_dot > last_slash) {
      extension = last_dot + 1;
    }
  }
  return extension;
}

static size_t pad_with_zeros(
  unsigned value, unsigned padding, size_t buf_size, char* buf
) {
  unsigned value_copy = value;
  unsigned length = 0;
  while (value_copy > 0) {
    length++;
    value_copy /= 10;
  }
  if (length == 0) {
    length = 1;
  }
  if (length < padding) {
    length = padding;
  }

  if (length >= buf_size) {
    return 0;
  }

  buf[length] = '\0';
  for (size_t i = length; i > 0; --i) {
    buf[i - 1] = (char) ('0' + value % 10);
    value /= 10;
  }

  return length;
}

unsigned long file_generate_name(
  const IndexedFile* file,
  unsigned short index,
  unsigned long buf_length,
  char* name_buf
) {
  PANIC_IF_NULL(file);
  PANIC_IF_NULL(name_buf);

  enum {
    INDEX_BUFSIZE = 6, /* XXXXX\0*/
    INDEX_PADDING = 3
  };
  char date_buf[FILE_DATE_BUFSIZE];
  file_format_date(file, date_buf, FILE_DATE_BUFSIZE);

  const char* tags[FILE_MAX_TAGS];
  size_t unique_count = file_get_unique_tags(file, FILE_MAX_TAGS, tags);

  const char* extension = get_extension(file->path);

  char index_buf[INDEX_BUFSIZE];
  pad_with_zeros(index, INDEX_PADDING, INDEX_BUFSIZE, index_buf);

  /* Catenate all parts of file name */
  unsigned long total_len = 0;

  name_buf[0] = '\0';
  append_string(name_buf, buf_length, date_buf);
  append_string(name_buf, buf_length, "_");
  total_len = append_string(name_buf, buf_length, index_buf);

  /* Add tags, separated by underscores */
  for (size_t i = 0; i < unique_count; ++i) {
    append_string(name_buf, buf_length, "_");
    total_len = append_string(name_buf, buf_length, tags[i]);
  }

  /* Add extension if present */
  if (extension[0] != '\0') {
    append_string(name_buf, buf_length, ".");
    total_len = append_string(name_buf, buf_length, extension);
  }

  return total_len;
}

int file_tag_is_valid(const char* tag) {
  PANIC_IF_NULL(tag);

  for (const char* ch = tag; *ch != '\0'; ++ch) {
    int is_lower_alpha = ('a' <= *ch && *ch <= 'z');
    int is_dash = (*ch == '-');

    if (!is_lower_alpha && !is_dash) {
      return 0;
    }
  }

  return 1;
}

file_error_t file_add_tag(IndexedFile* file, const char* tag) {
  PANIC_IF_NULL(file);
  PANIC_IF_NULL(tag);

  if (!file_tag_is_valid(tag)) {
    return FERR_INVALID_VALUE;
  }

  /* Find insertion position in sorted array; detect duplicates */
  size_t insert_pos = 0;
  for (; insert_pos < file->tag_count; insert_pos++) {
    int cmp = strcmp(file->tags[insert_pos], tag);
    if (cmp == 0) {
      return FERR_NONE; /* Duplicate — silent no-op */
    }
    if (cmp > 0) {
      break;
    }
  }

  if (file->tag_count == FILE_MAX_TAGS) {
    return FERR_INVALID_OPERATION;
  }

  /* Shift elements right to make room at insert_pos */
  for (size_t i = file->tag_count; i > insert_pos; i--) {
    file->tags[i] = file->tags[i - 1];
  }
  file->tags[insert_pos] = copy_string(tag);
  ++file->tag_count;

  return FERR_NONE;
}

size_t file_get_unique_tags(
  const IndexedFile* file,
  size_t unique_count,
  const char* unique_tags[]
) {
  PANIC_IF_NULL(file);
  PANIC_IF_NULL(unique_tags);

  size_t count =
    file->tag_count < unique_count 
      ? file->tag_count
      : unique_count;
  for (size_t i = 0; i < count; i++) {
    unique_tags[i] = file->tags[i];
  }
  return count;
}

int file_remove_tag(IndexedFile* file, const char* tag) {
  PANIC_IF_NULL(file);
  PANIC_IF_NULL(tag);

  /* Tags are sorted and deduplicated, so at most one match exists */
  for (size_t i = 0; i < file->tag_count; i++) {
    if (strcmp(file->tags[i], tag) != 0) {
      continue;
    }

    free(file->tags[i]);
    /* Shift remaining elements left to preserve sorted order */
    for (size_t j = i; j < file->tag_count - 1; j++) {
      file->tags[j] = file->tags[j + 1];
    }
    file->tags[file->tag_count - 1] = NULL;
    file->tag_count--;
    return 1;
  }

  return 0;
}

void file_clear_tags(IndexedFile* file) {
  PANIC_IF_NULL(file);

  for (size_t i = 0; i < file->tag_count; ++i) {
    free(file->tags[i]);
    file->tags[i] = NULL;
  }
  file->tag_count = 0;
}

/**
 * Cleans up `out->tags[0..count)` on a parse failure partway through.
 */
static void cleanup_partial_tags(OrganizedName* out, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    free(out->tags[i]);
    out->tags[i] = NULL;
  }
}

file_error_t file_parse_organized_name(
  const char* filename,
  OrganizedName* out
) {
  PANIC_IF_NULL(filename);
  PANIC_IF_NULL(out);

  out->date = 0;
  out->index = 0;
  out->tag_count = 0;
  out->extension = NULL;
  for (size_t i = 0; i < FILE_MAX_TAGS; ++i) {
    out->tags[i] = NULL;
  }

  enum {
    DATE_PREFIX_LEN = 11 /* "YYYY-MM-DD_" */
  };

  if (strlen(filename) <= DATE_PREFIX_LEN
      || filename[4] != '-' || filename[7] != '-'
      || filename[10] != '_') {
    return FERR_INVALID_VALUE;
  }
  for (size_t i = 0; i < 10; ++i) {
    if (i == 4 || i == 7) {
      continue;
    }
    if (filename[i] < '0' || filename[i] > '9') {
      return FERR_INVALID_VALUE;
    }
  }

  unsigned year = (unsigned) (filename[0] - '0') * 1000u
                + (unsigned) (filename[1] - '0') * 100u
                + (unsigned) (filename[2] - '0') * 10u
                + (unsigned) (filename[3] - '0');
  unsigned month = (unsigned) (filename[5] - '0') * 10u
                 + (unsigned) (filename[6] - '0');
  unsigned day = (unsigned) (filename[8] - '0') * 10u
               + (unsigned) (filename[9] - '0');
  if (year < 1 || month < 1 || month > 12 || day < 1 || day > 31) {
    return FERR_INVALID_VALUE;
  }

  /* Work on a mutable copy of "XXX_tag1_tag2.ext" so it can be split
   * in place with '\0' separators. */
  char* work = copy_string(filename + DATE_PREFIX_LEN);
  PANIC_ON_BAD_ALLOC(work);

  char* extension = NULL;
  char* last_dot = strrchr(work, '.');
  if (last_dot != NULL) {
    *last_dot = '\0';
    extension = last_dot + 1;
  }

  char* index_part = work;
  char* underscore = strchr(work, '_');
  if (underscore != NULL) {
    *underscore = '\0';
  }

  if (index_part[0] == '\0') {
    free(work);
    return FERR_INVALID_VALUE;
  }
  for (const char* ch = index_part; *ch != '\0'; ++ch) {
    if (*ch < '0' || *ch > '9') {
      free(work);
      return FERR_INVALID_VALUE;
    }
  }
  unsigned long index_value = strtoul(index_part, NULL, 10);
  if (index_value > USHRT_MAX) {
    free(work);
    return FERR_INVALID_VALUE;
  }

  size_t tag_count = 0;
  char* cursor = (underscore != NULL) ? underscore + 1 : NULL;
  while (cursor != NULL) {
    char* next = strchr(cursor, '_');
    if (next != NULL) {
      *next = '\0';
    }

    if (*cursor == '\0' || !file_tag_is_valid(cursor)
        || tag_count >= FILE_MAX_TAGS) {
      cleanup_partial_tags(out, tag_count);
      free(work);
      return FERR_INVALID_VALUE;
    }
    out->tags[tag_count] = copy_string(cursor);
    ++tag_count;

    cursor = (next != NULL) ? next + 1 : NULL;
  }

  out->date = compute_utc_timestamp(
    (long long) year, (long long) month - 1, (long long) day, 0, 0, 0
  );
  out->index = (unsigned short) index_value;
  out->tag_count = tag_count;
  out->extension = copy_string(extension != NULL ? extension : "");
  free(work);

  return FERR_NONE;
}

void file_organized_name_cleanup(OrganizedName* name) {
  PANIC_IF_NULL(name);

  cleanup_partial_tags(name, name->tag_count);
  name->tag_count = 0;
  free(name->extension);
  name->extension = NULL;
}
