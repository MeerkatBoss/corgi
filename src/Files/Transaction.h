/**
 * @file Transaction.h
 * @author Ivan Solodovnikov (solodovnikov.ia@phystech.edu)
 * @brief File operation functions with two-phase commit for safe operations
 * @version 0.1
 * @date 2025-09-10
 * 
 * @copyright Ivan Solodovnikov (c) 2025
 */
#ifndef __FILES_TRANSACTION_H
#define __FILES_TRANSACTION_H

#include "Files/Error.h"
#include "Files/File.h"
#include "Files/Index.h"
#include "Common/List.h"

/**
 * @brief Options for file operation execution
 */
typedef struct {
  int dry_run;    /*!< If true, operations are simulated without actual file changes */
  int verbose;    /*!< If true, print detailed operation information */
} TransactionOptions;

/**
 * @brief State of a prepared operation
 */
enum PreparedOperationState {
  PREP_STATE_NONE,      /*!< No operation prepared */
  PREP_STATE_COPY,      /*!< File copied, ready for commit */
  PREP_STATE_MOVE,      /*!< File copied for move, source pending deletion */
  PREP_STATE_DELETE,    /*!< File marked for deletion */
  PREP_STATE_IGNORE     /*!< Operation ignored */
};

typedef int prepared_operation_state_t;

/**
 * @brief Prepared operation for a single file
 */
typedef struct {
  LinkedListNode as_node;
  
  const IndexedFile* source_file;       /*!< Reference to source file */
  char* target_path;                    /*!< Target file path (allocated) */
  prepared_operation_state_t state;     /*!< Current state of operation */
} PreparedOperation;

/**
 * @brief Transaction context for two-phase operations
 */
typedef struct {
  LinkedList operations;  /*!< List of prepared operations */
  char* target_directory; /*!< Target directory path (allocated) */
  size_t operation_count; /*!< Number of operations */
} FileTransaction;

/**
 * @brief Initialize operation transaction
 *
 * @return FERR_NONE on success,
 *         FERR_INVALID_VALUE if target_dir is invalid,
 *         FERR_ACCESS_DENIED if directory cannot be accessed
 */
file_error_t file_transaction_init(
  FileTransaction* transaction,
  const char* target_dir
);

/**
 * @brief Clean up transaction and free resources
 *
 * @param transaction Transaction to cleanup
 */
void file_transaction_cleanup(FileTransaction* transaction);

/**
 * @brief Prepare transaction for files in index
 *
 * @return FERR_NONE on success
 *         error code on failure (no operations committed)
 */
file_error_t file_transaction_prepare(
  FileTransaction* transaction,
  const FileIndex* index,
  const TransactionOptions* options
);

/**
 * @brief Commit all prepared operations
 *
 * @return FERR_NONE on success,
 *         error code on failure (partial commit may have occurred)
 */
file_error_t file_transaction_commit(
  FileTransaction* transaction,
  const TransactionOptions* options
);

/**
 * @brief Rollback all prepared operations
 *
 * @return FERR_NONE on success,
 *         error code if rollback failed (filesystem may be inconsistent but no files are lost)
 */
file_error_t file_transaction_rollback(
  FileTransaction* transaction,
  const TransactionOptions* options
);

#endif /* Transaction.h */
