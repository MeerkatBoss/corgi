#include "Cli.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Common/Panic.h"

typedef struct {
  const char* long_name;
  int short_name; /*!< Short option char, 0 if none */
  int id;         /*!< Dispatch id for apply_option(), unique per option */
  const char* arg_name;
  const char* help;
} CliOptionDef;

typedef struct {
  int argc;
  char** argv;
  int arg_index;
  CliArgs* result;
} CliParseState;

enum {
  OPT_ID_DRY_RUN = 256,
  OPT_ID_SET_YEAR,
  OPT_ID_SET_MONTH,
  OPT_ID_SET_DAY,
  OPT_ID_DATE
};

static const CliOptionDef CliOptions[] = {
  {"tag",      't', 't', "TAG",  "Add tag to indexed files (can be used multiple times)"},
  {"source",   's', 's', "DIR",  "Source directory (required)"},
  {"target",   'd', 'd', "DIR",  "Target directory (required)"},
  {"verbose",  'v', 'v',  NULL,  "Print source and generated target file names"},
  {"force",    'f', 'f',  NULL,  "Allow overwriting existing files in target directory"},
  {"dry-run",    0, OPT_ID_DRY_RUN, NULL, "Do not copy files"},
  {"help",     'h', 'h',  NULL,  "Print this help message"},
  {"set-year",   0, OPT_ID_SET_YEAR, "YEAR",
    "Set year of override date (requires same date across source files)"},
  {"set-month",  0, OPT_ID_SET_MONTH, "MONTH",
    "Set month of override date (requires same date across source files)"},
  {"set-day",    0, OPT_ID_SET_DAY, "DAY",
    "Set day of override date (requires same date across source files)"},
  {"date",       0, OPT_ID_DATE, "VALUE",
    "Set override date: absolute 'YYYY-MM-DD' (requires same date across "
    "source files) or offset tokens, e.g. '+1Y -1M +10D' (repeatable)"},
};

enum {
  CLI_OPTION_COUNT = sizeof(CliOptions)/sizeof(*CliOptions)
};

/**
 * Find a long option by prefix match.
 * Returns index into CliOptions on unique match,
 *  -1 if not found, -2 if ambiguous.
 */
static int find_long_option(const char* name, size_t name_len) {
  int match_index = -1;
  int match_count = 0;

  for (size_t i = 0; i < CLI_OPTION_COUNT; ++i) {
    if (strncmp(CliOptions[i].long_name, name, name_len) == 0) {
      if (strlen(CliOptions[i].long_name) == name_len) {
        /* Exact match */
        return (int) i;
      }
      match_index = (int) i;
      ++match_count;
    }
  }

  if (match_count == 1) {
    return match_index;
  }
  if (match_count > 1) {
    return -2;
  }
  return -1;
}

static int find_short_option(char ch) {
  for (size_t i = 0; i < CLI_OPTION_COUNT; ++i) {
    if (CliOptions[i].short_name == ch) {
      return (int) i;
    }
  }
  return -1;
}

static int has_next_arg(const CliParseState* state) {
  return state->arg_index + 1 < state->argc;
}

static char* next_arg(CliParseState* state) {
  if (has_next_arg(state)) {
    ++state->arg_index;
    return state->argv[state->arg_index];
  }
  return NULL;
}

static char* current_arg(const CliParseState* state) {
  if (state->arg_index < state->argc) {
    return state->argv[state->arg_index];
  }
  return NULL;
}

static int is_terminator(const char* arg) {
  return strcmp(arg, "--") == 0;
}

static int is_long_option(const char* arg) {
  return strncmp(arg, "--", 2) == 0 && strlen(arg) > 2;
}

static int is_short_option(const char* arg) {
  return strlen(arg) > 1 && arg[0] == '-' && arg[1] != '-';
}

/**
 * Parse a signed decimal integer from a string.
 *
 * @return 0 on success, -1 if str is not a valid, fully-consumed integer
 */
static int parse_int(const char* str, int* out) {
  if (str == NULL || *str == '\0') {
    return -1;
  }

  char* end = NULL;
  errno = 0;
  long value = strtol(str, &end, 10);
  if (*end != '\0' || errno == ERANGE || value > INT_MAX || value < INT_MIN) {
    return -1;
  }

  *out = (int) value;
  return 0;
}

/**
 * Parse a strict "YYYY-MM-DD" date string.
 *
 * @return 0 on success, -1 on malformed or out-of-range input
 */
static int parse_absolute_date(
  const char* value,
  unsigned* year,
  unsigned* month,
  unsigned* day
) {
  if (strlen(value) != 10 || value[4] != '-' || value[7] != '-') {
    return -1;
  }
  for (size_t i = 0; i < 10; ++i) {
    if (i == 4 || i == 7) {
      continue;
    }
    if (value[i] < '0' || value[i] > '9') {
      return -1;
    }
  }

  *year = (unsigned) (value[0] - '0') * 1000u
        + (unsigned) (value[1] - '0') * 100u
        + (unsigned) (value[2] - '0') * 10u
        + (unsigned) (value[3] - '0');
  *month = (unsigned) (value[5] - '0') * 10u + (unsigned) (value[6] - '0');
  *day = (unsigned) (value[8] - '0') * 10u + (unsigned) (value[9] - '0');

  if (*year < 1 || *month < 1 || *month > 12 || *day < 1 || *day > 31) {
    return -1;
  }
  return 0;
}

/**
 * Parse a single "[+-]<digits><Y|M|D>" offset token, accumulating it
 * additively into the matching component of `override`.
 *
 * @return 0 on success, -1 on malformed token
 */
static int parse_date_offset_token(const char* token, CliDateOverride* override) {
  enum {
    MAX_DIGITS = 9 /*!< Enough for realistic offsets, avoids int overflow */
  };

  size_t len = strlen(token);
  if (len < 3 || len - 2 > MAX_DIGITS) {
    return -1;
  }
  if (token[0] != '+' && token[0] != '-') {
    return -1;
  }
  for (size_t i = 1; i < len - 1; ++i) {
    if (token[i] < '0' || token[i] > '9') {
      return -1;
    }
  }

  int magnitude = 0;
  for (size_t i = 1; i < len - 1; ++i) {
    magnitude = magnitude * 10 + (token[i] - '0');
  }
  int value = (token[0] == '-') ? -magnitude : magnitude;

  switch (token[len - 1]) {
  case 'Y':
  case 'y':
    override->offset_year += value;
    break;
  case 'M':
  case 'm':
    override->offset_month += value;
    break;
  case 'D':
  case 'd':
    override->offset_day += value;
    break;
  default:
    return -1;
  }

  override->has_offset = 1;
  return 0;
}

/**
 * Parse whitespace-separated offset tokens from a (mutable) --date value.
 *
 * @return 0 on success, -1 if any token is malformed
 */
static int parse_date_offset_tokens(char* value, CliDateOverride* override) {
  char* pos = value;

  while (*pos != '\0') {
    while (*pos == ' ' || *pos == '\t') {
      ++pos;
    }
    if (*pos == '\0') {
      break;
    }

    char* token_start = pos;
    while (*pos != '\0' && *pos != ' ' && *pos != '\t') {
      ++pos;
    }
    int had_separator = (*pos != '\0');
    if (had_separator) {
      *pos = '\0';
    }

    int res = parse_date_offset_token(token_start, override);

    if (had_separator) {
      *pos = ' ';
      ++pos;
    }
    if (res != 0) {
      return -1;
    }
  }

  return 0;
}

static int date_override_has_absolute(const CliDateOverride* override) {
  return override->year != 0 || override->month != 0 || override->day != 0;
}

static int apply_date_option(char* value, CliArgs* parsed) {
  CliDateOverride* override = &parsed->date_override;

  if (value[0] == '+' || value[0] == '-') {
    if (date_override_has_absolute(override)) {
      fprintf(stderr,
              "Error: '--date' offset cannot be combined with an absolute "
              "date override.\n");
      return -1;
    }
    if (parse_date_offset_tokens(value, override) != 0) {
      fprintf(stderr, "Error: invalid '--date' offset token in '%s'.\n", value);
      return -1;
    }
    return 0;
  }

  if (override->has_offset) {
    fprintf(stderr,
            "Error: '--date' absolute date cannot be combined with an "
            "offset override.\n");
    return -1;
  }
  unsigned year = 0;
  unsigned month = 0;
  unsigned day = 0;
  if (parse_absolute_date(value, &year, &month, &day) != 0) {
    fprintf(stderr, "Error: invalid date '%s', expected 'YYYY-MM-DD'.\n", value);
    return -1;
  }
  override->year = year;
  override->month = month;
  override->day = day;
  return 0;
}

static int apply_set_component_option(
  int id,
  char* value,
  CliArgs* parsed
) {
  CliDateOverride* override = &parsed->date_override;
  const char* name = NULL;
  unsigned min = 1;
  unsigned max = UINT_MAX;
  unsigned* field = NULL;

  switch (id) {
  case OPT_ID_SET_YEAR:
    name = "set-year";
    field = &override->year;
    break;
  case OPT_ID_SET_MONTH:
    name = "set-month";
    max = 12;
    field = &override->month;
    break;
  case OPT_ID_SET_DAY:
    name = "set-day";
    max = 31;
    field = &override->day;
    break;
  default:
    PANIC("Unreachable: invalid set-component option id");
  }

  if (override->has_offset) {
    fprintf(stderr,
            "Error: '--%s' cannot be combined with a '--date' offset.\n",
            name);
    return -1;
  }

  int parsed_value = 0;
  if (parse_int(value, &parsed_value) != 0
      || parsed_value < (int) min
      || (max != UINT_MAX && parsed_value > (int) max)) {
    fprintf(stderr, "Error: invalid value '%s' for option '--%s'.\n", value, name);
    return -1;
  }

  *field = (unsigned) parsed_value;
  return 0;
}

static int apply_option(int option_idx, char* value, CliArgs* parsed) {
  const CliOptionDef* opt = &CliOptions[option_idx];

  if (!opt->arg_name) {
    /* No argument required */
    assert(value == NULL);
    switch (opt->id) {
    case 'v':
      parsed->verbose = 1;
      break;
    case 'f':
      parsed->force = 1;
      break;
    case 'h':
      print_help(parsed->program_name);
      exit(0);
    case OPT_ID_DRY_RUN:
      parsed->dry_run = 1;
      break;
    default:
      fprintf(stderr, "Unknown option '--%s'\n", opt->long_name);
      return -1;
    }
    return 0;
  }

  /* Argument is passed via value */
  assert(value != NULL);
  switch (opt->id) {
  case 't':
    if (parsed->tag_count < CLI_MAX_TAGS) {
      parsed->tags[parsed->tag_count] = value;
      ++parsed->tag_count;
    } else {
      fprintf(stderr, "Too many tags (max %d)\n", CLI_MAX_TAGS);
      return -1;
    }
    break;
  case 's':
    if (parsed->source_dir) {
      fprintf(stderr, "Source directory can only be specified once\n");
      return -1;
    }
    if (strlen(value) == 0) {
      fprintf(stderr, "Source directory name cannot be empty\n");
      return -1;
    }
    parsed->source_dir = value;
    break;
  case 'd':
    if (parsed->target_dir) {
      fprintf(stderr, "Target directory can only be specified once\n");
      return -1;
    }
    if (strlen(value) == 0) {
      fprintf(stderr, "Target directory name cannot be empty\n");
      return -1;
    }
    parsed->target_dir = value;
    break;
  case OPT_ID_DATE:
    return apply_date_option(value, parsed);
  case OPT_ID_SET_YEAR:
  case OPT_ID_SET_MONTH:
  case OPT_ID_SET_DAY:
    return apply_set_component_option(opt->id, value, parsed);
  default:
    fprintf(stderr, "Unknown option '--%s'\n", opt->long_name);
    return -1;
  }

  return 0;
}

static int parse_long_option(CliParseState* state) {
  char* arg = current_arg(state);
  char* name_start = arg + 2;
  char* eq = strchr(name_start, '=');
  size_t name_len = eq ? (size_t)(eq - name_start) : strlen(name_start);

  int opt_idx = find_long_option(name_start, name_len);
  if (opt_idx == -2) {
    fprintf(stderr, "Ambiguous option '%s'.\n", arg);
    return -1;
  }
  if (opt_idx < 0) {
    fprintf(stderr, "Invalid option '%s'.\n", arg);
    return -1;
  }

  char* value = NULL;
  if (CliOptions[opt_idx].arg_name) {
    if (eq) {
      value = eq + 1;
    } else if (has_next_arg(state)) {
      value = next_arg(state);
    } else {
      fprintf(stderr,
              "Missing required argument '%s' for option '--%s'.\n",
              CliOptions[opt_idx].arg_name,
              CliOptions[opt_idx].long_name);
      return -1;
    }
  }

  int res = apply_option(opt_idx, value, state->result);
  if (res != 0) {
    return res;
  }

  return 0;
}

static int parse_short_options(CliParseState* state) {
  char* arg = current_arg(state);
  char* ch = arg + 1;

  while (*ch != '\0') {
    int opt_idx = find_short_option(*ch);
    if (opt_idx < 0) {
      fprintf(stderr, "Invalid option '-%c'.\n", *ch);
      return -1;
    }

    char* value = NULL;
    if (CliOptions[opt_idx].arg_name) {
      /* This option requires a value */
      if (*(ch + 1) != '\0') {
        value = ch + 1;
      } else if (has_next_arg(state)) {
        value = next_arg(state);
      } else {
        fprintf(stderr, "Missing required argument '%s' for option '-%c'.\n",
                CliOptions[opt_idx].arg_name, *ch);
        return -1;
      }
    }

    int res = apply_option(opt_idx, value, state->result);
    if (res != 0) {
      return res;
    }

    if (value) {
      /* Option handling ends after reading argument */
      break;
    }

    ++ch;
  }

  return 0;
}

void print_help(const char* progname) {
  printf("Usage: %s -s DIR -d DIR [options]\n", progname);
  for (size_t i = 0; i < CLI_OPTION_COUNT; ++i) {
    printf("  ");
    if (CliOptions[i].short_name) {
      printf("-%c", CliOptions[i].short_name);
      if (CliOptions[i].arg_name)
        printf(" %s", CliOptions[i].arg_name);
      printf(", ");
    }
    printf("--%s", CliOptions[i].long_name);
    if (CliOptions[i].arg_name)
      printf("=%s", CliOptions[i].arg_name);
    printf("\t\t%s\n", CliOptions[i].help);
  }
}

int parse_args(int argc, char** argv, CliArgs* parsed) {
  parsed->program_name = argv[0];
  parsed->source_dir = NULL;
  parsed->target_dir = NULL;
  parsed->tag_count = 0;
  parsed->dry_run = 0;
  parsed->verbose = 0;
  parsed->force = 0;
  memset(&parsed->date_override, 0, sizeof(parsed->date_override));

  CliParseState state = {
    .argc = argc,
    .argv = argv,
    .arg_index = 0,
    .result = parsed
  };

  while (has_next_arg(&state)) {
    const char* arg = next_arg(&state);

    if (is_terminator(arg)) {
      break;
    }

    if (is_long_option(arg)) {
      int res = parse_long_option(&state);
      if (res != 0) {
        return res;
      }
      continue;
    }

    if (is_short_option(arg)) {
      int res = parse_short_options(&state);
      if (res != 0) {
        return res;
      }
      continue;
    }

    fprintf(stderr, "Unexpected argument '%s'\n", arg);
    return -1;
  }

  if (!parsed->source_dir || !parsed->target_dir) {
    fprintf(stderr, "Error: --source and --target are required.\n");
    return -1;
  }

  if (has_next_arg(&state)) {
    char* unexpected = next_arg(&state);
    fprintf(stderr, "Unexpected argument '%s'\n", unexpected);
    return -1;
  }

  /* Remove trailing slashes from source */
  size_t source_len = strlen(parsed->source_dir);
  char* source_end = parsed->source_dir + source_len - 1;
  while (*source_end == '/' && source_end > parsed->source_dir) {
    --source_end;
  }
  ++source_end;
  *source_end = '\0';

  /* Remove trailing slashes from target */
  size_t target_len = strlen(parsed->target_dir);
  char* target_end = parsed->target_dir + target_len - 1;
  while (*target_end == '/' && target_end > parsed->target_dir) {
    --target_end;
  }
  ++target_end;
  *target_end = '\0';

  return 0;
}
