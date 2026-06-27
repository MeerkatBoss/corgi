# Global Formatting Rules

These apply to every file in the repository -- source, headers, shell
tests, Makefile, CI YAML, Markdown, commit messages, and PR bodies.
They are not scoped to `src/**`; they hold everywhere.

## Line Length

- Hard limit: 80 characters per line.
- Wrap prose, comments, and code before column 81.
- Break long function calls, conditions, and string literals across
  lines rather than exceeding the limit.

### Exemptions (only these)

- A single unbreakable token that cannot be split, e.g. a long URL.
- YAML frontmatter values that must stay on one line, such as
  `description:` and `tools:` in skills and agents.
- Generated files (`Doxyfile`, `compile_commands.json`).

When an exemption applies, keep the overflowing content alone on its
line; do not also pack breakable text past column 80.

## ASCII Only

- Use only ASCII (bytes 0x00-0x7F). No Unicode characters anywhere:
  not in code, comments, identifiers, strings, Markdown, or commits.
- Replace common typographic characters with ASCII equivalents:
  - em dash -> `--`            en dash -> `-`
  - arrow  -> `->` / `<->`     ellipsis -> `...`
  - curly quotes -> `'` / `"`  non-breaking space -> regular space
- User-facing strings printed by the program are also ASCII only.
- The one tolerated exception is content the program reads at runtime
  (e.g. a media filename supplied by the user); never author such
  bytes into the repository yourself.

## Before Finishing Any Edit

Verify the touched files contain no byte above 0x7F and no line longer
than 80 characters, then proceed.
