---
name: next-milestone
description: Select, plan, and start the next development milestone for corgi. Reads .claude/plan/overview.md to choose the next eligible milestone (dependencies merged, not yet done), reviews and refines its task checklist, then creates the milestone branch before work begins. Use whenever starting a new milestone, finishing one and moving on, or when the user asks what to work on next or to "start"/"plan" the next milestone.
disable-model-invocation: false
allowed-tools: Read, Edit, Glob, Bash(git status), Bash(git branch:*), Bash(git switch:*), Bash(git pull:*), Bash(git push:*), Bash(git log:*), Bash(make check), Bash(gh pr:*)
---

# Next Milestone Skill

Plan and start the next development milestone. Each milestone (M1-M7, see
`.claude/plan/`) is developed on its own branch and merged through a single
pull request. Invoke this skill at the start of a milestone: it selects the
next eligible milestone, refines its plan, and sets up its branch before any
code is written.

Run this in the main session, not a forked context -- the branch it creates
and the milestone it selects are the working state the session continues
with.

## 1. Select the Next Milestone

Read `.claude/plan/overview.md` for the milestone table, dependency map, and
current state. Decide which milestone comes next:

- A milestone is eligible when every milestone it depends on is already
  merged into `master`.
- Skip milestones that are already done -- merged, or fully checked off in
  their `mN-*.md` file.
- Among eligible milestones, prefer the lowest number unless the user asks
  for a specific one.
- Confirm the chosen milestone with the user before creating a branch.

Inspect what is already merged before deciding:

```sh
git switch master
git pull --ff-only
git log --oneline --decorate -15
git branch --merged master
```

## 2. Review and Refine the Plan

Open the chosen milestone's plan file (`.claude/plan/mN-*.md`) and read its
scope, task checklist, and exit criteria.

- If tasks are vague or missing, expand the checklist into concrete,
  commit-sized items. Each item should map to roughly one `/project:commit`.
- Leave the exit criteria unchanged unless the user agrees to revise them.
- Edit the plan file directly to record any refinements.

This refinement is the "planning" half of the skill: the session should not
leave this step until the milestone has an actionable, ordered task list.

## 3. Create the Milestone Branch

Branch off `master` (see Base Branch below):

```sh
git switch -c milestone/mN-<slug>
```

- One branch per milestone. Never commit milestone work to `master`.
- The branch name's `mN-<slug>` matches the plan file
  (e.g. `.claude/plan/m2-update.md` -> `milestone/m2-update`).
- If the milestone depends on another that is not yet merged, branch off
  that dependency's branch instead of `master`.

## 4. Hand Off to Milestone Work

Development now proceeds on the branch. Work in small, self-contained units
and commit at each logical boundary by invoking `/project:commit` (see
`.claude/rules/commit-granularity.md`). Check off tasks in the milestone's
`mN-*.md` file as they are completed.

## 5. Finish: Open a Pull Request

When the milestone's exit criteria are met and CI is green locally
(`make check`), push the branch and open exactly one pull request:

```sh
git push -u origin milestone/mN-<slug>
gh pr create --base master --fill
```

Do not open PRs by any other means.

## Base Branch

- Before the v1.0.0 release, the PR base branch is always `master`.
- After v1.0.0, the base depends on the release being targeted (for example
  a `release/x.y` branch). Update this skill and the `--base` argument when
  that time comes.

## Rules

- Always create the branch before writing milestone code; never push
  milestone commits to `master` directly.
- Exactly one PR per milestone, created with `gh`.
- Keep the branch up to date with its base (rebase or merge) when the base
  advances, so the PR stays mergeable.
- Never force-push a shared branch without explicit user approval.
