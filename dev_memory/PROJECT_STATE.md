# Project State

## Snapshot

- Workspace: `/home/linhao/Toolchain/development/ExoPlayer`
- Date of this handoff memory: `2026-05-19`
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

Latest current-turn validation on this machine, refreshed on `2026-05-19`:

- Build/native link: `Pass`
- JVM unit tests: `Pass`
- AndroidTest package build: `Pass`
- Demo build: `Pass`
- Connected instrumentation: `Pass`; Android 16 AVD `emulator-5554` passed `139/139` connected
  tests (`27/27` JNI/value smoke plus `112/112` player/runtime smoke).
- Demo manual validation: previous Stage 6 demo UI smoke installed and launched `MainActivity` on
  Android 16 emulator; the current demo subtitle/audio update has been package-built but still needs
  a connected AVD or RPI4 manual playback pass.

Commands that passed:

- `./gradlew :demo-cppbridge:assembleDebug :lib-exoplayer-cppbridge:assembleDebugAndroidTest --console=plain`
- `./gradlew :lib-exoplayer-cppbridge:testDebugUnitTest --console=plain`
- `ANDROID_SDK_ROOT=$HOME/Android/Sdk PATH=$HOME/Android/Sdk/platform-tools:$HOME/Android/Sdk/emulator:$PATH bash scripts/cppbridge/run_validation.sh --serial emulator-5554`
- `adb -s emulator-5554 shell am instrument -w -r -e class androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest androidx.media3.exoplayer.cppbridge.test/androidx.test.runner.AndroidJUnitRunner`
- `python3 scripts/cppbridge/api_parity_inventory.py --check`
- `git diff --check`

Interpretation:

- The current workspace contains the new demo multi-subtitle smoke, and the Android 16 connected
  baseline is now verified at `139/139`.
- RPI4 board validation is the next manual device gate; use `docs/cppbridge/RPI4_TESTING_GUIDE.md`
  when board access returns.
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
  `CppBridgeNativePlayerInstrumentationTest.java`: `139`
- Total `@Test` count across cppbridge Android instrumentation and JVM unit sources: `170`

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
- Stage 5 has started with a deterministic custom `MediaSource.Factory` playback smoke:
  `nativeCustomMediaSourceFactoryPlaybackSmokeTest_preparesSmoothAndRtspViaCppConfig` registers a
  Java `FakeMediaSourceFactory`, passes the token through C++ `PlayerConfig`, and prepares
  SmoothStreaming / RTSP `MediaItemDescriptor` source types via the C++ `SetMediaItem` /
  `Prepare` / `Play` path.
- Stage 5 also covers HTTP data-source config in real playback:
  `nativeHttpDataSourceConfigPlaybackSmokeTest_sendsHeadersThroughCppConfig` sets request headers,
  user agent, timeout, and redirect config from C++, then verifies the local MockWebServer request
  receives the expected headers and UA.
- A follow-up C++ API / CppBridge coverage audit found no remaining exact public-method gaps
  at the method-name / alias level after
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
- Stage 3 has started with `MediaItem.RequestMetadata.extras` and `MediaMetadata.extras` decoded
  `Bundle` value transport: C++ `BundleValueInfo` and Java `CppBundleValue` now cover stable
  string, integer-like, floating-point, boolean, and byte-array entries while preserving opaque
  token fallback for arbitrary Java-only values.
- Stage 3 now also deepens timeline object identity fields: C++ `ObjectValueInfo` captures
  presence, class name, reduced value type, and stable string/number/boolean payloads for
  `Timeline.Window.uid`, `Timeline.Window.manifest`, `Timeline.Period.id`,
  `Timeline.Period.uid`, and `Timeline.Period.adsId`, while retaining opaque-token identity
  baselines.
- Stage 3 now also deepens `MediaItem.LocalConfiguration.tag` and
  `MediaItem.AdsConfiguration.adsId`: Java `CppObjectValue` plus C++ `ObjectValueInfo` metadata
  expose reduced class/type/scalar payload visibility while retaining existing string fallback and
  opaque-token identity behavior.
- Stage 3 now also deepens representative `MediaMetadata` text/`CharSequence` fields: Java
  `CppMediaMetadata` exposes `CppObjectValue` fields for title, artist, album title/artist,
  display title, subtitle, description, writer, author, composer, conductor, genre, compilation,
  and station; C++ `MediaMetadataSnapshot` mirrors them through `ObjectValueInfo` while preserving
  token-first and string fallback semantics.
- Stage 1 full API inventory closed the direct `Player.Listener#onIsLoadingChanged` gap through
  C++ `OnIsLoadingChanged` and Java `nativeOnIsLoadingChanged`; Stage 6 then closed the remaining
  exact `ExoPlayer.Builder#setAudioOutputProvider` inventory gap with a token-injected
  app-owned `AudioOutputProvider` path. The audio-output-provider registry now mirrors the media
  source factory token registry behavior for explicit tokens, generated token reuse, and null/empty
  fallback. Current exact gap report shows `Player`/`ExoPlayer`, `ExoPlayer.Builder`, and direct
  `Player.Listener` gaps all at `0`.
- Stage 6 stabilization also deduplicates C++ opaque-token collection and smoke-covers
  `OpaqueTokenBatch::Release` clear/idempotency behavior, while keeping cleanup explicit.
- Stage 6 validation scripts now have a `--local-only` path for no-device environments, covering
  API inventory, JVM UT, AndroidTest packaging, and demo packaging in one command; the connected
  path now verifies the requested `--serial` is actually online before running Gradle.
- Android 16/API 36 connected validation is clean for the current Stage 6 baseline: `27/27`
  JNI/value smoke, `112/112` player/runtime smoke, `139/139` aggregate.
- Stage 4 has completed reduced `AnalyticsListener` method-name coverage. It started with the
  independent `AnalyticsListener#onAudioAttributesChanged` callback carrying
  `AudioAttributesDescriptor`, then added 25 remaining reduced analytics callbacks for load,
  format, decoder-counters, audio/video error, DRM, renderer-ready, scrubbing, and player-release
  events. Coverage includes `nativeAnalyticsAudioAttributesChangedSmokeTest_reportsConcreteAnalyticsEvent`
  and `nativeAnalyticsStage4RemainingCallbacksSmokeTest_reportsConcreteAnalyticsEvents`.

## Practical conclusion

For the next AI:

- Treat this project as "reduced endpoint is broad and heavily smoke-documented".
- Treat the current local environment as validated for the smoke suite on Android 16.
- Treat RPI4 validation as pending manual execution, with the checklist in
  `docs/cppbridge/RPI4_TESTING_GUIDE.md`.
- The highest-risk next work moved past the first callback-style bridge slice, the
  codec-parameter multi-listener immediate-notification edge case, the planned Stage 3 reduced
  object/value-model slices, and Stage 4 reduced analytics callback completeness; remaining work is
  Stage 5 playback/source integration plus full-object parity for `MediaItem`, `Timeline`,
  `MediaMetadata`, `Tracks`, and richer payload fidelity beyond the reduced descriptors.
