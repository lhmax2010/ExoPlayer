# Project State

## Snapshot

- Workspace: `/home/linhao/Toolchain/development/ExoPlayer`
- Date of this handoff memory: `2026-05-18`
- Module focus: `libraries/exoplayer_cppbridge`
- Goal: Java-side ExoPlayer usage replaced by a reduced but usable C++ bridge/SDK surface, with smoke coverage and validation docs.

## Current status from repo docs

According to the legacy reduced-endpoint tracker in `docs/cppbridge/API_MAPPING_STATUS.md`:

- `Done`: `99`
- `Partial`: `0`
- `Not started`: `0`

Important nuance:

- These numbers describe the currently exposed reduced endpoint tracker.
- They do not mean full `api.txt` parity is complete.
- A 2026-05-15 parity expansion added more runtime setter/getter coverage after that tracker was
  created. The new APIs are documented as a parity addendum rather than folded into the old row
  count.
- Remaining work lives in the "next-phase capability gaps" and "full parity" sections of the docs.

## Validation status

Latest local validation on this machine, refreshed on `2026-05-18`:

- Build/native link: `Pass`
- JNI/value smoke: `Pass`
- Player/runtime smoke: `Pass`
- Demo build: `Pass`
- Demo manual validation: not manually exercised in this pass
- Logcat review: not separately audited in this pass

Commands that passed:

- `./gradlew :lib-exoplayer-cppbridge:assembleDebugAndroidTest`
- `./gradlew :lib-exoplayer-cppbridge:testDebugUnitTest`
- `./gradlew :lib-exoplayer-cppbridge:connectedDebugAndroidTest`
- `./gradlew :demo-cppbridge:assembleDebug`
- `git diff --check`

Interpretation:

- The current Android 16 emulator run verified the smoke suite end to end.
- The current Android 16 emulator run reported `125/125` connected instrumentation tests passed.
- Passing smoke tests proves the reduced bridge surface described by the tests, not full Java
  `api.txt` parity.
- The 2026-05-18 Stage 1 inventory report is now checked in at
  `docs/cppbridge/API_PARITY_GAP_REPORT.md`; regenerate/check with
  `python3 scripts/cppbridge/api_parity_inventory.py --write` / `--check`.
- The 2026-05-18 Stage 2 `Format` / `Tracks` slice expands `CppTrackInfo` / `TrackInfo` beyond
  scalar counts: labels, metadata/custom tokens, auxiliary type, initialization bytes, DRM
  scheme-data shape, projection bytes, and `ColorInfo` HDR fields now round-trip through Java,
  JNI, and native smoke coverage.

## Smoke footprint

Current androidTest count found in the workspace:

- Total `@Test` count across `CppBridgeNativeSmokeTest.java` and
  `CppBridgeNativePlayerInstrumentationTest.java`: `125`

This is consistent with a large smoke-first validation strategy.

## High-value implemented areas visible in current workspace

- Core playback lifecycle and seek/navigation
- Playlist mutation and query APIs
- Track selection parameters, tracks snapshots, timeline snapshots, cue snapshots
- Media metadata and playlist metadata round-trip
- Rich analytics reduced coverage, including many concrete event callbacks
- Image output reduced callback path
- PriorityTaskManager reduced wrapper path
- Preload reduced target-duration path
- Player message reduced send/cancel/runtime smoke path
- Opaque token baseline support for representative object identity fields
- Runtime parity setters/getters added in this pass:
  `SetHandleAudioBecomingNoisy`, `SetForegroundMode`, `SetPauseAtEndOfMediaItems`,
  runtime seek increments, max-seek-to-previous-position, video scaling mode, and video
  change-frame-rate strategy
- Advanced audio/device/scrubbing/codec APIs added in this pass:
  audio session ID, aux effect info, preferred audio device, virtual device ID, scrubbing mode
  parameters, and audio/video codec parameter maps
- Renderer/device-state getters added in this pass:
  renderer count/type, sleeping-for-offload, tunneling enabled, and released state
- Callback-style ExoPlayer extension APIs added in this pass:
  `CodecParametersChangeListener`, `VideoFrameMetadataListener`, and `CameraMotionListener`
  reduced C++ registration plus callback dispatch
- Codec-parameter multi-listener immediate notification semantics are now covered by
  `nativeCodecParametersMultiListenerParitySmokeTest_routesImmediateCallbacks`
- HTTP progressive, HLS, and DASH playback through C++ `SetMediaItem` / `Prepare` / `Play` are
  covered by `nativeHttpHlsDashPlaybackSmokeTest_preparesLocalStreamsThroughCppApi`
- `CppBridgeConverters` now maps Media3 `CONTENT_TYPE_OTHER` back to C++
  `MediaSourceType::kProgressive`, so HTTP/progressive current-item snapshots no longer collapse
  back to `kDefault`
- A follow-up C++ API / CppBridge coverage audit found no remaining exact public-method gaps after
  adding direct smoke markers for raw `Surface`, playlist mutation/navigation, tracks getters,
  device volume/mute setters, codec-parameter bridge registration/clear, builder
  `SetMediaSourceFactoryConfig`, and SDK `ClearPriorityTaskManager`
- The next full-parity slice started with `VideoFrameMetadataListener` payload depth:
  `VideoFrameMetadataSnapshot` now carries representative `MediaFormat` mime, size, frame-rate,
  rotation, and color metadata in addition to the previous presence/summary baseline
- Review follow-up closed the video-frame metadata fallback edge case: C++ `format_bitrate` now
  reaches Java simulation as average bitrate when average/peak are unset, and absent color/audio
  shape fields preserve `Format.NO_VALUE` semantics.
- The next Tracks/Format payload slice expanded `TrackInfo` with average/peak bitrate,
  initialization/DRM counts, subsample/preroll, decoded/projection/stereo/color, max sublayers,
  PCM/encoder, tile, and crypto fields, covered by Java converter, JNI conversion, and native
  current-tracks smoke tests.
- Stage 2 then deepened the same `TrackInfo` route with label arrays, metadata/custom-data tokens,
  initialization byte vectors, DRM scheme-data uuid/license/mime/bytes/has-data, projection byte
  vectors, auxiliary track type, and `ColorInfo` HDR static info plus luma/chroma bitdepth.
- Stage 1 full API inventory closed the direct `Player.Listener#onIsLoadingChanged` gap through
  C++ `OnIsLoadingChanged` and Java `nativeOnIsLoadingChanged`; current exact gap report shows
  `Player`/`ExoPlayer` method gaps `0`, direct `Player.Listener` callback gaps `0`, and one
  remaining `ExoPlayer.Builder` gap: `setAudioOutputProvider`.

## Practical conclusion

For the next AI:

- Treat this project as "reduced endpoint is broad and heavily smoke-documented".
- Treat the current local environment as validated for the smoke suite on Android 16.
- The highest-risk next work moved past the first callback-style bridge slice and the
  codec-parameter multi-listener immediate-notification edge case; remaining work is deeper
  full-object parity and richer payload fidelity beyond the reduced callback descriptors.
