# Handoff Prompt

Use the prompt below for the next AI.

---

You are taking over the `exoplayer_cppbridge` work in the repository at:

`c:\Users\hao.lin\Downloads\media-release`

Your job is to continue the C++ bridge project without re-discovering the repo from scratch.

Start by reading these files in order:

1. `dev_memory/PROJECT_STATE.md`
2. `dev_memory/DECISIONS_AND_CONVENTIONS.md`
3. `dev_memory/WORKSPACE_DIFFS.md`
4. `docs/cppbridge/API_MAPPING_STATUS.md`
5. `docs/cppbridge/API_MAPPING_QUICK_REFERENCE.md`
6. `docs/cppbridge/DATA_STRUCTURE_MAPPING.md`
7. `docs/cppbridge/DATA_STRUCTURE_QUICK_REFERENCE.md`
8. `docs/cppbridge/FILE_MAP.md`
9. `docs/cppbridge/VALIDATION_GUIDE.md`
10. `docs/cppbridge/VALIDATION_RESULTS_SUMMARY.md`

Then inspect these implementation roots:

- `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_bridge.h`
- `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_sdk.h`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_sdk.cpp`
- `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_bridge.cpp`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_common.cpp`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_smoke_tests.cpp`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_player_tests.cpp`

Important context:

- This workspace is not a git repo. It appears to be an extracted snapshot.
- The current workspace uses the newer `API_MAPPING_STATUS.md` / quick-reference doc system.
- Do not assume older docs like `API_STATUS_TRACKER.md` or `RECOVERY_CHECKLIST.md` exist here.
- The reduced endpoint tracker currently claims `Done: 99`, `Partial: 0`, `Not started: 0`, but environment validation is still pending.
- The highest-value next work is likely validation, full-parity expansion, or closing capability gaps beyond the reduced endpoint, not basic bridge scaffolding.

When you report status, separate these clearly:

1. reduced endpoint coverage
2. full Java/api.txt parity gaps
3. environment validation status
4. repo-transfer/git status

Before making large changes, verify whether the current task is:

- validation and stabilization
- full parity expansion
- documentation consistency
- git/repo transfer

If asked to continue development, prefer using the current docs as source-of-truth instead of reconstructing status manually.

---
