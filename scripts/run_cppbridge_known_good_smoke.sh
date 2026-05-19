#!/usr/bin/env bash

set -u

# Usage:
#   bash scripts/run_cppbridge_known_good_smoke.sh
#
# Equivalent manual commands:
#   adb uninstall androidx.media3.exoplayer.cppbridge.test
#   rm -rf libraries/exoplayer_cppbridge/buildout/intermediates/cxx
#   rm -rf libraries/exoplayer_cppbridge/buildout/.cxx
#   rm -rf libraries/exoplayer_cppbridge/buildout/intermediates/stripped_native_libs
#   rm -rf libraries/exoplayer_cppbridge/buildout/intermediates/merged_native_libs
#   rm -rf libraries/exoplayer_cppbridge/buildout/outputs/apk/androidTest
#   ./gradlew :lib-exoplayer-cppbridge:assembleDebugAndroidTest --rerun-tasks
#   ./gradlew :lib-exoplayer-cppbridge:installDebugAndroidTest
#   adb shell am instrument -w -e class <TEST_CASE> \
#     androidx.media3.exoplayer.cppbridge.test/androidx.test.runner.AndroidJUnitRunner

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

TEST_PACKAGE="androidx.media3.exoplayer.cppbridge.test/androidx.test.runner.AndroidJUnitRunner"
TEST_APP_ID="androidx.media3.exoplayer.cppbridge.test"

wait_for_android_ready() {
  adb wait-for-device
  local boot_completed=""
  until [ "${boot_completed}" = "1" ]; do
    boot_completed="$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')"
    if [ "${boot_completed}" = "1" ]; then
      break
    fi
    sleep 1
  done
}

TEST_CASES=(
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest#nativeOpaqueTokenReleaseSmokeTest_releasesRegisteredTokensThroughSdk"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest#nativeBuilderPreloadRoundTripSmokeTest_updatesAndRestoresPreloadTarget"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest#nativeListenerLifecycleNegativeSmokeTest_handlesRepeatedAndNullRemoval"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest#nativeCueSnapshotConversionSmokeTest_returnsStructuredSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeCreateConfiguredPlayerSnapshotForTest_returnsConfiguredState"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeCreatePlaylistSnapshotForTest_returnsPlaylistState"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeLifecycleSmokeTest_runsThroughLifecycle"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativePostReleaseCallSafetySmokeTest_doesNotCrashOrHang"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeMediaItemAtSmokeTest_returnsSnapshotAndHandlesOutOfBounds"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeSubtitleSmokeTest_returnsSubtitleSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeDrmSmokeTest_returnsDrmSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeClippingSmokeTest_returnsClippingSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeLiveConfigurationSmokeTest_returnsLiveSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeMultiSubtitleSmokeTest_returnsSubtitleAndPreferenceSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativePlaylistMetadataSmokeTest_roundTripsPlaylistMetadata"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeMediaItemOpaqueTokenSmokeTest_resolvesRegisteredObjects"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeVideoAndMetadataSmokeTest_returnsQuerySummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeAvailableCommandsSmokeTest_returnsContainsStyleSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeTrackSelectionRoundTripForTest_returnsUpdatedParameters"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeMediaSetOverloadsSmokeTest_returnsUpdatedPlaylistSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeSeekParametersSmokeTest_roundTripsSeekParameters"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeSeekNavigationSmokeTest_runsNavigationCalls"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeSeekAliasSmokeTest_runsJavaNameParityAliases"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeDeviceAndSkipSilenceSmokeTest_returnsDeviceSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativePlaylistMutationSmokeTest_returnsUpdatedPlaylistState"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativePlayerMessageSmokeTest_returnsDeliverySummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativePlayerMessageCancelSmokeTest_returnsCanceledSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeCurrentTracksSmokeTest_returnsTracksSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeCurrentTimelineSmokeTest_returnsTimelineDetails"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeSourceTypeSmokeTest_returnsInferredMimeSummary"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeListenerSmokeTest_reportsExtendedCallbacks"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeListenerCallbackDetailSmokeTest_reportsSupplementalCallbacks"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest#nativeListenerMetadataCueDetailSmokeTest_reportsSupplementalPayloads"
)

failures=()

echo "==> Waiting for device"
wait_for_android_ready

echo "==> Clearing logcat"
adb logcat -c

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

echo "==> Running ${#TEST_CASES[@]} known-good smoke cases"
for test_case in "${TEST_CASES[@]}"; do
  echo
  echo "---- ${test_case}"
  wait_for_android_ready
  if ! adb shell am instrument -w -e class "${test_case}" "${TEST_PACKAGE}"; then
    failures+=("${test_case}")
  fi
done

echo
if [ "${#failures[@]}" -eq 0 ]; then
  echo "All known-good cppbridge smoke cases passed."
  exit 0
fi

echo "Failed cppbridge smoke cases:"
for test_case in "${failures[@]}"; do
  echo "  - ${test_case}"
done
exit 1
