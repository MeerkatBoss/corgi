---
paths:
  - "src/**"
---
# Error Message Conventions

## Format

All user-facing errors follow: `"Error: <action> '<path/value>': <reason>"`

Errors must be actionable -- tell the user what's wrong and how to fix it.

## Error-to-String Functions

- `file_error_to_string()` -- generic messages (`Files/Error.c`)
- `directory_error_to_string()` -- directory access errors (`Main.c`)
- `file_tag_error_to_string()` -- tag validation errors (`Main.c`)

Each new module with user-facing errors should define its own
context-specific `*_error_to_string()` helper.

## Examples

```
Error: Failed to read source dir '.tmp/none': path does not exist
Warning: Failed to add tag '2024' to 'photo.jpg': invalid characters
Hint: use --force to allow overwriting of files
```
