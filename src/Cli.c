#include "Cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

typedef struct {
  const char* long_name;
  int short_name;
  const char* arg_name;
  const char* help;
} CliOptionDef;

static const CliOptionDef CliOptions[] = {
  {"tag",     't', "TAG",   "Add tag to indexed files (can be used multiple times)"},
  {"source",  's', "DIR",   "Source directory (required)"},
  {"target",  'd', "DIR",   "Target directory (required)"},
  {"verbose", 'v',  NULL,   "Print source and generated target file names"},
  {"dry-run",   0,  NULL,   "Do not copy files"},
  {"help",    'h',  NULL,   "Print this help message"},
};

enum {
  CLI_OPTION_COUNT = sizeof(CliOptions)/sizeof(*CliOptions)
};

static void get_long_options(struct option* long_options) {
  for (size_t i = 0; i < CLI_OPTION_COUNT; ++i) {
    long_options[i].name = CliOptions[i].long_name;
    long_options[i].has_arg = CliOptions[i].arg_name ? required_argument : no_argument;
    long_options[i].flag = NULL;
    long_options[i].val = CliOptions[i].short_name;
  }
  long_options[CLI_OPTION_COUNT].name = 0;
  long_options[CLI_OPTION_COUNT].has_arg = 0;
  long_options[CLI_OPTION_COUNT].flag = 0;
  long_options[CLI_OPTION_COUNT].val = 0;
}

static size_t get_optstring(char* optstring, size_t maxlen) {
  if (maxlen == 0) {
    return 0;
  }
  maxlen--;

  size_t idx = 0;
  if (idx < maxlen) {
    optstring[idx] = ':';
    ++idx;
  }

  for (size_t i = 0; i < CLI_OPTION_COUNT; ++i) {
    if (CliOptions[i].short_name) {
      if (idx < maxlen) {
        optstring[idx] = (char) CliOptions[i].short_name;
        ++idx;
      }
      if (CliOptions[i].arg_name) {
        if (idx < maxlen) {
          optstring[idx] = ':';
          ++idx;
        }
      }
    }
  }

  optstring[idx] = '\0';
  return idx;
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
  enum {
    MAX_OPTSTRING = CLI_OPTION_COUNT*2 + 2
  };
  int opt = 0;
  int option_index = 0;
  struct option long_options[CLI_OPTION_COUNT + 1];
  char optstring[MAX_OPTSTRING];
  get_long_options(long_options);
  get_optstring(optstring, sizeof(optstring));

  parsed->source_dir = NULL;
  parsed->target_dir = NULL;
  parsed->tag_count = 0;
  parsed->dry_run = 0;
  parsed->verbose = 0;

  while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
    switch (opt) {
    case 't':
      if (parsed->tag_count < CLI_MAX_TAGS) {
        parsed->tags[parsed->tag_count] = optarg;
        ++parsed->tag_count;
      } else {
        fprintf(stderr, "Too many tags (max %d)\n", CLI_MAX_TAGS);
        return -1;
      }
      break;
    case 's':
      parsed->source_dir = optarg;
      break;
    case 'd':
      parsed->target_dir = optarg;
      break;
    case 'v':
      parsed->verbose = 1;
      break;
    case 0:
      parsed->dry_run = 1;
      break;
    case 'h':
      print_help(argv[0]);
      exit(0);
    case ':':
      if (optopt == 0 && long_options[option_index].name != NULL) {
        fprintf(
          stderr,
          "Missing argument for option '--%s'.\n",
          long_options[option_index].name
        );
      } else {
        fprintf(stderr, "Missing argument for option '-%c'.\n", optopt);
      }
      return -1;
    case '?':
      fprintf(stderr, "Invalid option '-%c'.\n", optopt);
      return -1;
    default:
      fprintf(stderr, "?? ERROR: getopt_long returned 0%o ??\n", opt);
      abort();
    }
  }
  if (!parsed->source_dir || !parsed->target_dir) {
    fprintf(stderr, "Error: --source and --target are required.\n");
    return -1;
  }

  if (argv[optind] != NULL) {
    fprintf(stderr, "Unexpected argument '%s'\n", argv[optind]);
    return -1;
  }

  return 0;
}
