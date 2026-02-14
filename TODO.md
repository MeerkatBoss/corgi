# General
- Keep file tags sorted and unique, without sorting them on each name generation
- Implement standard Make targets (check, dist, distcheck, install, uninstall)
  and support standard variables (DESTDIR, PREFIX)
# Batch execution
- Optionally (if found) use Freedesktop `trash-put` utility for deleting files
- Allow overriding of timestamp, set timestamp on moving/copying file
- Allow selecting between move and copy
# CI
- Check for building with musl instead of glibc
