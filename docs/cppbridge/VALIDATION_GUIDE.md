# C++ Bridge Validation Guide

Last updated: 2026-05-15

This guide is for moving the current `exoplayer_cppbridge` work to another machine and validating
that the JNI bridge, C++ API surface, smoke coverage, and demo app all behave as expected.

## 0. Read This First

Two scope notes matter before you start:

1. The current tracker treats the exposed reduced endpoint as `Done`.
2. The bridge is not yet at full Java `api.txt` parity for every object family.

In practice:

- passing smoke tests proves the current reduced endpoint still behaves as documented
- passing smoke tests does not, by itself, prove full Java parity
- demo checks are meant to help manual analysis, not replace instrumentation coverage

Current readiness snapshot:

- test entry points are present for JNI/value smoke and player/runtime smoke
- negative smoke coverage now includes double-release and listener lifecycle mutation checks
- helper scripts are present for full validation and demo launch
- demo now exposes manual queries for tracks, current item, timeline, metadata, and cues
- native build now separates core bridge code from smoke/player-test/demo entrypoints through
  `exoplayer_cppbridge_jni` and `exoplayer_cppbridge_jni_testhooks`
- explicit opaque-token cleanup helpers now exist for the main C++ query APIs, but they are
  developer-facing convenience helpers rather than a separately validated tester flow in this pass
- latest local validation on 2026-05-15 passed on the Android 16 AVD
  `cppbridge_android16_api36`
- the current connected suite contains 124 instrumentation tests after the runtime/audio/codec/
  auxiliary-callback and HTTP/HLS/DASH playback parity addenda

## 1. Validation Goals

The new environment must verify four things:

1. Native library builds and links into `lib-exoplayer-cppbridge`.
2. JNI smoke and player instrumentation tests pass on device/emulator.
3. Demo app launches and exercises the C++ bridge end to end.
4. Human reviewers can compare actual output against expected smoke summaries.

## 2. Required Environment

- JDK compatible with the project Gradle configuration
- Android SDK with platform tools
- Android NDK and CMake available to Gradle
- One Android device or emulator online in `adb devices`
- Network access for demo/sample media URLs

Recommended checks:

```bash
java -version
adb version
adb devices
./gradlew -version
```

Windows note:

- `scripts/cppbridge/run_validation.py` and `scripts/cppbridge/launch_demo.py` now default to
  `gradlew.bat` on Windows and `./gradlew` elsewhere.

## 2A. Server Migration Checklist

Use this checklist when moving the repository to a new build or validation server.

1. Sync the repository, including `scripts/cppbridge/`, `docs/cppbridge/`, and `demos/cppbridge/`.
2. Confirm the server has:
   - JDK
   - Android SDK
   - Android NDK
   - CMake
   - at least one online emulator or device
3. Provide SDK discovery by either:
   - setting `ANDROID_SDK_ROOT`, or
   - creating `local.properties`
4. Confirm `gradlew` / `gradlew.bat` is executable on the target OS.
5. Run a build-only smoke first:

```bash
./gradlew :lib-exoplayer-cppbridge:assembleDebugAndroidTest :demo-cppbridge:installDebug
```

Windows:

```powershell
gradlew.bat :lib-exoplayer-cppbridge:assembleDebugAndroidTest :demo-cppbridge:installDebug
```

6. Run instrumentation only after the build-only smoke succeeds.
7. Launch the demo only after instrumentation starts passing.

Optional packaging check:

- for production-only native packaging, verify that the core target still builds with:

```bash
./gradlew :lib-exoplayer-cppbridge:assemble -PcppbridgeIncludeTestEntrypoints=OFF
```

If the server is missing Python, use the shell scripts or manual Gradle commands instead of the
Python wrappers.

## 3. Directly Runnable Scripts

Scripts are in `scripts/cppbridge/`.
Run results can be written back into:
`docs/cppbridge/TEST_RESULTS_TEMPLATE.md`
Final rollup can be summarized in:
`docs/cppbridge/VALIDATION_RESULTS_SUMMARY.md`

Recommended write-back order:

1. Fill `TEST_RESULTS_TEMPLATE.md` with raw command results and observed markers.
2. Fill `VALIDATION_RESULTS_SUMMARY.md` with the pass/fail rollup and release decision.

Choose one validation implementation based on what you prefer:

1. Use `run_validation.sh` or `run_validation.py` for the normal full validation flow.
2. Use `launch_demo.sh` or `launch_demo.py` only when you want demo install/launch verification without running the full smoke flow.

### Full validation

```bash
./scripts/cppbridge/run_validation.sh
python3 ./scripts/cppbridge/run_validation.py
```

Useful options:

```bash
./scripts/cppbridge/run_validation.sh --serial emulator-5554
./scripts/cppbridge/run_validation.sh --launch-demo
./scripts/cppbridge/run_validation.sh --test-class androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest
python3 ./scripts/cppbridge/run_validation.py --serial emulator-5554
python3 ./scripts/cppbridge/run_validation.py --launch-demo
python3 ./scripts/cppbridge/run_validation.py --test-class androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest
```

### Demo only

```bash
./scripts/cppbridge/launch_demo.sh
python3 ./scripts/cppbridge/launch_demo.py
```

Do not treat all script variants as mandatory for every run:

- Normal validation run: choose one of `run_validation.sh` or `run_validation.py`
- Demo-only manual check: choose one of `launch_demo.sh` or `launch_demo.py`

Recommended execution order for a fresh environment:

1. Run one full validation script variant.
2. If smoke passes, launch the demo.
3. Compare the observed demo status output against the checklist below.
4. Write raw outputs into `TEST_RESULTS_TEMPLATE.md`.
5. Write the overall pass/fail judgment into `VALIDATION_RESULTS_SUMMARY.md`.

## 4. Manual Gradle Commands

```bash
./gradlew :lib-exoplayer-cppbridge:assembleDebugAndroidTest
./gradlew :lib-exoplayer-cppbridge:testDebugUnitTest
./gradlew :lib-exoplayer-cppbridge:assemble -PcppbridgeIncludeTestEntrypoints=OFF
./gradlew :lib-exoplayer-cppbridge:connectedDebugAndroidTest -Pandroid.testInstrumentationRunnerArguments.class=androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest
./gradlew :lib-exoplayer-cppbridge:connectedDebugAndroidTest -Pandroid.testInstrumentationRunnerArguments.class=androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest
./gradlew :lib-exoplayer-cppbridge:connectedDebugAndroidTest
./gradlew :demo-cppbridge:installDebug
adb shell am start -n androidx.media3.demo.cppbridge/.MainActivity
```

Recommended manual order:

1. `assembleDebugAndroidTest`
2. `testDebugUnitTest`
3. optional production-only build with `-PcppbridgeIncludeTestEntrypoints=OFF`
4. `CppBridgeNativeSmokeTest`
5. `CppBridgeNativePlayerInstrumentationTest`
6. full `connectedDebugAndroidTest`
7. `:demo-cppbridge:installDebug`
8. demo manual checklist

## 5. Smoke Classes To Run

### JNI/value conversion smoke

Path:
`libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativeSmokeTest.java`

Important negative smoke in this class now includes:

- double-release idempotence
- listener lifecycle mutation safety
- repeated/null analytics listener removal
- explicit opaque-token batch release through the native SDK wrapper

### Player/runtime smoke

Path:
`libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerInstrumentationTest.java`

Native target note:

- `exoplayer_cppbridge_jni` contains the production bridge core
- `exoplayer_cppbridge_jni_testhooks` contains smoke, player-test, and demo JNI entrypoints
- the default validation build includes both targets

Current high-value parity smokes in this class include:

- `nativeRuntimeControlParitySmokeTest_updatesPhaseOneRuntimeControls`
- `nativeAudioAndScrubbingParitySmokeTest_updatesAdvancedRuntimeControls`
- `nativeCodecParametersParitySmokeTest_setsAudioAndVideoCodecParameters`
- `nativeRendererAndDeviceStateGetterSmokeTest_readsRendererAndDeviceState`
- `nativeHttpHlsDashPlaybackSmokeTest_preparesLocalStreamsThroughCppApi`

## 6. Human Acceptance Checklist

The run is acceptable if all of the following are true:

1. Both instrumentation classes pass without crashes or hangs.
2. No JNI exception spam appears in logcat for the bridge tag.
3. Demo app launches and renders media in `PlayerView`.
4. Demo buttons complete without native crash:
   - `Play`
   - `Pause`
   - `Seek 30s`
   - `Playlist`
   - `Volume`
   - `Speed`
   - `Load Subtitle`
   - `Prefer Text`
   - `Tracks`
   - `Item`
   - `Timeline`
   - `Metadata`
   - `Cues`
5. `Tracks` button prints a non-empty summary in the demo status area.
6. `Item`, `Timeline`, `Metadata`, and `Cues` each print a non-empty summary in the demo status area.

## 7. Demo Validation Checklist

Demo path:
`demos/cppbridge/src/main/java/androidx/media3/demo/cppbridge/MainActivity.java`

Manual steps:

1. Launch the app.
2. Confirm the default URL is prefilled.
3. Tap `Play` and confirm video starts.
4. Tap `Pause` and confirm playback stops.
5. Tap `Seek` and confirm playback jumps near 30 seconds.
6. Tap `Playlist` and confirm no crash and playback remains usable.
7. Tap `Load Subtitle` and confirm status text updates.
8. Tap `Prefer Text` and confirm status text updates.
9. Tap `Tracks` and confirm the returned summary is not empty.
10. Tap `Item` and confirm the returned summary contains at least `mediaId=` and `uri=`.
11. Tap `Timeline` and confirm the returned summary contains at least `windowCount=` and `window0MediaId=`.
12. Tap `Metadata` and confirm the returned summary contains at least `title=` and `artworkUri=`.
13. Tap `Cues` after loading subtitles and confirm the returned summary contains at least `cueCount=`.
14. Close the app and confirm no teardown crash.

## 8. Expected High-Signal Output Examples

Markers that should appear:

- `lifecycle-ok`
- `released=1`
- `state=1`
- `window0MediaId=timeline-query-item-1`
- `timelineWindow0MediaId=listener-item-1`
- `timelineWindow0MediaUri=https://example.com/listener.mp4`
- `cue0Text=Listener Cue 1`
- `cue1Text=Listener Cue 2`
- `oldTagTokenPresent=1`
- `newTagTokenPresent=1`
- `mediaTitle=Video Metadata Title`
- `mediaArtworkUri=https://example.com/video-metadata-artwork.jpg`
- `playlistArtworkUri=https://example.com/metadata-playlist-artwork.jpg`
- `sourceType=2`
- `httpPrepared=1`
- `httpAdvanced=1`
- `httpSourceType=5`
- `hlsPrepared=1`
- `hlsAdvanced=1`
- `hlsSourceType=2`
- `dashPrepared=1`
- `dashAdvanced=1`
- `dashSourceType=1`
- `analyticsCb=2`
- `bufferSize=4096`
- `droppedFrames=8`
- `bitrateEstimate=999999`
- `uri=https://example.com/analytics-final.m3u8`
- `uri=https://example.com/analytics-final-complete.m3u8`
- `sampleMimeType=audio/final`
- `decoderName=c2.android.eac3.decoder`
- `decoderName=c2.android.hevc.decoder`
- `decoderName=c2.android.eac3.decoder`
- `decoderName=c2.android.hevc.decoder`
- `renderTimeMs=456`
- `pixelWidthHeightRatio=1.250000`
- `playoutStartSystemTimeMs=2222`
- `totalProcessingOffsetUs=67890`
- `frameCount=8`
- `volume=0.750000`
- `audioSessionId=700042`
- `skipSilenceEnabled=1`
- `volume=7`
- `muted=0`
- `playbackState=3`
- `isPlaying=1`
- `playWhenReady=1`
- `reason=2`
- `playbackSuppressionReason=1`
- `isLoading=1`
- `repeatMode=2`
- `shuffleModeEnabled=1`
- `sampleMimeType=video/final`
- `speed=1.500000`
- `pitch=0.750000`
- `commandCount=3`
- `contains8=1`
- `eventCount=3`
- `contains9=1`
- `playbackType=1`
- `routingControllerId=route-final`
- `title=Analytics Media Final`
- `displayTitle=Analytics Display Final`
- `title=Analytics Playlist Final`
- `displayTitle=Analytics Playlist Display Final`
- `seekBackIncrementMs=15000`
- `seekForwardIncrementMs=25000`
- `maxSeekToPreviousPositionMs=12000`
- `nativeAnalyticsTimelineChangedSmokeTest_reportsConcreteAnalyticsEvent` -> `reason=2`
- `nativeAnalyticsPositionDiscontinuitySmokeTest_reportsConcreteAnalyticsEvent` -> `reason=5`
- `nativeAnalyticsSeekStartedSmokeTest_reportsConcreteAnalyticsEvent` -> `started=1`
- `nativeAnalyticsPlayerErrorSmokeTest_reportsConcreteAnalyticsEvent` -> `errorCode=2002`; `message=analytics-final-error`
- `nativeAnalyticsPlayerErrorChangedSmokeTest_reportsConcreteAnalyticsEvent` -> `errorCode=4004`; `message=analytics-final-changed`
- `nativeAnalyticsTracksChangedSmokeTest_reportsConcreteAnalyticsEvent` -> `groupCount=2`; `firstGroupId=video-main`; `containsVideo=1`
- `nativeAnalyticsMediaItemTransitionSmokeTest_reportsConcreteAnalyticsEvent` -> `mediaId=analytics-transition-final`; `sourceType=2`; `reason=2`
- `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` -> `cueCount=2`; `cuePresentationTimeUs=456789`; `cue0Text=Query Cue 1`; `cue0TextTokenPresent=1`; `cue0BitmapTokenPresent=1`; `cue1Text=Query Cue 2`; `cue1TextTokenPresent=1`; `cue1BitmapTokenPresent=0`; `tracksGroupCount=`; `trackGroupVectorCount=`; `bridgeTracksGroupCount=`; `bridgeTrackGroupVectorCount=`
- `nativeCurrentTracksSmokeTest_returnsTracksSummary` -> `track0AverageBitrate=2000000`; `track0PeakBitrate=2500000`; `track0RotationDegrees=90`; `track0PixelRatio=1.250000`; `track0Color=1:2:3`; `group1Track0AverageBitrate=160000`; `group1Track0PeakBitrate=192000`
- `nativeTracksSnapshotConversionSmokeTest_returnsStructuredSummary` -> `track0AverageBitrate=2000000`; `track0PeakBitrate=2500000`; `track0RotationDegrees=90`; `track0PixelRatio=1.250000`; `track0Color=1:2:3`; `group1Track0AverageBitrate=160000`; `group1Track0PeakBitrate=192000`
- `nativePlaylistMutationSmokeTest_returnsUpdatedPlaylistState` -> `moveRangeFirstMediaId=item-4`; `singleRemoveRestoredCount=3`; `nextIndex=1`; `previousIndex=-1`; `hasNext=1`; `hasPrevious=0`
- `nativeDeviceAndSkipSilenceSmokeTest_returnsDeviceSummary` -> `deviceControlCalls=1`
- `nativeAuxiliaryCallbackParitySmokeTest_reportsCodecVideoAndCameraCallbacks` -> `bridgeCodecRegistrationSafe=1`; `frameLabel=Main Camera`; `frameContainerMime=video/mp4`; `frameBitrates=333000:222000:333000`; `frameRotation=180`; `framePixelRatio=1.500000`; `frameColor=1:2:3`; `frameAudioShape=2:48000`; `frameFlags=5:7`; `frameMediaFormatMime=video/avc`; `frameMediaFormatSize=1920x1080`; `frameMediaFormatFrameRate=23.976000`; `frameMediaFormatRotation=90`; `frameMediaFormatColor=1:2:3`
- `nativeVideoFrameMetadataSimulationFallbackSmokeTest_preservesFallbackFields` -> `fallbackBitrates=123000:123000:-1`; `fallbackColor=1:-1:-1`; `fallbackAudioShape=-1:-1`; `fallbackApplied=1`
- `nativePriorityTaskManagerWrapperSmokeTest_returnsStructuredSummary` -> `sdkClearPriorityTaskManagerSafe=1`
- `nativeAnalyticsCuesSmokeTest_reportsConcreteAnalyticsEvent` -> `cueCount=2`; `presentationTimeUs=654321`; `text0=Analytics Cue Final`; `text0TokenPresent=1`; `bitmap0TokenPresent=1`; `text1=Analytics Cue Final 2`; `text1TokenPresent=1`; `bitmap1TokenPresent=0`
- `nativeAnalyticsMetadataSmokeTest_reportsConcreteAnalyticsEvent` -> `entryCount=2`; `firstEntryType=MdtaMetadataEntry`; `firstEntryText=analytics-metadata-final`
- `nativeAnalyticsLoadErrorSmokeTest_reportsConcreteAnalyticsEvent` -> `uri=https://example.com/analytics-error-final.m3u8`; `dataType=4`; `trackType=2`; `message=analytics-load-final`; `wasCanceled=0`
- `imageCount=3`

Additional high-signal markers worth checking for the newest reduced-scope work:

- `factoryToken=test-injected-media-source-factory`
- `injectedFactoryUsed=1`
- `fallbackApplied=1`
- `tokensIsolated=1`
- `beforeRemoveCb=2`
- `callbackStopped=1`
- `bufferSizeMs=87`
- `elapsedSinceLastFeedMs=23`
- `elapsedMs=41`
- `bytesTransferred=67890`
- `retryCount=2`
- `dataType=4`
- `sampleRate=48000`
- `initializedTimestampMs=222`
- `initializationDurationMs=19`
- `initializedTimestampMs=444`
- `initializationDurationMs=29`
- `frameRate=59.939999`
- `lastAllocationByteCount=384`
- `lastRowBytes=48`
- `lastIsPremultiplied=1`
- `reattachLastBitmapConfig=RGB_565`
- `reattachLastHasAlpha=0`
- demo `Tracks` query -> `groupCount=` / `audioSelected=`
- demo `Item` query -> `mediaId=` / `subtitleCount=`
- demo `Timeline` query -> `windowCount=` / `window0MediaId=`
- demo `Metadata` query -> `title=` / `artworkUri=`
- demo `Cues` query -> `cueCount=`
- `afterReapply=effectCount=7`
- `effect3=rgbAdjustment:redScale=1.0:greenScale=1.0:blueScale=1.0`
- `effect4=scaleAndRotate:scaleX=1.0:scaleY=1.0:rotationDegrees=0.0`
- `effect6=presentation:width=320:height=240:layout=1`

## 9. Useful Logcat Filter

```bash
adb logcat | grep -E "cppbridge|CppExoPlayerBridge|exoplayer_cppbridge"
```
