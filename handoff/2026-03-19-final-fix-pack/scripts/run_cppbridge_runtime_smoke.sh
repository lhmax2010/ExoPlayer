#!/usr/bin/env bash

set -u

# Usage:
#   bash scripts/run_cppbridge_runtime_smoke.sh
#
# This script is for runtime-sensitive cppbridge smoke cases that prepare/play the player
# internally before validating behavior.
# It performs URL/network preflight checks before starting tests.
# Current runtime-sensitive cases in this script:
#   - nativeRendererPlayerMessageSmokeTest_returnsRuntimeSummary
#
# Cases stay out of this script if they have already been converted into fixed-fixture or
# bridge-only smoke tests and no longer depend on real player runtime state.
#
# Equivalent manual command after rebuild/install:
#   adb shell am instrument -w -e class \
#     androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeRendererPlayerMessageSmokeTest_returnsRuntimeSummary \
#     androidx.media3.exoplayer.cppbridge.test/androidx.test.runner.AndroidJUnitRunner

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

TEST_PACKAGE="androidx.media3.exoplayer.cppbridge.test/androidx.test.runner.AndroidJUnitRunner"
TEST_APP_ID="androidx.media3.exoplayer.cppbridge.test"

NETWORK_REQUIRED_URLS=(
  "https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"
)

TEST_CASES=(
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeRendererPlayerMessageSmokeTest_returnsRuntimeSummary"
)

failures=()

check_url() {
  local url="$1"
  if command -v curl >/dev/null 2>&1; then
    curl -fsIL --max-time 20 "$url" >/dev/null 2>&1
    return $?
  fi
  if command -v wget >/dev/null 2>&1; then
    wget -q --spider --timeout=20 "$url"
    return $?
  fi
  echo "Neither curl nor wget is available for runtime URL preflight."
  return 1
}

echo "==> Clearing logcat"
adb logcat -c

echo "==> Checking required runtime URLs"
for url in "${NETWORK_REQUIRED_URLS[@]}"; do
  echo "---- ${url}"
  if ! check_url "${url}"; then
    echo "Runtime smoke preflight failed: cannot access required URL:"
    echo "  ${url}"
    echo "Please fix network access before running runtime smoke tests."
    exit 1
  fi
done

echo "==> Uninstalling old androidTest APK"
adb uninstall "${TEST_APP_ID}" >/dev/null 2>&1 || true

echo "==> Cleaning cppbridge androidTest build outputs"
rm -rf libraries/exoplayer_cppbridge/buildout/intermediates/cxx
rm -rf libraries/exoplayer_cppbridge/buildout/.cxx
rm -rf libraries/exoplayer_cppbridge/buildout/intermediates/stripped_native_libs
rm -rf libraries/exoplayer_cppbridge/buildout/intermediates/merged_native_libs
rm -rf libraries/exoplayer_cppbridge/buildout/outputs/apk/androidTest

echo "==> Rebuilding androidTest APK"
./gradlew :lib-exoplayer-cppbridge:assembleDebugAndroidTest --rerun-tasks || exit 1

echo "==> Installing androidTest APK"
./gradlew :lib-exoplayer-cppbridge:installDebugAndroidTest || exit 1

echo "==> Running ${#TEST_CASES[@]} runtime smoke cases"
for test_case in "${TEST_CASES[@]}"; do
  echo
  echo "---- ${test_case}"
  if ! adb shell am instrument -w -e class "${test_case}" "${TEST_PACKAGE}"; then
    failures+=("${test_case}")
  fi
done

echo
if [ "${#failures[@]}" -eq 0 ]; then
  echo "All cppbridge runtime smoke cases passed."
  exit 0
fi

echo "Failed cppbridge runtime smoke cases:"
for test_case in "${failures[@]}"; do
  echo "  - ${test_case}"
done
exit 1
