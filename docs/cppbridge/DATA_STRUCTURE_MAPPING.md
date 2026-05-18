# Data Structure Mapping

Last updated: 2026-05-15

This document tracks the reduced-model mapping for value objects used by the bridge. Each entry
includes its development status and the smoke test that currently validates it.

Important scope note:

- `Done` here means the current reduced DTO is code-complete and smoke-observed for the exposed
  bridge endpoint.
- `Done` here does not mean full `api.txt` Java-object parity for every source type or nested
  object graph.

## 0. Direct Structure Lookup

Use this section when you already know a Java DTO name or a C++ reduced struct name and want to
jump directly to the implementation.

Reading rule:

- `Java DTO`: the transport object passed between JNI and `CppExoPlayerBridge`
- `Java converter/runtime`: the Java conversion or runtime entry that uses the DTO
- `C++ reduced struct`: the public/native-side reduced value model
- `JNI create/parse`: the JNI helper that converts between Java DTOs and C++ structs
- `Primary runtime use`: the bridge method or callback path where the mapping is actually used

| Search key | Java DTO | Java converter / runtime | C++ reduced struct | JNI create / parse | Primary runtime use |
| --- | --- | --- | --- | --- | --- |
| media item | `CppMediaItem.java` | `CppBridgeConverters.java`: `toMediaItem`, `fromMediaItem`; `CppExoPlayerBridge.java`: `setMediaItem`, `getCurrentMediaItem`, `getMediaItemAt` | `include/exoplayer_bridge.h`: `MediaItemDescriptor` | `exoplayer_cppbridge_jni_common.cpp`: `CreateJavaMediaItem`, `CreateJavaMediaItemArray`, `FromJavaMediaItem` | `exoplayer_cppbridge_jni_bridge.cpp`: playlist mutation, current item query, analytics media-item transition |
| media metadata | `CppMediaMetadata.java` | `CppBridgeConverters.java`: `toMediaMetadata`, `fromMediaMetadata`; `CppExoPlayerBridge.java`: `getMediaMetadata`, `getPlaylistMetadata`, `setPlaylistMetadata` | `include/exoplayer_bridge.h`: `MediaMetadataSnapshot` | `exoplayer_cppbridge_jni_common.cpp`: `CreateJavaMediaMetadata`, `FromJavaMediaMetadata` | playlist metadata query/setter, media metadata callbacks, analytics metadata callbacks |
| track selection parameters | `CppTrackSelectionParameters.java` | `CppBridgeConverters.java`: `toTrackSelectionParameters`, `fromTrackSelectionParameters`; `CppExoPlayerBridge.java`: `setTrackSelectionParameters`, `getTrackSelectionParameters` | `include/exoplayer_bridge.h`: `TrackSelectionParametersDescriptor` | `exoplayer_cppbridge_jni_common.cpp`: `FromJavaTrackSelectionParameters` | `exoplayer_cppbridge_jni_bridge.cpp`: set/get track-selection parameters |
| tracks / groups / formats | `CppTracks.java`, `CppTrackGroup.java`, `CppTrackInfo.java` | `CppExoPlayerBridge.java`: `getTracks`; Java-side grouping helpers live in the bridge runtime | `include/exoplayer_bridge.h`: `TracksSnapshot`, `TrackGroupSnapshot`, `TrackInfo` | `exoplayer_cppbridge_jni_common.cpp`: `CreateJavaTracks`, `FromJavaTracks` | tracks query, listener callbacks, analytics tracks changed |
| cues | `CppCue.java` | `CppExoPlayerBridge.java`: `getCurrentCues` and cue callbacks | `include/exoplayer_bridge.h`: `CueSnapshot` | `exoplayer_cppbridge_jni_common.cpp`: `CreateJavaCueArray`, `FromJavaCues` | current cues query, listener callbacks, analytics cues |
| seek parameters | `CppSeekParameters.java` | `CppBridgeConverters.java`: `toSeekParameters`, `fromSeekParameters`; `CppExoPlayerBridge.java`: `setSeekParameters`, `getSeekParameters` | `include/exoplayer_bridge.h`: `SeekParametersDescriptor` | `exoplayer_cppbridge_jni_common.cpp`: `FromJavaSeekParameters` | `exoplayer_cppbridge_jni_bridge.cpp`: set/get seek parameters |
| audio attributes | Java uses `int[]` transport from `CppExoPlayerBridge.java`: `getAudioAttributesConfig` / `setAudioAttributesConfig` | `CppExoPlayerBridge.java`: audio config getters/setters | `include/exoplayer_bridge.h`: `AudioAttributesDescriptor` | `exoplayer_cppbridge_jni_common.cpp`: `FromJavaAudioAttributes` | audio attribute query and setter flow |
| aux effect info | no Java DTO file; scalar runtime transport | `CppExoPlayerBridge.java`: `setAuxEffectInfoConfig`, `clearAuxEffectInfo` | `include/exoplayer_bridge.h`: `AuxEffectInfoDescriptor` | direct scalar JNI call | `SetAuxEffectInfo`, `ClearAuxEffectInfo` |
| scrubbing mode parameters | no Java DTO file; Java returns string rows for getter | `CppExoPlayerBridge.java`: `setScrubbingModeParametersConfig`, `getScrubbingModeParametersConfig` | `include/exoplayer_bridge.h`: `ScrubbingModeParametersDescriptor` | bridge-side string parsing in `exoplayer_cppbridge_jni_bridge.cpp` | `SetScrubbingModeParameters`, `GetScrubbingModeParameters` |
| codec parameters | `CppCodecParameter.java` | `CppExoPlayerBridge.java`: `setAudioCodecParametersConfig`, `setVideoCodecParametersConfig`, `toCodecParameters`, `fromCodecParameters` | `include/exoplayer_bridge.h`: `CodecParameterDescriptor`, `CodecParametersDescriptor` | `exoplayer_cppbridge_jni_bridge.cpp`: `CreateJavaCodecParameterArray`; `exoplayer_cppbridge_jni_common.cpp`: `CreateJavaByteArray`, `FromJavaCodecParameterArray` | setters plus `CodecParametersChangeListener` callback paths |
| video frame metadata callback | no standalone Java DTO; Java forwards `Format` and `MediaFormat` fields from `VideoFrameMetadataListener` | `CppExoPlayerBridge.java`: `setNativeVideoFrameMetadataListener`, `nativeOnVideoFrameAboutToBeRendered` | `include/exoplayer_bridge.h`: `VideoFrameMetadataSnapshot` | scalar JNI callback fields plus reduced format/media-format assembly | `SetVideoFrameMetadataListener` and `SimulateVideoFrameAboutToBeRenderedForTest` |
| camera motion callback | no standalone Java DTO; Java forwards `CameraMotionListener` values | `CppExoPlayerBridge.java`: `setNativeCameraMotionListener`, `nativeOnCameraMotion`, `nativeOnCameraMotionReset` | `include/exoplayer_bridge.h`: `CameraMotionSnapshot` | `exoplayer_cppbridge_jni_common.cpp`: `CreateJavaFloatArray`, `JFloatArrayToVector` | `SetCameraMotionListener` and camera-motion/reset simulation paths |
| device info | `CppDeviceInfo.java` | `CppExoPlayerBridge.java`: `getDeviceInfo` | `include/exoplayer_bridge.h`: `DeviceInfoDescriptor` | `exoplayer_cppbridge_jni_common.cpp`: `FromJavaDeviceInfo` | device info query, listener callbacks, analytics device-info changed |
| video size | `CppVideoSize.java` | `CppExoPlayerBridge.java`: `getVideoSize` | `include/exoplayer_bridge.h`: `VideoSizeSnapshot` | `exoplayer_cppbridge_jni_common.cpp`: `FromJavaVideoSize` | video size query, listener callbacks, analytics video-size changed |
| playback parameters | `CppPlaybackParameters.java` | `CppExoPlayerBridge.java`: playback-parameters getter/callback path | `include/exoplayer_bridge.h`: `PlaybackParametersSnapshot` | `exoplayer_cppbridge_jni_common.cpp`: `FromJavaPlaybackParameters` | current playback query and playback-parameters listener/analytics paths |
| playback snapshot | no standalone Java DTO; summary is built inside runtime/test helpers | `CppExoPlayerBridge.java`: direct player getters | `include/exoplayer_bridge.h`: `PlaybackSnapshot` | `exoplayer_cppbridge_jni_bridge.cpp`: snapshot assembly logic | runtime query helpers and smoke summaries |
| timeline | no single Java DTO; Java runtime exposes row strings and snapshot helpers | `CppExoPlayerBridge.java`: `getTimelineSnapshotData`, `getTimelineWindowRows`, `getTimelinePeriodRows` | `include/exoplayer_bridge.h`: `TimelineSnapshot`, `TimelineWindowSnapshot`, `TimelinePeriodSnapshot`, `TimelineDetailsSnapshot` | `exoplayer_cppbridge_jni_bridge.cpp`: `SplitString`, numeric parsers, row-to-struct parsing | timeline query helpers and timeline-related callbacks |
| analytics aggregate | no standalone Java DTO; Java runtime serializes analytics strings | `CppExoPlayerBridge.java`: `getAnalyticsStrings`, `dispatchAnalyticsUpdated` | `include/exoplayer_bridge.h`: `AnalyticsSnapshot` | `exoplayer_cppbridge_jni_bridge.cpp`: analytics string parsing and listener dispatch | analytics query and aggregate listener callback |
| video effects | `CppVideoEffect.java` | `CppBridgeConverters.java`: `toVideoEffects`; `CppExoPlayerBridge.java`: `setVideoEffects` | `include/exoplayer_bridge.h`: `VideoEffectDescriptor` | `exoplayer_cppbridge_jni_common.cpp`: `CreateJavaVideoEffectArray` | runtime `setVideoEffects` and test helpers |
| image output | no Java DTO file; runtime forwards bitmap metadata directly | `CppExoPlayerBridge.java`: image output callbacks | `include/exoplayer_bridge.h`: `ImageFrameSnapshot` | `exoplayer_cppbridge_jni_bridge.cpp`: image callback assembly | image output listener bridge |

### Suggested Exact Search Keys

Use these search chains when you already know the data object name and want exact implementation
entrypoints on both sides of the bridge.

| Object family | Java DTO search key | Java converter search key | C++ struct search key | JNI helper search key | Runtime/callback search key |
| --- | --- | --- | --- | --- | --- |
| media item | `class CppMediaItem` | `toMediaItem(` / `fromMediaItem(` | `struct MediaItemDescriptor` | `CreateJavaMediaItem(` / `FromJavaMediaItem(` | `getCurrentMediaItem(` / `setMediaItem(` / `nativeOnAnalyticsMediaItemTransition` |
| media metadata | `class CppMediaMetadata` | `toMediaMetadata(` / `fromMediaMetadata(` | `struct MediaMetadataSnapshot` | `CreateJavaMediaMetadata(` / `FromJavaMediaMetadata(` | `getMediaMetadata(` / `setPlaylistMetadata(` / `nativeOnMediaMetadataChanged` |
| track selection parameters | `class CppTrackSelectionParameters` | `toTrackSelectionParameters(` / `fromTrackSelectionParameters(` | `struct TrackSelectionParametersDescriptor` | `FromJavaTrackSelectionParameters(` | `setTrackSelectionParameters(` / `getTrackSelectionParameters(` |
| tracks | `class CppTracks` | no dedicated converter method; runtime assembles directly | `struct TracksSnapshot` | `CreateJavaTracks(` / `FromJavaTracks(` | `getTracks(` / `nativeOnTracksChanged` / `nativeOnAnalyticsTracksChanged` |
| track group | `class CppTrackGroup` | no dedicated converter method; runtime assembles directly | `struct TrackGroupSnapshot` | `CreateJavaTracks(` / `FromJavaTracks(` | `getTracks(` / track listener payloads |
| track info / format | `class CppTrackInfo` | no dedicated converter method; runtime assembles directly | `struct TrackInfo` | `CreateJavaTracks(` / `FromJavaTracks(` | `getTracks(` / analytics tracks changed |
| cue | `class CppCue` | no dedicated converter method; runtime uses cue arrays directly | `struct CueSnapshot` | `CreateJavaCueArray(` / `FromJavaCues(` | `getCurrentCues(` / `nativeOnCues` / `nativeOnAnalyticsCues` |
| seek parameters | `class CppSeekParameters` | `toSeekParameters(` / `fromSeekParameters(` | `struct SeekParametersDescriptor` | `FromJavaSeekParameters(` | `setSeekParameters(` / `getSeekParameters(` |
| audio attributes | no `CppAudioAttributes`; Java transport is `int[]` | no converter class; runtime uses `getAudioAttributesConfig(` / `setAudioAttributesConfig(` | `struct AudioAttributesDescriptor` | `FromJavaAudioAttributes(` | `GetAudioAttributes(` / `SetAudioAttributes(` |
| aux effect info | no Java DTO class | no converter class; runtime uses scalar setter | `struct AuxEffectInfoDescriptor` | direct scalar JNI call | `SetAuxEffectInfo(` / `ClearAuxEffectInfo(` |
| scrubbing mode parameters | no Java DTO class | runtime uses `getScrubbingModeParametersConfig(` string rows | `struct ScrubbingModeParametersDescriptor` | bridge-side string parsing | `SetScrubbingModeParameters(` / `GetScrubbingModeParameters(` |
| codec parameters | `class CppCodecParameter` | `toCodecParameters(` / `fromCodecParameters(` | `struct CodecParameterDescriptor` / `struct CodecParametersDescriptor` | `CreateJavaCodecParameterArray(` / `FromJavaCodecParameterArray(` | `SetAudioCodecParameters(` / `SetVideoCodecParameters(` / codec-parameter listener callbacks |
| video frame metadata callback | no Java DTO class | `setNativeVideoFrameMetadataListener(` | `struct VideoFrameMetadataSnapshot` | scalar JNI callback conversion | `nativeOnVideoFrameAboutToBeRendered` / `SetVideoFrameMetadataListener(` |
| camera motion callback | no Java DTO class | `setNativeCameraMotionListener(` | `struct CameraMotionSnapshot` | `CreateJavaFloatArray(` / `JFloatArrayToVector(` | `nativeOnCameraMotion` / `nativeOnCameraMotionReset` / `SetCameraMotionListener(` |
| device info | `class CppDeviceInfo` | no converter class; runtime returns DTO directly | `struct DeviceInfoDescriptor` | `FromJavaDeviceInfo(` | `getDeviceInfo(` / `nativeOnDeviceInfoChanged` / `nativeOnAnalyticsDeviceInfoChanged` |
| video size | `class CppVideoSize` | no converter class; runtime returns DTO directly | `struct VideoSizeSnapshot` | `FromJavaVideoSize(` | `getVideoSize(` / `nativeOnVideoSizeChanged` |
| playback parameters | `class CppPlaybackParameters` | `fromPlaybackParameters(` and runtime playback-parameters callbacks | `struct PlaybackParametersSnapshot` | `FromJavaPlaybackParameters(` | `getPlaybackParameters(` / `nativeOnPlaybackParametersChanged` |
| playback snapshot | no Java DTO class | no converter; built in bridge/test helper code | `struct PlaybackSnapshot` | snapshot assembly in `exoplayer_cppbridge_jni_bridge.cpp` | `GetSnapshot(` / configured-player smoke helpers |
| timeline summary | no Java DTO class | runtime emits summary/row strings | `struct TimelineSnapshot` | `SplitString(` plus timeline parsing in bridge | `getTimelineSnapshotData(` |
| timeline window | no Java DTO class | runtime emits window rows | `struct TimelineWindowSnapshot` | `SplitString(` plus timeline parsing in bridge | `getTimelineWindowRows(` / timeline listener payload |
| timeline period | no Java DTO class | runtime emits period rows | `struct TimelinePeriodSnapshot` | `SplitString(` plus timeline parsing in bridge | `getTimelinePeriodRows(` / timeline listener payload |
| analytics aggregate | no Java DTO class | runtime emits analytics strings | `struct AnalyticsSnapshot` | `SplitString(` / numeric parsers in bridge | `getAnalyticsStrings(` / `dispatchAnalyticsUpdated(` |
| video effect | `class CppVideoEffect` | `toVideoEffects(` | `struct VideoEffectDescriptor` | `CreateJavaVideoEffectArray(` | `setVideoEffects(` |
| player message | no Java DTO class in current bridge package | runtime/player-message helpers | `struct PlayerMessageDescriptor` | player-message parsing helpers in bridge | `SendPlayerMessage(` / `CancelPlayerMessage(` |
| image output | no Java DTO class in current bridge package | runtime image callbacks | `struct ImageFrameSnapshot` | image snapshot assembly in bridge | `nativeOnImageOutputAvailable` |

## Full-Support Planning View

Use this section when deciding what still needs to be built next. It is stricter than the reduced
DTO tracker below.

| Object family | Full-support status | Currently supported | Still missing |
| --- | --- | --- | --- |
| `MediaItem` | Partial | reduced descriptor, subtitles/clipping/live/DRM, tag/adsId/requestMetadata opaque-token baselines | full arbitrary-object semantics and full Java object parity |
| `Timeline` | Partial | reduced summary/window/period snapshots, uid/id/manifest token baselines, multi-window/multi-period smoke visibility | full Java `Timeline.Window` / `Timeline.Period` semantics |
| `Tracks` | Partial | reduced tracks/group/format snapshots, group and label token baselines, representative query/listener coverage | full `Tracks.Group` / `Format` parity and deeper second-group parity |
| `MediaMetadata` | Partial | representative text fields, artwork, extras token baseline, query/listener/playlist smoke coverage | full Java `MediaMetadata` semantics beyond reduced snapshot |
| `Cue` | Partial | representative text, bitmap token baseline, layout/style smoke coverage, query/listener/analytics visibility | full Java `Cue` styled-text and bitmap-object parity |

Field observability conventions:

- `Directly observed by smoke`: fields that appear in current smoke summaries and have matching test
  assertions.
- `Present in bridge but not directly smoke-observed`: fields that exist in the reduced DTO and are
  serialized through the bridge, but are not explicitly asserted by the current smoke suite.

## 1. Construction And Runtime Config

| Java type | C++ type | Status | Preserved fields / concepts | Smoke test reference |
| --- | --- | --- | --- | --- |
| `ExoPlayer.Builder` config | `PlayerConfig` | Done | audio focus, noisy handling, lazy prep, seek increments, wake mode, priority, preload target | `CppBridgeNativeSmokeTest.nativeBuilderConfigSmokeTest_returnsConfiguredSnapshot`; `CppBridgeNativeSmokeTest.nativeBuilderBuildSmokeTest_buildsConfiguredPlayer`; `CppBridgeNativeSmokeTest.nativeBuilderPreloadRoundTripSmokeTest_updatesAndRestoresPreloadTarget`; `CppBridgeNativePlayerInstrumentationTest.nativePlayerConfigFlagsSmokeTest_returnsCreateTimeFlags`; `CppBridgeNativePlayerInstrumentationTest.nativePreloadRoundTripSmokeTest_updatesAndRestoresPreloadTarget`; `CppBridgeNativePlayerInstrumentationTest.nativePreloadBridgeRuntimeSmokeTest_updatesAndMatchesBridgeFlags` |
| `DefaultMediaSourceFactory` baseline config | `PlayerConfig::MediaSourceFactoryConfig` | Done | token-registered and registry-generated-token factory selection, safe fallback-to-default behavior, replacement-registration observability, and multi-token isolation for native-create and builder-build paths, direct builder `SetMediaSourceFactoryConfig` coverage, subtitle parsing, selected-track loading, headers, user agent, timeouts, redirect flag, live defaults | `CppBridgeNativeSmokeTest.nativeBuilderConfigSmokeTest_returnsConfiguredSnapshot`; `CppBridgeNativeSmokeTest.nativeBuilderBuildSmokeTest_buildsConfiguredPlayer`; `CppBridgeNativeSmokeTest.nativeBuilderMediaSourceFactoryInjectionSmokeTest_buildsWithRegisteredFactoryToken`; `CppBridgeNativeSmokeTest.nativeBuilderMediaSourceFactoryFallbackSmokeTest_buildsWithDefaultFactoryWhenTokenIsMissing`; `CppBridgeNativeSmokeTest.nativeBuilderMediaSourceFactoryInjectionReplacementSmokeTest_usesLatestRegisteredFactory`; `CppBridgeNativeSmokeTest.nativeBuilderMediaSourceFactoryInjectionMultiTokenSmokeTest_keepsTokensIsolated`; `CppBridgeNativeSmokeTest.nativeBuilderMediaSourceFactoryGeneratedTokenSmokeTest_buildsWithGeneratedRegistryToken`; `CppBridgeNativePlayerInstrumentationTest.nativeMediaSourceFactoryConfigSmokeTest_returnsConfigSummary`; `CppBridgeNativePlayerInstrumentationTest.nativeMediaSourceFactoryInjectionSmokeTest_usesRegisteredFactoryToken`; `CppBridgeNativePlayerInstrumentationTest.nativeMediaSourceFactoryInjectionFallbackSmokeTest_fallsBackWhenTokenIsMissing`; `CppBridgeNativePlayerInstrumentationTest.nativeMediaSourceFactoryInjectionReplacementSmokeTest_usesLatestRegisteredFactory`; `CppBridgeNativePlayerInstrumentationTest.nativeMediaSourceFactoryInjectionMultiTokenSmokeTest_keepsTokensIsolated`; `CppBridgeNativePlayerInstrumentationTest.nativeMediaSourceFactoryGeneratedTokenSmokeTest_usesGeneratedRegistryToken` |
| priority wrapper | `ExoPlayerSdkPriorityTaskManager` | Done | add/remove/proceed state through reduced C++ wrapper plus SDK player `ClearPriorityTaskManager` path | `CppBridgeNativeSmokeTest.nativePriorityTaskManagerWrapperSmokeTest_returnsStructuredSummary`; `CppBridgeNativePlayerInstrumentationTest.nativePriorityTaskManagerSmokeTest_returnsBridgeState` |

## 2. Media And Playlist Objects

| Java type | C++ type | Status | Preserved fields / concepts | Smoke test reference |
| --- | --- | --- | --- | --- |
| `MediaItem` | `MediaItemDescriptor` | Done | uri, media id, mime type, source type, reduced local tag observability (`tagPresent`, `tagString`) plus opaque token round-trip baseline, reduced metadata identity fields, reduced request metadata (`mediaUri`, `searchQuery`, extras-presence`) plus opaque extras token baseline, reduced ads config plus opaque `adsId` token baseline, subtitles, clipping, live, DRM | `CppBridgeNativePlayerInstrumentationTest.nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary`; `nativeSourceTypeSmokeTest_returnsInferredMimeSummary`; `nativeHttpHlsDashPlaybackSmokeTest_preparesLocalStreamsThroughCppApi`; `nativeMediaItemAtSmokeTest_returnsSnapshotAndHandlesOutOfBounds`; `nativeMediaItemOpaqueTokenSmokeTest_resolvesRegisteredObjects` |
| `MediaItem.RequestMetadata` | `MediaItemDescriptor::RequestMetadataDescriptor` | Done | `mediaUri`, `searchQuery`, extras presence flag, extras key count, opaque extras token baseline | `nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary`; `nativeMediaItemAtSmokeTest_returnsSnapshotAndHandlesOutOfBounds`; `nativeMediaItemOpaqueTokenSmokeTest_resolvesRegisteredObjects` |
| `MediaItem.SubtitleConfiguration` | `MediaItemDescriptor::SubtitleConfigurationDescriptor` | Done | uri, mime type, language, label, id, selection flags, role flags | `nativeSubtitleSmokeTest_returnsSubtitleSummary`; `nativeMultiSubtitleSmokeTest_returnsSubtitleAndPreferenceSummary` |
| `MediaItem.ClippingConfiguration` | `MediaItemDescriptor::ClippingConfigurationDescriptor` | Done | start/end position, live/default/keyframe/unseekable flags | `nativeClippingSmokeTest_returnsClippingSummary` |
| `MediaItem.LiveConfiguration` | `MediaItemDescriptor::LiveConfigurationDescriptor` | Done | target/min/max offsets, min/max speed | `nativeLiveConfigurationSmokeTest_returnsLiveSummary` |
| `MediaItem.DrmConfiguration` | `MediaItemDescriptor::DrmConfigurationDescriptor` | Done | scheme UUID, license URI, request headers, forced session track types, key-set id, core flags | `nativeDrmSmokeTest_returnsDrmSummary` |
| `MediaItem.AdsConfiguration` | `MediaItemDescriptor::AdsConfigurationDescriptor` | Done | `adTagUri`, string `adsId` | `nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary`; `nativeMediaItemAtSmokeTest_returnsSnapshotAndHandlesOutOfBounds` |

## 3. Playback State And Query Snapshots

| Java type | C++ type | Status | Preserved fields / concepts | Smoke test reference |
| --- | --- | --- | --- | --- |
| synthetic player state aggregate | `PlaybackSnapshot` | Done | state, playWhenReady, isPlaying, loading, shuffle, index/count, repeat, positions, duration, volume, speed, last error | `nativeCreateConfiguredPlayerSnapshotForTest_returnsConfiguredState`; `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` |
| `PlaybackException` | `PlayerError` | Done | error code, message | `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary`; listener smoke |
| `PlaybackParameters` | `PlaybackParametersSnapshot` | Done | speed, pitch | `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary`; `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| `SeekParameters` | `SeekParametersDescriptor` | Done | tolerance before/after | `nativeSeekParametersSmokeTest_roundTripsSeekParameters` |
| `Player.PositionInfo` | `PositionInfoSnapshot` | Done | media item index, reduced media item plus nested tag opaque token baseline, period index, position, content position, ad indices | `nativeListenerSmokeTest_reportsExtendedCallbacks`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |
| `Player.Commands` | `AvailableCommandsSnapshot` | Done | integer command codes only | `nativeAvailableCommandsSmokeTest_returnsContainsStyleSummary`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |
| `Player.Events` | `PlayerEventsSnapshot` | Done | integer event codes only | `nativeListenerSmokeTest_reportsExtendedCallbacks`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |
| `Looper` | `ApplicationLooperDescriptor` | Done | thread name, thread id, current-thread match | `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` |

## 4. Track-Related Objects

| Java type | C++ type | Status | Preserved fields / concepts | Smoke test reference |
| --- | --- | --- | --- | --- |
| `TrackSelectionParameters` | `TrackSelectionParametersDescriptor` | Done | preferred audio/text language, arrays, role flags, viewport, max bitrate/size, text defaults, undetermined text, disabled track types, overrides | `nativeTrackSelectionRoundTripForTest_returnsUpdatedParameters` |
| `Tracks` | `TracksSnapshot` | Done | group array plus contains/selected/supported summary by type | `nativeCurrentTracksSmokeTest_returnsTracksSummary`; `CppBridgeNativeSmokeTest.nativeTracksSnapshotConversionSmokeTest_returnsStructuredSummary` |
| `Tracks.Group` | `TrackGroupSnapshot` | Done | group id plus opaque token baseline, type, adaptive support, selected, supported, tracks | `nativeCurrentTracksSmokeTest_returnsTracksSummary`; `CppBridgeNativeSmokeTest.nativeTracksSnapshotConversionSmokeTest_returnsStructuredSummary`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |
| representative `Format` fields | `TrackInfo` | Done | id, language, label plus opaque token baseline, label list language/value arrays, mime/container mime, codecs, bitrate/average bitrate/peak bitrate, metadata entry count plus metadata token, custom-data token, auxiliary track type, max input/reorder size, initialization-data count/total plus byte arrays, DRM scheme type plus scheme-data uuid/license/mime/bytes/has-data, subsample offset, preroll flag, width/height, decoded width/height, frame rate, rotation, pixel width-height ratio, projection length plus bytes, stereo mode, color info plus HDR static info and luma/chroma bitdepth, max sublayers, sample rate, channel count, PCM encoding, encoder delay/padding, accessibility/cue/tile/crypto fields, flags, support, selected | `nativeCurrentTracksSmokeTest_returnsTracksSummary`; `CppBridgeNativeSmokeTest.nativeTracksSnapshotConversionSmokeTest_returnsStructuredSummary`; `CppBridgeNativeSmokeTest.nativeTracksFullPayloadConversionSmokeTest_roundTripsFormatPayload`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |

## 5. Timeline Objects

| Java type | C++ type | Status | Preserved fields / concepts | Smoke test reference |
| --- | --- | --- | --- | --- |
| `Timeline` | `TimelineDetailsSnapshot` | Done | summary + window list + period list | `nativeCurrentTimelineSmokeTest_returnsTimelineDetails`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |
| timeline summary | `TimelineSnapshot` | Done | window/period count, empty, current/next/previous indices, has next/previous, current item dynamic/live/seekable | `nativeCurrentTimelineSmokeTest_returnsTimelineDetails`; `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` |
| `Timeline.Window` | `TimelineWindowSnapshot` | Done | media item index, media item id, media item uri, media item tag presence/string plus opaque token baseline, uid plus opaque token baseline, reduced live-configuration fields, manifest presence/string plus opaque token baseline, first/last period index, start/duration/default position fields, seekable/dynamic/live/placeholder | `nativeCurrentTimelineSmokeTest_returnsTimelineDetails`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary`; `nativeListenerSmokeTest_reportsExtendedCallbacks` |
| `Timeline.Period` | `TimelinePeriodSnapshot` | Done | id plus opaque token baseline, uid plus opaque token baseline, ads id plus opaque token baseline, window index, ad-group count, duration, position-in-window, placeholder | `nativeCurrentTimelineSmokeTest_returnsTimelineDetails`; `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary`; `CppBridgeNativeSmokeTest.nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` |

## 6. Audio / Device / Video / Metadata / Cue Objects

| Java type | C++ type | Status | Preserved fields / concepts | Smoke test reference |
| --- | --- | --- | --- | --- |
| `AudioAttributes` | `AudioAttributesDescriptor` | Done | content type, flags, usage, capture policy, spatialization behavior | `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` |
| `AuxEffectInfo` | `AuxEffectInfoDescriptor` | Done | effect ID, send level, clear-to-default behavior | `nativeAudioAndScrubbingParitySmokeTest_updatesAdvancedRuntimeControls` |
| `ScrubbingModeParameters` | `ScrubbingModeParametersDescriptor` | Done | min/max seek interval, seek-to-current-position delay, min/max playback speed | `nativeAudioAndScrubbingParitySmokeTest_updatesAdvancedRuntimeControls` |
| `CodecParameters` | `CodecParametersDescriptor` / `CodecParameterDescriptor` | Done | typed key/value entries for integer, long, float, string, byte-buffer, and null values, including setter, reduced listener callback delivery, and multi-listener immediate routing | `nativeCodecParametersParitySmokeTest_setsAudioAndVideoCodecParameters`; `nativeAuxiliaryCallbackParitySmokeTest_reportsCodecVideoAndCameraCallbacks`; `nativeCodecParametersMultiListenerParitySmokeTest_routesImmediateCallbacks` |
| `DeviceInfo` | `DeviceInfoDescriptor` | Done | playback type, min volume, max volume, routing controller id | `nativeDeviceAndSkipSilenceSmokeTest_returnsDeviceSummary` |
| `VideoSize` | `VideoSizeSnapshot` | Done | width, height, unapplied rotation degrees, pixel ratio | `nativeVideoAndMetadataSmokeTest_returnsQuerySummary` |
| `VideoFrameMetadataListener` callback | `VideoFrameMetadataSnapshot` | Done | presentation time, release time, representative `Format` id/mime/codecs/size/frame-rate/label/language/container MIME/bitrate/rotation/pixel-ratio/color/audio-shape/flags fields, media-format presence and summary string, plus representative `MediaFormat` mime/width/height/frame-rate/rotation/color-standard/color-range/color-transfer fields | `nativeAuxiliaryCallbackParitySmokeTest_reportsCodecVideoAndCameraCallbacks` |
| `CameraMotionListener` callback | `CameraMotionSnapshot` | Done | motion `timeUs`, rotation float vector, and reset callback delivery/removal behavior | `nativeAuxiliaryCallbackParitySmokeTest_reportsCodecVideoAndCameraCallbacks` |
| `MediaMetadata` | `MediaMetadataSnapshot` | Done | common text metadata, representative text opaque token baseline (`title`, `artist`, `albumTitle`, `albumArtist`, `displayTitle`, `subtitle`, `description`, `writer`, `author`, `composer`, `conductor`, `genre`, `compilation`, `station`), extras presence/key-count plus opaque token baseline, artwork uri/data/type, browsable/playable/folder fields, dates, credits, disc/track counts, media type | `nativeVideoAndMetadataSmokeTest_returnsQuerySummary`; `nativePlaylistMetadataSmokeTest_roundTripsPlaylistMetadata`; `nativePlaylistMetadataOpaqueTokenSmokeTest_resolvesRegisteredObjects` |
| `CueGroup` / `Cue` | `CueSnapshot` | Done | cue count, presentation time, cue text list plus representative text opaque token baseline, cue layout/style descriptors, bitmap opaque token baseline, bitmap height, shear, z-index, window color, bitmap presence | `CppBridgeNativeSmokeTest.nativeCueSnapshotConversionSmokeTest_returnsStructuredSummary`; `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary`; `nativeAnalyticsCuesSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics aggregate | `AnalyticsSnapshot` | Done | bitrate estimate, dropped frames, load started/completed counts, derived load delta, last audio/video mime, multi-update last-value overwrite semantics | `nativeAnalyticsSmokeTest_returnsAnalyticsSummary`; `nativeAnalyticsCallbackSmokeTest_reportsListenerDelivery`; `nativeAnalyticsListenerRegistrationSmokeTest_addsAndRemovesAnalyticsOnlyListener` |
| analytics audio underrun event | `AudioUnderrunEvent` | Done | buffer size, buffer size ms, elapsed since last feed ms, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsAudioUnderrunSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics dropped video frames event | `DroppedVideoFramesEvent` | Done | dropped frame count, elapsed ms, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsDroppedVideoFramesSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics bandwidth estimate event | `BandwidthEstimateEvent` | Done | elapsed ms, bytes transferred, bitrate estimate, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsBandwidthEstimateSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics load started event | `LoadStartedEvent` | Done | URI, data type, track type, retry count, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsLoadStartedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics load completed event | `LoadCompletedEvent` | Done | URI, data type, track type, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsLoadCompletedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics audio input format changed event | `AudioInputFormatChangedEvent` | Done | sample mime type, codecs, channel count, sample rate, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsAudioInputFormatChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics audio decoder initialized event | `AudioDecoderInitializedEvent` | Done | decoder name, initialized timestamp ms, initialization duration ms, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsAudioDecoderInitializedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics video decoder initialized event | `VideoDecoderInitializedEvent` | Done | decoder name, initialized timestamp ms, initialization duration ms, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsVideoDecoderInitializedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics audio decoder released event | `AudioDecoderReleasedEvent` | Done | decoder name, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsAudioDecoderReleasedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics video decoder released event | `VideoDecoderReleasedEvent` | Done | decoder name, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsVideoDecoderReleasedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics rendered first frame event | `AnalyticsRenderedFirstFrameEvent` | Done | render time ms, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsRenderedFirstFrameSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics video size changed event | `AnalyticsVideoSizeChangedEvent` | Done | width, height, pixel-width-height ratio, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsVideoSizeChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics audio position advancing event | `AudioPositionAdvancingEvent` | Done | playout-start system time ms, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsAudioPositionAdvancingSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics video frame processing offset event | `VideoFrameProcessingOffsetEvent` | Done | total processing offset us, frame count, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsVideoFrameProcessingOffsetSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics volume changed event | `VolumeChangedEvent` | Done | volume, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsVolumeChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics audio session id changed event | `AudioSessionIdChangedEvent` | Done | audio session id, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsAudioSessionIdChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics skip silence enabled changed event | `AnalyticsSkipSilenceEnabledChangedEvent` | Done | skip-silence-enabled flag, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsSkipSilenceEnabledChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics device volume changed event | `AnalyticsDeviceVolumeChangedEvent` | Done | volume, muted flag, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsDeviceVolumeChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics playback state changed event | `AnalyticsPlaybackStateChangedEvent` | Done | playback state, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsPlaybackStateChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics is playing changed event | `AnalyticsIsPlayingChangedEvent` | Done | is-playing flag, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsIsPlayingChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics play when ready changed event | `AnalyticsPlayWhenReadyChangedEvent` | Done | play-when-ready flag, reason, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsPlayWhenReadyChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics playback suppression reason changed event | `AnalyticsPlaybackSuppressionReasonChangedEvent` | Done | playback suppression reason, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsPlaybackSuppressionReasonChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics is loading changed event | `AnalyticsIsLoadingChangedEvent` | Done | is-loading flag, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsIsLoadingChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics repeat mode changed event | `AnalyticsRepeatModeChangedEvent` | Done | repeat mode, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsRepeatModeChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics shuffle mode changed event | `AnalyticsShuffleModeChangedEvent` | Done | shuffle-mode-enabled flag, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsShuffleModeChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics video input format changed event | `VideoInputFormatChangedEvent` | Done | sample mime type, codecs, width, height, frame rate, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsVideoInputFormatChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics playback parameters changed event | `AnalyticsPlaybackParametersChangedEvent` | Done | speed, pitch, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsPlaybackParametersChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics available commands changed event | `AnalyticsAvailableCommandsChangedEvent` | Done | reduced command-code list, representative first-command and contains-style observation, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsAvailableCommandsChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics events batch event | `AnalyticsEventsEvent` | Done | reduced event-code list, representative first-event and contains-style observation, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsEventsSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics seek back increment changed event | `AnalyticsSeekBackIncrementChangedEvent` | Done | seek-back increment ms, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsSeekBackIncrementChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics seek forward increment changed event | `AnalyticsSeekForwardIncrementChangedEvent` | Done | seek-forward increment ms, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsSeekForwardIncrementChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics max seek to previous position changed event | `AnalyticsMaxSeekToPreviousPositionChangedEvent` | Done | max-seek-to-previous-position ms, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsMaxSeekToPreviousPositionChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics timeline changed event | `AnalyticsTimelineChangedEvent` | Done | timeline-change reason only, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsTimelineChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics position discontinuity event | `AnalyticsPositionDiscontinuityEvent` | Done | discontinuity reason only, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsPositionDiscontinuitySmokeTest_reportsConcreteAnalyticsEvent` |
| analytics seek started event | `AnalyticsSeekStartedEvent` | Done | seek-start delivery marker, repeated delivery counting, remove-listener stop-delivery behavior | `nativeAnalyticsSeekStartedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics device info changed event | `DeviceInfoDescriptor` | Done | playback type, min volume, max volume, routing controller id, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsDeviceInfoChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics media metadata changed event | `MediaMetadataSnapshot` | Done | title, artist, display title, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsMediaMetadataChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics playlist metadata changed event | `MediaMetadataSnapshot` | Done | title, artist, display title, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsPlaylistMetadataChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics player error event | `PlayerError` | Done | error code, message, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsPlayerErrorSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics player error changed event | `PlayerError` | Done | error code, message, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsPlayerErrorChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics tracks changed event | `TracksSnapshot` | Done | reduced group count, first-group identity/type, first-track count, contains-audio/video flags, selection summary, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsTracksChangedSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics media item transition event | `AnalyticsMediaItemTransitionEvent` | Done | reduced media-item identity, source type, transition reason, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsMediaItemTransitionSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics cues event | `CueSnapshot` | Done | cue count, presentation time us, representative first cue text plus opaque text/bitmap token baseline, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsCuesSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics metadata event | `AnalyticsMetadataEvent` | Done | entry count, representative first-entry type, representative first-entry text, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsMetadataSmokeTest_reportsConcreteAnalyticsEvent` |
| analytics load error event | `AnalyticsLoadErrorEvent` | Done | uri, data type, track type, representative error message, was-canceled flag, multi-update last-value overwrite semantics, remove-listener stop-delivery behavior | `nativeAnalyticsLoadErrorSmokeTest_reportsConcreteAnalyticsEvent` |
| image output aggregate | `ImageFrameSnapshot` | Done | `presentationTimeUs`, width, height, byte count, allocation byte count, row bytes, alpha flag, premultiplied flag, mutable flag, bitmap config, including non-`ARGB_8888` config observation, runtime enable/disable transitions, listener disable/remove stop-delivery behavior, reattach behavior | `nativeImageOutputSmokeTest_returnsListenerSummary` |
| player message | `PlayerMessageDescriptor` / `PlayerMessageResult` | Done | reduced type/payload/target/delete/cancel/block timeout result model | `nativePlayerMessageSmokeTest_returnsDeliverySummary`; `nativeTimedPlayerMessageSmokeTest_returnsRuntimeDeliverySummary`; `nativeRendererPlayerMessageSmokeTest_returnsRuntimeSummary`; `nativePlayerMessageCancelSmokeTest_returnsCanceledSummary` |
| video effects | `VideoEffectDescriptor` | Done | reduced scale/rotate, RGB adjustment, and presentation descriptor parity, plus clear/reset, reapply ordering, duplicate effect-type behavior, and boundary/default-value observation for all three supported effect families | `CppBridgeNativeSmokeTest.nativeVideoEffectsConversionSmokeTest_returnsStructuredSummary` |

## 7. Current Reduced-Model Boundaries

- current `Done` rows are reduced-endpoint complete, not full `api.txt` object parity
- no full Java `MediaItem` parity
- `MediaItem.LocalConfiguration.tag` now has presence-plus-string observability and opaque token round-trip baseline, but still not full arbitrary Java object semantics across processes or persistence boundaries
- `MediaItem.AdsConfiguration.adsId` now has string identity plus opaque token round-trip baseline, but still not full arbitrary Java object semantics across processes or persistence boundaries
- `MediaItem.RequestMetadata.extras` now has presence-plus-opaque-token round-trip baseline, but still not a fully decoded `Bundle` value-model parity surface in C++
- no full Java `Tracks` or `Timeline` parity
- no full `AnalyticsListener` parity
- no full `Cue` bitmap object transfer
- no arbitrary Java target parity for `PlayerMessage`
- no fully arbitrary `MediaSource.Factory` injection beyond token-registered and registry-generated-token baseline support
- no full bitmap-payload transfer for image output beyond reduced frame and bitmap-layout metadata

## 8. Field-Level Observability Notes

### `MediaItemDescriptor`

Directly observed by smoke:

- `uri`
- `media_id`
- `mime_type`
- `source_type`
- `tag_present`
- `tag_string`
- `tag_token`
- `ads_configuration.ad_tag_uri`
- `ads_configuration.ads_id`
- `ads_configuration.ads_id_token`
- `request_metadata.media_uri`
- `request_metadata.search_query`
- `request_metadata.extras_present`
- `request_metadata.extras_token`
- `subtitle_configurations.size()`
- representative subtitle fields:
  `subtitle_configurations[0].uri`,
  `subtitle_configurations[0].mime_type`,
  `subtitle_configurations[0].language`,
  `subtitle_configurations[0].label`,
  `subtitle_configurations[0].id`,
  `subtitle_configurations[0].selection_flags`,
  `subtitle_configurations[0].role_flags`
- representative secondary subtitle entry fields:
  `subtitle_configurations[1].uri`,
  `subtitle_configurations[1].language`,
  `subtitle_configurations[1].label`,
  `subtitle_configurations[1].id`
- `media_metadata.title`
- `media_metadata.artist`
- `media_metadata.album_title`
- `media_metadata.album_artist`
- `media_metadata.display_title`
- `media_metadata.subtitle`
- `media_metadata.description`
- `media_metadata.author`
- `media_metadata.composer`
- `media_metadata.conductor`
- `media_metadata.release_month`
- `media_metadata.release_day`
- `media_metadata.compilation`
- `media_metadata.artwork_uri`
- `media_metadata.artwork_data.size()`
- `media_metadata.artwork_data_type`
- `request_metadata.media_uri`
- `request_metadata.search_query`
- `request_metadata.extras_present`
- representative clipping fields:
  `clipping_configuration.start_position_ms`,
  `clipping_configuration.end_position_ms`,
  `clipping_configuration.relative_to_live_window`,
  `clipping_configuration.relative_to_default_position`,
  `clipping_configuration.starts_at_key_frame`,
  `clipping_configuration.allow_unseekable_media`
- representative live fields:
  `live_configuration.target_offset_ms`,
  `live_configuration.min_offset_ms`,
  `live_configuration.max_offset_ms`,
  `live_configuration.min_playback_speed`,
  `live_configuration.max_playback_speed`
- representative DRM fields:
  `drm_configuration.scheme_uuid`,
  `drm_configuration.license_uri`,
  `drm_configuration.license_request_header_names.size()`,
  representative `drm_configuration.license_request_header_names[0/1]` and values,
  `drm_configuration.forced_session_track_types.size()`,
  `drm_configuration.key_set_id.size()`,
  `drm_configuration.multi_session`,
  `drm_configuration.force_default_license_uri`,
  `drm_configuration.play_clear_content_without_key`

Present in bridge but not directly smoke-observed:

- no additional high-signal `MediaItemDescriptor` fields remain completely unobserved in the current smoke set

Primary smoke evidence:

- `nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary`
- `nativeMediaItemAtSmokeTest_returnsSnapshotAndHandlesOutOfBounds`
- `nativeSourceTypeSmokeTest_returnsInferredMimeSummary`
- `nativeHttpHlsDashPlaybackSmokeTest_preparesLocalStreamsThroughCppApi`

### `TimelineWindowSnapshot`

Directly observed by smoke:

- `media_item_index`
- `media_item_id`
- `media_item_uri`
- `media_item_tag_present`
- `media_item_tag_string`
- `media_item_tag_token`
- `uid`
- `uid_token`
- `live_configuration_present`
- `live_target_offset_ms`
- `live_min_offset_ms`
- `live_max_offset_ms`
- `live_min_playback_speed`
- `live_max_playback_speed`
- `manifest_present`
- `manifest_string`
- `manifest_token`
- `first_period_index`
- `last_period_index`
- `presentation_start_time_ms`
- `window_start_time_ms`
- `elapsed_realtime_epoch_offset_ms`
- `duration_ms`
- `default_position_ms`
- `position_in_first_period_ms`
- `position_in_first_period_us`
- `default_position_us`
- `duration_us`
- `is_seekable`
- `is_dynamic`
- `is_live`
- `is_placeholder`

Present in bridge but not directly smoke-observed:

- no additional high-signal window timing fields remain completely unobserved in the current smoke set

Primary smoke evidence:

- `nativeCurrentTimelineSmokeTest_returnsTimelineDetails`
- `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary`
- `nativeListenerSmokeTest_reportsExtendedCallbacks`

### `TimelinePeriodSnapshot`

Directly observed by smoke:

- `id`
- `id_token`
- `uid`
- `uid_token`
- `ads_id`
- `ads_id_token`
- `window_index`
- `ad_group_count`
- `duration_ms`
- `duration_us`
- `position_in_window_ms`
- `position_in_window_us`
- `is_placeholder`

Present in bridge but not directly smoke-observed:

- no additional high-signal period timing/count fields remain completely unobserved in the current smoke set

Primary smoke evidence:

- `nativeCurrentTimelineSmokeTest_returnsTimelineDetails`
- `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary`
- `nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary`

### `MediaMetadataSnapshot`

Directly observed by smoke:

- `title`
- `title_token`
- `artist`
- `artist_token`
- `album_title`
- `album_artist`
- `display_title`
- `display_title_token`
- `subtitle`
- `description`
- `artwork_uri`
- `artwork_data.size()`
- `artwork_data_type`
- `duration_ms`
- `track_number`
- `total_track_count`
- `is_browsable`
- `is_playable`
- `folder_type`
- `recording_year`
- `recording_month`
- `recording_day`
- `release_year`
- `release_month`
- `release_day`
- `writer`
- `author`
- `composer`
- `conductor`
- `disc_number`
- `total_disc_count`
- `genre`
- `compilation`
- `media_type`
- `station`

Present in bridge but not directly smoke-observed:

- no additional high-signal metadata identity/date/credit fields remain completely unobserved in the current smoke set

Primary smoke evidence:

- `nativeVideoAndMetadataSmokeTest_returnsQuerySummary`
- `nativePlaylistMetadataSmokeTest_roundTripsPlaylistMetadata`
- `nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary`
- `nativeMediaItemAtSmokeTest_returnsSnapshotAndHandlesOutOfBounds`
- `nativeListenerSmokeTest_reportsExtendedCallbacks`

### `TracksSnapshot` / `TrackGroupSnapshot` / `TrackInfo`

Directly observed by smoke:

- group count
- group `id`
- group `type`
- group adaptive support
- group selected/supported summary flags
- track count per first group
- first track `id`
- first track `language`
- first track `label`
- first track `label_token`
- first track label list language/value payload
- first track metadata/custom-data opaque tokens
- first track `auxiliary_track_type`
- group-level `group_token`
- first track `mime_type`
- first track `container_mime_type`
- first track `codecs`
- first track `bitrate`
- first track width/height/frame rate in value smoke
- first track initialization-data byte vectors
- first track DRM scheme type, scheme-data uuid/license/mime/bytes/has-data
- first track projection byte vector
- first track `ColorInfo` HDR static info length and luma/chroma bitdepth
- first track `accessibility_channel`
- first track `role_flags`
- first track `selection_flags`
- first track `selected`
- first track `format_support`
- first track `supported_within_capabilities`
- top-level contains/selected/supported booleans by media type
- representative audio track `label` plus opaque token baseline, `language`, `sample_rate`, and `channel_count`

Present in bridge but not directly smoke-observed:

- no additional high-signal secondary support flags remain completely unobserved in the current smoke set

Primary smoke evidence:

- `nativeCurrentTracksSmokeTest_returnsTracksSummary`
- `nativeTracksSnapshotConversionSmokeTest_returnsStructuredSummary`
- `nativeTracksFullPayloadConversionSmokeTest_roundTripsFormatPayload`
- `nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary`
- `nativeListenerSmokeTest_reportsExtendedCallbacks`

### `CueSnapshot`

Directly observed by smoke:

- cue count
- presentation time in value smoke
- presentation time in current player query smoke for a two-cue case
- representative cue text
- representative cue text opaque token baseline
- representative cue bitmap opaque token baseline
- text alignment
- multi-row alignment
- `line`
- `line_type`
- `line_anchor`
- `position`
- `position_anchor`
- `size`
- `bitmap_height`
- `text_size`
- `text_size_type`
- `vertical_type`
- `shear_degrees`
- `z_index`
- `window_color_set`
- `window_color`
- `has_bitmap`
- `texts.size()`
- representative `texts[0]` and `texts[1]`
- representative second cue text/token and bitmap-token absence in current player and analytics smoke

Present in bridge but not directly smoke-observed:

- no additional high-signal cue text vector fields remain completely unobserved in the current smoke set

Primary smoke evidence:

- `nativeCueSnapshotConversionSmokeTest_returnsStructuredSummary`
- `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary`
- `nativeListenerSmokeTest_reportsExtendedCallbacks`

### `PlaybackSnapshot` / Related Query DTOs

Directly observed by smoke:

- playback state
- playWhenReady
- isPlaying
- isLoading
- shuffle mode
- repeat mode
- current media item index
- media item count
- current/buffered/duration positions
- volume
- player error code
- playback speed / pitch
- buffered/content/live/ad query fields included in audio/query smoke

Present in bridge but not directly smoke-observed:

- full error message stability
- all snapshot fields in every callback path

Primary smoke evidence:

- `nativeCreateConfiguredPlayerSnapshotForTest_returnsConfiguredState`
- `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary`
- `nativeListenerSmokeTest_reportsExtendedCallbacks`
