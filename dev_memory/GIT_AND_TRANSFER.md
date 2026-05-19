# Git And Transfer Notes

## Current local state

- The workspace is a live git repository at `/home/linhao/Toolchain/development/ExoPlayer`.
- There is local commit history plus a dirty worktree with prior bridge/demo/script edits.
- The target URL provided by the user is:
  `https://github.com/lhmax2010/ExoPlayer.git`

## Safe interpretation

Uploading this project now means one of these:

1. Commit the current intended changes on the active branch and push.
2. Create a new branch from the current dirty workspace, commit, then push that branch.
3. If the remote history differs, clone/compare separately before pushing anything.

Because the worktree is dirty, do not assume all changes belong to a single commit. Review the diff
scope before staging.

## Recommended transfer procedure

1. Run `git status --short`.
2. Review current branch and remotes with `git branch --show-current` and `git remote -v`.
3. Review staged/unstaged scope before committing.
4. Commit only the intended bridge/docs/test changes.
5. Push the active branch or a new review branch, depending on user preference.

## Authentication caveat

Even if the repo is reachable, push will still require valid GitHub auth in this environment.

## Suggested commit message

`Expand exoplayer cppbridge runtime parity`
