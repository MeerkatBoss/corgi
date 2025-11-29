#include <assert.h>
#include <stdio.h>
#include <getopt.h>

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
  default:
    return "unknown error";
  }
}

int main(int argc, char** argv) {
  CliArgs args = {0};
  int parse_result = parse_args(argc, argv, &args);
  if (parse_result != 0) {
    return 1;
  }

  file_error_t result = FERR_NONE;
  FileIndex index;
  FileTransaction transaction;
  int index_initialized = 0;
  int transaction_initialized = 0;

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

  /* Apply tags from CLI to all indexed files */
  LIST_FOREACH(node, index.files) {
    IndexedFile* file = (IndexedFile*) node;

    for (size_t i = 0; i < args.tag_count; ++i) {
      result = file_add_tag(file, args.tags[i]);
      if (result != FERR_NONE) {
        fprintf(stderr, "Warning: Failed to add tag '%s' to file '%s': %s\n",
                args.tags[i], file->path, file_tag_error_to_string(result));
      }
    }

    file->changes.action = FACT_COPY;
  }

  result = file_transaction_init(&transaction, args.target_dir);
  if (result != FERR_NONE) {
    fprintf(stderr, "Error: Failed to initialize transaction for target '%s': %s\n",
            args.target_dir, directory_error_to_string(result));
    goto cleanup;
  }
  transaction_initialized = 1;

  TransactionOptions options = {
    .dry_run = args.dry_run,
    .verbose = args.verbose
  };

  result = file_transaction_prepare(&transaction, &index, &options);
  if (result != FERR_NONE) {
    fprintf(stderr, "Error: Failed to prepare file operations: %s\n",
            directory_error_to_string(result));
    goto cleanup;
  }

  result = file_transaction_commit(&transaction, &options);
  if (result != FERR_NONE) {
    fprintf(stderr, "Error: Failed to commit operations: %s\n",
            directory_error_to_string(result));
    fprintf(stderr, "Rolling back changes...\n");
    file_transaction_rollback(&transaction, &options);
    goto cleanup;
  }

  if (args.verbose || args.dry_run) {
    printf("Successfully processed %zu files.\n", index.file_count);
  }

cleanup:
  if (transaction_initialized) {
    file_transaction_cleanup(&transaction);
  }
  if (index_initialized) {
    file_index_clear(&index);
  }

  return result == FERR_NONE ? 0 : 1;
}
