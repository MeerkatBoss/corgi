# General
- [x] Keep file tags sorted and unique, without sorting them on each name
      generation
- [x] Implement standard Make targets (check, dist, distcheck, install,
      uninstall) and support standard variables (DESTDIR, PREFIX)
# Batch execution
- [ ] Optionally (if found) use Freedesktop `trash-put` utility for deleting
      files
- [ ] Allow overriding of timestamp, set timestamp on moving/copying file
- [ ] Use file descriptors for source and destination directories and files
      to avoid TOCTOU issues
- [x] Allow selecting between move and copy
# M2 Prerequisites (Timestamp Override CLI & Application)
- [x] Add `parse_int()` helper in `Cli.c`
- [x] Add `--set-year`/`-month`/`-day` flags (`Cli.h`/`Cli.c`)
- [x] Add `--date` flag: absolute `YYYY-MM-DD` parsing
- [x] Add `--date` flag: offset token parsing
- [x] Reject mixing offset and absolute categories at parse time
- [x] Apply timestamp overrides to indexed files
- [x] Check same-date precondition for absolute overrides
- [x] Persist `override_timestamp` as destination mtime/atime
- [x] Implement per-date numbering
# CI
- [x] Check for building with musl instead of glibc
