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
- demo UI target: single-screen player with no scroll/log panel; source, speed, playlist, file,
  audio, and subtitle choices live behind the bottom-right `Menu` button

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

For a one-command UT pass that runs the host-side checks, board-side connected instrumentation, and
log collection, use:

```bash
bash scripts/cppbridge/run_rpi4_ut_validation.sh --serial "$RPI4_SERIAL"
```

The script runs on the host machine connected to the board. The connected instrumentation tests
execute on the RPI4 through adb.

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

## 4A. One-Command UT Validation Script

Use this when the goal is to answer "do all cppbridge UT and board-side instrumentation tests pass
on the RPI4?":

```bash
cd /home/linhao/Toolchain/development/ExoPlayer

export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export ANDROID_SDK_ROOT=$HOME/Android/Sdk
export PATH=$JAVA_HOME/bin:$ANDROID_SDK_ROOT/platform-tools:$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:$ANDROID_SDK_ROOT/emulator:$PATH

adb devices
bash scripts/cppbridge/run_rpi4_ut_validation.sh --serial <rpi4-serial>
```

What it runs:

- `bash scripts/cppbridge/run_validation.sh --local-only`
- `./gradlew :lib-exoplayer-cppbridge:assembleDebugAndroidTest --console=plain`
- `adb install -r -t` for the cppbridge androidTest APK
- `adb shell am instrument` for `CppBridgeNativeSmokeTest` and
  `CppBridgeNativePlayerInstrumentationTest`
- board facts capture into `board_facts.txt`
- full and high-signal logcat capture after the connected run

The RPI4 script uses direct adb instrumentation by default so the board-side phase does not depend
on Gradle's Unified Test Platform host plugins. This avoids failures where Gradle tries to download
`com.android.tools.utp:*` before it can talk to the board. If you need to compare with the standard
Gradle connected task, pass `--gradle-connected`.

The script does not fail fast between phases. If the host-side local JVM checks fail because
Robolectric cannot download its runtime artifacts, it still runs the board-side connected
instrumentation phase and writes both phase statuses to `summary.txt`. The final exit code remains
non-zero if either phase fails.

Useful options:

```bash
# Skip host-side local checks and run only board-side instrumentation.
bash scripts/cppbridge/run_rpi4_ut_validation.sh --serial <rpi4-serial> --connected-only

# Run one board-side instrumentation class.
bash scripts/cppbridge/run_rpi4_ut_validation.sh --serial <rpi4-serial> --connected-only \
  --test-class androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest

# Run only local checks without touching the board.
bash scripts/cppbridge/run_rpi4_ut_validation.sh --local-only

# Pick the output folder explicitly.
bash scripts/cppbridge/run_rpi4_ut_validation.sh --serial <rpi4-serial> --out "$PWD/buildout/my-rpi4-run"

# Launch the demo after UT passes.
bash scripts/cppbridge/run_rpi4_ut_validation.sh --serial <rpi4-serial> --launch-demo

# Use Gradle connectedDebugAndroidTest instead of direct adb instrumentation.
bash scripts/cppbridge/run_rpi4_ut_validation.sh --serial <rpi4-serial> --gradle-connected
```

Default output path:

```text
buildout/cppbridge-rpi4-ut-YYYYMMDD-HHMMSS/
```

Important files in the output folder:

- `summary.txt`
- `local_validation.txt`
- `connected_validation.txt`
- `instrument-CppBridgeNativeSmokeTest.txt`
- `instrument-CppBridgeNativePlayerInstrumentationTest.txt`
- `board_facts.txt`
- `logcat-full.txt`
- `logcat-high-signal.txt`
- `tombstones-list.txt`

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

1. Confirm the app opens as a single-screen player. There should be no scroll area and no visible
   debug log/status panel.
2. Confirm the bottom overlay is compact: `Play/Pause`, progress, time, and `Menu`.
3. Use `Menu` -> `HTTP`; playback should load and start immediately.
4. Press `Pause`, then `Play`; confirm playback pauses and resumes.
5. Scrub the progress bar, then use `Menu` -> `Back 10s` and `Forward 10s`; confirm seeking works.
6. Use `Menu` -> `0.5x`, `1.0x`, `1.5x`, and `2.0x`; confirm speed changes take effect.
7. Use `Menu` -> `HLS`; playback should load and start immediately.
8. Use `Menu` -> `DASH`; playback should load and start immediately.
9. Use `Menu` -> `Playlist`; confirm playback remains usable.
10. If track selection is required, use `Menu` -> `Audio +`, `Text +`, and `Text EN`.
11. Use `Menu` -> `File` for a local/USB content URI progressive playback check when needed.

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
| HTTP progressive | `Menu` -> `HTTP` | starts video/audio; controls respond |  |  |
| HLS | `Menu` -> `HLS` | manifest loads; playback starts |  |  |
| DASH | `Menu` -> `DASH` | manifest loads; playback starts |  |  |
| Playlist | `Menu` -> `Playlist` | playback remains usable |  |  |
| Track controls | `Menu` -> `Audio +`, `Text +`, `Text EN` | track selection controls respond without crash |  |  |
| USB/local file | `Menu` -> `File` | content URI loads as progressive |  |  |

## 9. Pass Criteria

RPI4 validation is considered pass when:

- Full connected validation passes on the board. Use the current emulator baseline as reference:
  `26/26`, `112/112`, `138/138`.
- Demo installs and launches.
- HTTP progressive, HLS, and DASH playback start through the demo without native crash or JNI error.
- Basic controls respond: play/pause, progress-bar scrub, menu seek shortcuts, menu source selection,
  speed selection, playlist, track-selection actions, and stop.
- The demo remains a single-screen player with no scrolling and no visible debug log/status panel.
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
