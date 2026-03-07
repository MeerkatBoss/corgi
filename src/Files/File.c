#include "File.h"

#include <assert.h>
#include <errno.h>
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

static size_t pad_with_zeros(unsigned value, unsigned padding, size_t buf_size, char* buf) {
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
    DATE_BUFSIZE = 11, /* YYYY-MM-DD\0 */
    INDEX_BUFSIZE = 6, /* XXXXX\0*/
    INDEX_PADDING = 3
  };
  /* Format timestamp */
  const struct tm* time = gmtime(&file->override_timestamp);
  char date_buf[DATE_BUFSIZE];
  strftime(date_buf, DATE_BUFSIZE, "%Y-%m-%d", time);

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
