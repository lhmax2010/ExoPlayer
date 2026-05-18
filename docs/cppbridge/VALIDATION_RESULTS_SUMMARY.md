# C++ Bridge Validation Results Summary

Last updated: 2026-05-18

Use this page after the new environment finishes build, device validation, and demo verification.
Write raw run details into `TEST_RESULTS_TEMPLATE.md`, then summarize the final outcome here.
If a result suggests the current reduced endpoint tracker is no longer accurate, cross-check the
status notes in:
`docs/cppbridge/API_MAPPING_STATUS.md`

## 1. Overall Result

| Area | Status | Evidence | Owner | Notes |
| --- | --- | --- | --- | --- |
| Build / native link | Pass | `:lib-exoplayer-cppbridge:assembleDebugAndroidTest`; `:lib-exoplayer-cppbridge:testDebugUnitTest`; `:demo-cppbridge:assembleDebug` | Codex | Native testhooks and demo build linked successfully. |
| JNI/value smoke | Pass | included in full `:lib-exoplayer-cppbridge:connectedDebugAndroidTest` | Codex | Full Android 16 connected suite passed. |
| Player/runtime smoke | Pass | `:lib-exoplayer-cppbridge:connectedDebugAndroidTest` reported `124/124` tests passed | Codex | Includes runtime/audio/scrubbing/codec/renderer getter parity smokes, auxiliary callback parity, video-frame fallback/sentinel smoke, track payload parity, and HTTP/HLS/DASH C++ playback smoke. |
| API parity inventory | Pass | `python3 scripts/cppbridge/api_parity_inventory.py --check` | Codex | Generated report is current; exact `Player`/`ExoPlayer` and direct `Player.Listener` gaps are now `0`, with one remaining builder gap: `setAudioOutputProvider`. |
| Demo manual validation | Pass with notes | `:demo-cppbridge:assembleDebug` | Codex | Demo compiled; manual UI playback was not separately exercised in this pass. |
| Logcat review | Pass with notes | no test failure or JNI exception surfaced during Gradle instrumentation | Codex | Dedicated logcat audit was not separately captured. |
| Release recommendation | Pass with notes | build + unit + connected smoke + demo build all passed | Codex | Reduced bridge is smoke-validated; continue development for deeper full-object parity and callback edge cases. |

Status values:

- `Pass`
- `Pass with notes`
- `Fail`
- `Blocked`
- `Pending`

## 2. Environment Summary

| Item | Value |
| --- | --- |
| Validation date | 2026-05-18 |
| Machine / host | `linhao-linux` |
| Branch / package snapshot | local dirty git workspace at `/home/linhao/Toolchain/development/ExoPlayer` |
| JDK | OpenJDK `17.0.18` |
| Gradle | Gradle `8.13` |
| Android SDK | `/home/linhao/Android/Sdk`, platforms include `android-35` and `android-36.1` |
| NDK / CMake | NDK `27.0.12077973` and `30.0.14904198`; CMake `4.1.2` available |
| Device / emulator | AVD `cppbridge_android16_api36`, Google APIs x86_64, Android 16.0 |
| `ANDROID_SERIAL` | default single connected emulator during the run |

## 3. Smoke Suite Rollup

| Suite | Result | Key evidence | Follow-up needed |
| --- | --- | --- | --- |
| `CppBridgeNativeSmokeTest` | Pass | covered by full `connectedDebugAndroidTest` | none for this pass |
| `CppBridgeNativePlayerInstrumentationTest` | Pass | covered by full `connectedDebugAndroidTest`; total connected suite `124/124` passed | none for this pass |
| `run_validation.sh` aggregate result | Pass with notes | equivalent manual Gradle commands were run directly instead of the wrapper | run wrapper later if a single archived transcript is needed |

## 3A. 2026-05-15 through 2026-05-18 Parity Addendum

| Area | New smoke | Result | Notes |
| --- | --- | --- | --- |
| Runtime controls | `nativeRuntimeControlParitySmokeTest_updatesPhaseOneRuntimeControls` | Pass | Covers noisy handling, foreground mode, pause-at-end, seek increment setters, max seek-to-previous, video scaling mode, and frame-rate strategy. |
| Advanced audio/scrubbing | `nativeAudioAndScrubbingParitySmokeTest_updatesAdvancedRuntimeControls` | Pass | Covers audio session ID, aux effect info, preferred audio device clear path, virtual device ID, and scrubbing mode parameters. |
| Codec parameters | `nativeCodecParametersParitySmokeTest_setsAudioAndVideoCodecParameters` | Pass | Covers typed audio/video codec parameter entries through `CppCodecParameter`. |
| Auxiliary callbacks | `nativeAuxiliaryCallbackParitySmokeTest_reportsCodecVideoAndCameraCallbacks` | Pass | Covers reduced codec-parameter change callbacks, video frame metadata, representative `Format` label/language/container MIME/bitrate/rotation/pixel-ratio/color/audio-shape/flags fields, richer `MediaFormat` mime/size/frame-rate/rotation/color payload fields, camera motion, camera reset, and remove-listener stop behavior. |
| Video-frame metadata fallback/sentinels | `nativeVideoFrameMetadataSimulationFallbackSmokeTest_preservesFallbackFields` | Pass | Covers C++ `format_bitrate` fallback into Java `Format.averageBitrate` when average/peak are unset, plus `Format.NO_VALUE` preservation for absent color/audio-shape fields. |
| Track format payload expansion | `nativeCurrentTracksSmokeTest_returnsTracksSummary`; `nativeTracksSnapshotConversionSmokeTest_returnsStructuredSummary`; `CppBridgeConvertersTest.toCppTrackGroups_mapsSelectionAndSupport` | Pass | Covers `TrackInfo` average/peak bitrate, initialization/DRM counts, subsample/preroll, decoded/projection/stereo/color, max sublayers, PCM/encoder, tile, and crypto fields through Java converter, JNI create/parse, and native current-tracks smoke paths. |
| Direct listener is-loading callback | `nativeListenerSmokeTest_reportsExtendedCallbacks` | Pass | Covers `Player.Listener#onIsLoadingChanged` through `OnIsLoadingChanged` / `nativeOnIsLoadingChanged` with `isLoadingCb=1` and `isLoading=1` markers. |
| Codec-parameter multi-listener callbacks | `nativeCodecParametersMultiListenerParitySmokeTest_routesImmediateCallbacks` | Pass | Covers Java-style immediate delivery only to the newly added listener and suppresses synthetic callbacks on internal re-registration after removal. |
| Renderer/device-state getters | `nativeRendererAndDeviceStateGetterSmokeTest_readsRendererAndDeviceState` | Pass | Covers renderer count/type, offload sleeping, tunneling enabled, and released state. |
| HTTP/HLS/DASH C++ playback | `nativeHttpHlsDashPlaybackSmokeTest_preparesLocalStreamsThroughCppApi` | Pass | Covers local MockWebServer playback for HTTP progressive, HLS, and DASH via C++ `SetMediaItem` / `Prepare` / `Play`; verifies source type round-trip markers `5`, `2`, and `1`. |
| Coverage top-up | existing surface / playlist / query / device / builder / priority smokes | Pass | Direct coverage added for raw `Surface` overloads, `RemoveMediaItem`, `MoveMediaItems`, playlist navigation getters, SDK and bridge tracks getters, device volume/mute setters, builder `SetMediaSourceFactoryConfig`, codec-parameter bridge registration/clear, and SDK `ClearPriorityTaskManager`. |

Recommended spot checks for the explicit opaque-token cleanup smoke:

- `released=1`
- `state=1`
- `tokenCount=4`

## 3B. Second-Batch Triage

| Bucket | Item | Validation-first or implementation-first | Result | Notes |
| --- | --- | --- | --- | --- |
| Second-batch closed | analytics aggregate / analytics-only listener | Validation-first | Pass | Covered by full Android 16 `connectedDebugAndroidTest`. |
| Second-batch closed | image output reduced callback behavior | Validation-first | Pass | Covered by full Android 16 `connectedDebugAndroidTest`. |
| Second-batch closed | renderer messaging reduced model | Validation-first | Pass | Covered by full Android 16 `connectedDebugAndroidTest`. |
| Second-batch closed | wake mode create-time + runtime setter | Validation-first | Pass | Covered by full Android 16 `connectedDebugAndroidTest`. |
| Second-batch closed | priority reduced wrapper/runtime state | Validation-first | Pass | Covered by full Android 16 `connectedDebugAndroidTest`. |
| Second-batch closed | preload target-duration behavior | Validation-first | Pass | Covered by full Android 16 `connectedDebugAndroidTest`. |
| Second-batch closed | builder-style reduced config/build path | Validation-first | Pass | Covered by full Android 16 `connectedDebugAndroidTest`. |
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
| Analytics audio session id changed | `audioSessionId=700042` |  |  |  |
| HTTP progressive C++ playback | `httpPrepared=1`; `httpAdvanced=1`; `httpSourceType=5`; `httpMimeType=audio/mp4` |  |  |  |
| HLS C++ playback | `hlsPrepared=1`; `hlsAdvanced=1`; `hlsSourceType=2`; `hlsMimeType=application/x-mpegURL` |  |  |  |
| DASH C++ playback | `dashPrepared=1`; `dashAdvanced=1`; `dashSourceType=1`; `dashMimeType=application/dash+xml` |  |  |  |
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
| `nativeCurrentTracksSmokeTest_returnsTracksSummary` | `group0Id=video-group`; `group0TokenPresent=1`; `track0LabelTokenPresent=1`; `track0AverageBitrate=2000000`; `track0PeakBitrate=2500000`; `track0MetadataEntryCount=2`; `track0InitializationData=2:7`; `track0DrmSchemeDataCount=1`; `track0SubsampleOffsetUs=987654`; `track0DecodedSize=1936x1096`; `track0ProjectionDataLength=4`; `track0Color=1:2:3`; `track0EncoderTrim=0:0`; `track0Tiles=5x6`; `track0CryptoType=2`; `group1Id=audio-group`; `group1TokenPresent=1`; `group1Track0LabelTokenPresent=1`; `group1Track0InitializationData=1:3`; `group1Track0PcmEncoding=2`; `group1Track0EncoderTrim=12:34` | `DATA_STRUCTURE_MAPPING.md` Tracks / `TracksSnapshot`, `TrackGroupSnapshot`, `TrackInfo` | Current tracks parity |
| `nativeTracksSnapshotConversionSmokeTest_returnsStructuredSummary` | `track0AverageBitrate=2000000`; `track0PeakBitrate=2500000`; `track0MetadataEntryCount=2`; `track0InitializationData=2:7`; `track0DrmSchemeDataCount=1`; `track0SubsampleOffsetUs=987654`; `track0DecodedSize=1936x1096`; `track0ProjectionDataLength=4`; `track0Color=1:2:3`; `track0EncoderTrim=0:0`; `track0Tiles=5x6`; `track0CryptoType=2`; `group1Track0InitializationData=1:3`; `group1Track0PcmEncoding=2`; `group1Track0EncoderTrim=12:34` | `DATA_STRUCTURE_MAPPING.md` Tracks / `TrackInfo`; `DATA_STRUCTURE_QUICK_REFERENCE.md` `CppTrackInfo` | Track snapshot conversion parity |
| `nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary` | `cueCount=2`; `cuePresentationTimeUs=456789`; `cue0Text=Query Cue 1`; `cue0TextTokenPresent=1`; `cue0BitmapTokenPresent=1`; `cue1Text=Query Cue 2`; `cue1TextTokenPresent=1`; `cue1BitmapTokenPresent=0`; `tracksGroupCount=`; `trackGroupVectorCount=`; `bridgeTracksGroupCount=`; `bridgeTrackGroupVectorCount=` | `API_MAPPING_STATUS.md` audio/query getters; `DATA_STRUCTURE_MAPPING.md` `CueSnapshot` / `TracksSnapshot` | Current cues and tracks query parity |
| `nativePlaylistMutationSmokeTest_returnsUpdatedPlaylistState` | `moveRangeFirstMediaId=item-4`; `singleRemoveRestoredCount=3`; `nextIndex=1`; `previousIndex=-1`; `hasNext=1`; `hasPrevious=0` | `API_MAPPING_STATUS.md` media item mutation / playlist navigation getters | Playlist mutation and navigation parity |
| `nativeListenerSmokeTest_reportsExtendedCallbacks` | `timelineWindow0MediaId=listener-item-1`; `timelineWindow0TagPresent=1`; `timelineWindow0TagString=listener-tag-1`; `timelineWindow0TagTokenPresent=1`; `timelineWindow1MediaItemIndex=1`; `timelineWindow1MediaId=listener-item-2`; `timelineWindow1TagString=listener-tag-2`; `timelineWindow1TagTokenPresent=1`; `timelineWindow1LiveConfigurationPresent=1`; `timelineWindow1LiveTargetOffsetMs=6100`; `timelineWindow1LiveMinOffsetMs=5200`; `timelineWindow1LiveMaxOffsetMs=7800`; `timelineWindow1LiveMinSpeed=0.940000`; `timelineWindow1LiveMaxSpeed=1.080000`; `timelineWindow1ManifestPresent=`; `timelineWindow1ManifestString=`; `timelineWindow1ManifestTokenPresent=`; `timelineWindow1FirstPeriodIndex=`; `timelineWindow1LastPeriodIndex=`; `timelineWindow1PresentationStartTimeMs=`; `timelineWindow1WindowStartTimeMs=`; `timelineWindow1ElapsedRealtimeEpochOffsetMs=`; `timelineWindow1DefaultPositionMs=`; `timelineWindow1DefaultPositionUs=`; `timelineWindow1DurationMs=`; `timelineWindow1DurationUs=`; `timelineWindow1Seekable=`; `timelineWindow1Live=`; `timelineWindow1Placeholder=`; `timelinePeriod1Id=`; `firstTrackGroupTokenPresent=1`; `secondTrackGroupId=`; `secondTrackLabel=`; `secondTrackLanguage=`; `secondTrackMimeType=`; `secondTrackAccessibilityChannel=`; `secondTrackRoleFlags=`; `secondTrackSelectionFlags=`; `secondTrackSelected=`; `secondTrackSupported=`; `secondTrackSupportedWithinCapabilities=`; `mediaMetadataAlbumTitle=Listener Item Album`; `mediaMetadataWriter=Listener Item Writer`; `mediaMetadataGenre=Listener Item Genre`; `mediaMetadataExtrasPresent=1`; `mediaMetadataExtrasKeyCount=1`; `mediaMetadataExtrasTokenPresent=1`; `mediaMetadataArtworkUri=https://example.com/listener-item-artwork.jpg`; `mediaMetadataArtworkDataLength=4`; `mediaMetadataArtworkDataType=6`; `playlistMetadataAlbumTitle=Listener Playlist Album`; `playlistMetadataConductor=Listener Playlist Conductor`; `playlistMetadataStation=Listener Playlist Station`; `playlistMetadataExtrasPresent=1`; `playlistMetadataExtrasKeyCount=1`; `playlistMetadataExtrasTokenPresent=1`; `playlistMetadataArtworkUri=https://example.com/listener-playlist-artwork.jpg`; `playlistMetadataArtworkDataLength=3`; `playlistMetadataArtworkDataType=8`; `cueCb=1`; `isLoadingCb=1`; `isLoading=1`; `cue0Text=Listener Cue 1`; `cue0TextAlignment=2`; `cue0Line=0.250000`; `cue1Text=Listener Cue 2`; `cue1LineType=1`; `cue1TextSize=22.000000`; `cue1VerticalType=1`; `oldTagTokenPresent=1`; `newTagTokenPresent=1` | `API_MAPPING_STATUS.md` direct listener families; `DATA_STRUCTURE_MAPPING.md` Timeline / Tracks / MediaMetadata / Cue / PositionInfo | Direct listener parity |
| `nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary` | `timelineWindow0MediaUri=https://example.com/payload-window-0.m3u8`; `timelineWindow0TagPresent=1`; `timelineWindow0TagString=payload-window-tag`; `timelineWindow0TagTokenPresent=1`; `timelineWindow0Uid=payload-window-uid`; `timelineWindow0LiveTargetOffsetMs=3333`; `timelineWindow1MediaId=payload-window-1`; `timelineWindow1TagTokenPresent=1`; `timelinePeriod0Id=payload-period-id`; `timelinePeriod0Uid=payload-period-0`; `timelinePeriod0AdsId=payload-period-ads-id`; `timelinePeriod1Id=payload-period-id-2`; `timelinePeriod1Uid=payload-period-1`; `timelinePeriod1AdsId=payload-period-ads-id-2`; `firstTrackGroupTokenPresent=1`; `firstTrackLabelTokenPresent=1`; `secondTrackGroupTokenPresent=1`; `secondTrackLabelTokenPresent=1`; `oldTagTokenPresent=1`; `newTagTokenPresent=1` | `API_MAPPING_STATUS.md` timeline changed / tracks changed / position discontinuity; `DATA_STRUCTURE_MAPPING.md` Timeline Period / TracksSnapshot / PositionInfoSnapshot | Listener payload parity |
| `nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary` | `mediaId=current-item`; `sourceType=2`; `subtitle1MimeType=text/vtt`; `subtitle1Label=Spanish`; `subtitle1SelectionFlags=3`; `subtitle1RoleFlags=13`; `requestMetadataExtrasKeyCount=1`; `requestMetadataExtrasTokenPresent=1`; `mediaMetadataAlbumTitle=Current Album`; `mediaMetadataDisplayTitle=Current Item Display`; `mediaMetadataTitleTokenPresent=1`; `mediaMetadataAlbumTitleTokenPresent=1`; `mediaMetadataAlbumArtistTokenPresent=1`; `mediaMetadataAuthorTokenPresent=1`; `mediaMetadataComposerTokenPresent=1`; `mediaMetadataConductorTokenPresent=1`; `mediaMetadataGenreTokenPresent=1`; `mediaMetadataCompilationTokenPresent=1`; `mediaMetadataStationTokenPresent=1`; `mediaMetadataExtrasPresent=1`; `mediaMetadataExtrasKeyCount=1`; `mediaMetadataExtrasTokenPresent=1`; `artworkUri=https://example.com/current-artwork.jpg`; `artworkDataLength=4`; `artworkDataType=3` | `API_MAPPING_STATUS.md` `getCurrentMediaItem`; `DATA_STRUCTURE_MAPPING.md` `MediaItemDescriptor` | Current item parity |
| `nativeMediaItemAtSmokeTest_returnsSnapshotAndHandlesOutOfBounds` | `mediaId=at-2`; `subtitle1MimeType=text/vtt`; `subtitle1Label=Spanish`; `subtitle1SelectionFlags=1`; `subtitle1RoleFlags=5`; `requestMetadataExtrasKeyCount=1`; `requestMetadataExtrasTokenPresent=1`; `mediaMetadataDisplayTitle=At Two Display`; `mediaMetadataDescription=At Two Description`; `mediaMetadataTitleTokenPresent=1`; `mediaMetadataAlbumTitleTokenPresent=1`; `mediaMetadataAlbumArtistTokenPresent=1`; `mediaMetadataAuthorTokenPresent=1`; `mediaMetadataComposerTokenPresent=1`; `mediaMetadataConductorTokenPresent=1`; `mediaMetadataGenreTokenPresent=1`; `mediaMetadataCompilationTokenPresent=1`; `mediaMetadataStationTokenPresent=1`; `mediaMetadataExtrasPresent=1`; `mediaMetadataExtrasKeyCount=1`; `mediaMetadataExtrasTokenPresent=1`; `artworkUri=https://example.com/at-two-artwork.jpg`; `artworkDataLength=4`; `artworkDataType=5` | `API_MAPPING_STATUS.md` `getMediaItemAt`; `DATA_STRUCTURE_MAPPING.md` `MediaItemDescriptor` | Indexed item parity |
| `nativeVideoAndMetadataSmokeTest_returnsQuerySummary` | `mediaTitle=Video Metadata Title`; `mediaArtworkUri=https://example.com/video-metadata-artwork.jpg`; `mediaArtworkDataLength=4`; `mediaArtworkDataType=3`; `mediaExtrasPresent=1`; `mediaExtrasKeyCount=1`; `mediaExtrasTokenPresent=1`; `playlistArtworkUri=https://example.com/metadata-playlist-artwork.jpg`; `playlistArtworkDataLength=3`; `playlistArtworkDataType=4`; `playlistExtrasPresent=1`; `playlistExtrasKeyCount=1`; `playlistExtrasTokenPresent=1` | `API_MAPPING_STATUS.md` media metadata; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` | Metadata query parity |
| `nativePlaylistMetadataSmokeTest_roundTripsPlaylistMetadata` | `title=Playlist Title`; `description=Playlist Description`; `artworkUri=https://example.com/playlist-artwork.jpg`; `artworkDataLength=3`; `artworkDataType=4`; `extrasPresent=1`; `extrasKeyCount=1`; `extrasTokenPresent=1` | `API_MAPPING_STATUS.md` playlist metadata set/get; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` | Playlist metadata round-trip |
| `nativePlaylistMetadataOpaqueTokenSmokeTest_resolvesRegisteredObjects` | `title=registered-title`; `albumTitle=registered-album-title`; `albumArtist=registered-album-artist`; `displayTitle=registered-display`; `subtitle=registered-subtitle`; `description=registered-description`; `writer=registered-writer`; `author=registered-author`; `composer=registered-composer`; `conductor=registered-conductor`; `genre=registered-genre`; `compilation=registered-compilation`; `station=registered-station`; `albumTitleTokenPresent=1`; `albumArtistTokenPresent=1`; `subtitleTokenPresent=1`; `descriptionTokenPresent=1`; `writerTokenPresent=1`; `authorTokenPresent=1`; `composerTokenPresent=1`; `conductorTokenPresent=1`; `genreTokenPresent=1`; `compilationTokenPresent=1`; `stationTokenPresent=1`; `extrasPresent=1`; `extrasKeyCount=1`; `extrasTokenPresent=1` | `API_MAPPING_STATUS.md` playlist metadata set/get; `DATA_STRUCTURE_MAPPING.md` `MediaMetadataSnapshot` | Playlist metadata opaque-token parity |
| `nativeSourceTypeSmokeTest_returnsInferredMimeSummary` | `sourceType=2` | `API_MAPPING_STATUS.md` `getCurrentMediaItem`; `DATA_STRUCTURE_MAPPING.md` `MediaItemDescriptor` | URI-based source type inference |
| `nativeHttpHlsDashPlaybackSmokeTest_preparesLocalStreamsThroughCppApi` | `httpPrepared=1`; `httpAdvanced=1`; `httpSourceType=5`; `hlsPrepared=1`; `hlsAdvanced=1`; `hlsSourceType=2`; `dashPrepared=1`; `dashAdvanced=1`; `dashSourceType=1` | `API_MAPPING_STATUS.md` `setMediaItem` / `prepare` / `play`; `DATA_STRUCTURE_MAPPING.md` `MediaItemDescriptor` | Local HTTP/HLS/DASH playback parity through C++ API |
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
| `nativeAnalyticsAudioSessionIdChangedSmokeTest_reportsConcreteAnalyticsEvent` | `beforeRemoveCb=2`; `callbackStopped=1`; `audioSessionId=700042` | `API_MAPPING_STATUS.md` analytics audio session id changed; `DATA_STRUCTURE_MAPPING.md` `AudioSessionIdChangedEvent` | Sixteenth concrete reduced analytics event |
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

- Summary: reduced bridge smoke suite is passing locally on Android 16 after the runtime/audio/codec/getter/callback parity addendum.
- Ship / continue development decision: continue development; next target is deeper full-object parity and callback edge cases beyond the first reduced callback slice.
- Owner: Codex
- Date: 2026-05-15
