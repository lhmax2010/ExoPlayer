# Knowledge Graph

Last updated: 2026-03-18

This file is a handoff graph for future engineers or AI agents. It is optimized for fast context
loading, not for narrative reading.

## 1. Core Nodes

### Module Nodes

- `:lib-exoplayer-cppbridge`
  - Android library module containing Java bridge, JNI bridge, and C++ SDK wrapper
- `:demo-cppbridge`
  - Android demo app that exercises the native bridge end to end

### Java Runtime Nodes

- `CppExoPlayerBridge.java`
  - Java-side runtime bridge around Media3 `ExoPlayer`
- `CppBridgeConverters.java`
  - Java reduced-model conversions between Media3 types and bridge DTO classes
- `Cpp*` DTO Java classes
  - Java-side transport objects for JNI crossing
- `MainActivity.java` in `demo-cppbridge`
  - manual validation surface for native player control

### Native Runtime Nodes

- `ExoPlayerSdkPlayer`
  - public native player-facing abstraction
- `ExoPlayerBridge`
  - JNI bridge abstraction used by the SDK wrapper
- `JniExoPlayerBridge`
  - concrete implementation binding native calls to Java bridge methods
- `exoplayer_sdk.cpp`
  - SDK wrapper that owns bridge object and exposes native-friendly API

### JNI Implementation Nodes

- `exoplayer_cppbridge_jni_common.cpp`
  - shared JNI helpers, conversions, registry, object creation/parsing
- `exoplayer_cppbridge_jni_bridge.cpp`
  - `JniExoPlayerBridge` implementation and narrow callback dispatch wrappers
- `exoplayer_cppbridge_jni_callbacks_demo.cpp`
  - JNI callback entrypoints and demo JNI entrypoints
- `exoplayer_cppbridge_jni_smoke_tests.cpp`
  - JNI/value conversion smoke helpers
- `exoplayer_cppbridge_jni_player_tests.cpp`
  - player/runtime smoke helpers
- `exoplayer_cppbridge_jni_internal.h`
  - shared internal declarations across JNI translation units

### Validation Nodes

- `CppBridgeNativeSmokeTest.java`
  - JNI/value conversion instrumentation coverage
- `CppBridgeNativePlayerInstrumentationTest.java`
  - player/runtime instrumentation coverage
- `scripts/cppbridge/run_validation.sh`
  - full validation entrypoint
- `scripts/cppbridge/run_validation.py`
  - Python full validation entrypoint
- `scripts/cppbridge/launch_demo.sh`
  - demo install/launch entrypoint
- `scripts/cppbridge/launch_demo.py`
  - Python demo install/launch entrypoint

## 2. Key Edges

- `MainActivity.java` -> native methods in `exoplayer_cppbridge_jni_callbacks_demo.cpp`
- `CppExoPlayerBridge.java` -> callback JNI calls into `exoplayer_cppbridge_jni_callbacks_demo.cpp`
- `exoplayer_cppbridge_jni_callbacks_demo.cpp` -> `BridgeOn*` helpers in `exoplayer_cppbridge_jni_bridge.cpp`
- `BridgeOn*` helpers -> bridge registry in `exoplayer_cppbridge_jni_common.cpp`
- `JniExoPlayerBridge` -> Java bridge methods on `CppExoPlayerBridge.java`
- `exoplayer_sdk.cpp` -> wraps `ExoPlayerBridge` and exposes stable native SDK API
- instrumentation tests -> `CppBridgeNativeSmokeTestHelper` / `CppBridgeNativePlayerTestHelper`
- helper Java classes -> JNI smoke methods in `exoplayer_cppbridge_jni_smoke_tests.cpp` / `exoplayer_cppbridge_jni_player_tests.cpp`

## 3. State Of The World

### Stable Enough

- source layout
- JNI translation unit split
- reduced bridge API surface
- reduced analytics aggregate plus forty-six concrete reduced `AnalyticsListener` event paths for
  `onAudioUnderrun`, `onDroppedVideoFrames`, `onBandwidthEstimate`, `onLoadStarted`,
  `onLoadCompleted`, `onAudioInputFormatChanged`, `onAudioDecoderInitialized`,
  `onVideoDecoderInitialized`, `onAudioDecoderReleased`, `onVideoDecoderReleased`,
  analytics `onRenderedFirstFrame`, analytics `onVideoSizeChanged`,
  analytics `onAudioPositionAdvancing`, analytics `onVideoFrameProcessingOffset`,
  analytics `onVolumeChanged`, analytics `onAudioSessionIdChanged`,
  analytics `onAudioAttributesChanged`,
  analytics `onSkipSilenceEnabledChanged`, analytics `onDeviceVolumeChanged`,
  analytics `onPlaybackStateChanged`, analytics `onIsPlayingChanged`,
  analytics `onPlayWhenReadyChanged`, analytics `onPlaybackSuppressionReasonChanged`,
  analytics `onIsLoadingChanged`, analytics `onRepeatModeChanged`,
  analytics `onShuffleModeChanged`, `onVideoInputFormatChanged`,
  analytics `onSeekBackIncrementChanged`, analytics `onSeekForwardIncrementChanged`,
  analytics `onMaxSeekToPreviousPositionChanged`, analytics `onTimelineChanged`,
  analytics `onPositionDiscontinuity`, analytics `onSeekStarted`,
  analytics `onPlayerError`, analytics `onPlayerErrorChanged`, and analytics `onTracksChanged`
- demo app wiring
- smoke coverage layout
- release/callback hardening direction

### Still Reduced / Not Full Parity

- `MediaItem`
- `Timeline`
- `Tracks`
- `MediaMetadata`
- `Cue`
- full Java `AnalyticsListener` parity
- richer image output parity beyond reduced frame metadata, bitmap-layout metadata, and callback behavior
- richer video effects parity beyond the current reduced effect set and boundary/default-value coverage
- broader arbitrary `MediaSource.Factory` injection beyond token-registered and registry-generated-token baseline support
- arbitrary Java target parity for player messaging

### Current External Blocker

- local Gradle environment in this workspace cannot complete build verification

## 4. Safe Reading Order For New Contributors

1. `docs/cppbridge/VALIDATION_GUIDE.md`
2. `docs/cppbridge/API_MAPPING_STATUS.md`
3. `docs/cppbridge/DATA_STRUCTURE_MAPPING.md`
4. `docs/cppbridge/FILE_MAP.md`
5. `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_sdk.h`
6. `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_sdk.cpp`
7. `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java`
8. `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_bridge.cpp`
9. `libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerInstrumentationTest.java`

## 5. High-Risk Change Areas

- JNI local/global ref management
- callback delivery during or after release
- Java/native field order coupling in serialized timeline row parsing
- DTO field additions that require updates in Java DTO, converter, JNI parser/serializer, and smoke assertions

## 6. Immediate Next Actions In A Healthy Build Environment

1. Run one full validation entrypoint: `scripts/cppbridge/run_validation.sh` or `scripts/cppbridge/run_validation.py`
2. Fix any compile/runtime breakage found there
3. Update `API_MAPPING_STATUS.md` counts if tracker state changes
4. Continue reduced parity expansion only after smoke remains green
