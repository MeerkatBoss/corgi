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
 * @brief Calendar date override requested through CLI flags
 *
 * `offset_year`/`offset_month`/`offset_day` are additive (from `--date`
 * offset tokens). `year`/`month`/`day` are absolute replacements (from
 * `--date`'s absolute form or `--set-year`/`-month`/`-day`); `0` means
 * "not given". The offset fields and the absolute fields are mutually
 * exclusive in practice (enforced at parse time), but both are kept on
 * this struct since it is the parse result.
 */
typedef struct {
  int offset_year;   /*!< Additive year offset */
  int offset_month;  /*!< Additive month offset */
  int offset_day;    /*!< Additive day offset */
  unsigned year;      /*!< Absolute year, 0 if not given */
  unsigned month;     /*!< Absolute month, 0 if not given */
  unsigned day;       /*!< Absolute day, 0 if not given */
  int has_offset;     /*!< Set if any offset token was given */
} CliDateOverride;

/**
 * @brief Command-line arguments structure
 */
typedef struct {
  char* program_name;             /*!< Name of the program (argv[0]) */
  char* source_dir;               /*!< Path to source directory */
  char* target_dir;               /*!< Path to target directory */
  const char* tags[CLI_MAX_TAGS]; /*!< Array of tag strings */
  size_t tag_count;               /*!< Number of tags */
  int verbose;                    /*!< Verbose output flag */
  int dry_run;                    /*!< Dry-run mode flag */
  int force;                      /*!< Force overwrite flag */
  CliDateOverride date_override;  /*!< Timestamp override from CLI flags */
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
