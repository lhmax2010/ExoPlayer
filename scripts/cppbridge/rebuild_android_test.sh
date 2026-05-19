#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

GRADLE_EXECUTABLE="./gradlew"
SERIAL="${ANDROID_SERIAL:-}"
TEST_CLASS=""
TIMEOUT_SECONDS=6

TEST_APP_ID="androidx.media3.exoplayer.cppbridge.test"
TEST_RUNNER="androidx.media3.exoplayer.cppbridge.test/androidx.test.runner.AndroidJUnitRunner"
FINGERPRINT_SOURCES=(
  "libraries/exoplayer_cppbridge/src/main/jni/CMakeLists.txt"
  "libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_callbacks_demo.cpp"
  "libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_bridge.cpp"
  "libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_player_tests.cpp"
  "libraries/exoplayer_cppbridge/src/main/jni/exoplayer_sdk.cpp"
  "libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerTestHelper.java"
  "libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativeSmokeTestHelper.java"
)
LOG_FILTER='nativeListenerSmokeTest|listenerSmoke populate|listenerSmoke state|buildMarker|coreBuildMarker|bridgeBuildMarker|selfDispatch|postSelfDispatchReset|listenerCallback (dispatch|enter|exit)|listenerLayout|canary|wait |mutexBusy|activeCallback|listenerRemoved|ForwardingPlayerListener|JniExoPlayerBridge|Release|release '

usage() {
  cat <<'EOF'
Usage:
  bash scripts/cppbridge/rebuild_android_test.sh [options]

What it does:
  1. Uninstalls the existing cppbridge androidTest APK
  2. Cleans native/androidTest build outputs
  3. Rebuilds :lib-exoplayer-cppbridge:assembleDebugAndroidTest --rerun-tasks
  4. Installs :lib-exoplayer-cppbridge:installDebugAndroidTest
  5. Optionally runs one instrumentation test and prints filtered logs

Options:
  --gradle <path>         Gradle executable, default: ./gradlew
  --serial <serial>       adb device serial; also exported as ANDROID_SERIAL
  --test-class <class>    Optional instrumentation class or class#method to run after install
  --timeout-seconds <n>   Timeout for the optional test run, default: 6
  -h, --help              Show this help

Examples:
  bash scripts/cppbridge/rebuild_android_test.sh

  bash scripts/cppbridge/rebuild_android_test.sh \
    --test-class androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeListenerSmokeTest_reportsExtendedCallbacks
EOF
}

run_checked() {
  echo ">> $*"
  "$@"
}

print_source_fingerprint() {
  echo "==> Current listener smoke source fingerprint"
  local hasher=""
  if command -v sha256sum >/dev/null 2>&1; then
    hasher="sha256sum"
  elif command -v shasum >/dev/null 2>&1; then
    hasher="shasum -a 256"
  fi

  for source_path in "${FINGERPRINT_SOURCES[@]}"; do
    if [[ ! -f "${source_path}" ]]; then
      echo "missing: ${source_path}"
      continue
    fi
    if [[ -n "${hasher}" ]]; then
      eval "${hasher} \"${source_path}\""
    else
      echo "sha256sum/shasum not found; skipping hash for ${source_path}."
    fi
    grep -nE 'listener-smoke-(v|core-v|bridge-v)|EXOPLAYER_CPPBRIDGE_INCLUDE_TEST_ENTRYPOINTS|callbacks_demo.cpp|System.loadLibrary|nativeOnAnalyticsEvents' \
      "${source_path}" || true
  done
}

adb_args=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --gradle)
      GRADLE_EXECUTABLE="$2"
      shift 2
      ;;
    --serial)
      SERIAL="$2"
      shift 2
      ;;
    --test-class)
      TEST_CLASS="$2"
      shift 2
      ;;
    --timeout-seconds)
      TIMEOUT_SECONDS="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ -n "${SERIAL}" ]]; then
  export ANDROID_SERIAL="${SERIAL}"
  adb_args=(-s "${SERIAL}")
fi

run_checked adb start-server

devices_output="$(adb "${adb_args[@]}" devices)"
if ! grep -qE '^[^[:space:]]+[[:space:]]+device$' <<< "${devices_output}"; then
  echo "No online adb device was found. Connect a device or start an emulator first." >&2
  exit 1
fi

echo "==> Uninstalling old androidTest APK"
adb "${adb_args[@]}" uninstall "${TEST_APP_ID}" >/dev/null 2>&1 || true

echo "==> Cleaning cppbridge androidTest/native outputs"
rm -rf libraries/exoplayer_cppbridge/.cxx
rm -rf libraries/exoplayer_cppbridge/buildout/intermediates/cxx
rm -rf libraries/exoplayer_cppbridge/buildout/intermediates/stripped_native_libs
rm -rf libraries/exoplayer_cppbridge/buildout/intermediates/merged_native_libs
rm -rf libraries/exoplayer_cppbridge/buildout/outputs/apk/androidTest

print_source_fingerprint

echo "==> Rebuilding androidTest APK"
run_checked "${GRADLE_EXECUTABLE}" :lib-exoplayer-cppbridge:assembleDebugAndroidTest --rerun-tasks

echo "==> Installing androidTest APK"
run_checked "${GRADLE_EXECUTABLE}" :lib-exoplayer-cppbridge:installDebugAndroidTest

if [[ -n "${TEST_CLASS}" ]]; then
  echo "==> Clearing logcat"
  run_checked adb "${adb_args[@]}" logcat -c

  echo "==> Running instrumentation test"
  if command -v timeout >/dev/null 2>&1; then
    echo ">> timeout ${TIMEOUT_SECONDS}s adb ${adb_args[*]} shell am instrument -w -e class ${TEST_CLASS} ${TEST_RUNNER}"
    timeout "${TIMEOUT_SECONDS}s" adb "${adb_args[@]}" shell am instrument -w -e class "${TEST_CLASS}" "${TEST_RUNNER}" || true
  else
    echo "timeout command not found; running without timeout."
    run_checked adb "${adb_args[@]}" shell am instrument -w -e class "${TEST_CLASS}" "${TEST_RUNNER}"
  fi

  echo "==> Filtered logcat"
  adb "${adb_args[@]}" logcat -d -v time -s ExoCppBridge cppbridge AndroidRuntime TestRunner | grep -aE "${LOG_FILTER}" || true
fi

echo
echo "cppbridge androidTest rebuild/install completed."
