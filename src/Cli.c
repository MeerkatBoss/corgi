#include "Cli.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char* long_name;
  int short_name;
  const char* arg_name;
  const char* help;
} CliOptionDef;

typedef struct {
  int argc;
  char** argv;
  int arg_index;
  CliArgs* result;
} CliParseState;

static const CliOptionDef CliOptions[] = {
  {"tag",     't', "TAG", "Add tag to indexed files (can be used multiple times)"},
  {"source",  's', "DIR", "Source directory (required)"},
  {"target",  'd', "DIR", "Target directory (required)"},
  {"verbose", 'v',  NULL, "Print source and generated target file names"},
  {"force",   'f',  NULL, "Allow overwriting existing files in target directory"},
  {"dry-run",   0,  NULL, "Do not copy files"},
  {"help",    'h',  NULL, "Print this help message"},
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

static int apply_option(int option_idx, char* value, CliArgs* parsed) {
  const CliOptionDef* opt = &CliOptions[option_idx];

  if (!opt->arg_name) {
    /* No argument required */
    assert(value == NULL);
    switch (opt->short_name) {
    case 'v':
      parsed->verbose = 1;
      break;
    case 'f':
      parsed->force = 1;
      break;
    case 'h':
      print_help(parsed->program_name);
      exit(0);
    case 0:
      parsed->dry_run = 1;
      break;
    default:
      fprintf(stderr, "Unknown option '-%c'\n", opt->short_name);
      return -1;
    }
    return 0;
  }

  /* Argument is passed via value */
  assert(value != NULL);
  switch (opt->short_name) {
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
  default:
    fprintf(stderr, "Unknown option '-%c'\n", opt->short_name);
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
