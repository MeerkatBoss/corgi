# M1: GNU Make Targets & musl Static Build

Standard GNU Make targets for packaging and fully static builds with musl.

## M1.1: Standard GNU Make Targets

Add targets: `install`, `uninstall`, `dist`, `distcheck`, `distclean`.
Add variables: `PREFIX` (`/usr/local`), `DESTDIR` (empty), `BINDIR`
(`$(PREFIX)/bin`). All overridable.

Reference: [GNU standard targets](https://www.gnu.org/software/make/manual/html_node/Standard-Targets.html)

## M1.2: musl Static Build

`make all MUSL=1 TARGET=Release` produces a static binary via `musl-gcc`.
Sanitizers disabled for static builds.

## M1.3: CI

CI job that builds with musl and verifies `file ... | grep "statically linked"`.

## Exit Criteria

`make install`, `make dist`, `make distcheck` work.
`make all MUSL=1` produces a static binary. CI verifies.

## Tasks

### M1.1: Make Targets
- [x] Add `prefix`, `DESTDIR`, `bindir` variables
- [x] Implement `install` target (create dir, install binary mode 755)
- [x] Implement `uninstall` target
- [x] Implement `dist` target (source tarball `$(PROJECT)-$(VERSION).tar.gz`)
- [x] Implement `distcheck` target (extract, build, test, cleanup)
- [x] Implement `distclean` target
- [x] Add all new targets to `.PHONY`
- [x] Write integration tests for install/uninstall
- [x] `make check` runs tests per GNU standard; clang-tidy moved to `make tidy`

### M1.2: musl
- [x] Add `MUSL` flag; when set, use `musl-gcc` and `-static`
- [x] Disable sanitizers for musl builds
- [x] Test: static binary produced and runs on minimal system

### M1.3: CI
- [x] Add `build-musl` CI job
- [x] Add `distcheck` CI job
- [x] Upload static binary as CI artifact
