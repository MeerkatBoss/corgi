# Commit Granularity

When working on a milestone or any task involving multiple logical changes,
do not accumulate all changes into a single commit. Instead, commit after
each self-contained unit of work by invoking `/project:commit`.

## What constitutes a commit boundary

- A new function or module is implemented and compiles
- A bug fix is complete and tests pass
- A Makefile target is added or modified
- Tests for a feature are written (separate from the feature itself)
- A refactoring is complete and all tests still pass
- A CI configuration change is made

## Workflow

1. Implement one logical change
2. Run `make all` to verify it compiles
3. Run `make test` if tests are affected
4. Invoke `/project:commit` to stage, update docs, and commit
5. Continue to the next logical change

## Rules

- Never combine unrelated changes in a single commit
- Separate refactoring from behavior changes
- Separate test additions from the code they test when practical
- If a change breaks the build or tests, fix it before committing
- When in doubt, prefer smaller commits over larger ones
