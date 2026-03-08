# M2: Directory Update Feature

Add only files from source that are newer than the most recent file
in the target directory. Continue index numbering from target.

## M2.1: Target Directory Scanning

Implement `file_parse_organized_name()` to extract date, index, tags,
extension from filenames. Scan target to determine max timestamp and
per-date max index.

## M2.2: Update Mode

`--update` / `-u` CLI flag. Skip source files not newer than target.
Respect existing index numbering.

## M2.3: Integration Tests

Test empty target, existing files, index continuity.

## Exit Criteria

`corgi -s src/ -d dst/ --update` copies only newer files and continues
numbering from where target left off.

## Tasks

- [ ] Implement `file_parse_organized_name()` in `Files/File.c`
- [ ] Implement target directory scanning (max timestamp, per-date max index)
- [ ] Tests for name parsing
- [ ] Add `--update` / `-u` CLI flag
- [ ] Filter source index by recency relative to target
- [ ] Set starting index per date from target's max + 1
- [ ] Handle edge case: target doesn't exist (copy all)
- [ ] Handle edge case: target has no matching filenames
- [ ] Test: update empty target → all copied
- [ ] Test: update with existing files → only new copied
- [ ] Test: index numbering continuity
- [ ] Test: update with `--dry-run`
