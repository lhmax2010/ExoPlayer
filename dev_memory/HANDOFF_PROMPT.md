# Handoff Prompt

Use the prompt below for the next AI.

---

You are taking over the `exoplayer_cppbridge` work in the repository at:

`/home/linhao/Toolchain/development/ExoPlayer`

Your job is to continue the C++ bridge project without re-discovering the repo from scratch.

Start by reading these files in order:

1. `dev_memory/PROJECT_STATE.md`
2. `dev_memory/DECISIONS_AND_CONVENTIONS.md`
3. `dev_memory/DEVELOPMENT_PLAN.md`
4. `dev_memory/WORKSPACE_DIFFS.md`
5. `docs/cppbridge/API_PARITY_GAP_REPORT.md`
6. `docs/cppbridge/API_MAPPING_STATUS.md`
7. `docs/cppbridge/API_MAPPING_QUICK_REFERENCE.md`
8. `docs/cppbridge/DATA_STRUCTURE_MAPPING.md`
9. `docs/cppbridge/DATA_STRUCTURE_QUICK_REFERENCE.md`
10. `docs/cppbridge/FILE_MAP.md`
11. `docs/cppbridge/VALIDATION_GUIDE.md`
12. `docs/cppbridge/VALIDATION_RESULTS_SUMMARY.md`

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
- Latest local validation passed on 2026-05-18 with Android 16 AVD `cppbridge_android16_api36`:
  `assembleDebugAndroidTest`, `testDebugUnitTest`, full `connectedDebugAndroidTest` (`128/128`),
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
- The next Tracks/Format payload slice expands `TrackInfo` with average/peak bitrate,
  initialization/DRM counts, subsample/preroll, decoded/projection/stereo/color, max sublayers,
  PCM/encoder, tile, and crypto fields, covered by `CppBridgeConvertersTest`,
  `nativeTracksSnapshotConversionSmokeTest_returnsStructuredSummary`, and
  `nativeCurrentTracksSmokeTest_returnsTracksSummary`.
- The 2026-05-18 Stage 2 TrackInfo full-payload pass then deepens that route with label arrays,
  metadata/custom opaque tokens, initialization byte arrays, DRM scheme type plus
  uuid/license/mime/data/has-data, projection bytes, HDR static info, color bitdepth, and
  auxiliary track type. Coverage now includes
  `nativeTracksFullPayloadConversionSmokeTest_roundTripsFormatPayload` plus the converter and
  current-tracks smoke assertions.
- The 2026-05-18 Stage 3 first slice adds decoded `Bundle` extras transport for
  `MediaItem.RequestMetadata.extras` and `MediaMetadata.extras`: C++ `BundleValueInfo` and Java
  `CppBundleValue` carry string, integer-like, floating-point, boolean, and byte-array entries,
  while opaque tokens remain the fallback for arbitrary Java-only values. Coverage includes
  converter UTs and current-item / playlist-metadata native smoke markers.
- The 2026-05-18 Stage 3 timeline object-value slice adds C++ `ObjectValueInfo` descriptors for
  `Timeline.Window.uid`, `Timeline.Window.manifest`, `Timeline.Period.id`,
  `Timeline.Period.uid`, and `Timeline.Period.adsId`. The bridge now preserves null/class/type
  metadata plus stable string/number/boolean values where practical, while retaining opaque-token
  baselines. Coverage includes expanded current-timeline smoke assertions and runtime query-smoke
  markers plus a native parser smoke that exercises string/long/double/boolean/null/other and
  malformed-row fallback behavior.
- The 2026-05-18 Stage 3 MediaItem object-value slice adds Java `CppObjectValue` and C++
  `MediaItemDescriptor.tag_value` / `AdsConfigurationDescriptor.ads_id_value` for
  `MediaItem.LocalConfiguration.tag` and `MediaItem.AdsConfiguration.adsId`. Coverage includes
  converter UTs, a JNI DTO round-trip smoke, and runtime current-item / indexed-item /
  opaque-token assertions for reduced class/type/scalar metadata.
- The 2026-05-18 Stage 3 MediaMetadata object-value slice adds Java `CppObjectValue` fields and
  C++ `ObjectValueInfo` metadata for representative `MediaMetadata` text/`CharSequence` fields:
  title, artist, album title/artist, display title, subtitle, description, writer, author,
  composer, conductor, genre, compilation, and station. Coverage includes converter UTs, a JNI
  media-metadata object-value round-trip smoke, current-item / playlist-metadata runtime
  assertions, and listener metadata object-value markers.
- `CppBridgeConverters` now normalizes HLS MIME aliases like
  `application/vnd.apple.mpegurl` / lowercase `application/x-mpegurl`, and maps Media3
  `CONTENT_TYPE_OTHER` back to C++ `MediaSourceType::kProgressive`.
- The 2026-05-18 Stage 1 inventory added `scripts/cppbridge/api_parity_inventory.py` and
  `docs/cppbridge/API_PARITY_GAP_REPORT.md`. Current exact inventory status is:
  `Player`/`ExoPlayer` method gaps `0`, `Player.Listener` callback gaps `0`, and one
  `ExoPlayer.Builder` gap: `setAudioOutputProvider`.
- Direct `Player.Listener#onIsLoadingChanged` is now bridged through
  `OnIsLoadingChanged` / `nativeOnIsLoadingChanged`, with `nativeListenerSmokeTest` checking
  `isLoadingCb=1` and `isLoading=1`.
- The planned Stage 3 reduced object/value-model slices are complete. The highest-value next
  development work is Stage 4 listener/analytics completeness or later-stage full Java object
  parity for `MediaItem`, `Timeline`, `MediaMetadata`, `Tracks`, and `Cue` beyond the reduced
  descriptors. Full Java/api.txt parity for non-player classes remains a later-stage concern.

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
