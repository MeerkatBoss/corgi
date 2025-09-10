#include "Transaction.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "Common/Strings.h"
#include "Files/Error.h"
#include "Files/File.h"

static file_error_t create_directory(const char* path) {
  if (path == NULL || path[0] == '\0') {
    return FERR_INVALID_VALUE;
  }

  struct stat st;
  int syscall_result = 0;
  syscall_result = stat(path, &st);

  if (syscall_result == 0) {
    if (S_ISDIR(st.st_mode)) {
      return FERR_NONE;
    }
    else {
      return FERR_INVALID_VALUE;
    }
  }
  if (errno == EACCES) {
    return FERR_ACCESS_DENIED;
  }
  if (errno != ENOENT) {
    return FERR_INVALID_VALUE;
  }

  char* path_copy = copy_string(path);
  file_error_t result = FERR_NONE;
  
  char* last_slash = strrchr(path_copy, '/');
  /* Create parent if necessary */
  if (last_slash != NULL && last_slash != path_copy) {
    *last_slash = '\0';
    
    result = create_directory(path_copy);
    if (result != FERR_NONE) {
      goto quit;
    }
    
    *last_slash = '/';
  }

  /* Create target directory */
  syscall_result = mkdir(path, 0755);
  if (syscall_result != 0) {
    if (errno == EEXIST) {
      syscall_result = stat(path, &st);
      if (syscall_result == 0 && S_ISDIR(st.st_mode)) {
        result = FERR_NONE;
      } else {
        result = FERR_INVALID_VALUE;
      }
    } else if (errno == EACCES || errno == EPERM) {
      result = FERR_ACCESS_DENIED;
    } else {
      result = FERR_INVALID_VALUE;
    }
  }

quit:
  free(path_copy);
  return result;
}

file_error_t file_transaction_init(
  FileTransaction* transaction,
  const char* target_dir
) {
  if (transaction == NULL || target_dir == NULL) {
    return FERR_INVALID_VALUE;
  }

  list_init(&transaction->operations);
  transaction->operation_count = 0;
  transaction->target_directory = copy_string(target_dir);

  file_error_t result = create_directory(target_dir);
  if (result != FERR_NONE) {
    free(transaction->target_directory);
    transaction->target_directory = NULL;
    return result;
  }

  return FERR_NONE;
}

static void prepared_operation_cleanup(PreparedOperation* op) {
  if (op == NULL) {
    return;
  }

  free(op->target_path);
  op->target_path = NULL;
  op->source_file = NULL;
  op->state = PREP_STATE_NONE;
}

void file_transaction_cleanup(FileTransaction* transaction) {
  if (transaction == NULL) {
    return;
  }

  LinkedListNode* node = NULL;
  while ((node = list_pop_front(&transaction->operations))) {
    PreparedOperation* op = (PreparedOperation*) node;
    prepared_operation_cleanup(op);
    free(op);
  }

  free(transaction->target_directory);
  transaction->target_directory = NULL;
  transaction->operation_count = 0;
}

static file_error_t copy_file(const char* source_path, const char* dest_path) {
  if (source_path == NULL || dest_path == NULL) {
    return FERR_INVALID_VALUE;
  }

  file_error_t result = FERR_NONE;
  FILE* source = NULL;
  FILE* dest = NULL;

  /* Open source and destination files */
  source = fopen(source_path, "rb");
  if (source == NULL) {
    if (errno == ENOENT) {
      result = FERR_INVALID_VALUE;
      goto quit;
    }
    result = FERR_ACCESS_DENIED;
    goto quit;
  }
  dest = fopen(dest_path, "wb");
  if (dest == NULL) {
    if (errno == ENOENT) {
      result = FERR_INVALID_VALUE;
      goto quit;
    }
    result = FERR_ACCESS_DENIED;
    goto quit;
  }

  /* Copy file contents in chunks */
  enum {
    BUFFER_SIZE = 4096
  };
  char buffer[BUFFER_SIZE];
  size_t bytes_read = 0;

  while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, source)) > 0) {
    if (fwrite(buffer, 1, bytes_read, dest) != bytes_read) {
      result = FERR_ACCESS_DENIED;
      goto quit;
    }
  }

  if (ferror(source) != 0 || ferror(dest) != 0) {
    result = FERR_ACCESS_DENIED;
    goto quit;
  }

quit:
  if (source != NULL) {
    fclose(source);
  }
  
  if (dest != NULL) {
    if (result != FERR_NONE) {
      /* Remove partially created file on error */
      unlink(dest_path);
    }
    fclose(dest);
  }

  return result;
}

static file_error_t link_or_copy_file(
  const char* source_path,
  const char* dest_path,
  int* used_hardlink
) {
  if (source_path == NULL || dest_path == NULL || used_hardlink == NULL) {
    return FERR_INVALID_VALUE;
  }

  *used_hardlink = 0;

  if (link(source_path, dest_path) == 0) {
    *used_hardlink = 1;
    return FERR_NONE;
  }

  if (errno != EXDEV && errno != EPERM) {
    if (errno == ENOENT) {
      return FERR_INVALID_VALUE;
    }
    return FERR_ACCESS_DENIED;
  }

  return copy_file(source_path, dest_path);
}

static file_error_t build_target_path(
  const char* target_dir,
  const char* filename,
  char** target_path
) {
  size_t dir_len = strlen(target_dir);
  size_t name_len = strlen(filename);
  size_t total_len = dir_len + name_len + 2;

  *target_path = calloc(total_len, sizeof(char));
  if (*target_path == NULL) {
    return FERR_ACCESS_DENIED;
  }

  (*target_path)[0] = '\0';
  append_string(*target_path, total_len, target_dir);
  append_string(*target_path, total_len, "/");
  append_string(*target_path, total_len, filename);

  return FERR_NONE;
}

static file_error_t prepare_ignore_operation(
  PreparedOperation* op,
  const IndexedFile* file,
  const TransactionOptions* options
) {
  op->state = PREP_STATE_IGNORE;
  if (options->verbose) {
    printf("  Ignoring: %s\n", file->path);
  }
  return FERR_NONE;
}

static file_error_t prepare_dry_run_operation(
  PreparedOperation* op,
  const IndexedFile* file,
  const TransactionOptions* options
) {
  prepared_operation_state_t state;
  const char* action_name;
  
  switch (file->changes.action) {
  case FACT_IGNORE:
    state = PREP_STATE_IGNORE;
    action_name = "Ignore";
    break;
  case FACT_DELETE:
    state = PREP_STATE_DELETE;
    action_name = "Delete";
    break;
  case FACT_MOVE:
    state = PREP_STATE_MOVE;
    action_name = "Move";
    break;
  case FACT_COPY:
    state = PREP_STATE_COPY;
    action_name = "Copy";
    break;
  default:
    return FERR_INVALID_OPERATION;
  }

  op->state = state;
  
  if (options->verbose) {
    if (state == FACT_COPY || state == FACT_MOVE) {
      printf("  [DRY RUN] %s: %s -> %s\n", action_name, file->path, op->target_path);
    }
    else {
      printf("  [DRY RUN] %s: %s\n", action_name, file->path);
    }
  }
  
  return FERR_NONE;
}

static file_error_t prepare_copy_operation(
  PreparedOperation* op,
  const IndexedFile* file,
  const TransactionOptions* options
) {
  file_error_t result = copy_file(file->path, op->target_path);
  if (result != FERR_NONE) {
    return result;
  }
  
  op->state = PREP_STATE_COPY;
  if (options->verbose) {
    printf("  Prepared copy: %s -> %s\n", file->path, op->target_path);
  }
  
  return FERR_NONE;
}

static file_error_t prepare_move_operation(
  PreparedOperation* op,
  const IndexedFile* file,
  const TransactionOptions* options
) {
  int used_hardlink = 0;
  file_error_t result = link_or_copy_file(file->path, op->target_path, &used_hardlink);
  if (result != FERR_NONE) {
    return result;
  }
  
  op->state = PREP_STATE_MOVE;
  if (options->verbose) {
    const char* method = used_hardlink ? "hardlink" : "copy";
    printf("  Prepared move (%s): %s -> %s\n", method, file->path, op->target_path);
  }
  
  return FERR_NONE;
}

static file_error_t prepare_delete_operation(
  PreparedOperation* op,
  const IndexedFile* file,
  const TransactionOptions* options
) {
  op->state = PREP_STATE_DELETE;
  if (options->verbose) {
    printf("  Prepared delete: %s\n", file->path);
  }
  return FERR_NONE;
}

static file_error_t prepare_single_operation(
  PreparedOperation* op,
  const IndexedFile* file,
  unsigned short file_index,
  const char* target_directory,
  const TransactionOptions* options
) {
  enum {
    FILENAME_BUFSIZE = FILENAME_MAX + 1
  };
  char filename[FILENAME_BUFSIZE];
  file_generate_name(file, file_index, FILENAME_BUFSIZE, filename);

  file_error_t result = build_target_path(target_directory, filename, &op->target_path);
  if (result != FERR_NONE) {
    return result;
  }

  if (options->dry_run) {
    return prepare_dry_run_operation(op, file, options);
  }
  
  switch (file->changes.action) {
  case FACT_IGNORE:
    return prepare_ignore_operation(op, file, options);
  case FACT_COPY:
    return prepare_copy_operation(op, file, options);
  case FACT_MOVE:
    return prepare_move_operation(op, file, options);
  case FACT_DELETE:
    return prepare_delete_operation(op, file, options);
  default:
    return FERR_INVALID_OPERATION;
  }
}

file_error_t file_transaction_prepare(
  FileTransaction* transaction,
  const FileIndex* index,
  const TransactionOptions* options
) {
  if (transaction == NULL || index == NULL || options == NULL) {
    return FERR_INVALID_VALUE;
  }

  if (options->verbose) {
    printf("Preparing %zu operations...\n", index->file_count);
  }

  file_error_t result = FERR_NONE;
  PreparedOperation* op = NULL;
  unsigned short file_index = 0;
  
  /* Process each file in the index */
  LIST_CONST_FOREACH(node, index->files) {
    const IndexedFile* file = (const IndexedFile*) node;
    
    op = calloc(1, sizeof(*op));
    
    list_node_init(&op->as_node);
    op->source_file = file;

    result = prepare_single_operation(
      op, 
      file, 
      file_index, 
      transaction->target_directory, 
      options
    );
    
    if (result != FERR_NONE) {
      if (options->verbose) {
        fprintf(stderr, "  Failed to prepare operation for: %s\n", file->path);
      }
      goto quit;
    }

    /* Add successful operation to transaction */
    list_push_back(&transaction->operations, &op->as_node);
    transaction->operation_count++;
    file_index++;
    op = NULL;
  }

quit:
  if (options->verbose) {
    if (result == FERR_NONE) {
      printf("All operations prepared successfully.\n");
    }
    else {
      fprintf(stderr, "Preparation failed.\n");
    }
  }

  if (result != FERR_NONE && op != NULL) {
    prepared_operation_cleanup(op);
    free(op);
  }
  return result;
}

file_error_t file_transaction_commit(
  FileTransaction* transaction,
  const TransactionOptions* options
) {
  if (transaction == NULL || options == NULL) {
    return FERR_INVALID_VALUE;
  }

  if (options->dry_run) {
    if (options->verbose) {
      printf("Commit phase (dry run) - no actual changes made.\n");
    }
    return FERR_NONE;
  }

  if (options->verbose) {
    printf("Committing %zu operations...\n", transaction->operation_count);
  }

  /* Execute the commit phase for each prepared operation */
  LIST_FOREACH(node, transaction->operations) {
    PreparedOperation* op = (PreparedOperation*) node;
    int unlink_result = 0;

    switch (op->state) {
    case PREP_STATE_MOVE:
      unlink_result = unlink(op->source_file->path);
      if (unlink_result != 0) {
        if (options->verbose) {
          fprintf(stderr, "  Failed to remove source: %s\n", op->source_file->path);
        }
        return FERR_ACCESS_DENIED;
      }
      if (options->verbose) {
        printf("  Committed move: %s\n", op->source_file->path);
      }
      break;

    case PREP_STATE_DELETE:
      unlink_result = unlink(op->source_file->path);
      if (unlink_result != 0) {
        if (errno == ENOENT) {
          if (options->verbose) {
            fprintf(stderr, "  No such file: %s\n", op->source_file->path);
          }
          return FERR_INVALID_VALUE;
        } else {
          if (options->verbose) {
            fprintf(stderr, "  Failed to delete: %s\n", op->source_file->path);
          }
          return FERR_ACCESS_DENIED;
        }
      } else if (options->verbose) {
        printf("  Committed delete: %s\n", op->source_file->path);
      }
      break;

    case PREP_STATE_COPY:
    case PREP_STATE_IGNORE:
      if (options->verbose && op->state == PREP_STATE_COPY) {
        printf(
          "  Nothing to commit for %s\n (%s)",
          op->source_file->path,
          op->state == PREP_STATE_COPY ? "copied" : "ignored"
        );
      }
      break;

    default:
      break;
    }
  }

  if (options->verbose) {
    printf("All operations committed successfully.\n");
  }
  return FERR_NONE;
}

file_error_t file_transaction_rollback(
  FileTransaction* transaction,
  const TransactionOptions* options
) {
  if (transaction == NULL || options == NULL) {
    return FERR_INVALID_VALUE;
  }

  if (options->dry_run) {
    if (options->verbose) {
      printf("Rollback (dry run) - no changes to undo.\n");
    }
    return FERR_NONE;
  }

  if (options->verbose) {
    printf("Rolling back %zu operations...\n", transaction->operation_count);
  }

  file_error_t result = FERR_NONE;
  
  /* Remove target files created during prepare phase */
  LIST_FOREACH(node, transaction->operations) {
    PreparedOperation* op = (PreparedOperation*) node;

    switch (op->state) {
    case PREP_STATE_COPY:
    case PREP_STATE_MOVE:
      if (op->target_path == NULL) {
        /* Nothing to rollback */
        continue;
      }
      int unlink_result = unlink(op->target_path);
      if (unlink_result != 0 && errno != ENOENT) {
        if (options->verbose) {
          fprintf(stderr, "  Failed to remove: %s\n", op->target_path);
        }
        result = FERR_ACCESS_DENIED;
      } else if (options->verbose && unlink_result == 0) {
        printf("  Removed target file: %s\n", op->target_path);
      }
      break;

    case PREP_STATE_DELETE:
    case PREP_STATE_IGNORE:
    default:
      break;
    }
  }

  if (options->verbose) {
    if (result == FERR_NONE) {
      printf("Rollback completed successfully.\n");
    } else {
      printf("Rollback completed with errors.\n");
    }
  }
  return result;
}