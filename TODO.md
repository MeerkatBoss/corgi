# General
- Get rid of dependency on glibc-specific `getopt_long()`, implement argument parsing properly
- Keep file tags sorted and unique, without sorting them on each name generation
# Batch execution
- Optionally (if found) use Freedesktop `trash-put` utility for deleting files
- Allow overriding of timestamp, set timestamp on moving/copying file