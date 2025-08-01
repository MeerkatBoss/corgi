#include "File.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "Common/List.h"
#include "Common/Strings.h"

void file_index_init(
    FileIndex* index,
    int override_timestamp,
    ...
) {
  list_init(&index->files);

  if (override_timestamp) {
    va_list args;
    va_start(args, override_timestamp);
    time_t override_value = va_arg(args, time_t);
    va_end(args);

    index->override_timestamp = 1;
    index->override_value = override_value;
  } else {
    index->override_timestamp = 0;
    index->override_value = 0;
  }
}

void file_index_clear(FileIndex* index) {
  LinkedListNode* node = NULL;
  while ((node = list_pop_front(&index->files))) {
    IndexedFile* file = (IndexedFile*) node;

    file_clear_tags(file);
    free(file->path);
    free(file);
  }
}

file_error_t file_add_to_index(FileIndex* index, const char* path) {
  /* Check if file exists and is readable */
  if (access(path, R_OK) != 0) {
    if (errno == ENOENT || errno == ENOTDIR) {
      return FERR_INVALID_PATH;
    }

    return FERR_ACCESS_DENIED;
  }

  time_t timestamp = 0;
  if (index->override_timestamp) {
    /* Override timestamp */
    timestamp = index->override_value;
  } else {
    /* Keep original timestamp */
    struct stat file_stat;
    int res = stat(path, &file_stat);
    if (res != 0) {
      return FERR_ACCESS_DENIED;
    }

    timestamp = file_stat.st_ctime;
  }

  /* Initialize IndexedFile */
  IndexedFile* file = (IndexedFile*) calloc(1, sizeof(*file));
  list_node_init(&file->as_node);
  file->path = copy_string(path);
  file->timestamp = timestamp;
  file->tag_count = 0;
  for (size_t i = 0; i < FILE_MAX_TAGS; ++i) {
    file->tags[i] = NULL;
  }

  /* Add file to index */
  list_push_back(&index->files, &file->as_node);

  return FERR_NONE;
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
  if (has_dot && has_slash && last_dot > last_slash) {
    extension = last_dot + 1;
  } else if (has_dot && !has_slash) {
    extension = last_dot + 1;
  }
  return extension;
}

unsigned long file_generate_name(
  const IndexedFile* file,
  unsigned long buf_length,
  char name_buf[]
) {
  enum {
    DATE_BUFSIZE = 11 /* YYYY-MM-DD\0 */
  };
  /* Format timestamp */
  const struct tm* time = gmtime(&file->timestamp);
  char date_buf[DATE_BUFSIZE];
  strftime(date_buf, DATE_BUFSIZE, "%Y-%m-%d", time);

  /* Get unique sorted tags */
  const char* tags[FILE_MAX_TAGS];
  size_t unique_count = get_unique_tags(file, FILE_MAX_TAGS, tags);

  /* Get file extension */
  const char* extension = get_extension(file->path);

  /* Catenate all parts of file name */
  unsigned long total_len = 0;

  name_buf[0] = '\0';
  total_len = append_string(name_buf, buf_length, date_buf);

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
      return FERR_INVALID_TAG;
    }
  }

  if (file->tag_count == FILE_MAX_TAGS) {
    return FERR_TOO_MANY_TAGS;
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
  qsort(tags, file->tag_count, sizeof(tags[0]), string_compare);
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
