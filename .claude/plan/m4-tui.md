# M4: Interactive Session — Core TUI

Terminal user interface for interactive file organization.

## M4.1: Terminal Handling (`src/Tui/Terminal.c`)

Raw mode via `termios`. Handle `SIGINT`/`SIGTERM` (restore + exit),
`SIGWINCH` (resize). Single-keypress input with escape sequence parsing.

## M4.2: Screen Rendering (`src/Tui/Screen.c`)

Double-buffered screen. Scrollable file list (filename, action icon,
tags). Status bar (mode, progress, default action). Message bar.

## M4.3: Session Logic (`src/Session/Session.c`)

Main loop: render → read key → dispatch. Key bindings: `j`/`k`/arrows
(navigate), `c`/`m`/`i`/`d` (actions), `t`/`T` (add/remove tag),
`v` (select), `A` (select all), `Enter` (finalize), `q` (quit).

## M4.4: Group Selection

Selection mode, extend with movement. Apply action to selection.
Deleted files deselected by default. Select all / select to end.

## M4.5: Session Finalization

Preview deletions (y/n confirm). Execute via `FileTransaction`.
Show summary.

## Exit Criteria

`corgi -s src/ -d dst/ --interactive` opens a TUI for tagging,
marking actions, and batch-processing files.

## Tasks

### Terminal
- [ ] `terminal_enter_raw_mode()` / `terminal_restore_mode()`
- [ ] `atexit` handler for terminal restoration
- [ ] Signal handling: `SIGINT`, `SIGTERM`, `SIGWINCH`
- [ ] `terminal_get_size()` and `terminal_read_key()`
- [ ] Escape sequence parsing (arrow keys, Home, End)

### Rendering
- [ ] Screen buffer with double-buffering
- [ ] Scrollable file list with highlight
- [ ] Status bar and message bar

### Session
- [ ] `Session` struct (index ref, cursor, selection, state)
- [ ] Main loop and key dispatch
- [ ] Navigation, action assignment, tag input sub-mode
- [ ] Group selection (range/bitmask, visual indication)
- [ ] Select all / select to end
- [ ] Deletion preview and confirmation
- [ ] Execute via `FileTransaction`, show summary

### CLI
- [ ] Add `--interactive` / `-I` flag
- [ ] CLI tags applied as defaults (modifiable per-file)

### Tests
- [ ] Scripted keystrokes via stdin pipe
- [ ] Verify actions applied correctly
- [ ] Quit without changes → no files modified
