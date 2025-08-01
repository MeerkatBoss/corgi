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
 * @brief Add file at `path` to index with given root node
 *
 * @return FileError status
 */
file_error_t file_add_to_index(
  FileIndex* index, /*!< [inout] List of indexed files */
  const char* path  /*!< [in]    Path to indexed file */
);

#endif /* Index.h */