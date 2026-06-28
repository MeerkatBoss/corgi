# M2: Directory Update Feature

Add only files from source that are newer than the most recent file
in the target directory. Continue index numbering from target.

Three prerequisites land first since M2.1's "per-date max index"
requirement depends on them: CLI control over `override_timestamp`,
persisting it as destination mtime/atime, and per-date (not global)
numbering. See the approved plan for full design rationale.

## Prereq 1: Timestamp override CLI flags

`--set-year`/`-month`/`-day N` (absolute, requires all source files to
share a date) and `--date VALUE` (absolute `YYYY-MM-DD`, same
precondition, or offset tokens like `"+1Y -1M +10D"`, mutually
exclusive with the absolute forms).

- [x] Add `parse_int()` helper in `Cli.c`
- [x] Add `--set-year`/`-month`/`-day` flags (`Cli.h`/`Cli.c`)
- [x] Add `--date` flag: absolute `YYYY-MM-DD` parsing
- [x] Add `--date` flag: offset token parsing (`[+-]<digits><Y|M|D>`,
      case-insensitive, repeatable, additive)
- [x] Reject mixing offset and absolute categories at parse time
- [x] `file_index_dates_match()` in `Files/Index.c`/`.h`
- [x] Same-date precondition check in `Main.c` (after indexing, before
      tagging)
- [x] `TimestampOverride` struct + `file_apply_timestamp_override()` in
      `Files/File.c`/`.h` (avoids `timegm()` with portable
      `days_from_civil()` algorithm)
- [x] Wire override application into `Main.c`
- [x] Tests: `--date` absolute/offset, `--set-*`, mutual exclusion and
      malformed-value error cases
      (`tests/integration/test_timestamp_override.sh`). Same-date-mismatch
      *rejection* and cross-date numbering *reset* are not covered here --
      both need two files with genuinely different `real_timestamp`
      (ctime) values, which no portable,
      non-root tool can fake; covered indirectly by M2.1's
      filename-driven date tests instead.

## Prereq 2: Persist override_timestamp as destination mtime/atime

- [x] `set_destination_timestamp()` in `Transaction.c` (`utime()`)
- [x] Call from `commit_copy_operation()` and `commit_move_operation()`
      (commit phase, not prepare -- avoids the hardlink/rollback hazard)
- [x] Test: destination file mtime/atime match computed override date
      (bracketed via `touch -t` + `find -newer`, portable across
      GNU/BSD/macOS without needing platform-specific `stat` flags)

## Prereq 3: Per-date file numbering

- [x] `file_truncate_to_day()` helper in `Files/File.c`/`.h`
- [x] `file_format_date()` helper (refactor out of `file_generate_name`)
- [x] Replace global counter in `file_transaction_prepare()` with
      per-date reset-on-change counter
- [x] Test: same-date files get sequential numbering (000, 001, ...);
      see note above on why cross-date reset isn't directly testable here

## M2.1: Target Directory Scanning

- [ ] `OrganizedName` struct + `file_parse_organized_name()` in
      `Files/File.c`/`.h`
- [ ] `DateIndexEntry`/`DateIndexTable` (intrusive linked list) in
      `Files/Index.c`/`.h`
- [ ] `file_index_scan_target()`: parse target filenames, track max
      timestamp + per-date max index, skip non-matching filenames
- [ ] Detect filename-date vs. mtime mismatch in target files: report
      error, exclude from accounting, continue scanning
- [ ] `// TODO`: configurable date source of truth (filename vs. mtime)
      and a way to repair drifted target dates -- out of scope for M2
- [ ] Handle target directory missing (`ENOENT`) -> empty table, no error
- [ ] Tests for name parsing and target scanning (including mismatch and
      missing-target cases)

## M2.2: Update Mode

- [ ] `--update`/`-u` CLI flag
- [ ] `file_index_filter_newer_than()` in `Files/Index.c`/`.h`
- [ ] Wire scan + filter into `Main.c` when `--update` is set
- [ ] Seed per-date counter in `file_transaction_prepare()` from the
      scanned `DateIndexTable` (extend its signature/options)
- [ ] Handle edge case: target doesn't exist (copy all)
- [ ] Handle edge case: target has no matching filenames

## M2.3: Integration Tests

- [ ] Test: update empty/missing target -> all copied
- [ ] Test: update with existing older files -> all copied, per-date
      numbering continues from target's max + 1
- [ ] Test: update with newer existing files -> only strictly-newer
      source files copied
- [ ] Test: index numbering continuity (per-date)
- [ ] Test: update with `--dry-run`

## Exit Criteria

`corgi -s src/ -d dst/ --update` copies only newer files and continues
per-date numbering from where target left off. Timestamp override flags
(`--date`, `--set-*`) work and are persisted as destination file
metadata. File numbering is per-date everywhere, not just under
`--update`.
