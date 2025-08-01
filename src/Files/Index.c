#include "Index.h"

#include <stdlib.h>
#include <unistd.h>

#include "Files/Error.h"
#include "Files/File.h"

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
}

file_error_t file_add_to_index(FileIndex* index, const char* path) {
  /* Initialize IndexedFile */
  IndexedFile* file = (IndexedFile*) calloc(1, sizeof(*file));
  file_error_t res = file_init(file, path);
  if (res != FERR_NONE) {
    free(file);
    return res;
  }

  list_push_back(&index->files, &file->as_node);
  index->file_count++;

  return FERR_NONE;
}
