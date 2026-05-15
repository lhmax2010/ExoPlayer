# Workspace Diffs And Caveats

## 1. This workspace is not a git repository

Current state:

- `.git` directory is absent
- `git status` fails from the workspace root

Implication:

- This looks like an extracted working snapshot rather than a live clone
- pushing to GitHub is a transfer task, not a normal branch push from an existing repo

## 2. Current docs differ from older handoff naming

The current workspace does not contain these previously referenced files:

- `docs/cppbridge/API_STATUS_TRACKER.md`
- `docs/cppbridge/RECOVERY_CHECKLIST.md`
- `docs/cppbridge/DEVELOPMENT_PLAN.md`

Instead, the current workspace uses:

- `docs/cppbridge/API_MAPPING_STATUS.md`
- `docs/cppbridge/API_MAPPING_QUICK_REFERENCE.md`
- `docs/cppbridge/DATA_STRUCTURE_MAPPING.md`
- `docs/cppbridge/DATA_STRUCTURE_QUICK_REFERENCE.md`
- `docs/cppbridge/FILE_MAP.md`
- `docs/cppbridge/VALIDATION_GUIDE.md`
- `docs/cppbridge/VALIDATION_RESULTS_SUMMARY.md`

Implication:

- If another AI was primed on the older doc names, it must be told that this workspace has moved to a newer document set.

## 3. There is an older packaged handoff bundle

Existing folder:

- `handoff/2026-03-19-final-fix-pack`

This may be useful for comparison, but the current top-level workspace should be treated as the active source of truth.

## 4. Current docs already mention newer capabilities

Visible examples in current docs:

- player message reduced send/cancel/runtime smoke
- preload target-duration behavior
- image output reduced callback
- priority wrapper behavior
- opaque token coverage

Implication:

- The current workspace is already more advanced than a naive "Phase 0 bridge only" mental model.

## 5. Validation is still open

Even though `API_MAPPING_STATUS.md` marks the reduced endpoint tracker as fully done, `VALIDATION_RESULTS_SUMMARY.md` is still pending across the major validation buckets.

This difference is important:

- implementation closure != environment validation closure
