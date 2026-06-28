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

#include <time.h>

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
 * @brief Add file at `path` to index. Files in index are sorted by
 * `real_timestamp` field.
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
 * @brief Remove and free all files whose `real_timestamp` does not come
 * strictly after `cutoff`
 *
 * @note Used by `--update` to drop source files that are not newer than
 * the most recent file already present in the target directory.
 */
void file_index_filter_newer_than(FileIndex* index, time_t cutoff);

/**
 * @brief Check whether all files in index share the same UTC calendar
 * date (based on `real_timestamp`)
 *
 * @return Nonzero if all files share a date (or index is empty), zero
 *         if at least two files disagree
 */
int file_index_dates_match(const FileIndex* index);

/**
 * @brief Per-date maximum file index, used to continue numbering from an
 * existing target directory under `--update`
 *
 * Entries are kept as an intrusive linked list (per project convention)
 * rather than an array -- the number of distinct dates in a target
 * directory is expected to be small relative to its file count, so
 * linear lookup is fine.
 */
typedef struct {
  LinkedListNode as_node;
  time_t date;            /*!< Midnight UTC of this date bucket */
  unsigned short max_index;
} DateIndexEntry;

typedef struct {
  LinkedList entries;
} DateIndexTable;

void file_date_index_table_init(DateIndexTable* table);
void file_date_index_table_cleanup(DateIndexTable* table);

/**
 * @brief Record `index` as the max seen so far for `date`
 *
 * Adds a new entry if `date` hasn't been seen before, otherwise updates
 * the existing entry if `index` is greater than what's recorded.
 */
void file_date_index_table_bump(
  DateIndexTable* table,
  time_t date,
  unsigned short index
);

/**
 * @brief Look up the recorded max index for `date`
 *
 * @return Nonzero and sets `*out_max_index` if `date` was bumped before,
 *         zero (leaving `*out_max_index` unset) if it was never seen
 *
 * @note A separate found/not-found result is necessary since a
 * legitimately recorded max index of 0 must not be confused with
 * "date never seen".
 */
int file_date_index_table_lookup(
  const DateIndexTable* table,
  time_t date,
  unsigned short* out_max_index
);

/**
 * @brief Scan an already-organized target directory for the most recent
 * file timestamp and per-date max index, for use by `--update`
 *
 * Filenames that don't match the `file_generate_name()` format are
 * skipped. A target file whose filename-encoded date disagrees with its
 * mtime is reported and excluded from both outputs, but does not abort
 * the scan.
 *
 * @return FERR_NONE on success (including when `target_dir` does not
 *         exist, in which case both outputs are left empty/zero),
 *         FERR_ACCESS_DENIED if the directory exists but cannot be read
 */
file_error_t file_index_scan_target(
  const char* target_dir,
  time_t* out_max_timestamp,
  DateIndexTable* out_table
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
