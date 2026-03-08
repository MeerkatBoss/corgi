# M3: Search by Date and Tags

Search/filter files in an organized directory by date range and/or tags.

## M3.1: Search Implementation

Reuse `file_parse_organized_name()` from M2. Implement filtering by
date range (inclusive) and tag presence (AND semantics).

## M3.2: Search CLI Mode

`--search` flag (mutually exclusive with default mode). Add `--date-from`
and `--date-to` (format: `YYYY-MM-DD`). Reuse `--tag` for filtering.
Output matching filenames to stdout, one per line.

## Exit Criteria

`corgi --search -d dir/ --tag vacation --date-from 2024-01-01` lists
matching files.

## Tasks

- [ ] Implement search filtering (date range, tag presence, combined)
- [ ] Add `--search` flag (mutually exclusive with default mode)
- [ ] Add `--date-from DATE` and `--date-to DATE` options
- [ ] Validate date format
- [ ] `--source` not required in search mode
- [ ] Output matching filenames to stdout
- [ ] Test: search by single tag
- [ ] Test: search by multiple tags (AND)
- [ ] Test: search by date range
- [ ] Test: combined date + tag
- [ ] Test: no matches → empty output, exit 0
- [ ] Test: empty directory
