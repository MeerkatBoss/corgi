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
- [ ] Add `PREFIX`, `DESTDIR`, `BINDIR` variables
- [ ] Implement `install` target (create dir, install binary mode 755)
- [ ] Implement `uninstall` target
- [ ] Implement `dist` target (source tarball `$(PROJECT)-$(VERSION).tar.gz`)
- [ ] Implement `distcheck` target (extract, build, test, cleanup)
- [ ] Implement `distclean` target
- [ ] Add all new targets to `.PHONY`
- [ ] Write integration tests for install/uninstall

### M1.2: musl
- [ ] Add `MUSL` flag; when set, use `musl-gcc` and `-static`
- [ ] Disable sanitizers for musl builds
- [ ] Test: static binary produced and runs on minimal system

### M1.3: CI
- [ ] Add `build-musl` CI job
- [ ] Add `distcheck` CI job
- [ ] Upload static binary as CI artifact
