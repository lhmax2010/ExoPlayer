# Handoff Prompt

Use the prompt below for the next AI.

---

You are taking over the `exoplayer_cppbridge` work in the repository at:

`/home/linhao/Toolchain/development/ExoPlayer`

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

- This workspace is a live git repo, but the worktree is dirty. Do not reset or revert unrelated
  edits unless the user explicitly asks.
- The current workspace uses the newer `API_MAPPING_STATUS.md` / quick-reference doc system.
- Do not assume older docs like `API_STATUS_TRACKER.md` or `RECOVERY_CHECKLIST.md` exist here.
- The legacy reduced endpoint tracker claims `Done: 99`, `Partial: 0`, `Not started: 0`; a
  2026-05-15 parity addendum adds more runtime/audio/scrubbing/codec/renderer getter APIs beyond
  that older row count.
- Latest local validation passed on Android 16 AVD `cppbridge_android16_api36`:
  `assembleDebugAndroidTest`, `testDebugUnitTest`, full `connectedDebugAndroidTest` (`124/124`),
  `:demo-cppbridge:assembleDebug`, and `git diff --check`.
- Callback-style reduced C++ APIs now exist for `CodecParametersChangeListener`,
  `VideoFrameMetadataListener`, and `CameraMotionListener`, with
  `nativeAuxiliaryCallbackParitySmokeTest_reportsCodecVideoAndCameraCallbacks` covering the main
  registration / callback / remove lifecycle.
- `nativeCodecParametersMultiListenerParitySmokeTest_routesImmediateCallbacks` covers the
  codec-parameter multi-listener immediate-callback routing edge case for audio and video listeners.
- `nativeHttpHlsDashPlaybackSmokeTest_preparesLocalStreamsThroughCppApi` covers local
  MockWebServer playback for HTTP progressive, HLS, and DASH through the C++ `SetMediaItem` /
  `Prepare` / `Play` path. It also verifies that `MediaItemDescriptor.source_type` now round-trips
  HTTP progressive sources as `kProgressive`.
- A follow-up C++ API / CppBridge method audit found no remaining exact public-method gaps after
  adding direct smoke markers for raw `Surface`, playlist mutation/navigation, tracks getters,
  device volume/mute setters, codec-parameter bridge registration/clear, builder
  `SetMediaSourceFactoryConfig`, and SDK `ClearPriorityTaskManager`.
- The first next-phase full-payload slice expands `VideoFrameMetadataListener`:
  `VideoFrameMetadataSnapshot` now carries representative `Format` label/language/container MIME,
  bitrate, rotation, pixel-ratio, color, audio-shape, and flags fields plus representative
  `MediaFormat` mime, dimensions, frame-rate, rotation, and color fields, covered by
  `nativeAuxiliaryCallbackParitySmokeTest_reportsCodecVideoAndCameraCallbacks`.
- Review follow-up fixed the video-frame metadata simulation edge case where C++ `format_bitrate`
  was ignored; `nativeVideoFrameMetadataSimulationFallbackSmokeTest_preservesFallbackFields`
  verifies fallback into Java average bitrate plus `Format.NO_VALUE` preservation for absent
  color/audio-shape fields.
- The next Tracks/Format payload slice expands `TrackInfo` with average/peak bitrate, rotation,
  pixel width-height ratio, and color info, covered by `CppBridgeConvertersTest`,
  `nativeTracksSnapshotConversionSmokeTest_returnsStructuredSummary`, and
  `nativeCurrentTracksSmokeTest_returnsTracksSummary`.
- `CppBridgeConverters` now normalizes HLS MIME aliases like
  `application/vnd.apple.mpegurl` / lowercase `application/x-mpegurl`, and maps Media3
  `CONTENT_TYPE_OTHER` back to C++ `MediaSourceType::kProgressive`.
- The highest-value next development work is deeper full-object parity and richer payload handling
  beyond the reduced callback descriptors, plus broader full Java/api.txt parity.

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
