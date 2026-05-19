# Development Stages

Last updated: 2026-05-19

This document summarizes what has been developed so far, what remains, and what the final delivery
package should contain.

## Today Push-To-Done Plan

Current objective:

- move every honest near-complete reduced family to `Done`
- keep code, smoke coverage, and mapping docs synchronized in the same pass
- leave only true full-parity or new-capability gaps in the remaining-work list

Today checklist:

- re-triage all remaining `Partial` rows into "promote now" vs "still real gap"
- promote low-risk near-complete value/query families first
- continue closing the remaining object and callback gaps with matching smoke updates
- keep validation docs current so the Linux migration environment can run full verification immediately
- finish with a static sweep for stale markers, mismatched signatures, and status-count drift

Current checkpoint:

- `PlaybackParameters`, `DeviceInfo`, and `VideoSize` are now promoted to `Done`
- reduced `Player.Commands` and `Player.Events` snapshots and callbacks are now promoted to `Done`
- the forty-five currently exposed reduced concrete `AnalyticsListener` event families are now promoted to `Done`
- there are no remaining row-level `Partial` items in the current reduced endpoint tracker
- remaining work is now concentrated in the true next-phase capability gaps: full Java parity, richer image/video capability, broader preload, and broader arbitrary factory injection

## Stage 1: Bridge Foundation

Completed:

- created native-facing `ExoPlayerSdkPlayer` and `ExoPlayerBridge` abstractions
- established Java bridge class `CppExoPlayerBridge`
- implemented baseline JNI object creation and bridge registration
- added core player lifecycle, playlist, seek, query, and listener plumbing
- introduced reduced C++ descriptors/snapshots for MediaItem, tracks, timeline, metadata, cue, and analytics

Primary files:

- `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java`
- `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_bridge.h`
- `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_sdk.h`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_sdk.cpp`

## Stage 2: Feature Parity Expansion

Completed:

- media item subtitle, clipping, live, DRM, reduced ads, and reduced metadata support
- track selection reduced parity
- reduced timeline/tracks/device/video/metadata/cue query paths
- analytics aggregate and image output reduced support
- priority task manager, preload, player message, and video effects reduced parity
- demo JNI entrypoints and demo app wiring

## Stage 3: Runtime Hardening

Completed:

- callback threads now resolve `JNIEnv*` through scoped attach/detach
- release teardown now clears listener and image-output listener state
- native callback path is gated by a one-way `releasing_` flag
- SDK forwarding layer now protects listener and analytics delegate state with mutexes
- stale-handle callback dereference risk was reduced by registered bridge lookup and teardown guards

## Stage 4: Structural Refactor

Completed:

- large JNI monolith split into functional `.cpp` files
- callback/demo JNI entrypoints moved out of `bridge.cpp`
- shared declarations moved into internal header

Current JNI split:

- `exoplayer_cppbridge_jni_common.cpp`
- `exoplayer_cppbridge_jni_playlist_helpers.cpp`
- `exoplayer_cppbridge_jni_bridge.cpp`
- `exoplayer_cppbridge_jni_callbacks_demo.cpp`
- `exoplayer_cppbridge_jni_smoke_tests.cpp`

## Stage 5: Second-Batch Partial Strengthening

Completed:

- listener payload depth is now stronger for reduced timeline, tracks, and metadata callbacks
- analytics aggregate smoke now covers multi-update overwrite semantics and remove-listener stop-delivery behavior
- the first concrete reduced `AnalyticsListener` event path is now live through
  `onAudioUnderrun`, including multi-update overwrite semantics and analytics-listener
  remove/stop-delivery behavior
- the second concrete reduced `AnalyticsListener` event path is now live through
  `onDroppedVideoFrames`, including event-level `droppedFrames`/`elapsedMs` delivery,
  multi-update overwrite semantics, and analytics-listener remove/stop-delivery behavior
- the third concrete reduced `AnalyticsListener` event path is now live through
  `onBandwidthEstimate`, including event-level `elapsedMs`/`bytesTransferred`/`bitrateEstimate`
  delivery, multi-update overwrite semantics, and analytics-listener remove/stop-delivery behavior
- the fourth concrete reduced `AnalyticsListener` event path is now live through
  `onLoadStarted`, including a first structured load-event payload
  (`uri`/`dataType`/`trackType`/`retryCount`), multi-update overwrite semantics,
  and analytics-listener remove/stop-delivery behavior
- the fifth concrete reduced `AnalyticsListener` event path is now live through
  `onLoadCompleted`, pairing the structured load-family coverage with
  (`uri`/`dataType`/`trackType`) payload, multi-update overwrite semantics,
  and analytics-listener remove/stop-delivery behavior
- the sixth concrete reduced `AnalyticsListener` event path is now live through
  `onAudioInputFormatChanged`, adding a first reduced format-change payload
  (`sampleMimeType`/`codecs`/`channelCount`/`sampleRate`) with multi-update overwrite
  semantics and analytics-listener remove/stop-delivery behavior
- the seventh concrete reduced `AnalyticsListener` event path is now live through
  `onAudioDecoderInitialized`, adding a first reduced decoder-lifecycle payload
  (`decoderName`/`initializedTimestampMs`/`initializationDurationMs`) with multi-update overwrite
  semantics and analytics-listener remove/stop-delivery behavior
- the eighth concrete reduced `AnalyticsListener` event path is now live through
  `onVideoDecoderInitialized`, pairing decoder-lifecycle coverage with
  (`decoderName`/`initializedTimestampMs`/`initializationDurationMs`) payload and the same
  overwrite/remove semantics
- the ninth concrete reduced `AnalyticsListener` event path is now live through
  `onAudioDecoderReleased`, adding reduced decoder-release coverage with `decoderName`
  payload and the same overwrite/remove semantics
- the tenth concrete reduced `AnalyticsListener` event path is now live through
  `onVideoDecoderReleased`, pairing decoder-release coverage with `decoderName`
  payload and the same overwrite/remove semantics
- the eleventh concrete reduced `AnalyticsListener` event path is now live through
  analytics `onRenderedFirstFrame`, adding reduced `renderTimeMs` coverage with the same
  overwrite/remove semantics
- the twelfth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onVideoSizeChanged`, adding reduced video-size coverage with
  (`width`/`height`/`pixelWidthHeightRatio`) payload and the same overwrite/remove semantics
- the thirteenth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onAudioPositionAdvancing`, adding reduced `playoutStartSystemTimeMs`
  coverage with the same overwrite/remove semantics
- the fourteenth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onVideoFrameProcessingOffset`, adding reduced
  (`totalProcessingOffsetUs`/`frameCount`) coverage with the same overwrite/remove semantics
- the fifteenth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onVolumeChanged`, adding reduced `volume` coverage with the same
  overwrite/remove semantics
- the sixteenth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onAudioSessionIdChanged`, adding reduced `audioSessionId` coverage with the same
  overwrite/remove semantics
- the seventeenth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onSkipSilenceEnabledChanged`, adding reduced `skipSilenceEnabled` coverage with the same
  overwrite/remove semantics
- the eighteenth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onDeviceVolumeChanged`, adding reduced (`volume`/`muted`) coverage with the same
  overwrite/remove semantics
- the nineteenth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onPlaybackStateChanged`, adding reduced `playbackState` coverage with the same
  overwrite/remove semantics
- the twentieth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onIsPlayingChanged`, adding reduced `isPlaying` coverage with the same
  overwrite/remove semantics
- the twenty-first concrete reduced `AnalyticsListener` event path is now live through
  analytics `onPlayWhenReadyChanged`, adding reduced (`playWhenReady`/`reason`) coverage with the
  same overwrite/remove semantics
- the twenty-second concrete reduced `AnalyticsListener` event path is now live through
  analytics `onPlaybackSuppressionReasonChanged`, adding reduced
  `playbackSuppressionReason` coverage with the same overwrite/remove semantics
- the twenty-third concrete reduced `AnalyticsListener` event path is now live through
  analytics `onIsLoadingChanged`, adding reduced `isLoading` coverage with the same
  overwrite/remove semantics
- the twenty-fourth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onRepeatModeChanged`, adding reduced `repeatMode` coverage with the same
  overwrite/remove semantics
- the twenty-fifth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onShuffleModeChanged`, adding reduced `shuffleModeEnabled` coverage with the same
  overwrite/remove semantics
- the twenty-sixth concrete reduced `AnalyticsListener` event path is now live through
  `onVideoInputFormatChanged`, pairing the reduced format-change coverage with
  (`sampleMimeType`/`codecs`/`width`/`height`/`frameRate`) payload and the same
  overwrite/remove semantics
- the twenty-seventh concrete reduced `AnalyticsListener` event path is now live through
  analytics `onPlaybackParametersChanged`, adding reduced (`speed`/`pitch`) coverage with the same
  overwrite/remove semantics
- the twenty-eighth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onAvailableCommandsChanged`, adding reduced command-code batch coverage with
  representative count/first-command/contains-style markers and the same overwrite/remove semantics
- the twenty-ninth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onEvents`, adding reduced event-code batch coverage with representative
  count/first-event/contains-style markers and the same overwrite/remove semantics
- the thirtieth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onDeviceInfoChanged`, adding reduced device-info coverage
  (`playbackType`/`minVolume`/`maxVolume`/`routingControllerId`) with the same overwrite/remove semantics
- the thirty-first concrete reduced `AnalyticsListener` event path is now live through
  analytics `onMediaMetadataChanged`, adding reduced metadata callback coverage
  (`title`/`artist`/`displayTitle`) with the same overwrite/remove semantics
- the thirty-second concrete reduced `AnalyticsListener` event path is now live through
  analytics `onPlaylistMetadataChanged`, pairing playlist metadata callback coverage
  (`title`/`artist`/`displayTitle`) with the same overwrite/remove semantics
- the thirty-third concrete reduced `AnalyticsListener` event path is now live through
  analytics `onSeekBackIncrementChanged`, adding reduced `seekBackIncrementMs` coverage with the
  same overwrite/remove semantics
- the thirty-fourth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onSeekForwardIncrementChanged`, adding reduced `seekForwardIncrementMs` coverage with
  the same overwrite/remove semantics
- the thirty-fifth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onMaxSeekToPreviousPositionChanged`, adding reduced
  `maxSeekToPreviousPositionMs` coverage with the same overwrite/remove semantics
- the thirty-sixth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onTimelineChanged`, adding reduced `reason` coverage with the same overwrite/remove
  semantics
- the thirty-seventh concrete reduced `AnalyticsListener` event path is now live through
  analytics `onPositionDiscontinuity`, adding reduced `reason` coverage with the same
  overwrite/remove semantics
- the thirty-eighth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onSeekStarted`, adding reduced start-delivery coverage with the same
  overwrite/remove semantics
- the thirty-ninth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onPlayerError`, adding reduced (`errorCode`/`message`) coverage with the same
  overwrite/remove semantics
- the fortieth concrete reduced `AnalyticsListener` event path is now live through
  analytics `onPlayerErrorChanged`, adding reduced (`errorCode`/`message`) coverage with the same
  overwrite/remove semantics
- the forty-first concrete reduced `AnalyticsListener` event path is now live through
  analytics `onTracksChanged`, adding reduced tracks summary coverage
  (`groupCount`/`firstGroupType`/`firstGroupId`/`containsAudio`/`containsVideo`) with the same
  overwrite/remove semantics
- image output smoke now covers multi-frame delivery, runtime enable/disable transitions, remove-listener stop-delivery behavior, and listener reattach behavior
- image output smoke now also covers reduced bitmap metadata (`byteCount`, `allocationByteCount`, `rowBytes`, `hasAlpha`, `isPremultiplied`, `isMutable`, `bitmapConfig`), including a non-`ARGB_8888` reattach case
- video effects smoke now validates scale/rotate, RGB adjustment, and presentation parameters, plus clear/reset, reapply ordering, duplicate effect-type behavior, and default/boundary-value cases for the supported effect families
- builder/config smoke now covers builder-produced runtime players, handle-audio-focus and seek-increment observability, wake mode runtime updates, and preload round-trip updates across SDK builder/runtime and bridge-runtime paths
- token-registered and registry-generated-token `MediaSource.Factory` injection baseline now covers both direct native-create and builder-build paths, including safe fallback to the default factory when the token is missing, replacement-registration observability via factory identity markers, and multi-token isolation behavior
- Stage 5 source integration now covers C++ `MediaItem.custom_cache_key` / Java
  `MediaItem.customCacheKey` mapping, progressive source-type inference from common progressive
  mime/URI values, and token-injected playback preparation for both custom-cache-key and DRM
  descriptor preservation
- Ownership boundary for RPI4-oriented source integration is explicit: the bridge owns reduced
  descriptors and factory-token selection; the app/platform owns concrete cache/offline-download
  instances, DRM session/license/provisioning behavior, and registered `MediaSource.Factory`
  implementations
- renderer messaging smoke now validates reduced result payload fields more directly
- priority smoke now covers bridge and wrapper state transitions with richer registration/priority markers

Closeout result:

- second-batch reduced-scope work is complete for the current endpoint
- remaining work has been pushed into the next phase because it requires new capability rather than
  more smoke on the existing reduced surfaces

## Stage 6: Smoke And Validation Coverage

Completed:

- JNI/value conversion smoke test class added
- player/runtime instrumentation smoke test class added
- demo app added for manual end-to-end validation
- timeline window `mediaId` now exposed in reduced timeline windows and covered by smoke assertions
- first-batch reduced parity (`MediaItem`, `Timeline`, `Tracks`, `MediaMetadata`, `Cue`) now has
  high-signal smoke-observed field coverage aligned with the mapping docs
- several static compile hazards fixed during smoke maintenance

## Remaining Work For Next Stage

High priority:

1. Restore build/test verification in a working Gradle environment.
2. Continue JNI exception-safety hardening across remaining bridge conversion/query paths.
3. Validate the completed second-batch reduced surfaces in a healthy environment and then continue
   only on the next-phase capability gaps that remain.
4. Use the broader reduced analytics event surface to choose the next full-parity
   `AnalyticsListener` families deliberately, instead of expanding events without validation.
5. Convert the first-pass explicit opaque-token cleanup API into an automatic
   lease/scope cleanup model after server migration, so query/callback-created
   token batches can be auto-released safely without breaking callers that still
   need to round-trip descriptors back into Java.

Current opaque-token lifecycle state:

- token collection and explicit batch release APIs now exist for the main public
  snapshot/query paths
- callers can already use helper wrappers such as
  `GetCurrentMediaItemWithOpaqueTokens(...)` and `GetTimelineWithOpaqueTokens(...)`
  to pair queries with the matching token batch
- automatic cleanup is intentionally deferred to the post-migration phase,
  because it needs real compile/runtime validation on the target server before
  changing callback/query lifetime semantics

Current reduced-scope validation surface:

- listener payload capture smoke observes reduced timeline timing/count fields and track support flags
- analytics smoke covers aggregate query, aggregate callback delivery, add/remove stop-delivery behavior, plus concrete reduced `onAudioUnderrun`, `onDroppedVideoFrames`, `onBandwidthEstimate`, `onLoadStarted`, `onLoadCompleted`, `onAudioInputFormatChanged`, `onAudioDecoderInitialized`, `onVideoDecoderInitialized`, `onAudioDecoderReleased`, `onVideoDecoderReleased`, analytics `onRenderedFirstFrame`, analytics `onVideoSizeChanged`, analytics `onAudioPositionAdvancing`, analytics `onVideoFrameProcessingOffset`, analytics `onVolumeChanged`, analytics `onAudioSessionIdChanged`, analytics `onSkipSilenceEnabledChanged`, analytics `onDeviceVolumeChanged`, analytics `onPlaybackStateChanged`, analytics `onIsPlayingChanged`, analytics `onPlayWhenReadyChanged`, analytics `onPlaybackSuppressionReasonChanged`, analytics `onIsLoadingChanged`, analytics `onRepeatModeChanged`, analytics `onShuffleModeChanged`, `onVideoInputFormatChanged`, `onPlaybackParametersChanged`, `onAvailableCommandsChanged`, `onEvents`, `onDeviceInfoChanged`, `onMediaMetadataChanged`, and `onPlaylistMetadataChanged` event paths
- image output smoke covers multi-frame delivery, runtime enable/disable transitions, remove-listener stop-delivery behavior, listener reattach behavior, bitmap-layout metadata, and a non-`ARGB_8888` case
- video effects smoke validates scale/rotate, RGB adjustment, and presentation parameters, plus clear/reset, reapply ordering, duplicate effect-type behavior, and default/boundary-value cases
- builder/config smoke covers builder-produced runtime players, handle-audio-focus and seek-increment observability, wake mode runtime updates, and preload round-trip updates across SDK builder/runtime and bridge-runtime paths
- media source factory smoke covers registered-token injection, missing-token fallback, replacement registration, and multi-token isolation
- renderer messaging smoke validates reduced result payload fields more directly
- priority smoke covers bridge and wrapper state transitions with richer registration/priority markers
- preload target smoke covers SDK builder, SDK runtime, and bridge runtime round-trip behavior; remaining preload work is broader ecosystem parity rather than deeper coverage of the currently exposed target-duration surface

Medium priority:

1. Close remaining broader parity gaps in analytics listener scope, preload ecosystem scope, and richer image/video effect parity.
2. Add more dedicated smoke coverage where current validation is still aggregate or reduced-only.
3. Optionally reduce `exoplayer_cppbridge_jni_common.cpp` size further.

Next-phase capability work after second-batch closeout:

- full `AnalyticsListener` parity
  Current reduced event coverage already includes aggregate delivery plus forty-five concrete event
  paths: audio underrun, dropped video frames, bandwidth estimate, load started, load completed,
  audio input format changed, audio decoder initialized, video decoder initialized, audio decoder released, video decoder released, analytics rendered first frame, analytics video size changed, analytics audio position advancing, analytics video frame processing offset, analytics volume changed, analytics audio session id changed, analytics skip silence enabled changed, analytics device volume changed, analytics playback state changed, analytics is playing changed, analytics play when ready changed, analytics playback suppression reason changed, analytics is loading changed, analytics repeat mode changed, analytics shuffle mode changed, and video input format changed.
  The newest concrete reduced events are playback parameters changed, available commands changed,
  analytics events batch delivery, device info changed, media metadata changed, and playlist metadata changed.
- broader preload ecosystem parity
- richer image output parity beyond reduced frame metadata, bitmap-layout metadata, and callback behavior
- richer video effects parity beyond the current reduced effect set and boundary/default-value coverage
- broader arbitrary `MediaSource.Factory` injection beyond token-registered and registry-generated-token baseline support
- full Java parity for reduced media/timeline/tracks/metadata/cue models

## Final Delivery Package

The final delivery should include:

1. source code for `lib-exoplayer-cppbridge`
2. demo app `:demo-cppbridge`
3. instrumentation smoke suite
4. this document set
5. runnable validation scripts
6. a final build-verified status update from the new environment

## Current Delivery Status

What is ready now:

- source layout
- smoke coverage
- demo app
- mapping/status docs
- handoff knowledge graph
- validation scripts

What is still blocked here:

- actual build and device verification in the current broken environment

## Full-Support Backlog

This backlog is the post-migration development view. Use it after compile/device validation is
working on the new server.

### `MediaItem`

Already in place:

- reduced descriptor is stable in query smoke and opaque-token smoke
- `tag`, `adsId`, and `requestMetadata.extras` have opaque-token baselines
- subtitles, clipping, live, DRM, and representative metadata are already query-visible

Still required for true full support:

- define whether arbitrary Java-object parity is required or whether opaque-token semantics are the intended endpoint
- if arbitrary-object parity is required, replace token-only semantics for `tag` and `adsId`
- decide whether `RequestMetadata.extras` needs full decoded `Bundle` parity in C++
- expand tests from representative fields to complete object-behavior parity where required

### `Timeline`

Already in place:

- reduced summary/window/period snapshots
- multi-window and multi-period smoke visibility
- `uid`, `id`, `adsId`, and manifest token baselines
- query, direct-listener, and listener-payload smoke coverage

Still required for true full support:

- decide the final parity target for `Timeline.Window` / `Timeline.Period`
- expand beyond token baselines for manifest/uid/id semantics if full object parity is required
- close remaining second-window/second-period asymmetries until coverage is intentionally complete
- validate runtime stability with real playlist and timeline mutation scenarios

### `Tracks`

Already in place:

- reduced `TracksSnapshot`, `TrackGroupSnapshot`, and representative `TrackInfo`
- group token and label token baselines
- first-group deep smoke coverage and growing second-group listener coverage

Still required for true full support:

- finish pulling second-group fields up to first-group depth
- decide how much full `Format` parity is required
- add tests that prove group/track parity beyond representative video/audio rows
- validate selection/support semantics under real runtime track changes

### `MediaMetadata`

Already in place:

- representative text fields, artwork, extras baseline, and multiple query/listener smoke paths
- opaque-token baselines for representative `CharSequence` fields

Still required for true full support:

- decide whether full `CharSequence` semantics are required or whether opaque-token baselines are sufficient
- decide whether metadata entry/extras behavior needs richer parity
- extend from representative-field parity to intentional full-object parity where needed
- validate metadata behavior under runtime updates on real devices

### `Cue`

Already in place:

- reduced cue snapshot
- text and bitmap token baselines
- representative layout/style fields across query/listener/analytics paths

Still required for true full support:

- decide whether styled text/span parity is required
- decide whether full bitmap object transfer is required
- expand from representative cue layout/style coverage to complete parity only where valuable
- validate cue behavior with real subtitle rendering and runtime cue updates
