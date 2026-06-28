#include "Index.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include "Files/Error.h"
#include "Files/File.h"
#include "Common/Panic.h"
#include "Common/Strings.h"

void file_index_init(FileIndex* index) {
  PANIC_IF_NULL(index);

  list_init(&index->files);
  index->file_count = 0;
}

void file_index_clear(FileIndex* index) {
  PANIC_IF_NULL(index);

  LinkedListNode* node = NULL;
  while ((node = list_pop_front(&index->files))) {
    IndexedFile* file = (IndexedFile*) node;
    file_cleanup(file);
    free(file);
  }
  index->file_count = 0;
}

file_error_t file_add_to_index(FileIndex* index, const char* path) {
  PANIC_IF_NULL(index);
  PANIC_IF_NULL(path);

  /* Initialize IndexedFile */
  IndexedFile* file = (IndexedFile*) calloc(1, sizeof(*file));
  PANIC_ON_BAD_ALLOC(file);
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

file_error_t file_index_read_directory(
  FileIndex* index, const char* source_path
) {
  PANIC_IF_NULL(index);
  PANIC_IF_NULL(source_path);

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
  PANIC_ON_BAD_ALLOC(full_path);
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

void file_date_index_table_init(DateIndexTable* table) {
  PANIC_IF_NULL(table);

  list_init(&table->entries);
}

void file_date_index_table_cleanup(DateIndexTable* table) {
  if (table == NULL) {
    return;
  }

  LinkedListNode* node = NULL;
  while ((node = list_pop_front(&table->entries))) {
    free((DateIndexEntry*) node);
  }
}

void file_date_index_table_bump(
  DateIndexTable* table,
  time_t date,
  unsigned short index
) {
  PANIC_IF_NULL(table);

  LIST_FOREACH(node, table->entries) {
    DateIndexEntry* entry = (DateIndexEntry*) node;
    if (entry->date == date) {
      if (index > entry->max_index) {
        entry->max_index = index;
      }
      return;
    }
  }

  DateIndexEntry* entry = calloc(1, sizeof(*entry));
  PANIC_ON_BAD_ALLOC(entry);
  list_node_init(&entry->as_node);
  entry->date = date;
  entry->max_index = index;
  list_push_back(&table->entries, &entry->as_node);
}

int file_date_index_table_lookup(
  const DateIndexTable* table,
  time_t date,
  unsigned short* out_max_index
) {
  PANIC_IF_NULL(table);
  PANIC_IF_NULL(out_max_index);

  LIST_CONST_FOREACH(node, table->entries) {
    const DateIndexEntry* entry = (const DateIndexEntry*) node;
    if (entry->date == date) {
      *out_max_index = entry->max_index;
      return 1;
    }
  }
  return 0;
}

static void report_target_date_mismatch(
  const char* path,
  time_t filename_date,
  time_t mtime_date
) {
  char filename_date_buf[FILE_DATE_BUFSIZE];
  char mtime_date_buf[FILE_DATE_BUFSIZE];
  strftime(filename_date_buf, FILE_DATE_BUFSIZE, "%Y-%m-%d",
           gmtime(&filename_date));
  strftime(mtime_date_buf, FILE_DATE_BUFSIZE, "%Y-%m-%d",
           gmtime(&mtime_date));

  /* TODO: let the user choose which source of truth to trust (filename
   * vs. mtime) when they disagree, and offer a way to repair drifted
   * dates in the target directory. Out of scope for M2. */
  fprintf(stderr,
          "Error: target file '%s' date mismatch: filename encodes %s, "
          "mtime is %s\n",
          path, filename_date_buf, mtime_date_buf);
}

file_error_t file_index_scan_target(
  const char* target_dir,
  time_t* out_max_timestamp,
  DateIndexTable* out_table
) {
  PANIC_IF_NULL(target_dir);
  PANIC_IF_NULL(out_max_timestamp);
  PANIC_IF_NULL(out_table);

  *out_max_timestamp = 0;

  enum {
    MAX_FILENAME = 256
  };

  DIR* dir = opendir(target_dir);
  if (!dir) {
    if (errno == ENOENT || errno == ENOTDIR) {
      /* Missing target: nothing to scan, not an error. */
      return FERR_NONE;
    }
    return FERR_ACCESS_DENIED;
  }

  size_t base_length = strlen(target_dir);
  size_t full_length = base_length + MAX_FILENAME + 2;
  char* full_path = calloc(full_length, 1);
  PANIC_ON_BAD_ALLOC(full_path);
  memcpy(full_path, target_dir, base_length);
  full_path[base_length] = '/';
  full_path[base_length + 1] = '\0';

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    OrganizedName parsed;
    if (file_parse_organized_name(entry->d_name, &parsed) != FERR_NONE) {
      continue;
    }

    full_path[base_length + 1] = '\0';
    append_string(full_path, full_length, entry->d_name);

    struct stat st;
    if (stat(full_path, &st) != 0 || !S_ISREG(st.st_mode)) {
      file_organized_name_cleanup(&parsed);
      continue;
    }

    time_t mtime_date = file_truncate_to_day(st.st_mtime);
    if (mtime_date != parsed.date) {
      report_target_date_mismatch(full_path, parsed.date, mtime_date);
      file_organized_name_cleanup(&parsed);
      continue;
    }

    if (st.st_mtime > *out_max_timestamp) {
      *out_max_timestamp = st.st_mtime;
    }
    file_date_index_table_bump(out_table, parsed.date, parsed.index);

    file_organized_name_cleanup(&parsed);
  }

  free(full_path);
  closedir(dir);
  return FERR_NONE;
}

void file_index_filter_newer_than(FileIndex* index, time_t cutoff) {
  PANIC_IF_NULL(index);

  LinkedListNode* node = index->files.root.next;
  while (node != &index->files.root) {
    IndexedFile* file = (IndexedFile*) node;
    LinkedListNode* next = node->next;

    if (file->real_timestamp <= cutoff) {
      list_take_node(node);
      file_cleanup(file);
      free(file);
      --index->file_count;
    }

    node = next;
  }
}

int file_index_dates_match(const FileIndex* index) {
  PANIC_IF_NULL(index);

  int has_first = 0;
  struct tm first_date;
  memset(&first_date, 0, sizeof(first_date));

  LIST_CONST_FOREACH(node, index->files) {
    const IndexedFile* file = (const IndexedFile*) node;
    struct tm date = *gmtime(&file->real_timestamp);

    if (!has_first) {
      first_date = date;
      has_first = 1;
      continue;
    }

    if (date.tm_year != first_date.tm_year
        || date.tm_mon != first_date.tm_mon
        || date.tm_mday != first_date.tm_mday) {
      return 0;
    }
  }

  return 1;
}

file_error_t file_index_add_tags(
  FileIndex* index, size_t tag_count, const char* tags[]
) {
  PANIC_IF_NULL(index);
  PANIC_IF_NULL(tags);

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
