# C++ Bridge RPI4 Testing Guide

Last updated: 2026-05-19

This guide is the manual Raspberry Pi 4 validation pass for `exoplayer_cppbridge`. The board is not
currently connected in the Codex environment, so the latest automated gate is Android 16 emulator
validation. Run this guide on the host machine when the RPI4 is reachable.

Use absolute dates in reports. The current emulator baseline was refreshed on 2026-05-19.

## 1. Scope

Validate that the `exoplayer_cppbridge` Android app and native bridge work on the RPI4 runtime:

1. The native library loads on the board ABI.
2. The connected JNI/player smoke suites pass.
3. The demo launches and can exercise HTTP progressive, HLS, and DASH playback through the C++ API.
4. Board-specific decoder, audio, network, and rendering issues are captured with useful evidence.

The expected Android 16 emulator baseline before RPI4 validation is:

- `CppBridgeNativeSmokeTest`: `26/26` passed
- `CppBridgeNativePlayerInstrumentationTest`: `112/112` passed
- connected total: `138/138` passed
- demo UI smoke: Play/Pause/Stop plus Playback/Tracks/Item/Timeline/Metadata/Cues status queries
  updated native `status_text`

## 1A. Quick Start

From the repo root:

```bash
cd /home/linhao/Toolchain/development/ExoPlayer

export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export ANDROID_SDK_ROOT=$HOME/Android/Sdk
export PATH=$JAVA_HOME/bin:$ANDROID_SDK_ROOT/platform-tools:$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:$ANDROID_SDK_ROOT/emulator:$PATH

adb devices
export RPI4_SERIAL=<replace-with-adb-serial>
export RPI4_OUT=$PWD/buildout/cppbridge-rpi4-validation-$(date +%Y%m%d-%H%M%S)
mkdir -p "$RPI4_OUT"

adb -s "$RPI4_SERIAL" logcat -c
bash scripts/cppbridge/run_validation.sh --serial "$RPI4_SERIAL" | tee "$RPI4_OUT/run_validation.txt"

./gradlew :demo-cppbridge:installDebug --console=plain | tee "$RPI4_OUT/demo_install.txt"
adb -s "$RPI4_SERIAL" shell am start -n androidx.media3.demo.cppbridge/.MainActivity \
  | tee "$RPI4_OUT/demo_launch.txt"
adb -s "$RPI4_SERIAL" shell pidof androidx.media3.demo.cppbridge \
  | tee "$RPI4_OUT/demo_pid.txt"

adb -s "$RPI4_SERIAL" logcat -d > "$RPI4_OUT/logcat-full.txt"
grep -iE "cppbridge|ExoPlayer|MediaCodec|AndroidRuntime|FATAL|tombstone|UnsatisfiedLinkError|JNI DETECTED ERROR|Fatal signal|ANR" \
  "$RPI4_OUT/logcat-full.txt" > "$RPI4_OUT/logcat-high-signal.txt" || true
```

If `run_validation.sh` passes and the demo can play HTTP/HLS/DASH manually, collect the files in
`$RPI4_OUT` with the board facts from section 3.

## 2. Host Prerequisites

Use the same host setup as Android 16 emulator validation:

```bash
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export ANDROID_SDK_ROOT=$HOME/Android/Sdk
export PATH=$JAVA_HOME/bin:$ANDROID_SDK_ROOT/platform-tools:$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:$ANDROID_SDK_ROOT/emulator:$PATH
```

Confirm the toolchain:

```bash
java -version
adb version
./gradlew -version
python3 --version
git status -sb
```

Expected host notes:

- JDK 17 is the tested Java runtime.
- Android SDK platform-tools must be on `PATH`; if `adb` is not found, re-export `PATH`.
- Do not run RPI4 validation from a half-built or unknown branch. Use the pushed branch that contains
  this guide and the Stage 6 C++ bridge changes.
- Keep the host awake during connected tests; instrumentation can fail if adb disconnects mid-run.

## 3. Board Prerequisites

Connect the board by USB adb or network adb. For network adb:

```bash
adb connect <rpi4-ip>:5555
adb devices
```

Pick the serial exactly as listed by `adb devices`:

```bash
export RPI4_SERIAL=<rpi4-serial-from-adb-devices>
```

Verify that the selected serial is online:

```bash
adb -s "$RPI4_SERIAL" get-state
```

Before running tests, collect the board facts:

```bash
adb devices
adb -s "$RPI4_SERIAL" shell getprop ro.build.version.release
adb -s "$RPI4_SERIAL" shell getprop ro.build.version.sdk
adb -s "$RPI4_SERIAL" shell getprop ro.product.cpu.abi
adb -s "$RPI4_SERIAL" shell getprop ro.product.cpu.abilist
adb -s "$RPI4_SERIAL" shell getprop ro.hardware
adb -s "$RPI4_SERIAL" shell getprop ro.board.platform
adb -s "$RPI4_SERIAL" shell getprop ro.build.fingerprint
adb -s "$RPI4_SERIAL" shell wm size
adb -s "$RPI4_SERIAL" shell wm density
```

Expected baseline:

- Android image close to the release target, preferably Android 16 / API 36 for parity with emulator
  validation.
- `arm64-v8a` or another ABI supported by the built APK.
- Stable USB or network adb connection.
- Network access from the board to any external media URLs used by manual demo testing.
- HDMI/display path available if video rendering is part of the manual check.
- Audio output path available if audio playback is part of the manual check.

Create an output folder for the run:

```bash
export RPI4_OUT=$PWD/buildout/cppbridge-rpi4-validation-$(date +%Y%m%d-%H%M%S)
mkdir -p "$RPI4_OUT"

{
  echo "date=$(date -Iseconds)"
  echo "serial=$RPI4_SERIAL"
  adb -s "$RPI4_SERIAL" shell getprop ro.build.version.release
  adb -s "$RPI4_SERIAL" shell getprop ro.build.version.sdk
  adb -s "$RPI4_SERIAL" shell getprop ro.product.cpu.abi
  adb -s "$RPI4_SERIAL" shell getprop ro.product.cpu.abilist
  adb -s "$RPI4_SERIAL" shell getprop ro.hardware
  adb -s "$RPI4_SERIAL" shell getprop ro.board.platform
  adb -s "$RPI4_SERIAL" shell getprop ro.build.fingerprint
} | tee "$RPI4_OUT/board_facts.txt"
```

## 4. Preflight Build

Run the local non-device checks first:

```bash
python3 -m unittest discover -s scripts/cppbridge -p '*_test.py'
python3 scripts/cppbridge/api_parity_inventory.py --check
./gradlew :lib-exoplayer-cppbridge:testDebugUnitTest --console=plain
./gradlew :lib-exoplayer-cppbridge:assembleDebugAndroidTest --console=plain
./gradlew :demo-cppbridge:assembleDebug --console=plain
./gradlew :lib-exoplayer-cppbridge:assemble -PcppbridgeIncludeTestEntrypoints=OFF --console=plain
```

If these fail, fix the host/build issue before using the RPI4.

Optional one-command local preflight:

```bash
bash scripts/cppbridge/run_validation.sh --local-only | tee "$RPI4_OUT/local_only.txt"
```

This does not touch the board and does not replace connected validation.

## 5. Connected Validation

Run the same full validation flow used for the Android 16 emulator, replacing the serial:

```bash
adb -s "$RPI4_SERIAL" logcat -c
bash scripts/cppbridge/run_validation.sh --serial "$RPI4_SERIAL" | tee "$RPI4_OUT/run_validation.txt"
```

`run_validation.sh` now checks that the exact `--serial` is online before running Gradle. If it says
the serial is missing, run `adb devices`, fix the connection, and retry.

Useful targeted fallback commands:

```bash
./gradlew :lib-exoplayer-cppbridge:connectedDebugAndroidTest \
  -Pandroid.testInstrumentationRunnerArguments.class=androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest \
  --console=plain | tee "$RPI4_OUT/native_smoke.txt"

./gradlew :lib-exoplayer-cppbridge:connectedDebugAndroidTest \
  -Pandroid.testInstrumentationRunnerArguments.class=androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest \
  --console=plain | tee "$RPI4_OUT/player_smoke.txt"
```

Expected result:

- `CppBridgeNativeSmokeTest`: all tests pass. The current emulator baseline is `26/26`.
- `CppBridgeNativePlayerInstrumentationTest`: all tests pass. The current emulator baseline is
  `112/112`.
- No `UnsatisfiedLinkError`, JNI exception, native crash, or instrumentation timeout.
- HTTP/HLS/DASH instrumentation smoke passes through
  `nativeHttpHlsDashPlaybackSmokeTest_preparesLocalStreamsThroughCppApi`.
- Stage 5 source integration smokes pass:
  `nativeHttpDataSourceConfigPlaybackSmokeTest_sendsHeadersThroughCppConfig` and
  `nativeCustomMediaSourceFactoryPlaybackSmokeTest_preparesSmoothAndRtspViaCppConfig`.

If connected validation fails, do not continue to release sign-off. Capture `$RPI4_OUT` and follow
section 11.

## 6. Demo Launch

Install and launch the demo:

```bash
./gradlew :demo-cppbridge:installDebug --console=plain | tee "$RPI4_OUT/demo_install.txt"
adb -s "$RPI4_SERIAL" shell am start -n androidx.media3.demo.cppbridge/.MainActivity \
  | tee "$RPI4_OUT/demo_launch.txt"
adb -s "$RPI4_SERIAL" shell pidof androidx.media3.demo.cppbridge \
  | tee "$RPI4_OUT/demo_pid.txt"
```

Manual playback focus:

1. Press `HTTP`, then `Play`.
2. Confirm video renders and audio behaves as expected for the board.
3. Press `Pause`, `Play`, `Seek Fwd`, `Seek Back`, `Seek To`, and `Stop`.
4. Press `Playback`, `Tracks`, `Item`, `Timeline`, `Metadata`, and `Cues`; confirm the status panel
   changes after each press.
5. Press `HLS`, then `Play`; confirm playback starts.
6. Press `DASH`, then `Play`; confirm playback starts.
7. Press `Mixed Playlist`, then use `Next` and `Prev`; confirm item transitions.
8. If subtitles are required, use `Load + Subtitle` and then `Text EN` / `Text +`.

Expected status markers:

- HTTP loaded through C++: status contains `Loaded media via C++ API`.
- Playback query: status contains `state=`, `playing=`, `itemIndex=`, `itemCount=`, and `sourceType=`.
- Tracks query: status contains track group details, or `No track groups` before media is prepared.
- Item query: status contains `mediaId=`, `uri=`, `sourceType=`, and `mimeType=`.
- Timeline query: status contains `windowCount=`, `periodCount=`, and `empty=`.
- Metadata query: status contains `title=`, `artist=`, and `extrasKeyCount=`.
- Cues query: status contains `cueCount=` and `presentationTimeUs=`.

The instrumentation suite already validates deterministic local HTTP/HLS/DASH, HTTP data-source
configuration, and custom source-factory SmoothStreaming / RTSP paths. The RPI4 manual demo check is
meant to catch board decoder, surface, audio, and network behavior that emulator tests cannot prove.

Optional launch without default remote media:

```bash
adb -s "$RPI4_SERIAL" shell am start -n androidx.media3.demo.cppbridge/.MainActivity \
  --ez skip_default_load true
```

Optional launch with a specific media URL:

```bash
adb -s "$RPI4_SERIAL" shell am start -n androidx.media3.demo.cppbridge/.MainActivity \
  --es media_url "https://storage.googleapis.com/exoplayer-test-media-0/BigBuckBunny_320x180.mp4" \
  --ei source_type 5 \
  --es mime_type "video/mp4" \
  --ez auto_play true
```

Source type values:

- `1`: DASH
- `2`: HLS
- `3`: SmoothStreaming
- `4`: RTSP
- `5`: HTTP / USB progressive

## 7. Log Capture

Clear logs before each connected run:

```bash
adb -s <rpi4-serial> logcat -c
```

After a test or demo run, capture high-signal logs:

```bash
adb -s "$RPI4_SERIAL" logcat -d > "$RPI4_OUT/logcat-full.txt"
grep -iE "cppbridge|ExoPlayer|MediaCodec|AndroidRuntime|FATAL|tombstone|UnsatisfiedLinkError|JNI DETECTED ERROR|Fatal signal|ANR" \
  "$RPI4_OUT/logcat-full.txt" > "$RPI4_OUT/logcat-high-signal.txt" || true
adb -s "$RPI4_SERIAL" shell ls -la /data/tombstones 2>/dev/null \
  | tee "$RPI4_OUT/tombstones-list.txt"
```

If playback fails, also collect:

```bash
adb -s "$RPI4_SERIAL" shell dumpsys media.codec > "$RPI4_OUT/media_codec.txt"
adb -s "$RPI4_SERIAL" shell dumpsys media.metrics > "$RPI4_OUT/media_metrics.txt" 2>/dev/null || true
adb -s "$RPI4_SERIAL" shell dumpsys SurfaceFlinger > "$RPI4_OUT/surfaceflinger.txt"
adb -s "$RPI4_SERIAL" shell dumpsys audio > "$RPI4_OUT/audio.txt"
```

Optional screenshots:

```bash
adb -s "$RPI4_SERIAL" exec-out screencap -p > "$RPI4_OUT/demo-screen.png"
```

## 8. RPI4-Specific Risk Checklist

Check these before treating a failure as a bridge regression:

- ABI mismatch or missing native library.
- Hardware codec limits for the test stream profile, level, bitrate, or container.
- HEVC, VP9, HDR, high-bitrate, or high-resolution streams exceeding board capability.
- Network reachability, DNS, proxy, or certificate differences from the host/emulator.
- HDMI/display mode or surface composition issues.
- Audio route, device volume, mute state, or output-device availability.
- Thermal throttling or power supply instability during longer playback.

For codec-sensitive failures, retry with a low-bitrate H.264/AAC progressive stream. If that works
but the original HLS/DASH stream fails, capture the media URL, codec details, and logcat output.

## 8A. Manual Media Matrix

Fill this during board testing:

| Media type | URL / source | Expected | Result | Notes |
| --- | --- | --- | --- | --- |
| HTTP progressive | default HTTP button | starts video/audio; controls respond |  |  |
| HLS | default HLS button | manifest loads; playback starts |  |  |
| DASH | default DASH button | manifest loads; playback starts |  |  |
| Mixed playlist | `Mixed Playlist` button | `Next` / `Prev` switch items |  |  |
| External subtitle | `Load + Subtitle` | subtitle track visible or selectable |  |  |
| USB/local file | `Pick USB/File` | content URI loads as progressive |  |  |

## 9. Pass Criteria

RPI4 validation is considered pass when:

- Full connected validation passes on the board. Use the current emulator baseline as reference:
  `26/26`, `112/112`, `138/138`.
- Demo installs and launches.
- HTTP progressive, HLS, and DASH playback start through the demo without native crash or JNI error.
- Basic controls respond: play, pause, seek, stop/release.
- Query buttons respond: Playback, Tracks, Item, Timeline, Metadata, and Cues all update the status
  panel.
- Logcat has no bridge crash, `UnsatisfiedLinkError`, fatal JNI exception, or repeated decoder crash.

RPI4 validation is considered blocked when:

- adb cannot keep the board online.
- the board image cannot install the debug APK.
- the board ABI cannot load the native bridge library.
- network or display/audio setup prevents media playback from being evaluated.

## 10. Evidence To Report Back

Record this with any pass/fail report:

- Board image/build fingerprint.
- Android version and API level.
- ABI and hardware properties.
- ADB serial and connection type.
- Exact command output for `run_validation.sh --serial <rpi4-serial>`.
- Demo media URLs used for HTTP, HLS, and DASH.
- Logcat snippets for any failure.
- Whether the same media works on the Android 16 emulator.

Suggested final report format:

```text
Date:
Branch / commit:
Board:
Android release / API:
ABI:
ADB serial:
Connected validation result:
Demo HTTP result:
Demo HLS result:
Demo DASH result:
Controls/query result:
Logcat high-signal result:
Artifacts folder:
Open issues:
```

## 11. Failure Triage

### adb offline or unauthorized

```bash
adb kill-server
adb start-server
adb devices
adb -s "$RPI4_SERIAL" get-state
```

If network adb is used, reconnect:

```bash
adb disconnect <rpi4-ip>:5555
adb connect <rpi4-ip>:5555
adb devices
```

### install fails

Check storage and package state:

```bash
adb -s "$RPI4_SERIAL" shell df -h
adb -s "$RPI4_SERIAL" uninstall androidx.media3.demo.cppbridge || true
adb -s "$RPI4_SERIAL" uninstall androidx.media3.exoplayer.cppbridge.test || true
./gradlew :demo-cppbridge:installDebug --console=plain
```

### native library load fails

Look for `UnsatisfiedLinkError`:

```bash
grep -i "UnsatisfiedLinkError" "$RPI4_OUT/logcat-full.txt"
adb -s "$RPI4_SERIAL" shell getprop ro.product.cpu.abilist
```

The build currently packages native libraries for common Android ABIs. If the board image reports an
unexpected ABI, capture `ro.product.cpu.abi` and `ro.product.cpu.abilist`.

### HTTP/HLS/DASH playback fails

Separate network, codec, and bridge causes:

1. Confirm connected instrumentation passed. If not, start with the failing test.
2. Confirm the board can reach the URL:

```bash
adb -s "$RPI4_SERIAL" shell ping -c 3 storage.googleapis.com
adb -s "$RPI4_SERIAL" shell ping -c 3 devstreaming-cdn.apple.com
```

3. Retry a low-bitrate H.264/AAC progressive file.
4. Capture codec and log evidence:

```bash
adb -s "$RPI4_SERIAL" shell dumpsys media.codec > "$RPI4_OUT/media_codec_after_failure.txt"
adb -s "$RPI4_SERIAL" logcat -d > "$RPI4_OUT/logcat-after-failure.txt"
```

### video is black but audio plays

Collect display/surface state:

```bash
adb -s "$RPI4_SERIAL" shell dumpsys SurfaceFlinger > "$RPI4_OUT/surfaceflinger-black-video.txt"
adb -s "$RPI4_SERIAL" exec-out screencap -p > "$RPI4_OUT/black-video-screen.png"
```

Then retry after changing HDMI/display mode or using a lower-resolution stream.

### audio does not play

Collect audio state:

```bash
adb -s "$RPI4_SERIAL" shell dumpsys audio > "$RPI4_OUT/audio-after-failure.txt"
adb -s "$RPI4_SERIAL" shell settings get system volume_music
adb -s "$RPI4_SERIAL" shell media volume --stream 3 --get 2>/dev/null || true
```

Check board mute state, HDMI audio route, and connected speakers.

## 12. What To Send Back

Send the result summary plus the `$RPI4_OUT` folder contents. The most useful files are:

- `board_facts.txt`
- `run_validation.txt`
- `demo_install.txt`
- `demo_launch.txt`
- `demo_pid.txt`
- `logcat-high-signal.txt`
- `logcat-full.txt` for failures
- screenshots or codec dumps for playback/rendering issues
