# Native Listener Smoke Issue Summary

Date: 2026-04-01

## Problem Statement

The instrumentation test

`androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeListenerSmokeTest_reportsExtendedCallbacks`

fails consistently on the Linux runner/device setup even though the latest native test code is confirmed to be installed and executed on device.

The failure does not currently look like a simple stale APK problem or a straightforward bad test assertion. The strongest signal is that the native test listener state appears corrupted while the bridge release path still completes normally.

## Reproduction

Repo on Linux:

`~/Android/development/HBBTV/ExoPlayer_CPP_Adaption`

Command:

```bash
bash scripts/cppbridge/rebuild_android_test.sh \
  --test-class androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeListenerSmokeTest_reportsExtendedCallbacks
```

Manual log extraction used during diagnosis:

```bash
adb logcat -d -v time -s ExoCppBridge cppbridge AndroidRuntime TestRunner | \
grep -E "buildMarker|listenerCallback|nativeListenerSmokeTest|listenerSmoke populate|wait |mutexBusy|activeCallback|listenerRemoved|Release|release "
```

Note:

- Device logcat timestamps looked like `01-08 ...` during the April 1, 2026 runs.
- The build marker is the reliable way to distinguish which diagnostic version was actually running.

## Latest Confirmed On-Device Run

Latest confirmed marker actually executed on device:

`listener-smoke-v2026-04-01-3`

Important:

- `v1`, `v2`, and `v3` were all confirmed on device in sequence.
- This rules out "new test code was not packaged/installed" as the current root cause.
- A `v4` diagnostic patch is prepared locally and packaged, but was not yet executed on device at the time of this summary.

## Expected Behavior

`nativeListenerSmokeTest(...)` should return a summary string containing values such as:

- `repeatMode=2`
- `shuffle=1`
- `text=en`
- `speed=1.100000`
- `pitch=0.900000`
- callback counters like `repeatCb=1`, `shuffleCb=1`, `trackCb=1`, etc.

The Java instrumentation test asserts those expected fields in:

`libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerInstrumentationTest.java`

## Actual Behavior

The test fails with a summary like:

```text
waitReady=0,summaryLockTimeout=1,activeCallback=clns-11,repeatCb=0,
shuffleCb=1114303568,seekBackCb=-1275068301,seekForwardCb=0,maxSeekPrevCb=0,
trackCb=1114303376,playbackParamsCb=-1275068301,suppressionCb=1919523760,
commandsCb=-1275068302,eventsCb=608064,timelineCb=113,tracksCb=-48913912,
mediaMetadataCb=113,playlistMetadataCb=351203400,cueCb=442237356,
positionCb=-48919136,timelineWindowCount=113,timelinePeriodCount=-930023756,
cueCount=116,textLangSeen=0,mediaTitleSeen=0,playlistTitleSeen=0,newMediaIdSeen=1
```

This contains multiple impossible values for callback counts and state fields.

## Strongest Evidence Collected So Far

### 1. New native test code is definitely running

The log showed:

```text
nativeListenerSmokeTest buildMarker=listener-smoke-v2026-04-01-3
```

and the Linux-side source fingerprint matched the updated C++ test source.

### 2. Scenario population completes

The following steps all log begin/end successfully:

- `SetListener`
- `SetMediaItems`
- `SetRepeatMode`
- `SetShuffleModeEnabled`
- `SimulateSeekBackIncrementChangedForTest`
- `SimulateSeekForwardIncrementChangedForTest`
- `SimulateMaxSeekToPreviousPositionChangedForTest`
- `SetPlaybackParameters`
- `SetTrackSelectionParameters`
- `SetPlaylistMetadata`
- `SimulateCurrentCuesForTest`
- `SeekToMediaItem`

This indicates the test setup itself is not aborting early.

### 3. Failure happens while waiting for listener state to settle

The wait loop repeatedly logs:

```text
mutexBusy=1,activeCallback=clns-11
```

and ends with:

```text
nativeListenerSmokeTest wait timeout ...
nativeListenerSmokeTest summary lock timeout timeoutMs=1000,activeCallback=clns-11
```

That means the diagnostic listener mutex is still considered busy when the summary is being built.

### 4. Release path still completes

Even after the failure, release logs are normal:

- `ExoPlayerSdkPlayerImpl::Release start`
- `ExoPlayerBridge::Release start`
- Java bridge release begins/ends
- listener removal completes

This argues against the failure being a simple "player release deadlock".

### 5. `clns-11` is not a known symbolic name in the repo

Searches for `clns-11` and `clns-` in the workspace did not find any code path intentionally generating this identifier.

Current interpretation:

- `clns-11` is unlikely to be a meaningful designed callback name.
- It is more likely a corrupted or otherwise invalid callback-name/state artifact.

## End-to-End Callback Path Reviewed

### Java layer

File:

`libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java`

Relevant facts:

- `CppExoPlayerBridge` implements both `Player.Listener` and `AnalyticsListener`.
- Player callbacks such as:
  - `onRepeatModeChanged`
  - `onShuffleModeEnabledChanged`
  - `onTimelineChanged`
  - `onTracksChanged`
  - `onPositionDiscontinuity`
  - `onEvents`
  - `onMediaMetadataChanged`
  - `onPlaylistMetadataChanged`
  - `onCues`
  - `onPlayerErrorChanged`
  all call matching `nativeOn...` methods.
- Test helper operations are implemented and not stubs:
  - `setRepeatMode`
  - `setShuffleModeEnabled`
  - `setPlaybackParametersConfig`
  - `setTrackSelectionParameters`
  - `setPlaylistMetadata`
  - `seekToMediaItem`
  - `simulateSeekBackIncrementChangedForTest`
  - `simulateSeekForwardIncrementChangedForTest`
  - `simulateMaxSeekToPreviousPositionChangedForTest`
  - `simulateCurrentCuesForTest`

Conclusion:

- The Java-side APIs exercised by this smoke are implemented.
- This does not look like the test is calling missing Java bridge methods.

### Native bridge layer

File:

`libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_bridge.cpp`

Relevant facts:

- `SetListener` / `RemoveListener` store a raw `PlayerListener*`.
- `WithListenerEnv(...)` fetches that listener and dispatches to it.
- Native bridge callback handlers exist for the expected smoke events:
  - `OnRepeatModeChanged`
  - `OnShuffleModeEnabledChanged`
  - `OnSeekBackIncrementChanged`
  - `OnSeekForwardIncrementChanged`
  - `OnMaxSeekToPreviousPositionChanged`
  - `OnTrackSelectionParametersChanged`
  - `OnPlaybackParametersChanged`
  - `OnAvailableCommandsChanged`
  - `OnEvents`
  - `OnTimelineChanged`
  - `OnTracksChanged`
  - `OnPositionDiscontinuity`
  - `OnMediaMetadataChanged`
  - `OnPlaylistMetadataChanged`
  - `OnCues`
  - `OnPlayerErrorChanged`

Conclusion:

- The JNI/native bridge does appear to have concrete implementations for the callback surface used by this smoke.
- The issue does not currently look like "callback not implemented at all".

### SDK forwarding layer

File:

`libraries/exoplayer_cppbridge/src/main/jni/exoplayer_sdk.cpp`

Relevant facts:

- `ExoPlayerSdkPlayerImpl` installs a `ForwardingPlayerListener`.
- `SetListener` sets the primary delegate.
- `AddAnalyticsListener` stores additional delegates.
- Normal player callbacks use `NotifyDelegate(...)`.
- Analytics-style callbacks use `SnapshotListeners()` and iterate primary + analytics delegates.

Conclusion:

- The forwarding layer is a plausible bug location because it mixes:
  - one primary delegate,
  - many analytics delegates,
  - raw pointers,
  - copied listener snapshots.

## Native Test Listener Under Investigation

File:

`libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_player_tests.cpp`

Important facts:

- `CapturingPlayerListener` is the test listener used by the failing smoke.
- It tracks both:
  - detailed callback payload fields, and
  - separate "smoke" atomics used by the wait loop.
- A recursive mutex was added for callback capture synchronization.
- `active_callback` was added for diagnostics.
- Additional wait diagnostics were added:
  - `wait pending`
  - `wait timeout`
  - `summary lock timeout`

Observed inconsistency:

- During the failing `v3` run, `activeCallback=clns-11` was reported
- but the expected `listenerCallback enter/exit ...` logs never appeared
- while callback counters in the same listener object were already corrupted

This combination is suspicious and suggests one of:

- listener object memory corruption,
- invalid callback-name storage,
- unexpected reentrancy/lifetime issue around the listener pointer,
- or a bug in test listener synchronization/capture logic itself

## Is This Just a Bad Test Case?

Current view: **not likely to be only a bad test case**, but the test design is somewhat overloaded.

Why it is probably not only a bad case:

- The case is exercising implemented APIs, not obvious stubs.
- The setup sequence executes normally.
- The returned summary contains clearly impossible values, not merely "callback did not fire".
- The `activeCallback=clns-11` artifact does not map to any intentional repo symbol.

Why the case is still not ideal:

- It mixes many expectations into one smoke.
- It uses a large summary string with `contains(...)` assertions instead of structured assertions.
- It combines real player callbacks with explicit test-only simulated callback injections.

So:

- the test may be too broad and brittle,
- but current evidence still points more strongly to a real implementation or lifecycle bug than to a purely invalid expectation.

## Most Likely Root Cause Areas

Ranked from most suspicious to least suspicious:

1. Listener lifetime / raw pointer handling across bridge and forwarding layers
2. Reentrancy or lock interaction inside `CapturingPlayerListener`
3. State corruption inside the native test listener object
4. Analytics/non-analytics forwarding interaction affecting the primary listener unexpectedly
5. A hidden bridge callback path updating the same listener through an unexpected route

## Things Already Ruled Out or De-Prioritized

- stale APK / stale native test source
- player release path hang
- missing Java-side implementation for the exercised smoke APIs
- missing JNI callback implementation for the main smoke callback surface

## Latest Diagnostic Change Prepared But Not Yet Run

A `v4` test patch has been prepared locally:

- build marker updated to `listener-smoke-v2026-04-01-4`
- all remaining generic callback lock macros in `CapturingPlayerListener` were converted to explicit named variants

Goal of `v4`:

- if the failure reproduces again, `activeCallback` should become a real callback name instead of an opaque value like `clns-11`

This should narrow the fault to a specific callback function if the listener object is still coherent enough to report its state.

## Suggested Expert Focus

If another engineer picks this up, the most useful next checks are:

1. Inspect raw listener pointer ownership and lifetime through:
   - `CppExoPlayerBridge.java`
   - `exoplayer_cppbridge_jni_bridge.cpp`
   - `exoplayer_sdk.cpp`
2. Check whether the primary listener can be invoked concurrently or reentrantly from:
   - normal player callbacks
   - analytics callbacks
   - test simulation callbacks
3. Verify whether `CapturingPlayerListener` memory can be overwritten indirectly by:
   - callback payload construction,
   - JNI object conversion,
   - stale raw listener pointers,
   - copied snapshots outliving objects they reference
4. Run the prepared `v4` diagnostic build and inspect the resulting `activeCallback`

## Key Files

- `libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerInstrumentationTest.java`
- `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerTestHelper.java`
- `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_player_tests.cpp`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_bridge.cpp`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_sdk.cpp`
- `scripts/cppbridge/rebuild_android_test.sh`

## One-Line Summary

This issue currently looks more like a real native bridge/listener state corruption or lifecycle bug than a simple bad smoke assertion, although the smoke test itself is broad and could be refactored once the underlying callback corruption is understood.
