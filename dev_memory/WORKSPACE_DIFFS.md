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
- Stage 5 custom source-factory playback smoke coverage for SmoothStreaming / RTSP descriptors
  routed through C++ factory-token config and Java `FakeMediaSourceFactory`
- Stage 5 HTTP data-source config playback smoke coverage proving C++ request headers and
  User-Agent reach MockWebServer during real progressive playback
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
- Stage 3 timeline object-value slice: timeline window `uid`/`manifest` and period
  `id`/`uid`/`adsId` now carry reduced `ObjectValueInfo` descriptors with class/type and stable
  scalar payload fields in addition to the existing string/token baselines
- Stage 3 MediaItem object-value slice: `MediaItemDescriptor.tag_value` and
  `AdsConfigurationDescriptor.ads_id_value` now use Java `CppObjectValue` / C++ `ObjectValueInfo`
  metadata for reduced scalar object visibility while preserving existing string fallback and
  opaque-token identity behavior
- Stage 3 MediaMetadata object-value slice: `CppMediaMetadata` and `MediaMetadataSnapshot` now
  carry reduced `ObjectValueInfo` descriptors for representative text/`CharSequence` fields
  (`title`, `artist`, album/display/subtitle/description, credits, `genre`, `compilation`, and
  `station`) while preserving token-first and string fallback behavior
- Stage 4 analytics callback slice: `AnalyticsListener#onAudioAttributesChanged` now has a
  distinct C++ callback (`OnAnalyticsAudioAttributesChanged`) and simulation path carrying
  `AudioAttributesDescriptor` content type, usage, flags, allowed-capture policy, and
  spatialization behavior
- Stage 4 remaining analytics callback closeout: 25 additional reduced `AnalyticsListener`
  callbacks are now routed through Java, JNI, C++ bridge forwarding, SDK analytics delegates, and
  native smoke coverage. The new batch covers player-state/loading aliases, track-selection
  parameters, load canceled, downstream/upstream format events, decoder-counters enabled/disabled
  events, audio/video codec and sink errors, audio-track init/release, surface size, DRM lifecycle
  and key events, renderer-ready, dropped-seeks-while-scrubbing, and player-released callbacks.
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

Older docs said validation was still open. The latest local pass on 2026-05-19 refreshed the main
smoke loop on an Android 16 emulator after the Stage 6 audio-output-provider and opaque-token batch
smokes were added.

Passed commands:

- `./gradlew :lib-exoplayer-cppbridge:assembleDebugAndroidTest`
- `./gradlew :lib-exoplayer-cppbridge:testDebugUnitTest`
- current connected instrumentation is verified on Android 16 AVD `emulator-5554`: `139/139`
  (`27/27` JNI/value smoke plus `112/112` player/runtime smoke)
- `./gradlew :demo-cppbridge:assembleDebug`
- `./gradlew :lib-exoplayer-cppbridge:assemble -PcppbridgeIncludeTestEntrypoints=OFF`
- `python3 -m unittest discover -s scripts/cppbridge -p '*_test.py'`
- `python3 scripts/cppbridge/api_parity_inventory.py --check`
- `git diff --check`

Script review follow-up:

- `run_validation.py` / `.sh` now fail early when `--serial` names a device that is not online,
  instead of only checking for any online adb device.

Remaining caveat:

- RPI4 board validation remains pending until the board is reachable; use
  `docs/cppbridge/RPI4_TESTING_GUIDE.md` for that manual pass
- demo remote-media playback still belongs to RPI4/manual media validation, but Android 16 emulator
  UI control/query smoke is now written up: Play/Pause/Stop and Playback/Tracks/Item/Timeline/
  Metadata/Cues buttons refreshed native status summaries, and high-signal logcat crash filters were
  clean.
