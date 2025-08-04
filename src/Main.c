#include <assert.h>
#include <stdio.h>

#include "Common/List.h"
#include "Files/Error.h"
#include "Files/File.h"
#include "Files/Index.h"

int main() {
  const char* directory = ".tmp";
  const char* tags[] = {"first", "second", "third"};
  file_error_t result = FERR_NONE;
  FileIndex index;
  file_index_init(&index);

  result = file_index_read_directory(&index, directory);
  if (result != FERR_NONE) {
    return result;
  }
  printf("Found %zu files\n", index.file_count);

  enum {
    PATH_MAX = 256
  };
  char buffer[PATH_MAX] = "";
  unsigned short file_id = 0;
  LIST_FOREACH(node, index.files) {
    IndexedFile* file = (IndexedFile*) node;
    for (size_t i = 0; i <= file_id; ++i) {
      file_add_tag(file, tags[i]);
    }
    size_t length = file_generate_name(file, file_id, PATH_MAX, buffer);
    if (length > PATH_MAX) {
      printf("%s...\n", buffer);
    } else {
      puts(buffer);
    }
    ++file_id;
  }
}
