# API Mapping Status

Last updated: 2026-05-19

Current reduced-endpoint tracker totals:

- `Done`: 99
- `Partial`: 0
- `Not started`: 0

Current review note:

- This tracker currently marks the exposed reduced endpoint, not full `api.txt` parity.
- The original row count above is kept as the legacy reduced-endpoint tracker. Additional
  2026-05-15 parity addendum rows are documented below and covered by smoke tests, but are not
  folded into that older total.
- The reduced families below are smoke-closed for their intended endpoint.
- There are no remaining row-level `Partial` items for the currently exposed reduced endpoint.
- Remaining work is tracked below as full-`api.txt` parity or next-phase capability gaps.
- New capability gaps now live under the remaining-work sections rather than as separate
  `Not started` tracker rows.
- Reduced `AnalyticsListener` coverage now extends beyond the aggregate snapshot into seventy-one
  concrete reduced event paths: audio underrun, dropped video frames, bandwidth estimate, load
  started, load completed, audio input format changed, audio decoder initialized, video decoder
  initialized, audio decoder released, video decoder released, analytics rendered first frame,
  analytics video size changed, audio position advancing, video frame processing offset, volume
  changed, audio session id changed, audio attributes changed, skip silence enabled changed, device
  volume changed, playback state changed, is playing changed, play-when-ready changed, playback
  suppression reason changed, and video input format changed.
- The newest reduced `AnalyticsListener` concrete events are playback parameters changed, available
  commands changed, analytics events batch delivery, seek-back-increment changed,
  seek-forward-increment changed, max-seek-to-previous-position changed, device info changed,
  media metadata changed, playlist metadata changed, player-error, player-error-changed,
  analytics-tracks-changed, analytics-media-item-transition, analytics-cues, and the Stage 4
  remaining-callback batch covering player-state/loading aliases, track-selection parameters, load
  canceled, downstream/upstream format events, decoder-counters enabled/disabled events,
  audio/video error events, audio-track init/release, surface size, DRM lifecycle/key events,
  renderer-ready, dropped-seeks-while-scrubbing, and player-released callbacks.
- Stage 5 source/runtime integration has started with a deterministic custom
  `MediaSource.Factory` playback smoke for SmoothStreaming and RTSP `MediaItemDescriptor` source
  types routed through C++ factory-token config, plus an HTTP data-source config playback smoke
  that verifies C++ headers and User-Agent on a real local progressive request.
- Timeline period snapshots now also preserve an opaque-token baseline for `period.uid`, with
  direct smoke visibility in both runtime timeline query and listener payload capture flows.
- Media metadata snapshots now preserve an opaque-token baseline for representative
  `CharSequence` identity fields (`title`, `artist`, `displayTitle`), with direct smoke visibility
  in query, playlist-metadata round-trip, and dedicated opaque-token playlist metadata smoke.
- Timeline window snapshots now preserve a manifest presence/string baseline plus opaque-token
  baseline, with direct smoke visibility in runtime timeline query and listener payload capture.
- Timeline object identity rows now also expose reduced `ObjectValueInfo` descriptors for window
  `uid`/`manifest` and period `id`/`uid`/`adsId`, with direct string/null smoke visibility in
  runtime timeline query and lighter query-smoke markers.
- MediaItem local `tag` and ads `adsId` now also expose reduced object-value metadata through
  `CppObjectValue` / `ObjectValueInfo`, with Java converter UTs plus JNI and runtime smoke
  visibility while retaining existing opaque-token baselines.
- MediaMetadata representative text/`CharSequence` fields now also expose reduced object-value
  metadata through `CppObjectValue` / `ObjectValueInfo`, with converter UTs, JNI DTO round-trip,
  runtime metadata query, playlist metadata, and listener smoke visibility while retaining
  token-first behavior.
- Track snapshots now preserve an opaque-token baseline for representative `label` identity, with
  direct smoke visibility in runtime tracks query, JNI tracks conversion smoke, and listener
  payload capture.
- 2026-05-15 runtime parity addendum: C++ now exposes runtime noisy handling, foreground mode,
  pause-at-end-of-media-items, seek increment setters, max-seek-to-previous setter, video scaling
  mode, video change-frame-rate strategy, advanced audio/device/scrubbing controls, audio/video
  codec parameter maps, renderer count/type getters, and offload/tunneling/released state getters.
  These are covered by Android instrumentation smokes and passed in the full Android 16 connected
  suite.
- 2026-05-15 callback addendum: reduced C++ listener APIs now cover
  `CodecParametersChangeListener`, `VideoFrameMetadataListener`, and `CameraMotionListener`,
  including registration, callback dispatch, remove-listener stop behavior, and codec-parameter
  multi-listener immediate-callback routing.

Smoke reference conventions:

- JNI/value smoke:
  `libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativeSmokeTest.java`
- Player/runtime smoke:
  `libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerInstrumentationTest.java`

## 0. Direct Implementation Lookup

Use this section when you already know an API name and want to jump straight to the code that
implements it.

Reading rule:

- `Public C++ API`: the method exposed to native callers through `ExoPlayerSdkPlayer`
- `Bridge implementation`: the `JniExoPlayerBridge` method that marshals into Java
- `Java runtime`: the `CppExoPlayerBridge` method that actually talks to Media3 `ExoPlayer`
- `Conversion/helper layer`: the place where DTO conversion or supporting parsing happens

| API family / search key | Public C++ API | Bridge implementation | Java runtime | Conversion / helper layer |
| --- | --- | --- | --- | --- |
| media item set/add/remove/replace/clear | `include/exoplayer_sdk.h`: `SetMediaItem`, `SetMediaItems`, `AddMediaItem(s)`, `RemoveMediaItem(s)`, `ReplaceMediaItem(s)`, `ClearMediaItems` | `exoplayer_cppbridge_jni_bridge.cpp`: same method names on `JniExoPlayerBridge` | `CppExoPlayerBridge.java`: `setMediaItem`, `setMediaItems`, `addMediaItem(s)`, `removeMediaItem(s)`, `replaceMediaItem(s)`, `clearMediaItems` | `CppBridgeConverters.java` + `exoplayer_cppbridge_jni_common.cpp`: `toMediaItem` / `fromMediaItem`, `CreateJavaMediaItem`, `CreateJavaMediaItemArray`, `FromJavaMediaItem` |
| playback lifecycle | `Prepare`, `Play`, `Pause`, `Stop`, `Release` | `exoplayer_cppbridge_jni_bridge.cpp`: lifecycle methods on `JniExoPlayerBridge` | `CppExoPlayerBridge.java`: `prepare`, `play`, `pause`, `stop`, `release` | `exoplayer_sdk.cpp`: public wrapper semantics and listener teardown |
| seek/navigation | `SeekTo*`, `SeekBack`, `SeekForward`, `SeekToNext*`, `SeekToPrevious*` | `exoplayer_cppbridge_jni_bridge.cpp`: seek/navigation methods | `CppExoPlayerBridge.java`: `seekTo*`, `seekBack`, `seekForward` | `exoplayer_cppbridge_jni_common.cpp`: `FromJavaSeekParameters`, `CreateJavaIntArray` and related helpers where needed |
| seek parameters | `SetSeekParameters`, `GetSeekParameters` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: `setSeekParameters`, `getSeekParameters` | `CppBridgeConverters.java` + `exoplayer_cppbridge_jni_common.cpp`: `toSeekParameters`, `fromSeekParameters`, `FromJavaSeekParameters` |
| track selection parameters | `SetTrackSelectionParameters`, `GetTrackSelectionParameters` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: `setTrackSelectionParameters`, `getTrackSelectionParameters` | `CppBridgeConverters.java` + `exoplayer_cppbridge_jni_common.cpp`: `toTrackSelectionParameters`, `fromTrackSelectionParameters`, `FromJavaTrackSelectionParameters` |
| current tracks | `GetTracks`, `GetTrackGroups` | `exoplayer_cppbridge_jni_bridge.cpp`: `GetTracksSnapshot`, `GetTrackGroups` | `CppExoPlayerBridge.java`: `getTracks` | `CppBridgeConverters.java` + `exoplayer_cppbridge_jni_common.cpp`: `toCppTrackGroups`, `fromTracks`, `CreateJavaTracks`, `FromJavaTracks` |
| current media item / playlist metadata / media metadata | `GetCurrentMediaItem`, `GetMediaItemAt`, `GetMediaMetadata`, `GetPlaylistMetadata`, `SetPlaylistMetadata` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: `getCurrentMediaItem`, `getMediaItemAt`, `getMediaMetadata`, `getPlaylistMetadata`, `setPlaylistMetadata` | `CppBridgeConverters.java` + `exoplayer_cppbridge_jni_common.cpp`: `fromMediaItem`, `toMediaItem`, `fromMediaMetadata`, `toMediaMetadata`, `CreateJavaMediaItem`, `CreateJavaMediaMetadata` |
| timeline queries | `GetTimeline`, `GetTimelineSnapshot`, `GetTimelineWindows`, `GetTimelinePeriods` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: `getTimelineSnapshotData`, `getTimelineWindowRows`, `getTimelinePeriodRows` | `exoplayer_cppbridge_jni_common.cpp`: `SplitString`, numeric parsers, timeline DTO helpers |
| cues queries | `GetCurrentCues` | `exoplayer_cppbridge_jni_bridge.cpp`: `GetCurrentCues` | `CppExoPlayerBridge.java`: `getCurrentCues` | `CppBridgeConverters.java` + `exoplayer_cppbridge_jni_common.cpp`: `fromCue`, `CreateJavaCueArray`, `FromJavaCues` |
| audio/device/video getters | `GetAudioAttributes`, `GetDeviceInfo`, `GetVideoSize`, `GetDeviceVolume`, `IsDeviceMuted`, `GetSkipSilenceEnabled` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: `getAudioAttributesConfig`, `getDeviceInfo`, `getVideoSize`, device-volume methods | `exoplayer_cppbridge_jni_common.cpp`: `FromJavaAudioAttributes`, `FromJavaDeviceInfo`, `FromJavaVideoSize` |
| audio/device/video setters | `SetAudioAttributes`, `SetDeviceVolume`, `AdjustDeviceVolume`, `IncreaseDeviceVolume`, `DecreaseDeviceVolume`, `SetDeviceMuted`, `SetSkipSilenceEnabled`, `SetVolume` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: `setAudioAttributesConfig`, `setDeviceVolumeWithFlags`, `adjustDeviceVolumeWithFlags`, `increaseDeviceVolumeWithFlags`, `decreaseDeviceVolumeWithFlags`, `setDeviceMutedWithFlags`, `setSkipSilenceEnabled`, `setVolume` | `CppBridgeConverters.java` + `exoplayer_cppbridge_jni_common.cpp` for DTO conversion only |
| runtime parity controls | `SetHandleAudioBecomingNoisy`, `SetForegroundMode`, `SetPauseAtEndOfMediaItems`, `GetPauseAtEndOfMediaItems`, `SetSeekBackIncrementMs`, `SetSeekForwardIncrementMs`, `SetMaxSeekToPreviousPositionMs`, `SetVideoScalingMode`, `GetVideoScalingMode`, `SetVideoChangeFrameRateStrategy`, `GetVideoChangeFrameRateStrategy` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: matching scalar runtime methods | primitive JNI calls; validated by `nativeRuntimeControlParitySmokeTest_updatesPhaseOneRuntimeControls` |
| advanced audio/device/scrubbing controls | `SetAudioSessionId`, `SetAuxEffectInfo`, `ClearAuxEffectInfo`, `SetPreferredAudioDevice`, `ClearPreferredAudioDevice`, `SetVirtualDeviceId`, `SetScrubbingModeEnabled`, `IsScrubbingModeEnabled`, `SetScrubbingModeParameters`, `GetScrubbingModeParameters` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: matching audio/device/scrubbing methods | `AuxEffectInfoDescriptor`, `ScrubbingModeParametersDescriptor`; validated by `nativeAudioAndScrubbingParitySmokeTest_updatesAdvancedRuntimeControls` |
| codec parameter maps | `SetAudioCodecParameters`, `SetVideoCodecParameters` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: `setAudioCodecParameters`, `setVideoCodecParameters`; `CppCodecParameter.java` | `CodecParameterDescriptor`, `CodecParametersDescriptor`, `CreateJavaCodecParameterArray`; validated by `nativeCodecParametersParitySmokeTest_setsAudioAndVideoCodecParameters` |
| auxiliary callback APIs | `AddAudioCodecParametersChangeListener`, `RemoveAudioCodecParametersChangeListener`, `AddVideoCodecParametersChangeListener`, `RemoveVideoCodecParametersChangeListener`, `SetVideoFrameMetadataListener`, `ClearVideoFrameMetadataListener`, `SetCameraMotionListener`, `ClearCameraMotionListener` | `exoplayer_cppbridge_jni_bridge.cpp`: listener registration plus `Simulate*ForTest` bridge hooks | `CppExoPlayerBridge.java`: `CodecParametersChangeListener`, `VideoFrameMetadataListener`, `CameraMotionListener` adapters and `nativeOn*` callbacks | `CodecParametersDescriptor`, `VideoFrameMetadataSnapshot`, `CameraMotionSnapshot`; validated by `nativeAuxiliaryCallbackParitySmokeTest_reportsCodecVideoAndCameraCallbacks` and `nativeCodecParametersMultiListenerParitySmokeTest_routesImmediateCallbacks` |
| renderer/device-state getters | `GetRendererCount`, `GetRendererType`, `IsSleepingForOffload`, `IsTunnelingEnabled`, `IsReleased` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: matching getter methods | primitive JNI calls; validated by `nativeRendererAndDeviceStateGetterSmokeTest_readsRendererAndDeviceState` |
| surface / PlayerView | `BindPlayerView`, `UnbindPlayerView`, `SetVideoSurface*`, `ClearVideoSurface*`, `SetVideoTextureView`, `ClearVideoTextureView` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: `bindPlayerView`, `unbindPlayerView`, `setVideoSurface*`, `clearVideoSurface*`, `setVideoTextureView`, `clearVideoTextureView` | no DTO layer; direct JNI/Java runtime path |
| analytics aggregate and concrete test events | `GetAnalyticsSnapshot`, `SimulateAnalytics*ForTest` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: `getAnalyticsStrings`, `simulateAnalytics*ForTest`, `dispatchAnalytics*` | `exoplayer_cppbridge_jni_common.cpp`: event DTO parsing helpers; `exoplayer_cppbridge_jni_player_tests.cpp` and `CppBridgeNativePlayerInstrumentationTest.java` show usage |
| player listener callbacks | `PlayerListener` methods in `include/exoplayer_bridge.h` | `exoplayer_cppbridge_jni_bridge.cpp`: `BridgeOn*` functions and listener forwarding | `CppExoPlayerBridge.java`: `on*` callback overrides and `nativeOn*` calls | `exoplayer_sdk.cpp`: forwarding listener implementation and snapshot fan-out |
| image output | `SetImageOutputEnabled`, `SetImageOutputListener`, `RemoveImageOutputListener` | `exoplayer_cppbridge_jni_bridge.cpp`: same names | `CppExoPlayerBridge.java`: `setImageOutputEnabled`, image output object wiring, `nativeOnImageOutputAvailable`, `nativeOnImageOutputDisabled` | `exoplayer_sdk.cpp`: image listener forwarding |
| opaque token cleanup | `ReleaseOpaqueObjectTokens` and `OpaqueTokenBatch<T>` helpers | `exoplayer_cppbridge_jni_bridge.cpp`: `ReleaseOpaqueObjectTokens` | `CppExoPlayerBridge.java`: `releaseOpaqueObjectTokens` | `exoplayer_bridge.h`: deduplicating `AppendOpaqueObjectTokens` / `CollectOpaqueObjectTokens`; `CppOpaqueObjectRegistry.java`; `nativeOpaqueTokenBatchReleaseSmokeTest_dedupesAndClearsCollectedTokens` |

### Suggested Exact Search Keys

Use these search chains when you want exact grep targets instead of reading the tables above.

| Intent | Public C++ search key | JNI bridge search key | Java runtime search key | DTO/helper search key |
| --- | --- | --- | --- | --- |
| set single media item | `SetMediaItem(` | `JniExoPlayerBridge::SetMediaItem(` | `setMediaItem(` | `CreateJavaMediaItem(` / `toMediaItem(` |
| set media-item list | `SetMediaItems(` | `JniExoPlayerBridge::SetMediaItems(` | `setMediaItems(` | `CreateJavaMediaItemArray(` / `toMediaItem(` |
| add media item | `AddMediaItem(` | `JniExoPlayerBridge::AddMediaItem(` | `addMediaItem(` | `CreateJavaMediaItem(` / `toMediaItem(` |
| replace media item | `ReplaceMediaItem(` | `JniExoPlayerBridge::ReplaceMediaItem(` | `replaceMediaItem(` | `CreateJavaMediaItem(` / `toMediaItem(` |
| get current media item | `GetCurrentMediaItem(` | `JniExoPlayerBridge::GetCurrentMediaItem(` | `getCurrentMediaItem(` | `FromJavaMediaItem(` / `fromMediaItem(` |
| get media item at index | `GetMediaItemAt(` | `JniExoPlayerBridge::GetMediaItemAt(` | `getMediaItemAt(` | `FromJavaMediaItem(` / `fromMediaItem(` |
| get media metadata | `GetMediaMetadata(` | `JniExoPlayerBridge::GetMediaMetadata(` | `getMediaMetadata(` | `FromJavaMediaMetadata(` / `fromMediaMetadata(` |
| set playlist metadata | `SetPlaylistMetadata(` | `JniExoPlayerBridge::SetPlaylistMetadata(` | `setPlaylistMetadata(` | `CreateJavaMediaMetadata(` / `toMediaMetadata(` |
| get tracks | `GetTracks(` | `JniExoPlayerBridge::GetTracks(` or `GetTracksSnapshot(` | `getTracks(` | `FromJavaTracks(` / `CreateJavaTracks(` |
| set track selection parameters | `SetTrackSelectionParameters(` | `JniExoPlayerBridge::SetTrackSelectionParameters(` | `setTrackSelectionParameters(` | `FromJavaTrackSelectionParameters(` / `toTrackSelectionParameters(` |
| get track selection parameters | `GetTrackSelectionParameters(` | `JniExoPlayerBridge::GetTrackSelectionParameters(` | `getTrackSelectionParameters(` | `FromJavaTrackSelectionParameters(` / `fromTrackSelectionParameters(` |
| set seek parameters | `SetSeekParameters(` | `JniExoPlayerBridge::SetSeekParameters(` | `setSeekParameters(` | `FromJavaSeekParameters(` / `toSeekParameters(` |
| get seek parameters | `GetSeekParameters(` | `JniExoPlayerBridge::GetSeekParameters(` | `getSeekParameters(` | `FromJavaSeekParameters(` / `fromSeekParameters(` |
| get current cues | `GetCurrentCues(` | `JniExoPlayerBridge::GetCurrentCues(` | `getCurrentCues(` | `FromJavaCues(` / `CreateJavaCueArray(` |
| get device info | `GetDeviceInfo(` | `JniExoPlayerBridge::GetDeviceInfo(` | `getDeviceInfo(` | `FromJavaDeviceInfo(` |
| get video size | `GetVideoSize(` | `JniExoPlayerBridge::GetVideoSize(` | `getVideoSize(` | `FromJavaVideoSize(` |
| get audio attributes | `GetAudioAttributes(` | `JniExoPlayerBridge::GetAudioAttributes(` | `getAudioAttributesConfig(` | `FromJavaAudioAttributes(` |
| set audio attributes | `SetAudioAttributes(` | `JniExoPlayerBridge::SetAudioAttributes(` | `setAudioAttributesConfig(` | `AudioAttributesDescriptor` |
| set video effects | `SetVideoEffects(` | `JniExoPlayerBridge::SetVideoEffects(` | `setVideoEffects(` | `CreateJavaVideoEffectArray(` / `toVideoEffects(` |
| analytics aggregate | `GetAnalyticsSnapshot(` | `JniExoPlayerBridge::GetAnalyticsSnapshot(` | `getAnalyticsStrings(` | `AnalyticsSnapshot` / `SplitString(` |
| analytics media-item transition callback | `OnAnalyticsMediaItemTransition(` | `Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsMediaItemTransition` | `dispatchAnalyticsMediaItemTransition(` | `FromJavaMediaItem(` / `AnalyticsMediaItemTransitionEvent` |
| listener tracks changed callback | `OnTracksChanged(` | `Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnTracksChanged` | `onTracksChanged(` | `FromJavaTracks(` |
| listener media metadata callback | `OnMediaMetadataChanged(` | `Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnMediaMetadataChanged` | `onMediaMetadataChanged(` | `FromJavaMediaMetadata(` |
| opaque token release | `ReleaseOpaqueObjectTokens(` | `JniExoPlayerBridge::ReleaseOpaqueObjectTokens(` | `releaseOpaqueObjectTokens(` | `CppOpaqueObjectRegistry` |

## 1. Player API Families

| Java family | C++ surface | Status | Smoke test reference |
| --- | --- | --- | --- |
| lifecycle/core playback | `Prepare/Play/Pause/Stop/Release` | Done | `nativeLifecycleSmokeTest_runsThroughLifecycle`; `nativePostReleaseCallSafetySmokeTest_doesNotCrashOrHang` |
| playWhenReady/playbackState/isPlaying/isLoading | direct getters/setters | Done | `nativeCreateConfiguredPlayerSnapshotForTest_returnsConfiguredState`; `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` |
| seek/navigation/default-position/aliases | `SeekTo*`, `SeekBack`, `SeekForward`, `SeekToNext*`, `SeekToPrevious*` | Done | `nativeSeekNavigationSmokeTest_runsNavigationCalls`; `nativeSeekAliasSmokeTest_runsJavaNameParityAliases` |
| seek parameters | `SetSeekParameters`, `GetSeekParameters` | Done | `nativeSeekParametersSmokeTest_roundTripsSeekParameters` |
| runtime seek increment and previous-threshold controls | `SetSeekBackIncrementMs`, `SetSeekForwardIncrementMs`, `SetMaxSeekToPreviousPositionMs` plus getters | Done | `nativeRuntimeControlParitySmokeTest_updatesPhaseOneRuntimeControls`; `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` |
| runtime playback/session controls | `SetHandleAudioBecomingNoisy`, `SetForegroundMode`, `SetPauseAtEndOfMediaItems`, `GetPauseAtEndOfMediaItems` | Done | `nativeRuntimeControlParitySmokeTest_updatesPhaseOneRuntimeControls` |
| playlist mutation | set/add/remove/move/replace/clear | Done | `nativePlaylistMutationSmokeTest_returnsUpdatedPlaylistState`; `nativeMediaSetOverloadsSmokeTest_returnsUpdatedPlaylistSummary` covers Java-default `setMediaItems(List)`, explicit overloads, and void-call exception capture |
| `getMediaItemAt` | `GetMediaItemAt(int)` | Done | `nativeMediaItemAtSmokeTest_returnsSnapshotAndHandlesOutOfBounds` |
| `getCurrentMediaItem` | `GetCurrentMediaItem()` | Done | `nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary`; `nativeSourceTypeSmokeTest_returnsInferredMimeSummary`; `nativeHttpHlsDashPlaybackSmokeTest_preparesLocalStreamsThroughCppApi`; `nativeHttpDataSourceConfigPlaybackSmokeTest_sendsHeadersThroughCppConfig`; `nativeCustomMediaSourceFactoryPlaybackSmokeTest_preparesSmoothAndRtspViaCppConfig` |
| playlist metadata set/get | `SetPlaylistMetadata`, `GetPlaylistMetadata` | Done | `nativePlaylistMetadataSmokeTest_roundTripsPlaylistMetadata`; `nativePlaylistMetadataOpaqueTokenSmokeTest_resolvesRegisteredObjects`; `nativeVideoAndMetadataSmokeTest_returnsQuerySummary` |
| repeat/shuffle | direct setter/getter parity | Done | `nativeCreateConfiguredPlayerSnapshotForTest_returnsConfiguredState`; `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| playback parameters | reduced `PlaybackParametersSnapshot` | Done | `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary`; `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| available commands | reduced integer command codes | Done | `nativeAvailableCommandsSmokeTest_returnsContainsStyleSummary`; `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| application looper query | reduced `ApplicationLooperDescriptor` | Done | `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` |
| ad-state query | current ad getters and `IsPlayingAd` | Done | `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` |
| renderer/device-state queries | `GetRendererCount`, `GetRendererType`, `IsSleepingForOffload`, `IsTunnelingEnabled`, `IsReleased` | Done | `nativeRendererAndDeviceStateGetterSmokeTest_readsRendererAndDeviceState` |

## 2. Audio / Device / Video / Surface

| Java family | C++ surface | Status | Smoke test reference |
| --- | --- | --- | --- |
| audio attributes set/get | `SetAudioAttributes`, `GetAudioAttributes` | Done | `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` |
| advanced audio controls | `SetAudioSessionId`, `SetAuxEffectInfo`, `ClearAuxEffectInfo`, `SetPreferredAudioDevice`, `ClearPreferredAudioDevice`, `SetVirtualDeviceId` | Done | `nativeAudioAndScrubbingParitySmokeTest_updatesAdvancedRuntimeControls` |
| codec parameter controls | `SetAudioCodecParameters`, `SetVideoCodecParameters` with reduced typed key/value descriptors | Done | `nativeCodecParametersParitySmokeTest_setsAudioAndVideoCodecParameters` |
| player volume | `SetVolume`, `GetVolume` | Done | `nativeCreateConfiguredPlayerSnapshotForTest_returnsConfiguredState`; demo manual flow |
| device volume/mute | device runtime methods | Done | `nativeDeviceAndSkipSilenceSmokeTest_returnsDeviceSummary` |
| `DeviceInfo` getter | reduced `GetDeviceInfo()` | Done | `nativeDeviceAndSkipSilenceSmokeTest_returnsDeviceSummary` |
| skip silence | set/get parity | Done | `nativeDeviceAndSkipSilenceSmokeTest_returnsDeviceSummary` |
| scrubbing mode | `SetScrubbingModeEnabled`, `IsScrubbingModeEnabled`, `SetScrubbingModeParameters`, `GetScrubbingModeParameters` | Done | `nativeAudioAndScrubbingParitySmokeTest_updatesAdvancedRuntimeControls` |
| `VideoSize` getter | reduced `GetVideoSize()` | Done | `nativeVideoAndMetadataSmokeTest_returnsQuerySummary` |
| video runtime mode controls | `SetVideoScalingMode`, `GetVideoScalingMode`, `SetVideoChangeFrameRateStrategy`, `GetVideoChangeFrameRateStrategy` | Done | `nativeRuntimeControlParitySmokeTest_updatesPhaseOneRuntimeControls` |
| surface family | surface/surface holder/view/texture view helpers | Done | `nativeSurfaceBridgeSmokeTest_runsSurfaceCalls` |
| `PlayerView` binding | bind/unbind helper | Done | `nativePlayerViewBridgeSmokeTest_bindsAndUnbindsPlayerView` |

## 3. Tracks / Timeline / Metadata / Cues

| Java family | C++ surface | Status | Smoke test reference |
| --- | --- | --- | --- |
| track selection parameters | reduced descriptor set/get | Done | `nativeTrackSelectionRoundTripForTest_returnsUpdatedParameters` |
| current tracks | `TracksSnapshot` plus grouped compatibility path, including expanded `TrackInfo` labels, metadata/custom tokens, initialization bytes, DRM scheme-data shape, subsample/preroll, decoded/projection bytes/stereo/color/HDR, PCM/encoder/tile/crypto, auxiliary type, and support fields | Done | `nativeCurrentTracksSmokeTest_returnsTracksSummary`; `CppBridgeNativeSmokeTest.nativeTracksSnapshotConversionSmokeTest_returnsStructuredSummary`; `CppBridgeNativeSmokeTest.nativeTracksFullPayloadConversionSmokeTest_roundTripsFormatPayload` |
| current timeline | reduced `TimelineDetailsSnapshot` / windows / periods | Done | `nativeCurrentTimelineSmokeTest_returnsTimelineDetails`; `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |
| media metadata | reduced `MediaMetadataSnapshot` plus representative text opaque token baseline and reduced text object-value metadata | Done | `nativeVideoAndMetadataSmokeTest_returnsQuerySummary`; `nativePlaylistMetadataSmokeTest_roundTripsPlaylistMetadata`; `nativePlaylistMetadataOpaqueTokenSmokeTest_resolvesRegisteredObjects`; `nativeMediaMetadataObjectValueConversionSmokeTest_roundTripsObjectTextFields` |
| current cues | reduced `CueSnapshot` plus representative text and bitmap opaque token baseline | Done | `CppBridgeNativeSmokeTest.nativeCueSnapshotConversionSmokeTest_returnsStructuredSummary`; `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary`; `nativeAnalyticsCuesSmokeTest_reportsConcreteAnalyticsEvent` |

## 4. Player.Listener Families

| Java callback family | C++ callback family | Status | Smoke test reference |
| --- | --- | --- | --- |
| playback state / playWhenReady / isPlaying / isLoading | direct callbacks | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| media item transition | direct callback | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| player error / error changed | reduced error callbacks | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| timeline changed | reduced `TimelineDetailsSnapshot` payload | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |
| tracks changed | reduced `TracksSnapshot` payload | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |
| position discontinuity | reduced old/new `PositionInfoSnapshot` | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |
| audio attributes changed | reduced descriptor callback | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| cues | reduced cue callback | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| repeat/shuffle changed | direct callbacks | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| seek increment / max-seek-to-previous changed | direct callbacks | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| track selection changed | reduced descriptor callback | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| playback parameters changed | reduced parameters callback | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| suppression reason changed | direct callback | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| available commands changed | reduced command-code callback | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |
| events batch | reduced event-code callback | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |
| device info / device volume / skip-silence changed | direct callbacks | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| video size / surface size / rendered-first-frame | direct callbacks | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| metadata / playlist metadata changed | reduced metadata callback | Done | `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| analytics-updated aggregate | reduced analytics callback | Done | `nativeAnalyticsCallbackSmokeTest_reportsListenerDelivery`; `nativeAnalyticsListenerRegistrationSmokeTest_addsAndRemovesAnalyticsOnlyListener`; `nativeAnalyticsSmokeTest_returnsAnalyticsSummary` |
| analytics audio underrun | first concrete reduced analytics event callback | Done | `nativeAnalyticsAudioUnderrunSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics dropped video frames | second concrete reduced analytics event callback | Done | `nativeAnalyticsDroppedVideoFramesSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics bandwidth estimate | third concrete reduced analytics event callback | Done | `nativeAnalyticsBandwidthEstimateSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics load started | fourth concrete reduced analytics event callback | Done | `nativeAnalyticsLoadStartedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics load completed | fifth concrete reduced analytics event callback | Done | `nativeAnalyticsLoadCompletedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics audio input format changed | sixth concrete reduced analytics event callback | Done | `nativeAnalyticsAudioInputFormatChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics audio decoder initialized | seventh concrete reduced analytics event callback | Done | `nativeAnalyticsAudioDecoderInitializedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics video decoder initialized | eighth concrete reduced analytics event callback | Done | `nativeAnalyticsVideoDecoderInitializedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics audio decoder released | ninth concrete reduced analytics event callback | Done | `nativeAnalyticsAudioDecoderReleasedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics video decoder released | tenth concrete reduced analytics event callback | Done | `nativeAnalyticsVideoDecoderReleasedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics rendered first frame | eleventh concrete reduced analytics event callback | Done | `nativeAnalyticsRenderedFirstFrameSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics video size changed | twelfth concrete reduced analytics event callback | Done | `nativeAnalyticsVideoSizeChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics audio position advancing | thirteenth concrete reduced analytics event callback | Done | `nativeAnalyticsAudioPositionAdvancingSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics video frame processing offset | fourteenth concrete reduced analytics event callback | Done | `nativeAnalyticsVideoFrameProcessingOffsetSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics volume changed | fifteenth concrete reduced analytics event callback | Done | `nativeAnalyticsVolumeChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics audio session id changed | sixteenth concrete reduced analytics event callback | Done | `nativeAnalyticsAudioSessionIdChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics skip silence enabled changed | seventeenth concrete reduced analytics event callback | Done | `nativeAnalyticsSkipSilenceEnabledChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics device volume changed | eighteenth concrete reduced analytics event callback | Done | `nativeAnalyticsDeviceVolumeChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics playback state changed | nineteenth concrete reduced analytics event callback | Done | `nativeAnalyticsPlaybackStateChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics is playing changed | twentieth concrete reduced analytics event callback | Done | `nativeAnalyticsIsPlayingChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics play when ready changed | twenty-first concrete reduced analytics event callback | Done | `nativeAnalyticsPlayWhenReadyChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics playback suppression reason changed | twenty-second concrete reduced analytics event callback | Done | `nativeAnalyticsPlaybackSuppressionReasonChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics is loading changed | twenty-third concrete reduced analytics event callback | Done | `nativeAnalyticsIsLoadingChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics repeat mode changed | twenty-fourth concrete reduced analytics event callback | Done | `nativeAnalyticsRepeatModeChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics shuffle mode changed | twenty-fifth concrete reduced analytics event callback | Done | `nativeAnalyticsShuffleModeChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics video input format changed | twenty-sixth concrete reduced analytics event callback | Done | `nativeAnalyticsVideoInputFormatChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics playback parameters changed | twenty-seventh concrete reduced analytics event callback | Done | `nativeAnalyticsPlaybackParametersChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics available commands changed | twenty-eighth concrete reduced analytics event callback | Done | `nativeAnalyticsAvailableCommandsChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics events batch | twenty-ninth concrete reduced analytics event callback | Done | `nativeAnalyticsEventsSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics seek back increment changed | thirtieth concrete reduced analytics event callback | Done | `nativeAnalyticsSeekBackIncrementChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics seek forward increment changed | thirty-first concrete reduced analytics event callback | Done | `nativeAnalyticsSeekForwardIncrementChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics max seek to previous position changed | thirty-second concrete reduced analytics event callback | Done | `nativeAnalyticsMaxSeekToPreviousPositionChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics timeline changed | thirty-third concrete reduced analytics event callback | Done | `nativeAnalyticsTimelineChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics position discontinuity | thirty-fourth concrete reduced analytics event callback | Done | `nativeAnalyticsPositionDiscontinuitySmokeTest_reportsConcreteAnalyticsEvent` |
| analytics seek started | thirty-fifth concrete reduced analytics event callback | Done | `nativeAnalyticsSeekStartedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics device info changed | thirty-sixth concrete reduced analytics event callback | Done | `nativeAnalyticsDeviceInfoChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics media metadata changed | thirty-seventh concrete reduced analytics event callback | Done | `nativeAnalyticsMediaMetadataChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics playlist metadata changed | thirty-eighth concrete reduced analytics event callback | Done | `nativeAnalyticsPlaylistMetadataChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics player error | thirty-ninth concrete reduced analytics event callback | Done | `nativeAnalyticsPlayerErrorSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics player error changed | fortieth concrete reduced analytics event callback | Done | `nativeAnalyticsPlayerErrorChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics tracks changed | forty-first concrete reduced analytics event callback | Done | `nativeAnalyticsTracksChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics media item transition | forty-second concrete reduced analytics event callback | Done | `nativeAnalyticsMediaItemTransitionSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics cues | forty-third concrete reduced analytics event callback | Done | `nativeAnalyticsCuesSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics metadata | forty-fourth concrete reduced analytics event callback | Done | `nativeAnalyticsMetadataSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics load error | forty-fifth concrete reduced analytics event callback | Done | `nativeAnalyticsLoadErrorSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics audio attributes changed | forty-sixth concrete reduced analytics event callback | Done | `nativeAnalyticsAudioAttributesChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics Stage 4 remaining callbacks | twenty-five additional concrete reduced analytics event callbacks, bringing reduced `AnalyticsListener` method-name coverage to seventy-one concrete callbacks beyond aggregate delivery | Done | `nativeAnalyticsStage4RemainingCallbacksSmokeTest_reportsConcreteAnalyticsEvents` |
| image output callback | reduced image frame callback | Done | `nativeImageOutputSmokeTest_returnsListenerSummary` |
| codec/video/camera auxiliary callbacks | reduced `CodecParametersChangeListener`, `VideoFrameMetadataListener`, and `CameraMotionListener` callbacks, including codec-parameter multi-listener immediate routing plus richer video-frame `Format` and `MediaFormat` payload fields, bitrate fallback, and sentinel preservation | Done | `nativeAuxiliaryCallbackParitySmokeTest_reportsCodecVideoAndCameraCallbacks`; `nativeCodecParametersMultiListenerParitySmokeTest_routesImmediateCallbacks`; `nativeVideoFrameMetadataSimulationFallbackSmokeTest_preservesFallbackFields` |
| listener detach behavior | remove listener / stop callback delivery | Done | `nativeListenerDetachSmokeTest_stopsCallbacksAfterRemoval` |

## 5. ExoPlayer / Builder Families

| Java family | C++ surface | Status | Smoke test reference |
| --- | --- | --- | --- |
| wake mode | create-time + runtime setter | Done | `nativeBuilderConfigSmokeTest_returnsConfiguredSnapshot`; `CppBridgeNativeSmokeTest.nativeBuilderBuildSmokeTest_buildsConfiguredPlayer`; `nativePlayerConfigFlagsSmokeTest_returnsCreateTimeFlags`; `nativeWakeModeRuntimeSmokeTest_updatesWakeMode` |
| media source factory baseline config | `PlayerConfig::MediaSourceFactoryConfig` | Done | `nativeMediaSourceFactoryConfigSmokeTest_returnsConfigSummary`; `nativeHttpDataSourceConfigPlaybackSmokeTest_sendsHeadersThroughCppConfig`; `nativeMediaSourceFactoryInjectionSmokeTest_usesRegisteredFactoryToken`; `nativeMediaSourceFactoryInjectionFallbackSmokeTest_fallsBackWhenTokenIsMissing`; `nativeMediaSourceFactoryInjectionReplacementSmokeTest_usesLatestRegisteredFactory`; `nativeMediaSourceFactoryInjectionMultiTokenSmokeTest_keepsTokensIsolated`; `nativeMediaSourceFactoryGeneratedTokenSmokeTest_usesGeneratedRegistryToken`; `nativeCustomMediaSourceFactoryPlaybackSmokeTest_preparesSmoothAndRtspViaCppConfig`; `CppBridgeNativeSmokeTest.nativeBuilderMediaSourceFactoryInjectionSmokeTest_buildsWithRegisteredFactoryToken`; `CppBridgeNativeSmokeTest.nativeBuilderMediaSourceFactoryFallbackSmokeTest_buildsWithDefaultFactoryWhenTokenIsMissing`; `CppBridgeNativeSmokeTest.nativeBuilderMediaSourceFactoryInjectionReplacementSmokeTest_usesLatestRegisteredFactory`; `CppBridgeNativeSmokeTest.nativeBuilderMediaSourceFactoryInjectionMultiTokenSmokeTest_keepsTokensIsolated`; `CppBridgeNativeSmokeTest.nativeBuilderMediaSourceFactoryGeneratedTokenSmokeTest_buildsWithGeneratedRegistryToken`; `nativeBuilderConfigSmokeTest_returnsConfiguredSnapshot`; `CppBridgeNativeSmokeTest.nativeBuilderBuildSmokeTest_buildsConfiguredPlayer` |
| analytics listener support | reduced analytics-only add/remove + aggregate snapshot, plus seventy-one concrete reduced event callbacks covering all current Java `AnalyticsListener` method names at reduced payload fidelity | Done | `nativeAnalyticsSmokeTest_returnsAnalyticsSummary`; `nativeAnalyticsListenerRegistrationSmokeTest_addsAndRemovesAnalyticsOnlyListener`; `nativeAnalyticsCallbackSmokeTest_reportsListenerDelivery`; concrete analytics smokes through `nativeAnalyticsAudioAttributesChangedSmokeTest_reportsConcreteAnalyticsEvent`; `nativeAnalyticsStage4RemainingCallbacksSmokeTest_reportsConcreteAnalyticsEvents` |
| auxiliary listener support | reduced codec-parameter change, video-frame-metadata, and camera-motion callback registration/removal, plus codec-parameter multi-listener immediate routing and video-frame metadata fallback/sentinel coverage | Done | `nativeAuxiliaryCallbackParitySmokeTest_reportsCodecVideoAndCameraCallbacks`; `nativeCodecParametersMultiListenerParitySmokeTest_routesImmediateCallbacks`; `nativeVideoFrameMetadataSimulationFallbackSmokeTest_preservesFallbackFields` |
| runtime noisy handling | create-time config + runtime setter | Done | `nativePlayerConfigFlagsSmokeTest_returnsCreateTimeFlags`; `nativeBuilderConfigSmokeTest_returnsConfiguredSnapshot`; `CppBridgeNativeSmokeTest.nativeBuilderBuildSmokeTest_buildsConfiguredPlayer`; `nativeRuntimeControlParitySmokeTest_updatesPhaseOneRuntimeControls` |
| runtime foreground and pause-at-end controls | foreground mode plus pause-at-end-of-media-items setter/getter | Done | `nativeRuntimeControlParitySmokeTest_updatesPhaseOneRuntimeControls` |
| priority config/runtime | create-time config + runtime setter + reduced wrapper | Done | `nativePriorityTaskManagerSmokeTest_returnsBridgeState`; `CppBridgeNativeSmokeTest.nativePriorityTaskManagerWrapperSmokeTest_returnsStructuredSummary` |
| preload | reduced target preload duration | Done | `nativePreloadConfigurationSmokeTest_returnsRuntimeConfig`; `nativePreloadRoundTripSmokeTest_updatesAndRestoresPreloadTarget`; `nativePreloadBridgeRuntimeSmokeTest_updatesAndMatchesBridgeFlags`; `CppBridgeNativeSmokeTest.nativeBuilderPreloadRoundTripSmokeTest_updatesAndRestoresPreloadTarget` |
| renderer messaging | reduced `PlayerMessage` send/cancel | Done | `nativePlayerMessageSmokeTest_returnsDeliverySummary`; `nativeTimedPlayerMessageSmokeTest_returnsRuntimeDeliverySummary`; `nativeRendererPlayerMessageSmokeTest_returnsRuntimeSummary`; `nativePlayerMessageCancelSmokeTest_returnsCanceledSummary` |
| video effects | reduced `setVideoEffects(...)` | Done | `CppBridgeNativeSmokeTest.nativeVideoEffectsConversionSmokeTest_returnsStructuredSummary` |
| image output | reduced image output parity | Done | `nativeImageOutputSmokeTest_returnsListenerSummary` |
| builder-style public C++ class | `ExoPlayerSdkPlayerBuilder` | Done | `CppBridgeNativeSmokeTest.nativeBuilderConfigSmokeTest_returnsConfiguredSnapshot`; `CppBridgeNativeSmokeTest.nativeBuilderBuildSmokeTest_buildsConfiguredPlayer`; `CppBridgeNativeSmokeTest.nativeBuilderPreloadRoundTripSmokeTest_updatesAndRestoresPreloadTarget` |

## 6. Still Not Complete

- exact `Player` / `ExoPlayer` method and direct `Player.Listener` callback names are now closed
  according to `API_PARITY_GAP_REPORT.md`; this document still separates those method-name wins
  from full object/value and analytics-payload parity
- full Java `MediaItem` parity
  current bridge now carries reduced `RequestMetadata` (`mediaUri`, `searchQuery`,
  extras-present/key-count/token) plus decoded stable extras values for strings, integer-like
  numbers, floating-point numbers, booleans, and byte arrays; opaque extras-token round-trip
  baseline remains for unsupported arbitrary Java values. It also carries reduced local tag
  observability (`tagPresent`, `tagString`) plus opaque tag-token round-trip baseline and reduced
  `ObjectValueInfo` metadata, and reduced `adsId` string identity plus opaque adsId-token
  round-trip baseline and reduced `ObjectValueInfo` metadata; it still does not provide
  unrestricted arbitrary-object semantics beyond the current opaque-token bridge model
  strongest new smoke for this boundary: `nativeMediaItemOpaqueTokenSmokeTest_resolvesRegisteredObjects`
- full Java `Timeline`, `Tracks`, `MediaMetadata`, and `Cue` parity
  timeline rows now carry reduced value descriptors for the Java object identity fields
  (`Window.uid`, `Window.manifest`, `Period.id`, `Period.uid`, and `Period.adsId`), but this is
  still a stable reduced model rather than unrestricted Java-object transfer
- richer/full-object Java `AnalyticsListener` payload parity beyond the current reduced aggregate
  and seventy-one concrete reduced callback families
- callback-style ExoPlayer extension APIs are now covered for the first reduced slice
  (`CodecParametersChangeListener`, `VideoFrameMetadataListener`, and `CameraMotionListener`);
  codec-parameter multi-listener immediate-notification routing is also smoke-covered; remaining
  risk is deeper full-object fidelity beyond the current reduced smoke
- broader preload ecosystem parity
- richer video effects parity beyond the current reduced effect set, default/boundary effect coverage, and full bitmap image-output parity

## 6A. Remaining Work After Second-Batch Closeout

Second-batch reduced-scope work is now considered complete. The items below are the capability
gaps that remain for the next phase.

### Still Requires New Capability

- richer/full-object Java `AnalyticsListener` payload parity beyond the reduced callback descriptors
- broader preload ecosystem parity beyond target preload duration
- richer image output parity beyond reduced frame metadata, bitmap-layout metadata, and callback behavior
- richer video effects parity beyond the currently exposed reduced effect types and boundary/default-value scenarios
- broader arbitrary `MediaSource.Factory` injection beyond token-registered and registry-generated-token baseline support
- full Java object parity for `MediaItem`, `Timeline`, `Tracks`, `MediaMetadata`, and `Cue`

## 7. Full-Support Development Matrix

This table is the planning view for continued development. It is intentionally stricter than the
reduced-endpoint tracker above.

| Theme | Full-support status | What is already supported | What is still missing for true full support |
| --- | --- | --- | --- |
| `MediaItem` | Partial | uri/media id/source type, subtitles, clipping/live/DRM, reduced request metadata with decoded stable extras values, reduced ads config, local tag/adsId observability, reduced `ObjectValueInfo` metadata for `tag` and `adsId`, opaque token baselines for `tag`, `adsId`, and unsupported `requestMetadata.extras` values | full arbitrary-object semantics for `tag` and `adsId`; arbitrary/nested `Bundle` parity for `requestMetadata.extras`; broader Java-object parity beyond the reduced descriptor |
| `Timeline` | Partial | reduced summary, window list, period list, window/period identity baselines, live config baseline, manifest presence/string plus opaque token baseline, reduced `ObjectValueInfo` descriptors for window `uid`/`manifest` and period `id`/`uid`/`adsId`, multi-window and multi-period smoke visibility | full Java `Timeline.Window` / `Timeline.Period` parity, arbitrary object graph semantics, and deeper ad/playback structure parity beyond the reduced row model |
| `Tracks` | Partial | tracks/group/format snapshots, group token baseline, first/second group smoke visibility, listener/query coverage for representative audio/video fields, and richer `Format` fields including label arrays, metadata/custom tokens, initialization bytes, DRM scheme-data shape, subsample/preroll, decoded/projection bytes/stereo/color/HDR, PCM/encoder/tile/crypto, and auxiliary type | full Java `Tracks.Group` object semantics and decoded arbitrary `Metadata.Entry` / `customData` object semantics beyond current token transfer |
| `MediaMetadata` | Partial | representative text fields with token baselines and reduced `ObjectValueInfo` metadata, artwork uri/data/type, extras presence/key-count/token baseline plus decoded stable extras values, current-item/query/listener/playlist smoke coverage | full Java `MediaMetadata` parity for styled span fidelity, arbitrary/nested extras semantics, and richer nested object behavior beyond the reduced snapshot |
| `Cue` | Partial | reduced cue snapshot, text token baseline, bitmap token baseline, representative layout/style fields, current-query/listener/analytics smoke coverage | full Java `Cue` parity, especially richer styled text/span semantics and full bitmap object transfer |
| `AnalyticsListener` | Partial | aggregate analytics snapshot plus seventy-one concrete reduced events and listener lifecycle coverage; current Java `AnalyticsListener` method names are reviewed for reduced callback routing | richer payload parity and full-object fidelity for already-covered reduced events |
| runtime ExoPlayer extension callbacks | Partial | setter/getter parity for codec parameters, several runtime controls, reduced callback descriptors for `CodecParametersChangeListener`, `VideoFrameMetadataListener`, and `CameraMotionListener`, codec-parameter multi-listener immediate routing, and representative `Format` / `MediaFormat` fields in video-frame metadata callbacks | deeper full-object callback fidelity beyond the reduced descriptors |
| preload ecosystem | Partial | target preload duration create-time/runtime round-trip | broader preload ecosystem parity beyond the current target-duration surface |
| image output | Partial | reduced frame metadata, bitmap layout metadata, callback lifecycle behavior | richer bitmap/image-output parity beyond reduced metadata and callback observation |
| video effects | Partial | reduced effect types, ordering/reset/default-boundary scenarios | broader effect family and richer parameter parity |
| arbitrary `MediaSource.Factory` injection | Partial | token-registered and generated-token baseline support, fallback/replacement/isolation behavior | full arbitrary Java factory injection semantics beyond registry-token baselines |

## 8. Partial Review Notes

There are no remaining row-level `Partial` items in the current reduced endpoint tracker.

| Area | Current status | Why not `Done` yet | Strongest smoke evidence |
| --- | --- | --- | --- |
| current reduced endpoint | Done | code, smoke, and docs are aligned for every currently exposed reduced family; broader full Java parity gaps remain tracked below as next-phase capability work | representative evidence: `nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary`; `nativeCurrentTimelineSmokeTest_returnsTimelineDetails`; `nativeAnalyticsLoadErrorSmokeTest_reportsConcreteAnalyticsEvent` |

## 9. Smoke Marker Crosswalk

Use this as the API-family version of the validation crosswalk when checking that the current
`Done` reduced endpoint still matches runtime behavior on a healthy environment.

| API family | Primary smoke test | Marker examples | Promotion gate |
| --- | --- | --- | --- |
| `getCurrentMediaItem` | `nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary` | `mediaId=current-item`; `sourceType=2`; `mediaMetadataAlbumTitle=Current Album` | promote only if reduced-model scope is accepted as final |
| `getMediaItemAt` | `nativeMediaItemAtSmokeTest_returnsSnapshotAndHandlesOutOfBounds` | `mediaId=at-2`; `mediaMetadataDisplayTitle=At Two Display` | promote only if reduced-model scope is accepted as final |
| playlist metadata set/get | `nativePlaylistMetadataSmokeTest_roundTripsPlaylistMetadata` | `title=Playlist Title`; `description=Playlist Description`; `artworkUri=https://example.com/playlist-artwork.jpg` | promote only if no additional metadata fields are required |
| current timeline | `nativeCurrentTimelineSmokeTest_returnsTimelineDetails` | `window0MediaUri=https://example.com/current-timeline-one.m3u8`; `period0Uid=` | promote only if reduced timeline parity is the intended endpoint |
| timeline changed callback | `nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` | `timelineWindow0MediaUri=https://example.com/payload-window-0.m3u8`; `timelineWindow0Uid=payload-window-uid`; `timelineWindow0LiveTargetOffsetMs=3333`; `timelinePeriod0Id=payload-period-id`; `timelinePeriod0Uid=payload-period-0`; `timelinePeriod0AdsId=payload-period-ads-id` | promote only if reduced callback payload is sufficient |
| media metadata | `nativeVideoAndMetadataSmokeTest_returnsQuerySummary` | `mediaTitle=Video Metadata Title`; `mediaArtworkUri=https://example.com/video-metadata-artwork.jpg` | promote only if reduced metadata scope is the intended endpoint |
