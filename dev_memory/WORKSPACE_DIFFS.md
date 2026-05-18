# Workspace Diffs And Caveats

## 1. Current workspace is a git repository with a dirty worktree

Current state:

- `.git` is present in `/home/linhao/Toolchain/development/ExoPlayer`
- `git status --short` works from the workspace root
- the worktree is intentionally dirty with many bridge/demo/script edits from prior work and this
  handoff pass

Implication:

- do not run destructive checkout/reset commands unless the user explicitly asks
- treat unrelated dirty files as user/prior-Codex work
- new work should build on the existing edits rather than trying to restore a clean upstream tree

Notable current dirty areas:

- `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_sdk.h`
- `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_bridge.h`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_bridge.cpp`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_sdk.cpp`
- `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java`
- `libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerInstrumentationTest.java`
- new Java DTO: `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppCodecParameter.java`
- callback reduced parity work: `CodecParametersChangeListener`, `VideoFrameMetadataListener`,
  and `CameraMotionListener` C++ registration and smoke coverage
- codec-parameter multi-listener immediate-callback routing fix and smoke coverage
- HTTP progressive / HLS / DASH C++ playback smoke coverage and source-type inference fixes
- androidTest local media assets from small DASH/HLS test-data folders plus androidTest
  `INTERNET` / cleartext manifest permissions for MockWebServer validation
- generated/local output folder: `artifacts/`
- direct coverage top-up for existing C++ API / CppBridge methods: raw `Surface` overloads,
  single/range playlist mutation, playlist navigation getters, tracks getters, device
  volume/mute setters, codec-parameter bridge listener registration/clear, builder
  `SetMediaSourceFactoryConfig`, and SDK `ClearPriorityTaskManager`
- full-parity payload slice for `VideoFrameMetadataListener`: representative `MediaFormat` mime,
  dimensions, frame-rate, rotation, and color fields now round-trip through the native snapshot and
  auxiliary callback smoke
- video-frame metadata fallback/sentinel fix: C++ `format_bitrate` now reaches Java simulation
  fallback behavior, and unset color/audio-shape fields preserve `Format.NO_VALUE`
- Tracks/Format payload slice: `TrackInfo` now includes average/peak bitrate, initialization/DRM
  counts, subsample/preroll, decoded/projection/stereo/color, max sublayers, PCM/encoder, tile, and
  crypto fields through Java DTO, JNI create/parse, converter unit tests, and native current-tracks
  smokes
- Stage 2 TrackInfo full-payload slice: `TrackInfo` now also carries label language/value arrays,
  metadata/custom opaque tokens, initialization byte arrays, DRM scheme type plus
  uuid/license/mime/data/has-data, projection bytes, HDR static info, color bitdepth, and auxiliary
  track type through Java DTO, JNI create/parse, converter unit tests, and native round-trip/current
  tracks smoke coverage
- Stage 3 extras value-model slice: `MediaItem.RequestMetadata.extras` and
  `MediaMetadata.extras` now carry decoded stable `Bundle` entries through `BundleValueInfo` /
  `CppBundleValue` for strings, integer-like numbers, floating-point numbers, booleans, and byte
  arrays, with opaque-token fallback preserved for arbitrary Java objects
- Stage 1 full API inventory added `dev_memory/DEVELOPMENT_PLAN.md`,
  `scripts/cppbridge/api_parity_inventory.py`, parser unit tests, and
  `docs/cppbridge/API_PARITY_GAP_REPORT.md`; it also closed direct
  `Player.Listener#onIsLoadingChanged` bridge coverage with `nativeListenerSmokeTest` markers
  `isLoadingCb=1` and `isLoading=1`

## 2. Current docs differ from older handoff naming

The current workspace does not contain these previously referenced files:

- `docs/cppbridge/API_STATUS_TRACKER.md`
- `docs/cppbridge/RECOVERY_CHECKLIST.md`
- `docs/cppbridge/DEVELOPMENT_PLAN.md`

Instead, the current workspace uses:

- `dev_memory/DEVELOPMENT_PLAN.md`
- `docs/cppbridge/API_MAPPING_STATUS.md`
- `docs/cppbridge/API_PARITY_GAP_REPORT.md`
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

## 5. Validation status was closed locally on Android 16

Older docs said validation was still open. The latest local pass on 2026-05-18 refreshed the main
smoke loop on an Android 16 emulator after the TrackInfo format-payload expansion.

Passed commands:

- `./gradlew :lib-exoplayer-cppbridge:assembleDebugAndroidTest`
- `./gradlew :lib-exoplayer-cppbridge:testDebugUnitTest`
- `./gradlew :lib-exoplayer-cppbridge:connectedDebugAndroidTest` (`125/125`)
- `./gradlew :demo-cppbridge:assembleDebug`
- `git diff --check`

Remaining caveat:

- demo manual interaction and logcat auditing were not separately written up in this pass
