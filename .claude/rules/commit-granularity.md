# Commit Granularity

When working on a milestone or any task with multiple logical changes,
do not accumulate everything into one commit. Commit after each
self-contained unit of work by delegating to the `commit` agent
(`.claude/agents/commit.md`).

## What constitutes a commit boundary

- A new function or module is implemented and compiles
- A bug fix is complete and tests pass
- A Makefile target is added or modified
- Tests for a feature are written (separate from the feature itself)
- A refactoring is complete and all tests still pass
- A CI configuration change is made

## Workflow at each boundary

1. Implement one logical change.
2. Run `make all` to verify it compiles.
3. Run `make test` if tests are affected.
4. Check off the matching task in the relevant `docs/plan/mN-*.md`
   file (edit it directly; plan files live outside `.claude/`).
5. Delegate to the `commit` agent. It updates `TODO.md` and, only for
   user-visible changes, `CHANGELOG.md`, then stages and commits.
6. Continue to the next logical change.

## Division of responsibility

- The main session edits source, tests, and `docs/plan/` files.
- The `commit` agent is the only context that writes `TODO.md` and
  `CHANGELOG.md`, and the only one that runs `git commit`.

## Rules

- Never combine unrelated changes in a single commit.
- Separate refactoring from behavior changes.
- Separate test additions from the code they test when practical.
- If a change breaks the build or tests, fix it before committing.
- When in doubt, prefer smaller commits over larger ones.
