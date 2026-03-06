# Git Workflow — Devana

A complete reference from making a change to getting it into `master`. This replaces the `git add --all && git commit -m "" && git push` habit with a workflow that gives you a clean history, isolated changes, and a clear record of why things changed.

---

## The mental model

`master` is always stable and buildable. You never commit directly to it. Every change — no matter how small — lives on its own branch until it's reviewed and merged. This sounds like overhead but it takes about 30 seconds and it means you can always get back to a working state instantly.

```
master  ──────────────────────────────────────────────────────►
              │                              ↑
              └── feature/zmq-harness ───────┘
                       (your work)        (merged)
```

---

## Branch naming

```
feature/short-description     # new functionality
fix/short-description         # bug fix
refactor/short-description    # restructuring, no behaviour change
chore/short-description       # build system, tooling, docs, cleanup
```

Examples from this project:
```
feature/scenario-loader
fix/signal-blocker-ub
refactor/namespace-okuden-to-devana
chore/zmq-cmake-detection
chore/platform-docker-build
```

Keep it lowercase, use hyphens, be specific enough that you know what it is in three months.

---

## Step-by-step workflow

### 1. Start from a clean, up-to-date master

Before creating a branch, make sure you're starting from the latest state:

```bash
git checkout master
git pull origin master
```

If you have uncommitted local changes you're not ready to lose:
```bash
git stash          # temporarily shelve changes
git pull origin master
git stash pop      # reapply them
```

### 2. Create a branch for your change

```bash
git checkout -b feature/your-feature-name
```

You are now on a new branch. `master` is untouched. Do all your work here.

### 3. Make your changes

Write code, run the build, iterate. Nothing special here — just work normally.

```bash
./devana build      # make sure it compiles
./devana doctor     # check deps if you changed build config
```

### 4. Stage changes deliberately

This is the biggest habit change from `git add --all`. Stage only what belongs to this commit.

```bash
# See what changed
git status

# See exactly what changed in each file
git diff

# Stage specific files
git add src/ui/TestHarness/ScenarioLoader.cpp
git add src/ui/TestHarness/ScenarioLoader.h

# Stage a specific file interactively (choose which hunks to include)
git add -p src/ui/TestHarness/TestHarnessWindow.cpp

# Stage everything in a directory
git add src/ui/TestHarness/

# Stage everything (use sparingly — only when all changes belong together)
git add -A
```

`git add -p` is worth learning. It lets you split a file's changes across multiple commits if you made unrelated edits in the same file.

### 5. Review what you're about to commit

```bash
# See what's staged
git diff --staged

# Make sure you haven't accidentally staged debug code, commented-out blocks, etc.
```

If something is staged that shouldn't be:
```bash
git restore --staged path/to/file    # unstage a file (keeps your changes)
```

### 6. Write a commit message

```bash
git commit
```

This opens your editor. Write the message in this format:

```
Short summary in imperative mood, under 72 chars

Optional longer explanation of WHY this change was needed, what
problem it solves, or any context that won't be obvious from the
diff. Leave the second line blank between summary and body.

If it closes an issue: Fixes #42
```

**Good summaries:**
```
Fix signal blocker UB in TestHarnessWindow::populatePicker
Add ZMQ optional build flag via DEVANA_ENABLE_ZMQ
Rename okuden namespace to devana in common headers
Move platform build scripts to platform/ directory
```

**Bad summaries:**
```
fix
changes
wip
update stuff
```

The rule of thumb: if you squint at `git log` six months from now, can you understand what happened without opening the diff?

For genuinely trivial commits you can use `-m` inline:
```bash
git commit -m "Remove stray root-level main.cpp (Sentinel copy)"
```

### 7. Keep your branch up to date (if working over multiple days)

If `master` has moved forward while you were working:

```bash
git fetch origin
git rebase origin/master
```

Rebase replays your commits on top of the latest master rather than creating a merge commit. This keeps history linear and clean. If there are conflicts:

```bash
# Git will pause and tell you which files conflict
# Open the conflicted file, resolve it, then:
git add path/to/resolved/file
git rebase --continue

# If it gets messy and you want to bail:
git rebase --abort
```

### 8. Clean up your commits before merging (optional but good)

If you made several "wip" commits while working, squash them into one clean commit before merging:

```bash
# Interactively rebase the last N commits
git rebase -i HEAD~3
```

This opens an editor showing your commits:
```
pick a1b2c3 Add ScenarioLoader class
pick d4e5f6 wip fix parsing
pick g7h8i9 fix typo

# Change 'pick' to 'squash' (or 's') to merge into the one above:
pick a1b2c3 Add ScenarioLoader class
squash d4e5f6 wip fix parsing
squash g7h8i9 fix typo
```

You'll then write one clean commit message for the combined commit.

### 9. Push your branch

```bash
git push origin feature/your-feature-name

# If you rebased after already pushing, you'll need to force push:
git push --force-with-lease origin feature/your-feature-name
```

`--force-with-lease` is safer than `--force` — it refuses to push if someone else has pushed to the same branch since your last fetch.

---

## Pull request vs merge request

They are the same thing with different names:

- **Pull request (PR)** — GitHub's name
- **Merge request (MR)** — GitLab's / Gitea's name

Since you're using Gitea, you'll see "merge request" in the UI. The concept is identical: you're asking for your branch to be reviewed and merged into `master`.

On a solo project or a 3-person team where you're also the reviewer, the MR is still useful as a checkpoint — it gives you a diff view of everything the branch changes before it hits master.

---

## Merging into master

### Option A — Merge request in Gitea (recommended)

1. Push your branch
2. Open Gitea → your repo → "Merge Requests" → "New Merge Request"
3. Set source branch to your feature branch, target to `master`
4. Review the diff — catch anything you missed
5. Merge (use "Squash and merge" if you want one clean commit, "Merge" if you want the full history)

### Option B — Merge locally

```bash
git checkout master
git merge --no-ff feature/your-feature-name
git push origin master
```

`--no-ff` (no fast-forward) preserves the branch structure in history so you can see what commits belonged to a feature. Without it, the commits get flattened into master directly.

### Option C — Rebase and merge (cleanest history)

```bash
git checkout feature/your-feature-name
git rebase origin/master          # bring up to date
git checkout master
git merge --ff-only feature/your-feature-name   # fast-forward only
git push origin master
```

This gives perfectly linear history — every commit lands directly on master with no merge commits.

---

## After merging

```bash
# Switch back to master and pull
git checkout master
git pull origin master

# Delete the local branch (it's been merged, you don't need it)
git branch -d feature/your-feature-name

# Delete the remote branch
git push origin --delete feature/your-feature-name
```

---

## Useful commands to build habits around

```bash
# See a visual branch history
git log --oneline --graph --decorate --all

# See what's different between your branch and master
git diff master...HEAD

# See only the files that changed
git diff master...HEAD --name-only

# Undo the last commit but keep the changes staged
git reset --soft HEAD~1

# Undo the last commit and unstage the changes (changes still on disk)
git reset HEAD~1

# Discard all uncommitted changes to a file (permanent)
git restore path/to/file

# See who changed what line in a file
git blame src/ui/TestHarness/ScenarioLoader.cpp

# Find which commit introduced a specific string
git log -S "findDefaultPath" --oneline

# Temporarily save unfinished work
git stash
git stash pop           # reapply most recent stash
git stash list          # see all stashes
git stash drop          # discard most recent stash
```

---

## Quick reference

```
Start work:
  git checkout master && git pull
  git checkout -b feature/name

Work:
  (edit files)
  git status
  git diff
  git add -p file          ← stage deliberately
  git diff --staged        ← review before committing
  git commit               ← write a real message

Stay current:
  git fetch origin
  git rebase origin/master

Finish:
  git push origin feature/name
  → open MR in Gitea
  → review diff
  → merge
  git checkout master && git pull
  git branch -d feature/name
```
