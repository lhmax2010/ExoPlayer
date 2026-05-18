# C++ Bridge Document Index

Last updated: 2026-05-18

This page is the handoff index for the `exoplayer_cppbridge` work. Use it to decide:

- which document to read first
- which files are bridge implementation code
- which files are test code
- which files are demo/manual-validation code

If you only send one file to a tester or a handoff owner, send this one first.

## 1. Core Documents

| Document | Purpose | Primary audience |
| --- | --- | --- |
| [API_MAPPING_STATUS.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/API_MAPPING_STATUS.md) | top-level API-family status tracker; now separates reduced-endpoint `Done` from full-support gaps | dev lead, reviewer, test lead |
| [API_PARITY_GAP_REPORT.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/API_PARITY_GAP_REPORT.md) | generated method/callback/builder/object inventory from `api.txt` versus the C++ bridge surface | dev lead, reviewer |
| [DATA_STRUCTURE_MAPPING.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/DATA_STRUCTURE_MAPPING.md) | value-object / DTO mapping status; explains what reduced snapshots preserve and what full-support still lacks | bridge developer, reviewer |
| [DEVELOPMENT_STAGES.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/DEVELOPMENT_STAGES.md) | project history, current stage, and full-support backlog for the next phase | dev lead, handoff owner |
| [VALIDATION_GUIDE.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/VALIDATION_GUIDE.md) | compile / instrumentation / demo validation guide for a new machine or server | tester, release owner |
| [TEST_RESULTS_TEMPLATE.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/TEST_RESULTS_TEMPLATE.md) | raw result write-back template during validation | tester |
| [VALIDATION_RESULTS_SUMMARY.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/VALIDATION_RESULTS_SUMMARY.md) | final pass/fail rollup after validation | test lead, release owner |
| [FILE_MAP.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/FILE_MAP.md) | file-to-responsibility map for code reading and modification | developer |
| [KNOWLEDGE_GRAPH.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/KNOWLEDGE_GRAPH.md) | cross-reference / concept graph for the bridge area | developer, reviewer |

## 1A. Directory Cheat Sheet

### Documentation root

- [docs/cppbridge](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge)

### Core bridge code roots

- [libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge)
- [libraries/exoplayer_cppbridge/src/main/jni](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni)
- [libraries/exoplayer_cppbridge/src/main/jni/include](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/include)

Native target split:

- `exoplayer_cppbridge_jni`: production bridge core
- `exoplayer_cppbridge_jni_testhooks`: smoke, player-test, and demo JNI entrypoints

### Test code roots

- [libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge)
- [libraries/exoplayer_cppbridge/src/main/jni](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni)

### Demo and validation roots

- [demos/cppbridge](/home/linhao/Toolchain/development/ExoPlayer/demos/cppbridge)
- [scripts/cppbridge](/home/linhao/Toolchain/development/ExoPlayer/scripts/cppbridge)

## 2. Which Document To Give To Whom

### For test personnel

- [VALIDATION_GUIDE.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/VALIDATION_GUIDE.md)
- [TEST_RESULTS_TEMPLATE.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/TEST_RESULTS_TEMPLATE.md)
- [VALIDATION_RESULTS_SUMMARY.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/VALIDATION_RESULTS_SUMMARY.md)
- [DOCUMENT_INDEX.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/DOCUMENT_INDEX.md)

Recent test-facing additions:

- negative smoke for double release and listener lifecycle mutation
- controllable native packaging switch via `-PcppbridgeIncludeTestEntrypoints=OFF`
- `nativeListenerSmokeTest_reportsExtendedCallbacks` now covers direct
  `Player.Listener#onIsLoadingChanged` via `isLoadingCb=1` / `isLoading=1`
- `nativeTracksFullPayloadConversionSmokeTest_roundTripsFormatPayload` now covers richer
  `TrackInfo` / `Format` payload round-trip fields, including initialization bytes, DRM
  scheme-data shape, metadata/custom tokens, projection bytes, and HDR fields
- `nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary` and
  `nativePlaylistMetadataSmokeTest_roundTripsPlaylistMetadata` now assert decoded stable
  `Bundle` extras values for string, numeric, boolean, and byte-array entries

Recent dev-facing additions:

- repeatable API inventory via
  [api_parity_inventory.py](/home/linhao/Toolchain/development/ExoPlayer/scripts/cppbridge/api_parity_inventory.py)
  and the generated
  [API_PARITY_GAP_REPORT.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/API_PARITY_GAP_REPORT.md)
- explicit opaque-token cleanup API via `ReleaseOpaqueObjectTokens(...)` /
  `releaseOpaqueObjectTokens(...)`
- high-frequency C++ convenience wrappers in
  [exoplayer_sdk.h](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_sdk.h):
  `GetCurrentMediaItemWithOpaqueTokens(...)`, `GetMediaMetadataWithOpaqueTokens(...)`,
  `GetTracksWithOpaqueTokens(...)`, `GetTimelineWithOpaqueTokens(...)`, and
  `GetCurrentCuesWithOpaqueTokens(...)`
- automatic opaque-token cleanup is still deferred to the post-migration phase and is tracked in
  [DEVELOPMENT_STAGES.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/DEVELOPMENT_STAGES.md)

### For developers continuing feature work

- [API_MAPPING_STATUS.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/API_MAPPING_STATUS.md)
- [API_PARITY_GAP_REPORT.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/API_PARITY_GAP_REPORT.md)
- [DATA_STRUCTURE_MAPPING.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/DATA_STRUCTURE_MAPPING.md)
- [DEVELOPMENT_STAGES.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/DEVELOPMENT_STAGES.md)
- [FILE_MAP.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/FILE_MAP.md)

### For reviewers who need both code and test context

- [DOCUMENT_INDEX.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/DOCUMENT_INDEX.md)
- [API_PARITY_GAP_REPORT.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/API_PARITY_GAP_REPORT.md)
- [API_MAPPING_STATUS.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/API_MAPPING_STATUS.md)
- [VALIDATION_GUIDE.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/VALIDATION_GUIDE.md)
- [FILE_MAP.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/FILE_MAP.md)

## 3. Core Implementation Code Paths

These are the main code files that were modified or extended as part of the bridge work. If someone asks
"where is the real implementation?", start here.

### Java bridge runtime

- [CppExoPlayerBridge.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java)
- [CppBridgeConverters.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBridgeConverters.java)
- [CppBridgeNativePlayerTestHelper.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerTestHelper.java)
- [CppBridgeNativeSmokeTestHelper.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativeSmokeTestHelper.java)

### Java DTO / transport types

Path root:
- [cppbridge Java package](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge)

Representative files:
- [CppMediaItem.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppMediaItem.java)
- [CppMediaMetadata.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppMediaMetadata.java)
- [CppBundleValue.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBundleValue.java)
- [CppCue.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppCue.java)
- [CppTrackGroup.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppTrackGroup.java)
- [CppTrackInfo.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppTrackInfo.java)
- [CppOpaqueObjectRegistry.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppOpaqueObjectRegistry.java)

### Native public bridge surface

- [exoplayer_bridge.h](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_bridge.h)
- [exoplayer_sdk.h](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_sdk.h)
- [exoplayer_sdk.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_sdk.cpp)

### Native JNI implementation

- [exoplayer_cppbridge_jni_internal.h](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_internal.h)
- [exoplayer_cppbridge_jni_common.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_common.cpp)
- [exoplayer_cppbridge_jni_bridge.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_bridge.cpp)
- [exoplayer_cppbridge_jni_callbacks_demo.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_callbacks_demo.cpp)
- [exoplayer_cppbridge_jni_playlist_helpers.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_playlist_helpers.cpp)
- [CMakeLists.txt](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/CMakeLists.txt)

## 4. Test Code Paths

These are the files that define or assert expected behavior. If someone asks "what exactly do we run or
compare during validation?", start here.

### Android instrumentation entry points

- [CppBridgeNativeSmokeTest.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativeSmokeTest.java)
- [CppBridgeNativePlayerInstrumentationTest.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerInstrumentationTest.java)

### Native smoke / runtime summary generators

- [exoplayer_cppbridge_jni_smoke_tests.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_smoke_tests.cpp)
- [exoplayer_cppbridge_jni_player_tests.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_player_tests.cpp)

Representative negative-smoke coverage:

- `nativePlayerDoubleReleaseSmokeTest(...)`
- `nativeListenerLifecycleNegativeSmokeTest(...)`
- `nativeOpaqueTokenReleaseSmokeTest(...)`

Representative structural cleanup:

- shared `ScopedEnv` now lives in `exoplayer_cppbridge_jni_internal.h` and `exoplayer_cppbridge_jni_common.cpp`
- smoke/player-test/demo JNI entrypoints now build through `exoplayer_cppbridge_jni_testhooks`

### Validation scripts

- [run_validation.sh](/home/linhao/Toolchain/development/ExoPlayer/scripts/cppbridge/run_validation.sh)
- [run_validation.py](/home/linhao/Toolchain/development/ExoPlayer/scripts/cppbridge/run_validation.py)
- [api_parity_inventory.py](/home/linhao/Toolchain/development/ExoPlayer/scripts/cppbridge/api_parity_inventory.py)
- [launch_demo.sh](/home/linhao/Toolchain/development/ExoPlayer/scripts/cppbridge/launch_demo.sh)
- [launch_demo.py](/home/linhao/Toolchain/development/ExoPlayer/scripts/cppbridge/launch_demo.py)

## 5. Demo / Manual Validation Paths

These are the files to use for hands-on validation on a device after compilation succeeds.

### Demo app

- [MainActivity.java](/home/linhao/Toolchain/development/ExoPlayer/demos/cppbridge/src/main/java/androidx/media3/demo/cppbridge/MainActivity.java)
- [activity_main.xml](/home/linhao/Toolchain/development/ExoPlayer/demos/cppbridge/src/main/res/layout/activity_main.xml)
- [strings.xml](/home/linhao/Toolchain/development/ExoPlayer/demos/cppbridge/src/main/res/values/strings.xml)
- [demos/cppbridge/build.gradle](/home/linhao/Toolchain/development/ExoPlayer/demos/cppbridge/build.gradle)

### Demo JNI entrypoints

- [exoplayer_cppbridge_jni_callbacks_demo.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_callbacks_demo.cpp)

## 6. Fast Reading Order

If you only have a few minutes:

1. [DOCUMENT_INDEX.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/DOCUMENT_INDEX.md)
2. [API_PARITY_GAP_REPORT.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/API_PARITY_GAP_REPORT.md)
3. [API_MAPPING_STATUS.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/API_MAPPING_STATUS.md)
4. [VALIDATION_GUIDE.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/VALIDATION_GUIDE.md)
5. [CppBridgeNativeSmokeTest.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativeSmokeTest.java)
6. [CppBridgeNativePlayerInstrumentationTest.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerInstrumentationTest.java)

If you are continuing development:

1. [FILE_MAP.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/FILE_MAP.md)
2. [DATA_STRUCTURE_MAPPING.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/DATA_STRUCTURE_MAPPING.md)
3. [DEVELOPMENT_STAGES.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/DEVELOPMENT_STAGES.md)
4. [CppExoPlayerBridge.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java)
5. [exoplayer_bridge.h](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_bridge.h)
6. [exoplayer_sdk.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_sdk.cpp)

## 7. One-Line Summary By File Type

### If you want to know what changed in the product code

- read [CppExoPlayerBridge.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java)
- read [CppBridgeConverters.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBridgeConverters.java)
- read [exoplayer_bridge.h](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_bridge.h)
- read [exoplayer_sdk.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_sdk.cpp)
- read [exoplayer_cppbridge_jni_bridge.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_bridge.cpp)
- read [exoplayer_cppbridge_jni_common.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_common.cpp)

### If you want to know what test code validates the bridge

- read [CppBridgeNativeSmokeTest.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativeSmokeTest.java)
- read [CppBridgeNativePlayerInstrumentationTest.java](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerInstrumentationTest.java)
- read [exoplayer_cppbridge_jni_smoke_tests.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_smoke_tests.cpp)
- read [exoplayer_cppbridge_jni_player_tests.cpp](/home/linhao/Toolchain/development/ExoPlayer/libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_player_tests.cpp)
- check `nativePlayerDoubleReleaseSmokeTest(...)` and `nativeListenerLifecycleNegativeSmokeTest(...)` first for lifecycle regressions

### If you want to know what to follow during manual testing

- read [VALIDATION_GUIDE.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/VALIDATION_GUIDE.md)
- read [TEST_RESULTS_TEMPLATE.md](/home/linhao/Toolchain/development/ExoPlayer/docs/cppbridge/TEST_RESULTS_TEMPLATE.md)
- read [MainActivity.java](/home/linhao/Toolchain/development/ExoPlayer/demos/cppbridge/src/main/java/androidx/media3/demo/cppbridge/MainActivity.java)
- read [activity_main.xml](/home/linhao/Toolchain/development/ExoPlayer/demos/cppbridge/src/main/res/layout/activity_main.xml)
