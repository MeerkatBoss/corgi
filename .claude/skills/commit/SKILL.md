---
name: commit
description: Prepare and create a git commit. Updates TODO.md, plan files, and CHANGELOG.md before committing with a Conventional Commits message.
disable-model-invocation: false
---

# Commit Skill

Prepare a clean commit for the current changes. Follow these steps in order.

## 1. Review Changes

Run `git diff --staged` and `git diff` to understand all changes.
If nothing is staged, stage the relevant files with `git add`.
Do not stage unrelated changes.

## 2. Update TODO.md

Check if any items in `TODO.md` were addressed by the changes.
- Mark completed items by removing them or noting they are done
- Add new TODOs discovered during implementation
- Do not modify items unrelated to the current changes

## 3. Update Plan Files

Check which milestone in `.claude/plan/` the changes belong to.
In the relevant `mN-*.md` file, check off completed tasks
(change `- [ ]` to `- [x]`). Only mark tasks that are fully done.

## 4. Update CHANGELOG.md

Add a brief entry under `### [Unreleased]` in the appropriate
subsection (Added, Changed, Fixed, Removed).
Follow Keep a Changelog format. Be concise — one line per change.

## 5. Stage Documentation Updates

```sh
git add TODO.md CHANGELOG.md .claude/plan/
```

Stage only the files that were actually modified.

## 6. Compose Commit Message

Use Conventional Commits format:

```
<type>[optional scope]: <short summary>

[optional body]
```

### Types

- `feat` — new feature or capability
- `fix` — bug fix
- `refactor` — code restructuring without behavior change
- `test` — adding or modifying tests
- `chore` — maintenance, CI, build system, documentation
- `docs` — documentation-only changes

### Scope

Use the module or area name: `cli`, `files`, `transaction`, `index`,
`common`, `makefile`, `ci`, `tests`

### Rules

- Summary line: imperative mood, lowercase, no period, max 72 chars
- Body: wrap at 72 chars, explain *what* and *why* (not *how*)
- Reference issue numbers if applicable

### Examples

```
feat(cli): add --update flag for incremental directory sync
fix(transaction): handle EEXIST race in create_directory
refactor(files): maintain sorted tag invariant on insertion
test(tags): add tests for duplicate tag handling
chore(makefile): add install and dist targets
```

## 7. Create Commit

```sh
git commit
```

Do not use `--no-verify`. Let any hooks run.
