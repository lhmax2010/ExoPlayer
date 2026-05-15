# C++ Bridge Test Results Template

Last updated: 2026-03-18

Use this file after running validation in the new environment. Copy it to a dated file if you want
to keep multiple runs.

## Run Metadata

- Date:
- Engineer:
- Machine:
- JDK:
- Android SDK:
- NDK:
- Device / Emulator:
- Branch / Commit:
- Gradle command path:

## Validation Commands

- Full validation:
  - `./scripts/cppbridge/run_validation.sh`
  - `python3 ./scripts/cppbridge/run_validation.py`
- Demo launch:
  - `./scripts/cppbridge/launch_demo.sh`
  - `python3 ./scripts/cppbridge/launch_demo.py`

## Build Result

- `:lib-exoplayer-cppbridge:assembleDebugAndroidTest`
  - Result: `PASS / FAIL`
  - Notes:
- `:demo-cppbridge:installDebug`
  - Result: `PASS / FAIL`
  - Notes:

## Instrumentation Result

### `CppBridgeNativeSmokeTest`

- Result: `PASS / FAIL`
- Notes:

### `CppBridgeNativePlayerInstrumentationTest`

- Result: `PASS / FAIL`
- Notes:

## Key Output Spot Checks

- `lifecycle-ok`
  - Result:
- `released=1`
  - Result:
- `state=1`
  - Result:
- `window0MediaId=timeline-query-item-1`
  - Result:
- `timelineWindow0MediaId=listener-item-1`
  - Result:
- `timelineWindow0MediaUri=https://example.com/listener.mp4`
  - Result:
- `tokenCount=4`
  - Result:
- `mediaTitle=Video Metadata Title`
  - Result:
- `mediaArtworkUri=https://example.com/video-metadata-artwork.jpg`
  - Result:
- `playlistArtworkUri=https://example.com/metadata-playlist-artwork.jpg`
  - Result:
- `sourceType=2`
  - Result:
- `analyticsCb=2`
  - Result:
- `bufferSize=4096`
  - Result:
- `droppedFrames=8`
  - Result:
- `bitrateEstimate=999999`
  - Result:
- `uri=https://example.com/analytics-final.m3u8`
  - Result:
- `uri=https://example.com/analytics-final-complete.m3u8`
  - Result:
- `volume=0.750000`
  - Result:
- `audioSessionId=700042`
  - Result:
- `skipSilenceEnabled=1`
  - Result:
- `volume=7`
  - Result:
- `muted=0`
  - Result:
- `playbackState=3`
  - Result:
- `isPlaying=1`
  - Result:
- `playWhenReady=1`
  - Result:
- `reason=2`
  - Result:
- `playbackSuppressionReason=1`
  - Result:
- `isLoading=1`
  - Result:
- `repeatMode=2`
  - Result:
- `shuffleModeEnabled=1`
  - Result:
- `sampleMimeType=audio/final`
  - Result:
- `sampleMimeType=video/final`
  - Result:
- `imageCount=3`
  - Result:
- `factoryToken=test-injected-media-source-factory`
  - Result:
- `fallbackApplied=1`
  - Result:
- `tokensIsolated=1`
  - Result:
- `lastAllocationByteCount=384`
  - Result:
- `reattachLastBitmapConfig=RGB_565`
  - Result:
- `afterReapply=effectCount=7`
  - Result:
- `effect6=presentation:width=320:height=240:layout=1`
  - Result:

## Smoke To Mapping Checklist

| Smoke test | Expected marker(s) to capture | Mapping doc to cross-check | Result | Notes |
| --- | --- | --- | --- | --- |
| `nativeCurrentTimelineSmokeTest_returnsTimelineDetails` | `window0MediaId=timeline-query-item-1`; `window0MediaUri=https://example.com/current-timeline-one.m3u8`; `window0TagPresent=1`; `window0TagString=timeline-query-tag-1`; `window0TagTokenPresent=1`; `window1MediaId=timeline-query-item-2`; `window1MediaUri=https://example.com/current-timeline-two.mp4`; `window1TagString=timeline-query-tag-2`; `window1TagTokenPresent=1`; `window1LiveConfigurationPresent=1`; `window1LiveTargetOffsetMs=7100`; `window1LiveMinOffsetMs=6400`; `window1LiveMaxOffsetMs=8200`; `window1LiveMinSpeed=0.930000`; `window1LiveMaxSpeed=1.070000`; `window1ManifestPresent=`; `window1ManifestString=`; `window1ManifestTokenPresent=`; `window1DurationMs=`; `window1Seekable=`; `window1Live=`; `period0Uid=`; `period1Id=`; `period1Uid=` | `DATA_STRUCTURE_MAPPING.md` Timeline section |  |  |
| `nativeCurrentTracksSmokeTest_returnsTracksSummary` | `group0Id=video-group`; `group0TokenPresent=1`; `track0LabelTokenPresent=1`; `track0AverageBitrate=2000000`; `track0PeakBitrate=2500000`; `track0RotationDegrees=90`; `track0PixelRatio=1.250000`; `track0Color=1:2:3`; `group1Id=audio-group`; `group1TokenPresent=1`; `group1Track0LabelTokenPresent=1`; `group1Track0AverageBitrate=160000`; `group1Track0PeakBitrate=192000` | `DATA_STRUCTURE_MAPPING.md` Tracks section |  |  |
| `nativeTracksSnapshotConversionSmokeTest_returnsStructuredSummary` | `track0AverageBitrate=2000000`; `track0PeakBitrate=2500000`; `track0RotationDegrees=90`; `track0PixelRatio=1.250000`; `track0Color=1:2:3`; `group1Track0AverageBitrate=160000`; `group1Track0PeakBitrate=192000` | `DATA_STRUCTURE_MAPPING.md` Tracks section; `DATA_STRUCTURE_QUICK_REFERENCE.md` `CppTrackInfo` row |  |  |
| `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` | `cueCount=2`; `cuePresentationTimeUs=456789`; `cue0Text=Query Cue 1`; `cue0TextTokenPresent=1`; `cue0BitmapTokenPresent=1`; `cue1Text=Query Cue 2`; `cue1TextTokenPresent=1`; `cue1BitmapTokenPresent=0` | `API_MAPPING_STATUS.md` audio/query getters; `DATA_STRUCTURE_MAPPING.md` `CueSnapshot` |  |  |
| `nativeListenerSmokeTest_reportsExtendedCallbacks` | `timelineWindow0MediaId=listener-item-1`; `timelineWindow0TagPresent=1`; `timelineWindow0TagString=listener-tag-1`; `timelineWindow0TagTokenPresent=1`; `timelineWindow1MediaItemIndex=1`; `timelineWindow1MediaId=listener-item-2`; `timelineWindow1TagString=listener-tag-2`; `timelineWindow1TagTokenPresent=1`; `timelineWindow1LiveConfigurationPresent=1`; `timelineWindow1LiveTargetOffsetMs=6100`; `timelineWindow1LiveMinOffsetMs=5200`; `timelineWindow1LiveMaxOffsetMs=7800`; `timelineWindow1LiveMinSpeed=0.940000`; `timelineWindow1LiveMaxSpeed=1.080000`; `timelineWindow1ManifestPresent=`; `timelineWindow1ManifestString=`; `timelineWindow1ManifestTokenPresent=`; `timelineWindow1FirstPeriodIndex=`; `timelineWindow1LastPeriodIndex=`; `timelineWindow1PresentationStartTimeMs=`; `timelineWindow1WindowStartTimeMs=`; `timelineWindow1ElapsedRealtimeEpochOffsetMs=`; `timelineWindow1DefaultPositionMs=`; `timelineWindow1DefaultPositionUs=`; `timelineWindow1DurationMs=`; `timelineWindow1DurationUs=`; `timelineWindow1Seekable=`; `timelineWindow1Live=`; `timelineWindow1Placeholder=`; `timelinePeriod1Id=`; `firstTrackGroupTokenPresent=1`; `secondTrackGroupId=`; `secondTrackLabel=`; `secondTrackLanguage=`; `secondTrackMimeType=`; `secondTrackAccessibilityChannel=`; `secondTrackRoleFlags=`; `secondTrackSelectionFlags=`; `secondTrackSelected=`; `secondTrackSupported=`; `secondTrackSupportedWithinCapabilities=`; `mediaMetadataAlbumTitle=Listener Item Album`; `mediaMetadataWriter=Listener Item Writer`; `mediaMetadataGenre=Listener Item Genre`; `mediaMetadataExtrasPresent=1`; `mediaMetadataExtrasKeyCount=1`; `mediaMetadataExtrasTokenPresent=1`; `mediaMetadataArtworkUri=https://example.com/listener-item-artwork.jpg`; `mediaMetadataArtworkDataLength=4`; `mediaMetadataArtworkDataType=6`; `playlistMetadataAlbumTitle=Listener Playlist Album`; `playlistMetadataConductor=Listener Playlist Conductor`; `playlistMetadataStation=Listener Playlist Station`; `playlistMetadataExtrasPresent=1`; `playlistMetadataExtrasKeyCount=1`; `playlistMetadataExtrasTokenPresent=1`; `playlistMetadataArtworkUri=https://example.com/listener-playlist-artwork.jpg`; `playlistMetadataArtworkDataLength=3`; `playlistMetadataArtworkDataType=8`; `cueCb=1`; `cue0Text=Listener Cue 1`; `cue0TextAlignment=2`; `cue0Line=0.250000`; `cue1Text=Listener Cue 2`; `cue1LineType=1`; `cue1TextSize=22.000000`; `cue1VerticalType=1`; `oldTagTokenPresent=1`; `newTagTokenPresent=1` | `API_MAPPING_STATUS.md` direct listener families; `DATA_STRUCTURE_MAPPING.md` Timeline / Tracks / MediaMetadata / Cue / PositionInfo |  |  |
| `nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` | `timelineWindow0MediaUri=https://example.com/payload-window-0.m3u8`; `timelineWindow0TagPresent=1`; `timelineWindow0TagString=payload-window-tag`; `timelineWindow0TagTokenPresent=1`; `timelineWindow0Uid=payload-window-uid`; `timelineWindow0LiveTargetOffsetMs=3333`; `timelineWindow1MediaId=payload-window-1`; `timelineWindow1TagTokenPresent=1`; `timelinePeriod0Id=payload-period-id`; `timelinePeriod0Uid=payload-period-0`; `timelinePeriod0AdsId=payload-period-ads-id`; `timelinePeriod1Id=payload-period-id-2`; `timelinePeriod1Uid=payload-period-1`; `timelinePeriod1AdsId=payload-period-ads-id-2`; `firstTrackGroupTokenPresent=1`; `firstTrackLabelTokenPresent=1`; `secondTrackGroupTokenPresent=1`; `secondTrackLabelTokenPresent=1`; `oldTagTokenPresent=1`; `newTagTokenPresent=1` | `API_MAPPING_STATUS.md` timeline changed / tracks changed / position discontinuity; `DATA_STRUCTURE_MAPPING.md` Timeline Period / TracksSnapshot / PositionInfoSnapshot |  |  |
| `nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary` | `mediaId=current-item`; `sourceType=2`; `subtitle1MimeType=text/vtt`; `subtitle1Label=Spanish`; `subtitle1SelectionFlags=3`; `subtitle1RoleFlags=13`; `requestMetadataExtrasKeyCount=1`; `requestMetadataExtrasTokenPresent=1`; `mediaMetadataAlbumTitle=Current Album`; `mediaMetadataDisplayTitle=Current Item Display`; `mediaMetadataTitleTokenPresent=1`; `mediaMetadataAlbumTitleTokenPresent=1`; `mediaMetadataAlbumArtistTokenPresent=1`; `mediaMetadataAuthorTokenPresent=1`; `mediaMetadataComposerTokenPresent=1`; `mediaMetadataConductorTokenPresent=1`; `mediaMetadataGenreTokenPresent=1`; `mediaMetadataCompilationTokenPresent=1`; `mediaMetadataStationTokenPresent=1`; `mediaMetadataExtrasPresent=1`; `mediaMetadataExtrasKeyCount=1`; `mediaMetadataExtrasTokenPresent=1`; `artworkUri=https://example.com/current-artwork.jpg`; `artworkDataLength=4`; `artworkDataType=3` | `API_MAPPING_STATUS.md` `getCurrentMediaItem`; `DATA_STRUCTURE_MAPPING.md` `MediaItemDescriptor` |  |  |
| `nativeMediaItemAtSmokeTest_returnsSnapshotAndHandlesOutOfBounds` | `mediaId=at-2`; `subtitle1MimeType=text/vtt`; `subtitle1Label=Spanish`; `subtitle1SelectionFlags=1`; `subtitle1RoleFlags=5`; `requestMetadataExtrasKeyCount=1`; `requestMetadataExtrasTokenPresent=1`; `mediaMetadataDisplayTitle=At Two Display`; `mediaMetadataDescription=At Two Description`; `mediaMetadataTitleTokenPresent=1`; `mediaMetadataAlbumTitleTokenPresent=1`; `mediaMetadataAlbumArtistTokenPresent=1`; `mediaMetadataAuthorTokenPresent=1`; `mediaMetadataComposerTokenPresent=1`; `mediaMetadataConductorTokenPresent=1`; `mediaMetadataGenreTokenPresent=1`; `mediaMetadataCompilationTokenPresent=1`; `mediaMetadataStationTokenPresent=1`; `mediaMetadataExtrasPresent=1`; `mediaMetadataExtrasKeyCount=1`; `mediaMetadataExtrasTokenPresent=1`; `artworkUri=https://example.com/at-two-artwork.jpg`; `artworkDataLength=4`; `artworkDataType=5` | `API_MAPPING_STATUS.md` `getMediaItemAt`; `DATA_STRUCTURE_MAPPING.md` `MediaItemDescriptor` |  |  |
| `nativeVideoAndMetadataSmokeTest_returnsQuerySummary` | `mediaTitle=Video Metadata Title`; `mediaArtworkUri=https://example.com/video-metadata-artwork.jpg`; `mediaArtworkDataLength=4`; `mediaArtworkDataType=3`; `mediaExtrasPresent=1`; `mediaExtrasKeyCount=1`; `mediaExtrasTokenPresent=1`; `playlistArtworkUri=https://example.com/metadata-playlist-artwork.jpg`; `playlistArtworkDataLength=3`; `playlistArtworkDataType=4`; `playlistExtrasPresent=1`; `playlistExtrasKeyCount=1`; `playlistExtrasTokenPresent=1` | `API_MAPPING_STATUS.md` media metadata; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` |  |  |
| `nativePlaylistMetadataSmokeTest_roundTripsPlaylistMetadata` | `title=Playlist Title`; `description=Playlist Description`; `artworkUri=https://example.com/playlist-artwork.jpg`; `artworkDataLength=3`; `artworkDataType=4`; `extrasPresent=1`; `extrasKeyCount=1`; `extrasTokenPresent=1` | `API_MAPPING_STATUS.md` playlist metadata set/get; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` |  |  |
| `nativePlaylistMetadataOpaqueTokenSmokeTest_resolvesRegisteredObjects` | `title=registered-title`; `albumTitle=registered-album-title`; `albumArtist=registered-album-artist`; `displayTitle=registered-display`; `subtitle=registered-subtitle`; `description=registered-description`; `writer=registered-writer`; `author=registered-author`; `composer=registered-composer`; `conductor=registered-conductor`; `genre=registered-genre`; `compilation=registered-compilation`; `station=registered-station`; `albumTitleTokenPresent=1`; `albumArtistTokenPresent=1`; `subtitleTokenPresent=1`; `descriptionTokenPresent=1`; `writerTokenPresent=1`; `authorTokenPresent=1`; `composerTokenPresent=1`; `conductorTokenPresent=1`; `genreTokenPresent=1`; `compilationTokenPresent=1`; `stationTokenPresent=1`; `extrasPresent=1`; `extrasKeyCount=1`; `extrasTokenPresent=1` | `API_MAPPING_STATUS.md` playlist metadata set/get; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` |  |  |
| `nativeSourceTypeSmokeTest_returnsInferredMimeSummary` | `sourceType=2` | `API_MAPPING_STATUS.md` `getCurrentMediaItem`; `DATA_STRUCTURE_MAPPING.md` `MediaItemDescriptor` |  |  |
| `nativeAnalyticsAudioUnderrunSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `bufferSize=4096`; `bufferSizeMs=87`; `elapsedSinceLastFeedMs=23` | `API_MAPPING_STATUS.md` analytics audio underrun; `DATA_STRUCTURE_MAPPING.md` `AudioUnderrunEvent` |  |  |
| `nativeAnalyticsDroppedVideoFramesSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `droppedFrames=8`; `elapsedMs=41` | `API_MAPPING_STATUS.md` analytics dropped video frames; `DATA_STRUCTURE_MAPPING.md` `DroppedVideoFramesEvent` |  |  |
| `nativeAnalyticsBandwidthEstimateSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `elapsedMs=34`; `bytesTransferred=67890`; `bitrateEstimate=999999` | `API_MAPPING_STATUS.md` analytics bandwidth estimate; `DATA_STRUCTURE_MAPPING.md` `BandwidthEstimateEvent` |  |  |
| `nativeAnalyticsLoadStartedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `uri=https://example.com/analytics-final.m3u8`; `dataType=3`; `trackType=1`; `retryCount=2` | `API_MAPPING_STATUS.md` analytics load started; `DATA_STRUCTURE_MAPPING.md` `LoadStartedEvent` |  |  |
| `nativeAnalyticsLoadCompletedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `uri=https://example.com/analytics-final-complete.m3u8`; `dataType=4`; `trackType=1` | `API_MAPPING_STATUS.md` analytics load completed; `DATA_STRUCTURE_MAPPING.md` `LoadCompletedEvent` |  |  |
| `nativeAnalyticsAudioInputFormatChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `sampleMimeType=audio/final`; `codecs=ec-3`; `channelCount=6`; `sampleRate=48000` | `API_MAPPING_STATUS.md` analytics audio input format changed; `DATA_STRUCTURE_MAPPING.md` `AudioInputFormatChangedEvent` |  |  |
| `nativeAnalyticsAudioDecoderInitializedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `decoderName=c2.android.eac3.decoder`; `initializedTimestampMs=222`; `initializationDurationMs=19` | `API_MAPPING_STATUS.md` analytics audio decoder initialized; `DATA_STRUCTURE_MAPPING.md` `AudioDecoderInitializedEvent` |  |  |
| `nativeAnalyticsVideoDecoderInitializedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `decoderName=c2.android.hevc.decoder`; `initializedTimestampMs=444`; `initializationDurationMs=29` | `API_MAPPING_STATUS.md` analytics video decoder initialized; `DATA_STRUCTURE_MAPPING.md` `VideoDecoderInitializedEvent` |  |  |
| `nativeAnalyticsAudioDecoderReleasedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `decoderName=c2.android.eac3.decoder` | `API_MAPPING_STATUS.md` analytics audio decoder released; `DATA_STRUCTURE_MAPPING.md` `AudioDecoderReleasedEvent` |  |  |
| `nativeAnalyticsVideoDecoderReleasedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `decoderName=c2.android.hevc.decoder` | `API_MAPPING_STATUS.md` analytics video decoder released; `DATA_STRUCTURE_MAPPING.md` `VideoDecoderReleasedEvent` |  |  |
| `nativeAnalyticsRenderedFirstFrameSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `renderTimeMs=456` | `API_MAPPING_STATUS.md` analytics rendered first frame; `DATA_STRUCTURE_MAPPING.md` `AnalyticsRenderedFirstFrameEvent` |  |  |
| `nativeAnalyticsVideoSizeChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `width=1920`; `height=1080`; `pixelWidthHeightRatio=1.250000` | `API_MAPPING_STATUS.md` analytics video size changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsVideoSizeChangedEvent` |  |  |
| `nativeAnalyticsAudioPositionAdvancingSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `playoutStartSystemTimeMs=2222` | `API_MAPPING_STATUS.md` analytics audio position advancing; `DATA_STRUCTURE_MAPPING.md` `AudioPositionAdvancingEvent` |  |  |
| `nativeAnalyticsVideoFrameProcessingOffsetSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `totalProcessingOffsetUs=67890`; `frameCount=8` | `API_MAPPING_STATUS.md` analytics video frame processing offset; `DATA_STRUCTURE_MAPPING.md` `VideoFrameProcessingOffsetEvent` |  |  |
| `nativeAnalyticsVolumeChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `volume=0.750000` | `API_MAPPING_STATUS.md` analytics volume changed; `DATA_STRUCTURE_MAPPING.md` `VolumeChangedEvent` |  |  |
| `nativeAnalyticsAudioSessionIdChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `audioSessionId=700042` | `API_MAPPING_STATUS.md` analytics audio session id changed; `DATA_STRUCTURE_MAPPING.md` `AudioSessionIdChangedEvent` |  |  |
| `nativeAnalyticsSkipSilenceEnabledChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `skipSilenceEnabled=1` | `API_MAPPING_STATUS.md` analytics skip silence enabled changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsSkipSilenceEnabledChangedEvent` |  |  |
| `nativeAnalyticsDeviceVolumeChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `volume=7`; `muted=0` | `API_MAPPING_STATUS.md` analytics device volume changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsDeviceVolumeChangedEvent` |  |  |
| `nativeAnalyticsPlaybackStateChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `playbackState=3` | `API_MAPPING_STATUS.md` analytics playback state changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsPlaybackStateChangedEvent` |  |  |
| `nativeAnalyticsIsPlayingChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `isPlaying=1` | `API_MAPPING_STATUS.md` analytics is playing changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsIsPlayingChangedEvent` |  |  |
| `nativeAnalyticsPlayWhenReadyChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `playWhenReady=1`; `reason=2` | `API_MAPPING_STATUS.md` analytics play when ready changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsPlayWhenReadyChangedEvent` |  |  |
| `nativeAnalyticsPlaybackSuppressionReasonChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `playbackSuppressionReason=1` | `API_MAPPING_STATUS.md` analytics playback suppression reason changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsPlaybackSuppressionReasonChangedEvent` |  |  |
| `nativeAnalyticsIsLoadingChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `isLoading=1` | `API_MAPPING_STATUS.md` analytics is loading changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsIsLoadingChangedEvent` |  |  |
| `nativeAnalyticsRepeatModeChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `repeatMode=2` | `API_MAPPING_STATUS.md` analytics repeat mode changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsRepeatModeChangedEvent` |  |  |
| `nativeAnalyticsShuffleModeChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `shuffleModeEnabled=1` | `API_MAPPING_STATUS.md` analytics shuffle mode changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsShuffleModeChangedEvent` |  |  |
| `nativeAnalyticsVideoInputFormatChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `sampleMimeType=video/final`; `codecs=hvc1.1.6.L93.B0`; `width=1920`; `height=1080`; `frameRate=59.939999` | `API_MAPPING_STATUS.md` analytics video input format changed; `DATA_STRUCTURE_MAPPING.md` `VideoInputFormatChangedEvent` |  |  |
| `nativeAnalyticsPlaybackParametersChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `speed=1.500000`; `pitch=0.750000` | `API_MAPPING_STATUS.md` analytics playback parameters changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsPlaybackParametersChangedEvent` |  |  |
| `nativeAnalyticsAvailableCommandsChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `commandCount=3`; `firstCommand=3`; `contains8=1` | `API_MAPPING_STATUS.md` analytics available commands changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsAvailableCommandsChangedEvent` |  |  |
| `nativeAnalyticsEventsSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `eventCount=3`; `firstEvent=7`; `contains9=1` | `API_MAPPING_STATUS.md` analytics events batch; `DATA_STRUCTURE_MAPPING.md` `AnalyticsEventsEvent` |  |  |
| `nativeAnalyticsSeekBackIncrementChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `seekBackIncrementMs=15000` | `API_MAPPING_STATUS.md` analytics seek back increment changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsSeekBackIncrementChangedEvent` |  |  |
| `nativeAnalyticsSeekForwardIncrementChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `seekForwardIncrementMs=25000` | `API_MAPPING_STATUS.md` analytics seek forward increment changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsSeekForwardIncrementChangedEvent` |  |  |
| `nativeAnalyticsMaxSeekToPreviousPositionChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `maxSeekToPreviousPositionMs=12000` | `API_MAPPING_STATUS.md` analytics max seek to previous position changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsMaxSeekToPreviousPositionChangedEvent` |  |  |
| `nativeAnalyticsTimelineChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `reason=2` | `API_MAPPING_STATUS.md` analytics timeline changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsTimelineChangedEvent` |  |  |
| `nativeAnalyticsPositionDiscontinuitySmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `reason=5` | `API_MAPPING_STATUS.md` analytics position discontinuity; `DATA_STRUCTURE_MAPPING.md` `AnalyticsPositionDiscontinuityEvent` |  |  |
| `nativeAnalyticsSeekStartedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `started=1` | `API_MAPPING_STATUS.md` analytics seek started; `DATA_STRUCTURE_MAPPING.md` `AnalyticsSeekStartedEvent` |  |  |
| `nativeAnalyticsPlayerErrorSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `errorCode=2002`; `message=analytics-final-error` | `API_MAPPING_STATUS.md` analytics player error; `DATA_STRUCTURE_MAPPING.md` `PlayerError` |  |  |
| `nativeAnalyticsPlayerErrorChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `errorCode=4004`; `message=analytics-final-changed` | `API_MAPPING_STATUS.md` analytics player error changed; `DATA_STRUCTURE_MAPPING.md` `PlayerError` |  |  |
| `nativeAnalyticsTracksChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `groupCount=2`; `firstGroupId=video-main`; `containsVideo=1` | `API_MAPPING_STATUS.md` analytics tracks changed; `DATA_STRUCTURE_MAPPING.md` `TracksSnapshot` |  |  |
| `nativeAnalyticsMediaItemTransitionSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `mediaId=analytics-transition-final`; `sourceType=2`; `reason=2` | `API_MAPPING_STATUS.md` analytics media item transition; `DATA_STRUCTURE_MAPPING.md` `AnalyticsMediaItemTransitionEvent` |  |  |
| `nativeAnalyticsCuesSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `cueCount=2`; `presentationTimeUs=654321`; `text0=Analytics Cue Final`; `text0TokenPresent=1`; `bitmap0TokenPresent=1`; `text1=Analytics Cue Final 2`; `text1TokenPresent=1`; `bitmap1TokenPresent=0` | `API_MAPPING_STATUS.md` analytics cues; `DATA_STRUCTURE_MAPPING.md` `CueSnapshot` |  |  |
| `nativeAnalyticsMetadataSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `entryCount=2`; `firstEntryType=MdtaMetadataEntry`; `firstEntryText=analytics-metadata-final` | `API_MAPPING_STATUS.md` analytics metadata; `DATA_STRUCTURE_MAPPING.md` `AnalyticsMetadataEvent` |  |  |
| `nativeAnalyticsLoadErrorSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `uri=https://example.com/analytics-error-final.m3u8`; `dataType=4`; `trackType=2`; `message=analytics-load-final`; `wasCanceled=0` | `API_MAPPING_STATUS.md` analytics load error; `DATA_STRUCTURE_MAPPING.md` `AnalyticsLoadErrorEvent` |  |  |
| `nativeAnalyticsDeviceInfoChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `playbackType=1`; `minVolume=2`; `maxVolume=15`; `routingControllerId=route-final` | `API_MAPPING_STATUS.md` analytics device info changed; `DATA_STRUCTURE_MAPPING.md` `DeviceInfoDescriptor` |  |  |
| `nativeAnalyticsMediaMetadataChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `title=Analytics Media Final`; `artist=Analytics Artist Final`; `displayTitle=Analytics Display Final` | `API_MAPPING_STATUS.md` analytics media metadata changed; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` |  |  |
| `nativeAnalyticsPlaylistMetadataChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `title=Analytics Playlist Final`; `artist=Analytics Playlist Artist Final`; `displayTitle=Analytics Playlist Display Final` | `API_MAPPING_STATUS.md` analytics playlist metadata changed; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` |  |  |
| `nativeMediaSourceFactoryInjectionSmokeTest_usesRegisteredFactoryToken` | `factoryToken=test-injected-media-source-factory`; `injectedFactoryUsed=1`; `factoryIdentity=` | `API_MAPPING_STATUS.md` media source factory baseline config; `DATA_STRUCTURE_MAPPING.md` `PlayerConfig::MediaSourceFactoryConfig` |  |  |
| `nativeMediaSourceFactoryInjectionFallbackSmokeTest_fallsBackWhenTokenIsMissing` | `factoryToken=missing-media-source-factory-token`; `fallbackApplied=1`; `factoryIdentity=0` | `API_MAPPING_STATUS.md` media source factory baseline config; `DATA_STRUCTURE_MAPPING.md` `PlayerConfig::MediaSourceFactoryConfig` |  |  |
| `nativeMediaSourceFactoryInjectionMultiTokenSmokeTest_keepsTokensIsolated` | `firstFactoryToken=multi-token-media-source-factory-a`; `secondFactoryToken=multi-token-media-source-factory-b`; `tokensIsolated=1` | `API_MAPPING_STATUS.md` media source factory baseline config; `DATA_STRUCTURE_MAPPING.md` `PlayerConfig::MediaSourceFactoryConfig` |  |  |
| `nativeImageOutputSmokeTest_returnsListenerSummary` | `lastAllocationByteCount=384`; `lastRowBytes=48`; `lastIsPremultiplied=1`; `reattachLastBitmapConfig=RGB_565`; `reattachLastHasAlpha=0` | `API_MAPPING_STATUS.md` image output; `DATA_STRUCTURE_MAPPING.md` `ImageFrameSnapshot` |  |  |
| `nativeVideoEffectsConversionSmokeTest_returnsStructuredSummary` | `afterReapply=effectCount=7`; `effect3=rgbAdjustment:redScale=1.0:greenScale=1.0:blueScale=1.0`; `effect4=scaleAndRotate:scaleX=1.0:scaleY=1.0:rotationDegrees=0.0`; `effect6=presentation:width=320:height=240:layout=1` | `API_MAPPING_STATUS.md` video effects; `DATA_STRUCTURE_MAPPING.md` `VideoEffectDescriptor` |  |  |

## Demo Manual Validation

- App launch
  - Result: `PASS / FAIL`
  - Notes:
- Play / Pause
  - Result: `PASS / FAIL`
  - Notes:
- Seek 30s
  - Result: `PASS / FAIL`
  - Notes:
- Playlist
  - Result: `PASS / FAIL`
  - Notes:
- Volume
  - Result: `PASS / FAIL`
  - Notes:
- Speed
  - Result: `PASS / FAIL`
  - Notes:
- Load Subtitle
  - Result: `PASS / FAIL`
  - Notes:
- Prefer Text
  - Result: `PASS / FAIL`
  - Notes:
- Tracks summary
  - Result: `PASS / FAIL`
  - Notes:
- App close / teardown
  - Result: `PASS / FAIL`
  - Notes:

## Logcat Review

- JNI exception spam seen:
  - `YES / NO`
- Teardown/release errors seen:
  - `YES / NO`
- Notes:

## Regression Summary

- New failures found:
- Suspected root cause:
- Files to inspect:
- Blocker level: `None / Low / Medium / High`

## Sign-Off

- Overall result: `READY / NOT READY`
- Recommended next action:
