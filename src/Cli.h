/**
 * @file Cli.h
 * @author Ivan Solodovnikov (solodovnikov.ia@phystech.edu)
 * @brief Command-line interface functions
 * @version 0.1
 * @date 2025-08-05
 * 
 * @copyright Ivan Solodovnikov (c) 2025
 */
#ifndef CLI_H
#define CLI_H

#include <stddef.h>

enum {
  CLI_MAX_TAGS = 16 /*!< Maximum amount of tags passed as options */
};

/**
 * @brief Command-line arguments structure
 */
typedef struct {
  char* source_dir;               /*!< Path to source directory */
  char* target_dir;               /*!< Path to target directory */
  const char* tags[CLI_MAX_TAGS]; /*!< Array of tag strings */
  size_t tag_count;               /*!< Number of tags */
  int verbose;                    /*!< Verbose output flag */
  int dry_run;                    /*!< Dry-run mode flag */
  int force;                      /*!< Force overwrite flag */
} CliArgs;

/**
 * @brief Print help message for the program
 */
void print_help(const char* progname);

/**
 * @brief Parse command-line arguments into CliArgs structure
 * 
 * @return 0 on success, non-zero on error
 */
int parse_args(int argc, char** argv, CliArgs* parsed);

#endif /* Cli.h */
