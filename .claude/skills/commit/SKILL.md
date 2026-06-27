---
name: commit
description: Prepare and create one atomic git commit for a completed logical unit of work in the corgi project. Reviews the diff, updates TODO.md, the relevant .claude/plan/ milestone file, and CHANGELOG.md, then commits with a Conventional Commits message and a mandatory Co-authored-by trailer. Use whenever changes are ready to commit, the user asks to commit, or /project:commit is invoked — including at each logical boundary during milestone work.
disable-model-invocation: false
context: fork
model: haiku
allowed-tools: Read, Edit, Glob, Bash(git status), Bash(git diff *), Bash(git add *), Bash(git commit *), Bash(git log *)
---

# Commit Skill

Prepare and create one atomic commit for the current logical unit of work.
An atomic commit captures a single coherent change together with the
documentation updates it implies. Follow these steps in order.

## 1. Review Changes

Run `git status` and `git diff` (plus `git diff --staged` if anything is
already staged) to understand every change in the working tree.
Identify which changes belong to the current logical unit. If unrelated
changes exist, stage only the relevant files and report the rest — never
`git add .` blindly.

## 2. Update TODO.md

Check whether any items in `TODO.md` were addressed by the changes.
- Mark completed items as done (or remove them).
- Add new TODOs discovered during implementation.
- Do not touch items unrelated to the current changes.

## 3. Update Plan Files

Determine which milestone in `.claude/plan/` the changes belong to.
In the relevant `mN-*.md` file, check off completed tasks
(change `- [ ]` to `- [x]`). Only mark tasks that are fully done.

## 4. Update CHANGELOG.md

Add a concise entry under `### [Unreleased]` in the appropriate subsection
(Added, Changed, Fixed, Removed), following Keep a Changelog format.
One line per change.

## 5. Stage the Commit

Stage only the source files and documentation that belong to this logical
unit:

```sh
git add <source files> TODO.md CHANGELOG.md .claude/plan/<milestone file>
```

Stage only files that were actually modified. If unrelated changes remain
in the working tree, leave them unstaged and note them.

## 6. Compose the Commit Message

Use Conventional Commits format:

```
<type>(<scope>): <short summary>

[optional body]

Co-authored-by: Claude <noreply@anthropic.com>
```

### Types

- `feat` — new feature or capability
- `fix` — bug fix
- `refactor` — code restructuring without behavior change
- `test` — adding or modifying tests
- `chore` — maintenance, CI, build system
- `docs` — documentation-only changes
- `style` — formatting or whitespace, no behavior change

### Scope

Use the corgi module or area name: `cli`, `files`, `transaction`, `index`,
`common`, `makefile`, `ci`, `tests`.

### Summary rules

- Imperative mood, lowercase start, no trailing period.
- HARD LIMIT: 50 characters total, including the `type(scope):` prefix.
- Be concise: `feat(files): add tag insertion` not
  `feat(files): add the ability to insert tags into the index`.
- If the summary exceeds 50 chars, shorten it — prefer abbreviation and
  dropping articles over adding a body to compensate.

### Body rules (rare)

- Only when the summary alone cannot convey what changed and why.
- Separate from the summary with a blank line. Wrap at 72 chars.
- Explain *what* and *why*, not *how*. Omit for simple commits.
- Reference issue numbers if applicable.

### Co-authored-by trailer (mandatory)

Every commit MUST end with this trailer, separated from the summary (or
body) by a blank line:

```
Co-authored-by: Claude <noreply@anthropic.com>
```

## 7. Create the Commit

Summary-only (the common case):

```sh
git commit -m "feat(cli): add --update flag for sync

Co-authored-by: Claude <noreply@anthropic.com>"
```

With a body (rare):

```sh
git commit -m "refactor(transaction): use two-phase commit

Stage all filesystem operations before applying any, so a
failure mid-commit leaves the media directory unchanged.

Co-authored-by: Claude <noreply@anthropic.com>"
```

Do not pass `--no-verify`; let any hooks run.

## 8. Verify

Confirm the result with:

```sh
git log --oneline -3
```

## Examples

Summary-only commits (the common case):

```
feat(cli): add --update flag for directory sync
fix(transaction): handle mkdir EEXIST race
refactor(files): keep tags sorted on insert
test(tags): cover duplicate tag handling
chore(makefile): add install and dist targets
```

## Rules

- Never `git add .` blindly — always review `git status` first.
- Stage only the files for the current logical unit; note any unrelated
  changes left behind.
- Never amend or force-push unless explicitly asked.
- If the working tree is clean, report that and do nothing.
- The `Co-authored-by` trailer is mandatory on every commit — never omit it.
- Do not use `--no-verify`.
