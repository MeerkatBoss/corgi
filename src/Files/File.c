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
#include "Common/Strings.h"
#include "Files/Error.h"

file_error_t file_init(IndexedFile* file, const char* path) {
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
  if (!list_node_is_null(&file->as_node)) {
    list_take_node(&file->as_node);
  }
  file_clear_tags(file);
  free(file->path);
  file->path = NULL;
}

static int string_compare(const void* lhs, const void* rhs) {
  return strcmp(*(const char* const*) lhs, *(const char* const*) rhs);
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
  char name_buf[]
) {
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
  size_t unique_count = get_unique_tags(file, FILE_MAX_TAGS, tags);

  const char* extension = get_extension(file->path);

  char index_buf[INDEX_BUFSIZE];
  pad_with_zeros(index, INDEX_PADDING, INDEX_BUFSIZE, index_buf);

  /* Catenate all parts of file name */
  unsigned long total_len = 0;

  name_buf[0] = '\0';
  total_len = append_string(name_buf, buf_length, date_buf);
  total_len = append_string(name_buf, buf_length, "_");
  total_len = append_string(name_buf, buf_length, index_buf);

  /* Add tags, separated by underscores */
  for (size_t i = 0; i < unique_count; ++i) {
    total_len = append_string(name_buf, buf_length, "_");
    total_len = append_string(name_buf, buf_length, tags[i]);
  }

  /* Add extension if present */
  if (extension[0] != '\0') {
    total_len = append_string(name_buf, buf_length, ".");
    total_len = append_string(name_buf, buf_length, extension);
  }

  return total_len;
}

file_error_t file_add_tag(IndexedFile* file, const char* tag) {
  for (const char* ch = tag; *ch != '\0'; ++ch) {
    int is_lower_alpha = ('a' <= *ch && *ch <= 'z');
    int is_dash = (*ch == '-');

    if (!is_lower_alpha && !is_dash) {
      return FERR_INVALID_VALUE;
    }
  }

  if (file->tag_count == FILE_MAX_TAGS) {
    return FERR_INVALID_OPERATION;
  }

  file->tags[file->tag_count] = copy_string(tag);
  ++file->tag_count;

  return FERR_NONE;
}

size_t get_unique_tags(
  const IndexedFile* file,
  size_t unique_count,
  const char* unique_tags[]
) {
  const char* tags[FILE_MAX_TAGS];
  for (size_t i = 0; i < file->tag_count; ++i) {
    tags[i] = file->tags[i];
  }
  qsort((void*) tags, file->tag_count, sizeof(tags[0]), string_compare);
  size_t count = 0;
  for (size_t i = 0; i < file->tag_count; ++i) {
    if (i == 0 || strcmp(tags[i], tags[i-1]) != 0) {
      if (unique_tags != NULL && count < unique_count) {
        unique_tags[count] = tags[i];
      }
      count++;
    }
  }
  return count;
}

int file_remove_tag(IndexedFile* file, const char* tag) {
  int removed_count = 0;
  size_t i = 0;
  
  while (i < file->tag_count) {
    if (strcmp(file->tags[i], tag) != 0) {
      i++;
      continue;
    }

    free(file->tags[i]);
    file->tags[i] = file->tags[file->tag_count - 1];
    file->tags[file->tag_count - 1] = NULL;
    file->tag_count--;
    removed_count++;
  }
  
  return removed_count;
}

void file_clear_tags(IndexedFile* file) {
  for (size_t i = 0; i < file->tag_count; ++i) {
    free(file->tags[i]);
    file->tags[i] = NULL;
  }
}
