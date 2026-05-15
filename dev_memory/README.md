# Dev Memory

This folder is the fast handoff package for the current `exoplayer_cppbridge` work.

Read in this order:

1. `PROJECT_STATE.md`
2. `DECISIONS_AND_CONVENTIONS.md`
3. `WORKSPACE_DIFFS.md`
4. `GIT_AND_TRANSFER.md`
5. `HANDOFF_PROMPT.md`

Primary source-of-truth documents already in the repo:

- `docs/cppbridge/API_MAPPING_STATUS.md`
- `docs/cppbridge/API_MAPPING_QUICK_REFERENCE.md`
- `docs/cppbridge/DATA_STRUCTURE_MAPPING.md`
- `docs/cppbridge/DATA_STRUCTURE_QUICK_REFERENCE.md`
- `docs/cppbridge/FILE_MAP.md`
- `docs/cppbridge/VALIDATION_GUIDE.md`
- `docs/cppbridge/VALIDATION_RESULTS_SUMMARY.md`

Primary implementation roots:

- `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_bridge.h`
- `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_sdk.h`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_sdk.cpp`
- `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_bridge.cpp`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_common.cpp`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_smoke_tests.cpp`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_player_tests.cpp`
