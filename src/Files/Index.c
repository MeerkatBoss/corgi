#include "Index.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include "Files/Error.h"
#include "Files/File.h"
#include "Common/Strings.h"

void file_index_init(FileIndex* index) {
  list_init(&index->files);
}

void file_index_clear(FileIndex* index) {
  LinkedListNode* node = NULL;
  while ((node = list_pop_front(&index->files))) {
    IndexedFile* file = (IndexedFile*) node;
    file_cleanup(file);
    free(file);
  }
  index->file_count = 0;
}

file_error_t file_add_to_index(FileIndex* index, const char* path) {
  /* Initialize IndexedFile */
  IndexedFile* file = (IndexedFile*) calloc(1, sizeof(*file));
  file_error_t res = file_init(file, path);
  if (res != FERR_NONE) {
    free(file);
    return res;
  }

  /* Insert in sorted order by real_timestamp */
  LinkedListNode* insert_after = &index->files.root;
  LIST_FOREACH(node, index->files) {
    IndexedFile* cur_file = (IndexedFile*) node;
    if (file->real_timestamp < cur_file->real_timestamp) {
      break;
    }
    insert_after = node;
  }
  list_insert_node(insert_after, &file->as_node);
  index->file_count++;

  return FERR_NONE;
}

file_error_t file_index_read_directory(FileIndex* index, const char* source_path) {
  enum {
    MAX_FILENAME = 256
  };

  DIR* dir = opendir(source_path);
  if (!dir) {
    if (errno == ENOENT || errno == ENOTDIR) {
      return FERR_INVALID_VALUE;
    }
    return FERR_ACCESS_DENIED;
  }

  /* Prepare buffer for full path */
  size_t base_length = strlen(source_path);
  size_t full_length = base_length + MAX_FILENAME + 2;
  char* full_path = calloc(full_length, 1);
  memcpy(full_path, source_path, base_length);
  full_path[base_length] = '/';
  full_path[base_length + 1] = '\0';

  struct dirent* entry;
  file_error_t result = FERR_NONE;

  /* Add all regular files from directory */
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    full_path[base_length + 1] = '\0';
    append_string(full_path, full_length, entry->d_name);

    struct stat st;
    if (stat(full_path, &st) != 0) {
      result = FERR_ACCESS_DENIED;
      break;
    }
    if (!S_ISREG(st.st_mode)) {
      continue;
    }

    result = file_add_to_index(index, full_path);
    if (result != FERR_NONE) {
      break;
    }
  }
  free(full_path);
  closedir(dir);

  /* If error occurred, rollback indexing */
  if (result != FERR_NONE) {
    file_index_clear(index);
  }
  return result;
}

int file_index_add_tags(FileIndex* index, size_t tag_count, const char* tags[]) {
  if (tag_count > FILE_MAX_TAGS) {
    return FERR_INVALID_OPERATION;
  }

  for (size_t i = 0; i < tag_count; ++i) {
    if (!file_tag_is_valid(tags[i])) {
      return FERR_INVALID_VALUE;
    }
  }

  LIST_FOREACH(node, index->files) {
    IndexedFile* file = (IndexedFile*) node;

    /* Check that all tags can be added */
    if (file->tag_count + tag_count > FILE_MAX_TAGS) {
      return FERR_INVALID_OPERATION;
    }
  }

  LIST_FOREACH(node, index->files) {
    IndexedFile* file = (IndexedFile*) node;

    for (size_t i = 0; i < tag_count; ++i) {
      /* After all checks, this cannot fail */
      file_add_tag(file, tags[i]);
    }
  }

  return FERR_NONE;
}
