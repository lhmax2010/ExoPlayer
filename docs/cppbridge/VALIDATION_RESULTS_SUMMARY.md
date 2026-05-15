# C++ Bridge Validation Results Summary

Last updated: 2026-03-18

Use this page after the new environment finishes build, device validation, and demo verification.
Write raw run details into `TEST_RESULTS_TEMPLATE.md`, then summarize the final outcome here.
If a result suggests the current reduced endpoint tracker is no longer accurate, cross-check the
status notes in:
`docs/cppbridge/API_MAPPING_STATUS.md`

## 1. Overall Result

| Area | Status | Evidence | Owner | Notes |
| --- | --- | --- | --- | --- |
| Build / native link | Pending |  |  |  |
| JNI/value smoke | Pending |  |  |  |
| Player/runtime smoke | Pending |  |  |  |
| Demo manual validation | Pending |  |  |  |
| Logcat review | Pending |  |  |  |
| Release recommendation | Pending |  |  |  |

Status values:

- `Pass`
- `Pass with notes`
- `Fail`
- `Blocked`
- `Pending`

## 2. Environment Summary

| Item | Value |
| --- | --- |
| Validation date |  |
| Machine / host |  |
| Branch / package snapshot |  |
| JDK |  |
| Gradle |  |
| Android SDK |  |
| NDK / CMake |  |
| Device / emulator |  |
| `ANDROID_SERIAL` |  |

## 3. Smoke Suite Rollup

| Suite | Result | Key evidence | Follow-up needed |
| --- | --- | --- | --- |
| `CppBridgeNativeSmokeTest` | Pending |  |  |
| `CppBridgeNativePlayerInstrumentationTest` | Pending |  |  |
| `run_validation.sh` aggregate result | Pending |  |  |

Recommended spot checks for the explicit opaque-token cleanup smoke:

- `released=1`
- `state=1`
- `tokenCount=4`

## 3A. Second-Batch Triage

| Bucket | Item | Validation-first or implementation-first | Result | Notes |
| --- | --- | --- | --- | --- |
| Second-batch closed | analytics aggregate / analytics-only listener | Validation-first | Pending |  |
| Second-batch closed | image output reduced callback behavior | Validation-first | Pending |  |
| Second-batch closed | renderer messaging reduced model | Validation-first | Pending |  |
| Second-batch closed | wake mode create-time + runtime setter | Validation-first | Pending |  |
| Second-batch closed | priority reduced wrapper/runtime state | Validation-first | Pending |  |
| Second-batch closed | preload target-duration behavior | Validation-first | Pending |  |
| Second-batch closed | builder-style reduced config/build path | Validation-first | Pending |  |
| Needs new capability | full Java `AnalyticsListener` parity | Implementation-first | Pending |  |
| Needs new capability | broader preload ecosystem parity | Implementation-first | Pending |  |
| Needs new capability | richer image output parity | Implementation-first | Pending |  |
| Needs new capability | richer video effects parity | Implementation-first | Pending |  |
| Needs new capability | broader arbitrary `MediaSource.Factory` injection | Implementation-first | Pending | token-registered baseline support exists; full arbitrary injection still pending |
| Needs new capability | full Java reduced-model family parity replacement | Implementation-first | Pending |  |

## 4. High-Signal Output Checks

| Marker | Expected | Actual | Result | Notes |
| --- | --- | --- | --- | --- |
| Listener timeline window URI | `timelineWindow0MediaUri=https://example.com/listener.mp4` |  |  |  |
| Timeline period UID | `timelinePeriod0Uid=` |  |  |  |
| Metadata title | `mediaTitle=Video Metadata Title` |  |  |  |
| Metadata artwork URI | `mediaArtworkUri=https://example.com/video-metadata-artwork.jpg` |  |  |  |
| Playlist artwork URI | `playlistArtworkUri=https://example.com/metadata-playlist-artwork.jpg` |  |  |  |
| Source type inference | `sourceType=2` |  |  |  |
| Analytics audio underrun | `bufferSize=4096` |  |  |  |
| Analytics dropped video frames | `droppedFrames=8` |  |  |  |
| Analytics bandwidth estimate | `bitrateEstimate=999999` |  |  |  |
| Analytics load started | `uri=https://example.com/analytics-final.m3u8` |  |  |  |
| Analytics load completed | `uri=https://example.com/analytics-final-complete.m3u8` |  |  |  |
| Analytics audio input format changed | `sampleMimeType=audio/final` |  |  |  |
| Analytics audio decoder initialized | `decoderName=c2.android.eac3.decoder` |  |  |  |
| Analytics video decoder initialized | `decoderName=c2.android.hevc.decoder` |  |  |  |
| Analytics audio decoder released | `decoderName=c2.android.eac3.decoder` |  |  |  |
| Analytics video decoder released | `decoderName=c2.android.hevc.decoder` |  |  |  |
| Analytics rendered first frame | `renderTimeMs=456` |  |  |  |
| Analytics video size changed | `pixelWidthHeightRatio=1.250000` |  |  |  |
| Analytics audio position advancing | `playoutStartSystemTimeMs=2222` |  |  |  |
| Analytics video frame processing offset | `totalProcessingOffsetUs=67890` |  |  |  |
| Analytics volume changed | `volume=0.750000` |  |  |  |
| Analytics audio session id changed | `audioSessionId=42` |  |  |  |
| Analytics skip silence enabled changed | `skipSilenceEnabled=1` |  |  |  |
| Analytics device volume changed | `volume=7`; `muted=0` |  |  |  |
| Analytics playback state changed | `playbackState=3` |  |  |  |
| Analytics is playing changed | `isPlaying=1` |  |  |  |
| Analytics play when ready changed | `playWhenReady=1`; `reason=2` |  |  |  |
| Analytics playback suppression reason changed | `playbackSuppressionReason=1` |  |  |  |
| Analytics is loading changed | `isLoading=1` |  |  |  |
| Analytics repeat mode changed | `repeatMode=2` |  |  |  |
| Analytics shuffle mode changed | `shuffleModeEnabled=1` |  |  |  |
| Analytics video input format changed | `sampleMimeType=video/final` |  |  |  |
| Analytics playback parameters changed | `speed=1.500000`; `pitch=0.750000` |  |  |  |
| Analytics available commands changed | `commandCount=3`; `contains8=1` |  |  |  |
| Analytics events batch | `eventCount=3`; `contains9=1` |  |  |  |
| Analytics device info changed | `playbackType=1`; `routingControllerId=route-final` |  |  |  |
| Analytics media metadata changed | `title=Analytics Media Final`; `displayTitle=Analytics Display Final` |  |  |  |
| Analytics playlist metadata changed | `title=Analytics Playlist Final`; `displayTitle=Analytics Playlist Display Final` |  |  |  |
| Analytics media item transition | `mediaId=analytics-transition-final`; `sourceType=2`; `reason=2` |  |  |  |
| Analytics cues | `cueCount=2`; `presentationTimeUs=654321`; `text0=Analytics Cue Final` |  |  |  |
| MediaSource.Factory registered token | `factoryToken=test-injected-media-source-factory` |  |  |  |
| MediaSource.Factory fallback | `fallbackApplied=1` |  |  |  |
| MediaSource.Factory multi-token isolation | `tokensIsolated=1` |  |  |  |
| Image output allocation metadata | `lastAllocationByteCount=384` |  |  |  |
| Image output non-ARGB config | `reattachLastBitmapConfig=RGB_565` |  |  |  |
| Video effects boundary reapply count | `afterReapply=effectCount=7` |  |  |  |
| Video effects secondary presentation layout | `effect6=presentation:width=320:height=240:layout=1` |  |  |  |

## 5. Smoke To Mapping Crosswalk

| Smoke test | Expected marker(s) | Mapping doc anchor | Area |
| --- | --- | --- | --- |
| `nativeCurrentTimelineSmokeTest_returnsTimelineDetails` | `window0MediaId=timeline-query-item-1`; `window0MediaUri=https://example.com/current-timeline-one.m3u8`; `window0TagPresent=1`; `window0TagString=timeline-query-tag-1`; `window0TagTokenPresent=1`; `window0Uid=`; `window1MediaId=timeline-query-item-2`; `window1MediaUri=https://example.com/current-timeline-two.mp4`; `window1TagString=timeline-query-tag-2`; `window1TagTokenPresent=1`; `window1LiveConfigurationPresent=1`; `window1LiveTargetOffsetMs=7100`; `window1LiveMinOffsetMs=6400`; `window1LiveMaxOffsetMs=8200`; `window1LiveMinSpeed=0.930000`; `window1LiveMaxSpeed=1.070000`; `window1ManifestPresent=`; `window1ManifestString=`; `window1ManifestTokenPresent=`; `window1DurationMs=`; `window1Seekable=`; `window1Live=`; `period0Id=`; `period0Uid=`; `period1Id=`; `period1Uid=` | `DATA_STRUCTURE_MAPPING.md` Timeline / `TimelineDetailsSnapshot`, `TimelineWindowSnapshot`, `TimelinePeriodSnapshot` | Timeline query parity |
| `nativeCurrentTracksSmokeTest_returnsTracksSummary` | `group0Id=video-group`; `group0TokenPresent=1`; `track0LabelTokenPresent=1`; `group1Id=audio-group`; `group1TokenPresent=1`; `group1Track0Id=`; `group1Track0Label=`; `group1Track0LabelTokenPresent=` | `DATA_STRUCTURE_MAPPING.md` Tracks / `TracksSnapshot`, `TrackGroupSnapshot`, `TrackInfo` | Current tracks parity |
| `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` | `cueCount=2`; `cuePresentationTimeUs=456789`; `cue0Text=Query Cue 1`; `cue0TextTokenPresent=1`; `cue0BitmapTokenPresent=1`; `cue1Text=Query Cue 2`; `cue1TextTokenPresent=1`; `cue1BitmapTokenPresent=0` | `API_MAPPING_STATUS.md` audio/query getters; `DATA_STRUCTURE_MAPPING.md` `CueSnapshot` | Current cues query parity |
| `nativeListenerSmokeTest_reportsExtendedCallbacks` | `timelineWindow0MediaId=listener-item-1`; `timelineWindow0TagPresent=1`; `timelineWindow0TagString=listener-tag-1`; `timelineWindow0TagTokenPresent=1`; `timelineWindow1MediaItemIndex=1`; `timelineWindow1MediaId=listener-item-2`; `timelineWindow1TagString=listener-tag-2`; `timelineWindow1TagTokenPresent=1`; `timelineWindow1LiveConfigurationPresent=1`; `timelineWindow1LiveTargetOffsetMs=6100`; `timelineWindow1LiveMinOffsetMs=5200`; `timelineWindow1LiveMaxOffsetMs=7800`; `timelineWindow1LiveMinSpeed=0.940000`; `timelineWindow1LiveMaxSpeed=1.080000`; `timelineWindow1ManifestPresent=`; `timelineWindow1ManifestString=`; `timelineWindow1ManifestTokenPresent=`; `timelineWindow1FirstPeriodIndex=`; `timelineWindow1LastPeriodIndex=`; `timelineWindow1PresentationStartTimeMs=`; `timelineWindow1WindowStartTimeMs=`; `timelineWindow1ElapsedRealtimeEpochOffsetMs=`; `timelineWindow1DefaultPositionMs=`; `timelineWindow1DefaultPositionUs=`; `timelineWindow1DurationMs=`; `timelineWindow1DurationUs=`; `timelineWindow1Seekable=`; `timelineWindow1Live=`; `timelineWindow1Placeholder=`; `timelinePeriod1Id=`; `firstTrackGroupTokenPresent=1`; `secondTrackGroupId=`; `secondTrackLabel=`; `secondTrackLanguage=`; `secondTrackMimeType=`; `secondTrackAccessibilityChannel=`; `secondTrackRoleFlags=`; `secondTrackSelectionFlags=`; `secondTrackSelected=`; `secondTrackSupported=`; `secondTrackSupportedWithinCapabilities=`; `mediaMetadataAlbumTitle=Listener Item Album`; `mediaMetadataWriter=Listener Item Writer`; `mediaMetadataGenre=Listener Item Genre`; `mediaMetadataExtrasPresent=1`; `mediaMetadataExtrasKeyCount=1`; `mediaMetadataExtrasTokenPresent=1`; `mediaMetadataArtworkUri=https://example.com/listener-item-artwork.jpg`; `mediaMetadataArtworkDataLength=4`; `mediaMetadataArtworkDataType=6`; `playlistMetadataAlbumTitle=Listener Playlist Album`; `playlistMetadataConductor=Listener Playlist Conductor`; `playlistMetadataStation=Listener Playlist Station`; `playlistMetadataExtrasPresent=1`; `playlistMetadataExtrasKeyCount=1`; `playlistMetadataExtrasTokenPresent=1`; `playlistMetadataArtworkUri=https://example.com/listener-playlist-artwork.jpg`; `playlistMetadataArtworkDataLength=3`; `playlistMetadataArtworkDataType=8`; `cueCb=1`; `cue0Text=Listener Cue 1`; `cue0TextAlignment=2`; `cue0Line=0.250000`; `cue1Text=Listener Cue 2`; `cue1LineType=1`; `cue1TextSize=22.000000`; `cue1VerticalType=1`; `oldTagTokenPresent=1`; `newTagTokenPresent=1` | `API_MAPPING_STATUS.md` direct listener families; `DATA_STRUCTURE_MAPPING.md` Timeline / Tracks / MediaMetadata / Cue / PositionInfo | Direct listener parity |
| `nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` | `timelineWindow0MediaUri=https://example.com/payload-window-0.m3u8`; `timelineWindow0TagPresent=1`; `timelineWindow0TagString=payload-window-tag`; `timelineWindow0TagTokenPresent=1`; `timelineWindow0Uid=payload-window-uid`; `timelineWindow0LiveTargetOffsetMs=3333`; `timelineWindow1MediaId=payload-window-1`; `timelineWindow1TagTokenPresent=1`; `timelinePeriod0Id=payload-period-id`; `timelinePeriod0Uid=payload-period-0`; `timelinePeriod0AdsId=payload-period-ads-id`; `timelinePeriod1Id=payload-period-id-2`; `timelinePeriod1Uid=payload-period-1`; `timelinePeriod1AdsId=payload-period-ads-id-2`; `firstTrackGroupTokenPresent=1`; `firstTrackLabelTokenPresent=1`; `secondTrackGroupTokenPresent=1`; `secondTrackLabelTokenPresent=1`; `oldTagTokenPresent=1`; `newTagTokenPresent=1` | `API_MAPPING_STATUS.md` timeline changed / tracks changed / position discontinuity; `DATA_STRUCTURE_MAPPING.md` Timeline Period / TracksSnapshot / PositionInfoSnapshot | Listener payload parity |
| `nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary` | `mediaId=current-item`; `sourceType=2`; `subtitle1MimeType=text/vtt`; `subtitle1Label=Spanish`; `subtitle1SelectionFlags=3`; `subtitle1RoleFlags=13`; `requestMetadataExtrasKeyCount=1`; `requestMetadataExtrasTokenPresent=1`; `mediaMetadataAlbumTitle=Current Album`; `mediaMetadataDisplayTitle=Current Item Display`; `mediaMetadataTitleTokenPresent=1`; `mediaMetadataAlbumTitleTokenPresent=1`; `mediaMetadataAlbumArtistTokenPresent=1`; `mediaMetadataAuthorTokenPresent=1`; `mediaMetadataComposerTokenPresent=1`; `mediaMetadataConductorTokenPresent=1`; `mediaMetadataGenreTokenPresent=1`; `mediaMetadataCompilationTokenPresent=1`; `mediaMetadataStationTokenPresent=1`; `mediaMetadataExtrasPresent=1`; `mediaMetadataExtrasKeyCount=1`; `mediaMetadataExtrasTokenPresent=1`; `artworkUri=https://example.com/current-artwork.jpg`; `artworkDataLength=4`; `artworkDataType=3` | `API_MAPPING_STATUS.md` `getCurrentMediaItem`; `DATA_STRUCTURE_MAPPING.md` `MediaItemDescriptor` | Current item parity |
| `nativeMediaItemAtSmokeTest_returnsSnapshotAndHandlesOutOfBounds` | `mediaId=at-2`; `subtitle1MimeType=text/vtt`; `subtitle1Label=Spanish`; `subtitle1SelectionFlags=1`; `subtitle1RoleFlags=5`; `requestMetadataExtrasKeyCount=1`; `requestMetadataExtrasTokenPresent=1`; `mediaMetadataDisplayTitle=At Two Display`; `mediaMetadataDescription=At Two Description`; `mediaMetadataTitleTokenPresent=1`; `mediaMetadataAlbumTitleTokenPresent=1`; `mediaMetadataAlbumArtistTokenPresent=1`; `mediaMetadataAuthorTokenPresent=1`; `mediaMetadataComposerTokenPresent=1`; `mediaMetadataConductorTokenPresent=1`; `mediaMetadataGenreTokenPresent=1`; `mediaMetadataCompilationTokenPresent=1`; `mediaMetadataStationTokenPresent=1`; `mediaMetadataExtrasPresent=1`; `mediaMetadataExtrasKeyCount=1`; `mediaMetadataExtrasTokenPresent=1`; `artworkUri=https://example.com/at-two-artwork.jpg`; `artworkDataLength=4`; `artworkDataType=5` | `API_MAPPING_STATUS.md` `getMediaItemAt`; `DATA_STRUCTURE_MAPPING.md` `MediaItemDescriptor` | Indexed item parity |
| `nativeVideoAndMetadataSmokeTest_returnsQuerySummary` | `mediaTitle=Video Metadata Title`; `mediaArtworkUri=https://example.com/video-metadata-artwork.jpg`; `mediaArtworkDataLength=4`; `mediaArtworkDataType=3`; `mediaExtrasPresent=1`; `mediaExtrasKeyCount=1`; `mediaExtrasTokenPresent=1`; `playlistArtworkUri=https://example.com/metadata-playlist-artwork.jpg`; `playlistArtworkDataLength=3`; `playlistArtworkDataType=4`; `playlistExtrasPresent=1`; `playlistExtrasKeyCount=1`; `playlistExtrasTokenPresent=1` | `API_MAPPING_STATUS.md` media metadata; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` | Metadata query parity |
| `nativePlaylistMetadataSmokeTest_roundTripsPlaylistMetadata` | `title=Playlist Title`; `description=Playlist Description`; `artworkUri=https://example.com/playlist-artwork.jpg`; `artworkDataLength=3`; `artworkDataType=4`; `extrasPresent=1`; `extrasKeyCount=1`; `extrasTokenPresent=1` | `API_MAPPING_STATUS.md` playlist metadata set/get; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` | Playlist metadata round-trip |
| `nativePlaylistMetadataOpaqueTokenSmokeTest_resolvesRegisteredObjects` | `title=registered-title`; `albumTitle=registered-album-title`; `albumArtist=registered-album-artist`; `displayTitle=registered-display`; `subtitle=registered-subtitle`; `description=registered-description`; `writer=registered-writer`; `author=registered-author`; `composer=registered-composer`; `conductor=registered-conductor`; `genre=registered-genre`; `compilation=registered-compilation`; `station=registered-station`; `albumTitleTokenPresent=1`; `albumArtistTokenPresent=1`; `subtitleTokenPresent=1`; `descriptionTokenPresent=1`; `writerTokenPresent=1`; `authorTokenPresent=1`; `composerTokenPresent=1`; `conductorTokenPresent=1`; `genreTokenPresent=1`; `compilationTokenPresent=1`; `stationTokenPresent=1`; `extrasPresent=1`; `extrasKeyCount=1`; `extrasTokenPresent=1` | `API_MAPPING_STATUS.md` playlist metadata set/get; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` | Playlist metadata opaque-token parity |
| `nativeSourceTypeSmokeTest_returnsInferredMimeSummary` | `sourceType=2` | `API_MAPPING_STATUS.md` `getCurrentMediaItem`; `DATA_STRUCTURE_MAPPING.md` `MediaItemDescriptor` | URI-based source type inference |
| `nativeAnalyticsAudioUnderrunSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `bufferSize=4096`; `bufferSizeMs=87`; `elapsedSinceLastFeedMs=23` | `API_MAPPING_STATUS.md` analytics audio underrun; `DATA_STRUCTURE_MAPPING.md` `AudioUnderrunEvent` | First concrete reduced analytics event |
| `nativeAnalyticsDroppedVideoFramesSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `droppedFrames=8`; `elapsedMs=41` | `API_MAPPING_STATUS.md` analytics dropped video frames; `DATA_STRUCTURE_MAPPING.md` `DroppedVideoFramesEvent` | Second concrete reduced analytics event |
| `nativeAnalyticsBandwidthEstimateSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `elapsedMs=34`; `bytesTransferred=67890`; `bitrateEstimate=999999` | `API_MAPPING_STATUS.md` analytics bandwidth estimate; `DATA_STRUCTURE_MAPPING.md` `BandwidthEstimateEvent` | Third concrete reduced analytics event |
| `nativeAnalyticsLoadStartedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `uri=https://example.com/analytics-final.m3u8`; `dataType=3`; `trackType=1`; `retryCount=2` | `API_MAPPING_STATUS.md` analytics load started; `DATA_STRUCTURE_MAPPING.md` `LoadStartedEvent` | Fourth concrete reduced analytics event |
| `nativeAnalyticsLoadCompletedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `uri=https://example.com/analytics-final-complete.m3u8`; `dataType=4`; `trackType=1` | `API_MAPPING_STATUS.md` analytics load completed; `DATA_STRUCTURE_MAPPING.md` `LoadCompletedEvent` | Fifth concrete reduced analytics event |
| `nativeAnalyticsAudioInputFormatChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `sampleMimeType=audio/final`; `codecs=ec-3`; `channelCount=6`; `sampleRate=48000` | `API_MAPPING_STATUS.md` analytics audio input format changed; `DATA_STRUCTURE_MAPPING.md` `AudioInputFormatChangedEvent` | Sixth concrete reduced analytics event |
| `nativeAnalyticsAudioDecoderInitializedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `decoderName=c2.android.eac3.decoder`; `initializedTimestampMs=222`; `initializationDurationMs=19` | `API_MAPPING_STATUS.md` analytics audio decoder initialized; `DATA_STRUCTURE_MAPPING.md` `AudioDecoderInitializedEvent` | Seventh concrete reduced analytics event |
| `nativeAnalyticsVideoDecoderInitializedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `decoderName=c2.android.hevc.decoder`; `initializedTimestampMs=444`; `initializationDurationMs=29` | `API_MAPPING_STATUS.md` analytics video decoder initialized; `DATA_STRUCTURE_MAPPING.md` `VideoDecoderInitializedEvent` | Eighth concrete reduced analytics event |
| `nativeAnalyticsAudioDecoderReleasedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `decoderName=c2.android.eac3.decoder` | `API_MAPPING_STATUS.md` analytics audio decoder released; `DATA_STRUCTURE_MAPPING.md` `AudioDecoderReleasedEvent` | Ninth concrete reduced analytics event |
| `nativeAnalyticsVideoDecoderReleasedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `decoderName=c2.android.hevc.decoder` | `API_MAPPING_STATUS.md` analytics video decoder released; `DATA_STRUCTURE_MAPPING.md` `VideoDecoderReleasedEvent` | Tenth concrete reduced analytics event |
| `nativeAnalyticsRenderedFirstFrameSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `renderTimeMs=456` | `API_MAPPING_STATUS.md` analytics rendered first frame; `DATA_STRUCTURE_MAPPING.md` `AnalyticsRenderedFirstFrameEvent` | Eleventh concrete reduced analytics event |
| `nativeAnalyticsVideoSizeChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `width=1920`; `height=1080`; `pixelWidthHeightRatio=1.250000` | `API_MAPPING_STATUS.md` analytics video size changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsVideoSizeChangedEvent` | Twelfth concrete reduced analytics event |
| `nativeAnalyticsAudioPositionAdvancingSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `playoutStartSystemTimeMs=2222` | `API_MAPPING_STATUS.md` analytics audio position advancing; `DATA_STRUCTURE_MAPPING.md` `AudioPositionAdvancingEvent` | Thirteenth concrete reduced analytics event |
| `nativeAnalyticsVideoFrameProcessingOffsetSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `totalProcessingOffsetUs=67890`; `frameCount=8` | `API_MAPPING_STATUS.md` analytics video frame processing offset; `DATA_STRUCTURE_MAPPING.md` `VideoFrameProcessingOffsetEvent` | Fourteenth concrete reduced analytics event |
| `nativeAnalyticsVolumeChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `volume=0.750000` | `API_MAPPING_STATUS.md` analytics volume changed; `DATA_STRUCTURE_MAPPING.md` `VolumeChangedEvent` | Fifteenth concrete reduced analytics event |
| `nativeAnalyticsAudioSessionIdChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `audioSessionId=42` | `API_MAPPING_STATUS.md` analytics audio session id changed; `DATA_STRUCTURE_MAPPING.md` `AudioSessionIdChangedEvent` | Sixteenth concrete reduced analytics event |
| `nativeAnalyticsSkipSilenceEnabledChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `skipSilenceEnabled=1` | `API_MAPPING_STATUS.md` analytics skip silence enabled changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsSkipSilenceEnabledChangedEvent` | Seventeenth concrete reduced analytics event |
| `nativeAnalyticsDeviceVolumeChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `volume=7`; `muted=0` | `API_MAPPING_STATUS.md` analytics device volume changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsDeviceVolumeChangedEvent` | Eighteenth concrete reduced analytics event |
| `nativeAnalyticsPlaybackStateChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `playbackState=3` | `API_MAPPING_STATUS.md` analytics playback state changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsPlaybackStateChangedEvent` | Nineteenth concrete reduced analytics event |
| `nativeAnalyticsIsPlayingChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `isPlaying=1` | `API_MAPPING_STATUS.md` analytics is playing changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsIsPlayingChangedEvent` | Twentieth concrete reduced analytics event |
| `nativeAnalyticsPlayWhenReadyChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `playWhenReady=1`; `reason=2` | `API_MAPPING_STATUS.md` analytics play when ready changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsPlayWhenReadyChangedEvent` | Twenty-first concrete reduced analytics event |
| `nativeAnalyticsPlaybackSuppressionReasonChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `playbackSuppressionReason=1` | `API_MAPPING_STATUS.md` analytics playback suppression reason changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsPlaybackSuppressionReasonChangedEvent` | Twenty-second concrete reduced analytics event |
| `nativeAnalyticsIsLoadingChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `isLoading=1` | `API_MAPPING_STATUS.md` analytics is loading changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsIsLoadingChangedEvent` | Twenty-third concrete reduced analytics event |
| `nativeAnalyticsRepeatModeChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `repeatMode=2` | `API_MAPPING_STATUS.md` analytics repeat mode changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsRepeatModeChangedEvent` | Twenty-fourth concrete reduced analytics event |
| `nativeAnalyticsShuffleModeChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `shuffleModeEnabled=1` | `API_MAPPING_STATUS.md` analytics shuffle mode changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsShuffleModeChangedEvent` | Twenty-fifth concrete reduced analytics event |
| `nativeAnalyticsVideoInputFormatChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `sampleMimeType=video/final`; `codecs=hvc1.1.6.L93.B0`; `width=1920`; `height=1080`; `frameRate=59.939999` | `API_MAPPING_STATUS.md` analytics video input format changed; `DATA_STRUCTURE_MAPPING.md` `VideoInputFormatChangedEvent` | Twenty-sixth concrete reduced analytics event |
| `nativeAnalyticsPlaybackParametersChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `speed=1.500000`; `pitch=0.750000` | `API_MAPPING_STATUS.md` analytics playback parameters changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsPlaybackParametersChangedEvent` | Twenty-seventh concrete reduced analytics event |
| `nativeAnalyticsAvailableCommandsChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `commandCount=3`; `firstCommand=3`; `contains8=1` | `API_MAPPING_STATUS.md` analytics available commands changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsAvailableCommandsChangedEvent` | Twenty-eighth concrete reduced analytics event |
| `nativeAnalyticsEventsSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `eventCount=3`; `firstEvent=7`; `contains9=1` | `API_MAPPING_STATUS.md` analytics events batch; `DATA_STRUCTURE_MAPPING.md` `AnalyticsEventsEvent` | Twenty-ninth concrete reduced analytics event |
| `nativeAnalyticsSeekBackIncrementChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `seekBackIncrementMs=15000` | `API_MAPPING_STATUS.md` analytics seek back increment changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsSeekBackIncrementChangedEvent` | Thirtieth concrete reduced analytics event |
| `nativeAnalyticsSeekForwardIncrementChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `seekForwardIncrementMs=25000` | `API_MAPPING_STATUS.md` analytics seek forward increment changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsSeekForwardIncrementChangedEvent` | Thirty-first concrete reduced analytics event |
| `nativeAnalyticsMaxSeekToPreviousPositionChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `maxSeekToPreviousPositionMs=12000` | `API_MAPPING_STATUS.md` analytics max seek to previous position changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsMaxSeekToPreviousPositionChangedEvent` | Thirty-second concrete reduced analytics event |
| `nativeAnalyticsTimelineChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `reason=2` | `API_MAPPING_STATUS.md` analytics timeline changed; `DATA_STRUCTURE_MAPPING.md` `AnalyticsTimelineChangedEvent` | Thirty-third concrete reduced analytics event |
| `nativeAnalyticsPositionDiscontinuitySmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `reason=5` | `API_MAPPING_STATUS.md` analytics position discontinuity; `DATA_STRUCTURE_MAPPING.md` `AnalyticsPositionDiscontinuityEvent` | Thirty-fourth concrete reduced analytics event |
| `nativeAnalyticsSeekStartedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `started=1` | `API_MAPPING_STATUS.md` analytics seek started; `DATA_STRUCTURE_MAPPING.md` `AnalyticsSeekStartedEvent` | Thirty-fifth concrete reduced analytics event |
| `nativeAnalyticsDeviceInfoChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `playbackType=1`; `minVolume=2`; `maxVolume=15`; `routingControllerId=route-final` | `API_MAPPING_STATUS.md` analytics device info changed; `DATA_STRUCTURE_MAPPING.md` `DeviceInfoDescriptor` | Thirty-sixth concrete reduced analytics event |
| `nativeAnalyticsMediaMetadataChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `title=Analytics Media Final`; `artist=Analytics Artist Final`; `displayTitle=Analytics Display Final` | `API_MAPPING_STATUS.md` analytics media metadata changed; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` | Thirty-seventh concrete reduced analytics event |
| `nativeAnalyticsPlaylistMetadataChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `title=Analytics Playlist Final`; `artist=Analytics Playlist Artist Final`; `displayTitle=Analytics Playlist Display Final` | `API_MAPPING_STATUS.md` analytics playlist metadata changed; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` | Thirty-eighth concrete reduced analytics event |
| `nativeAnalyticsPlayerErrorSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `errorCode=2002`; `message=analytics-final-error` | `API_MAPPING_STATUS.md` analytics player error; `DATA_STRUCTURE_MAPPING.md` `PlayerError` | Thirty-ninth concrete reduced analytics event |
| `nativeAnalyticsPlayerErrorChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `errorCode=4004`; `message=analytics-final-changed` | `API_MAPPING_STATUS.md` analytics player error changed; `DATA_STRUCTURE_MAPPING.md` `PlayerError` | Fortieth concrete reduced analytics event |
| `nativeAnalyticsTracksChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `groupCount=2`; `firstGroupId=video-main`; `containsVideo=1` | `API_MAPPING_STATUS.md` analytics tracks changed; `DATA_STRUCTURE_MAPPING.md` `TracksSnapshot` | Forty-first concrete reduced analytics event |
| `nativeAnalyticsMediaItemTransitionSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `mediaId=analytics-transition-final`; `sourceType=2`; `reason=2` | `API_MAPPING_STATUS.md` analytics media item transition; `DATA_STRUCTURE_MAPPING.md` `AnalyticsMediaItemTransitionEvent` | Forty-second concrete reduced analytics event |
| `nativeAnalyticsCuesSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `cueCount=2`; `presentationTimeUs=654321`; `text0=Analytics Cue Final`; `text0TokenPresent=1`; `bitmap0TokenPresent=1`; `text1=Analytics Cue Final 2`; `text1TokenPresent=1`; `bitmap1TokenPresent=0` | `API_MAPPING_STATUS.md` analytics cues; `DATA_STRUCTURE_MAPPING.md` `CueSnapshot` | Forty-third concrete reduced analytics event |
| `nativeAnalyticsMetadataSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `entryCount=2`; `firstEntryType=MdtaMetadataEntry`; `firstEntryText=analytics-metadata-final` | `API_MAPPING_STATUS.md` analytics metadata; `DATA_STRUCTURE_MAPPING.md` `AnalyticsMetadataEvent` | Forty-fourth concrete reduced analytics event |
| `nativeAnalyticsLoadErrorSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `uri=https://example.com/analytics-error-final.m3u8`; `dataType=4`; `trackType=2`; `message=analytics-load-final`; `wasCanceled=0` | `API_MAPPING_STATUS.md` analytics load error; `DATA_STRUCTURE_MAPPING.md` `AnalyticsLoadErrorEvent` | Forty-fifth concrete reduced analytics event |
| `nativeMediaSourceFactoryInjectionSmokeTest_usesRegisteredFactoryToken` | `factoryToken=test-injected-media-source-factory`; `injectedFactoryUsed=1`; `factoryIdentity=` | `API_MAPPING_STATUS.md` media source factory baseline config; `DATA_STRUCTURE_MAPPING.md` `PlayerConfig::MediaSourceFactoryConfig` | Registered factory-token injection |
| `nativeMediaSourceFactoryInjectionFallbackSmokeTest_fallsBackWhenTokenIsMissing` | `factoryToken=missing-media-source-factory-token`; `fallbackApplied=1`; `factoryIdentity=0` | `API_MAPPING_STATUS.md` media source factory baseline config; `DATA_STRUCTURE_MAPPING.md` `PlayerConfig::MediaSourceFactoryConfig` | Missing-token fallback |
| `nativeMediaSourceFactoryInjectionMultiTokenSmokeTest_keepsTokensIsolated` | `firstFactoryToken=multi-token-media-source-factory-a`; `secondFactoryToken=multi-token-media-source-factory-b`; `tokensIsolated=1` | `API_MAPPING_STATUS.md` media source factory baseline config; `DATA_STRUCTURE_MAPPING.md` `PlayerConfig::MediaSourceFactoryConfig` | Multi-token isolation |
| `nativeImageOutputSmokeTest_returnsListenerSummary` | `lastAllocationByteCount=384`; `lastRowBytes=48`; `lastIsPremultiplied=1`; `reattachLastBitmapConfig=RGB_565`; `reattachLastHasAlpha=0` | `API_MAPPING_STATUS.md` image output; `DATA_STRUCTURE_MAPPING.md` `ImageFrameSnapshot` | Image-output bitmap-layout metadata |
| `nativeVideoEffectsConversionSmokeTest_returnsStructuredSummary` | `afterReapply=effectCount=7`; `effect3=rgbAdjustment:redScale=1.0:greenScale=1.0:blueScale=1.0`; `effect4=scaleAndRotate:scaleX=1.0:scaleY=1.0:rotationDegrees=0.0`; `effect6=presentation:width=320:height=240:layout=1` | `API_MAPPING_STATUS.md` video effects; `DATA_STRUCTURE_MAPPING.md` `VideoEffectDescriptor` | Video-effects boundary/default-value parity |

## 6. Demo Verification Rollup

| Check | Result | Notes |
| --- | --- | --- |
| Demo app installs | Pending |  |
| Demo app launches | Pending |  |
| Player controls respond | Pending |  |
| C++ callbacks visible in logs/output | Pending |  |
| No crash / ANR during manual flow | Pending |  |

## 7. Blocking Issues

| ID | Severity | Area | Symptom | Next action |
| --- | --- | --- | --- | --- |
|  |  |  |  |  |

## 8. Recommended Next Step

- Summary:
- Ship / continue development decision:
- Owner:
- Date:
