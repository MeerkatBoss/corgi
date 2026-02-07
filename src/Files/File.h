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
#include "Files/Error.h"

/**
 * @brief Description of action that should be performed on file
 */
enum FileAction {
  FACT_COPY,    /*!< File should be copied to destination directory */
  FACT_MOVE,    /*!< File should be moved to destination directory */
  FACT_IGNORE,  /*!< File should be skipped */
  FACT_DELETE   /*!< File should be deleted from source directory */
};

typedef enum FileAction file_action_t;

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

  char* path;                 /*!< Full path to file */
  time_t real_timestamp;      /*!< File creation date */
  time_t override_timestamp;  /*!< Timestamp used for file name */

  unsigned tag_count;         /*!< Number of tags added to file */
  char* tags[FILE_MAX_TAGS];  /*!< File tags */

  FileChanges changes;  /*!< Changes to be applied */
} IndexedFile;

/**
 * @brief Initialize file from path
 * 
 * @return FERR_NONE on success
 *         FERR_INVALID_VALUE on invalid path,
 *         FERR_ACCESS_DENIED if path cannot be accessed
 */
file_error_t file_init(IndexedFile* file, const char* path);

/**
 * @brief Deallocate resources used by IndexedFile
 */
void file_cleanup(IndexedFile* file);

/**
 * @brief Validate tag string format
 *
 * @return 0 if tag is invalid, nonzero if it is valid
 *
 * @note Tags can only contain lowercase latin letters and dash symbol '-'
 */
int file_tag_is_valid(const char* tag);

/**
 * @brief Add tag to list of file tags. Duplicate tags are ignored.
 *
 * @return FERR_NONE on success,
 *         FERR_INVALID_VALUE if tag contains invalid characters,
 *         FERR_INVALID_OPERATION if number of tags exceeds FILE_MAX_TAGS
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
size_t file_get_unique_tags(
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

/**
 * @brief Generate new name for file based on timestamp, index and tags
 *
 * @note
 * File name has the following format:
 * `YYYY-MM-DD_XXX_underscore_separated_tags.extension`
 * Tags are sorted alphabetically and duplicate tags are removed.
 *
 * @return Length of generated name. If this length exceeds buffer length,
 *         the name is truncated to fit.
 */
unsigned long file_generate_name(
  const IndexedFile* file,    /*!< [in]  Target file */
  unsigned short file_index,  /*!< [in]  Index of file in list */
  unsigned long buf_length,   /*!< [in]  Length of `name_buf` */
  char* name_buf              /*!< [out] Output buffer for file name */
);

#endif /* File.h */
