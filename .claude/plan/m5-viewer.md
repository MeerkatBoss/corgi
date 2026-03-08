# M5: JPEG Viewer Integration

Display the current file during interactive session.

## M5.1: Viewer Abstraction (`src/Viewer/Viewer.c`)

`viewer_init()`, `viewer_show(path)`, `viewer_close()`.
Backend priority: `feh` → `imv` → `sxiv` → `xdg-open`.

## M5.2: Process Management

Fork+exec child. Track PID. Kill+relaunch on navigation.
Handle `SIGCHLD` for crash detection.

## M5.3: TUI Integration

`viewer_show()` on cursor movement. `--no-viewer` flag to disable.
Status bar message if viewer unavailable.

## M5.4: SDL Embedded Viewer (stretch)

SDL2 window + stb_image.h for JPEG. Commands via pipe.
Fall back to external viewer if unavailable. Defer to post-MVP
if external viewer is sufficient.

## Exit Criteria

During interactive session, current file displayed in viewer.
Navigation updates displayed image.

## Tasks

- [ ] Define `Viewer` struct and interface
- [ ] Implement external viewer backend detection
- [ ] `viewer_show()` — launch/update child process
- [ ] `viewer_close()` — SIGTERM → wait → SIGKILL
- [ ] Handle `SIGCHLD` (viewer crash → set flag)
- [ ] TUI integration: call `viewer_show()` on navigation
- [ ] Add `--no-viewer` flag
- [ ] Test: `--no-viewer` works without viewer binary
- [ ] Test: session works if no viewer found
- [ ] (Stretch) SDL2 + stb_image embedded viewer
