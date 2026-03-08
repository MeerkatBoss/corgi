# M7: Release Preparation

Final integration, polish, and release of v1.0.0.

## M7.1: Integration & Regression

Merge all feature branches. Full test suite on all platforms.
`make distcheck`. Build and test musl static binary.

## M7.2: Documentation

README.md, CHANGELOG.md for v1.0.0, Doxygen clean build.

## M7.3: Release Artifacts

Tag `v1.0.0`. GitHub release with source tarball, static binary,
changelog excerpt.

## Exit Criteria

v1.0.0 tagged and released with documentation, static binaries,
and passing CI.

## Tasks

### Integration
- [ ] Merge all feature branches into master
- [ ] Full test suite: Linux + macOS + FreeBSD, gcc + clang
- [ ] `make distcheck`
- [ ] musl static binary build and test
- [ ] Manual smoke test with real photo collection

### Documentation
- [ ] Write README.md (description, install, usage, build, contributing)
- [ ] Update CHANGELOG.md for v1.0.0
- [ ] `make doc` — Doxygen builds without warnings

### Release
- [ ] Bump `VERSION` in Makefile to `1.0.0`
- [ ] Tag `v1.0.0`
- [ ] Create GitHub release (tarball, static binary, release notes)
