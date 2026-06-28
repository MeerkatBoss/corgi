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
# CI
- [x] Check for building with musl instead of glibc
