#include <assert.h>
#include <stdio.h>

#include "Common/List.h"
#include "Files/Error.h"
#include "Files/File.h"
#include "Files/Index.h"
#include "Files/Transaction.h"
#include "Cli.h"

static const char* file_tag_error_to_string(file_error_t error) {
  switch (error) {
  case FERR_NONE:
    return "no error";
  case FERR_INVALID_VALUE:
    return "tag contains invalid characters (only lowercase letters and '-' allowed)";
  case FERR_INVALID_OPERATION:
    return "maximum number of tags exceeded (limit: 8 tags per file)";
  case FERR_ACCESS_DENIED:
  case FERR_ALREADY_EXISTS:
  default:
    return "unknown error";
  }
}

static const char* directory_error_to_string(file_error_t error) {
  switch (error) {
  case FERR_NONE:
    return "no error";
  case FERR_INVALID_VALUE:
    return "target directory is not found";
  case FERR_ACCESS_DENIED:
    return "permission denied";
  case FERR_INVALID_OPERATION:
  case FERR_ALREADY_EXISTS:
  default:
    return "unknown error";
  }
}

static file_error_t execute_operations(
  FileIndex* index,
  const char* target_dir,
  const TransactionOptions* options
) {
  file_error_t result = FERR_NONE;
  FileTransaction transaction;
  int transaction_initialized = 0;

  result = file_transaction_init(&transaction, target_dir, options);
  if (result != FERR_NONE) {
    fprintf(stderr, "Error: Failed to initialize transaction for target '%s': %s\n",
            target_dir, directory_error_to_string(result));
    goto cleanup;
  }
  transaction_initialized = 1;

  result = file_transaction_prepare(&transaction, index, options);
  if (result != FERR_NONE) {
    fprintf(stderr, "Error: Failed to prepare file operations: %s\n",
            file_error_to_string(result));
    if (result == FERR_ALREADY_EXISTS) {
      fprintf(stderr, "Hint: use --force to allow overwriting of files\n");
    }
    fprintf(stderr, "Rolling back changes...\n");
    file_error_t rollback_result = file_transaction_rollback(&transaction, options);
    if (rollback_result != FERR_NONE) {
      fprintf(stderr, "Error: Failed to rollback operations: %s\n",
              file_error_to_string(rollback_result));
    }
    goto cleanup;
  }

  result = file_transaction_commit(&transaction, options);
  if (result != FERR_NONE) {
    fprintf(stderr, "Error: Failed to commit operations: %s\n",
            directory_error_to_string(result));
    fprintf(stderr, "Warning: Cannot safely rollback after commit failure.\n");
    fprintf(stderr, "Target files have been preserved to prevent data loss.\n");
    goto cleanup;
  }

  if (options->verbose || options->dry_run) {
    printf("Successfully processed %zu files.\n", index->file_count);
  }

cleanup:
  if (transaction_initialized) {
    file_transaction_cleanup(&transaction);
  }

  return result;
}

int main(int argc, char** argv) {
  CliArgs args = {0};
  int parse_result = parse_args(argc, argv, &args);
  if (parse_result != 0) {
    print_help(args.program_name);
    return 1;
  }

  file_error_t result = FERR_NONE;
  FileIndex index;
  int index_initialized = 0;

  file_index_init(&index);
  index_initialized = 1;

  result = file_index_read_directory(&index, args.source_dir);
  if (result != FERR_NONE) {
    fprintf(stderr, "Error: Failed to read source directory '%s': %s\n",
            args.source_dir, directory_error_to_string(result));
    goto cleanup;
  }

  if (args.verbose) {
    printf("Found %zu files in '%s'\n", index.file_count, args.source_dir);
  }

  if (index.file_count == 0) {
    fprintf(stderr, "Warning: No files to process.\n");
    goto cleanup;
  }

  result = file_index_add_tags(&index, args.tag_count, args.tags);
  if (result != FERR_NONE) {
    fprintf(stderr, "Error: Failed to add tags to files: %s\n",
            file_tag_error_to_string(result));
    goto cleanup;
  }

  LIST_FOREACH(node, index.files) {
    IndexedFile* file = (IndexedFile*) node;
    file->changes.action = FACT_COPY;
  }

  TransactionOptions options = {
    .dry_run = args.dry_run,
    .verbose = args.verbose,
    .force = args.force
  };

  result = execute_operations(&index, args.target_dir, &options);

cleanup:
  if (index_initialized) {
    file_index_clear(&index);
  }

  return result == FERR_NONE ? 0 : 1;
}
