/**
 * @file File.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.su)
 *
 * @brief Data structures for indexing and tagging files
 *
 * @version 0.1
 * @date 2024-09-04
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __FILES_FILE_H
#define __FILES_FILE_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "Common/List.h"

/**
 * @brief Maximum number of tags that can be attached to file
 */
#define FILE_MAX_TAGS 8

/**
 * @brief Success status of file operation
 */
enum FileError {
  FERR_NONE,          /*!< No error */
  FERR_INVALID_PATH,  /*!< Invalid path to file */
  FERR_ACCESS_DENIED, /*!< Denied access to file */
  FERR_INVALID_TAG,   /*!< Tag contains invalid characters */
  FERR_TOO_MANY_TAGS  /*!< Number of tags exceeds FILE_MAX_TAGS */
};

typedef int file_error_t;

/**
 * @brief Description of action that should be performed on file
 */
enum FileAction {
  FACT_COPY,    /*!< File should be copied to destination directory */
  FACT_MOVE,    /*!< File should be moved to destination directory */
  FACT_IGNORE,  /*!< File should be skipped */
  FACT_DELETE   /*!< File should be deleted from source directory */
};

typedef int file_action_t;

/**
 * @brief Description of pending changes to file
 */
typedef struct {
  file_action_t action;
} FileChanges;

/**
 * @brief Description of file found in source directory
 */
typedef struct {
  LinkedListNode as_node;

  char* path;       /*!< Full path to file */
  time_t timestamp; /*!< File creation date */

  unsigned tag_count;         /*!< Number of tags added to file */
  char* tags[FILE_MAX_TAGS];  /*!< File tags */

  FileChanges changes;  /*!< Changes to be applied */
} IndexedFile;

/**
 * @brief Description of all files found in source directory
 */
typedef struct {
  LinkedList files; /*!< List of indexed files */

  int override_timestamp;
  time_t override_value;
} FileIndex;

/**
 * @brief Initialize empty file index
 */
void file_index_init(
    FileIndex* index,
    int override_timestamp,
    ... /* time_t override_value */
);

/**
 * @brief Remove all files from index
 */
void file_index_clear(FileIndex* index);

/**
 * @brief Add file at `path` to index with given root node
 *
 * @return FileError status
 */
file_error_t file_add_to_index(
  FileIndex* index, /*!< [inout] List of indexed files */
  const char* path  /*!< [in]    Path to indexed file */
);

/**
 * @brief Generate new name for file based on timestamp and tags
 *
 * @note
 * File name has the following format:
 * `YYYY-MM-DD_underscore_separated_tags.extension`
 * Tags are sorted alphabetically and duplicate tags are removed.
 *
 * @return Length of generated name. If this length exceeds buffer length,
 *         the name is truncated to fit.
 */
unsigned long file_generate_name(
  const IndexedFile* file,    /*!< [in]  Target file */
  unsigned long buf_length,   /*!< [in]  Length of `name_buf` */
  char name_buf[]             /*!< [out] Output buffer for file name */
);

/**
 * @brief Add tag to list of file tags. Duplicate tags are ignored.
 *
 * @note Tags can only contain lowercase latin letters and dash symbol '-'
 */
file_error_t file_add_tag(IndexedFile* file, const char* tag);

/**
 * @brief Get unique sorted tags from file
 * 
 * @return Number of unique tags. If this exceeds `unique_count`, returned list of tags
 *         is truncated to fit in buffer.
 */
size_t get_unique_tags(
  const IndexedFile* file,      /*!< [in]  Target file */
  size_t unique_count,          /*!< [in]  Maximum amount of unique tags to store in buffer */
  const char* unique_tags[]     /*!< [out] Buffer for unique tags. Ignored if `NULL` */
);

/**
 * @brief Remove provided tag from file
 * 
 * @return Number of removed tags
 *
 * @note This operation does not preserve order of tags
 */
int file_remove_tag(IndexedFile* file, const char* tag);

/**
 * @brief Remove all tags from file
 */
void file_clear_tags(IndexedFile* file);

#endif /* File.h */
