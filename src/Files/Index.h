/**
 * @file Index.h
 * @author Ivan Solodovnikov (solodovnikov.ia@phystech.edu)
 * @brief File index structure and functions
 * @version 0.1
 * @date 2025-08-01
 * 
 * @copyright Ivan Solodovnikov (c) 2025
 */
#ifndef __FILES_INDEX_H
#define __FILES_INDEX_H

#include "Common/List.h"
#include "Files/Error.h"

/**
 * @brief Description of all files found in source directory
 */
typedef struct {
  LinkedList files; /*!< List of indexed files */
  size_t file_count;
} FileIndex;

/**
 * @brief Initialize empty file index
 */
void file_index_init(FileIndex* index);

/**
 * @brief Remove all files from index
 */
void file_index_clear(FileIndex* index);

/**
 * @brief Add file at `path` to index. Files in index are sorted by `real_timestamp` field.
 *
 * @return FERR_NONE on success,
 *         FERR_INVALID_VALUE if the path is invalid,
 *         FERR_ACCESS_DENIED if the file cannot be accessed
 */
file_error_t file_add_to_index(
  FileIndex* index, /*!< [inout] List of indexed files */
  const char* path  /*!< [in]    Path to indexed file */
);

/**
 * @brief Add all files from directory to index
 *
 * @return FERR_NONE on success,
 *         FERR_INVALID_VALUE if the path is invalid,
 *         FERR_ACCESS_DENIED if directory or its contents cannot be accessed
 */
file_error_t file_index_read_directory(
  FileIndex* index,
  const char* source_path
);

/**
 * @brief Add multiple tags to all files in index
 *
 * @return FERR_NONE on success
 *         FERR_INVALID_VALUE if one of the tags is invalid
 *         FERR_INVALID_OPERATION if one of the files exceeds tag limit
 */
file_error_t file_index_add_tags(
  FileIndex* index,
  size_t tag_count,
  const char* tags[]
);

#endif /* Index.h */