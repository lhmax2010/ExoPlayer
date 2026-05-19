#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

GRADLE_EXECUTABLE="./gradlew"
SERIAL="${ANDROID_SERIAL:-}"
SKIP_BUILD=0
LOCAL_ONLY=0
LAUNCH_DEMO=0
TEST_CLASSES=(
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest"
)

usage() {
  cat <<'EOF'
Usage:
  ./scripts/cppbridge/run_validation.sh [options]

Options:
  --gradle <path>        Gradle executable, default: ./gradlew
  --serial <serial>      adb device serial; also exported as ANDROID_SERIAL
  --test-class <class>   instrumentation class to run; can be repeated
  --skip-build           skip Gradle build/install steps
  --local-only           run API inventory plus local build/unit/package checks without adb
  --launch-demo          launch demo activity after tests
  -h, --help             show this help
EOF
}

run_checked() {
  echo ">> $*"
  "$@"
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
      if [[ ${#TEST_CLASSES[@]} -eq 2 && "${TEST_CLASSES[0]}" == "androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest" && "${TEST_CLASSES[1]}" == "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest" ]]; then
        TEST_CLASSES=()
      fi
      TEST_CLASSES+=("$2")
      shift 2
      ;;
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    --local-only)
      LOCAL_ONLY=1
      shift
      ;;
    --launch-demo)
      LAUNCH_DEMO=1
      shift
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

if [[ "${LOCAL_ONLY}" -eq 1 ]]; then
  if [[ "${SKIP_BUILD}" -eq 1 ]]; then
    echo "--local-only cannot be combined with --skip-build." >&2
    exit 1
  fi
  if [[ "${LAUNCH_DEMO}" -eq 1 ]]; then
    echo "--local-only cannot be combined with --launch-demo." >&2
    exit 1
  fi
  if command -v python3 >/dev/null 2>&1; then
    run_checked python3 scripts/cppbridge/api_parity_inventory.py --check
  else
    echo "python3 not found; skipping API parity inventory check." >&2
  fi
  run_checked "${GRADLE_EXECUTABLE}" \
    :lib-exoplayer-cppbridge:testDebugUnitTest \
    :lib-exoplayer-cppbridge:assembleDebugAndroidTest \
    :demo-cppbridge:assembleDebug
  echo
  echo "Local validation completed."
  exit 0
fi

run_checked adb start-server

devices_output="$(adb devices)"
if [[ -n "${SERIAL}" ]]; then
  if ! awk -v serial="${SERIAL}" '$1 == serial && $2 == "device" { found = 1 } END { exit found ? 0 : 1 }' <<< "${devices_output}"; then
    echo "No online adb device with serial ${SERIAL} was found. Connect it or start the requested emulator first." >&2
    exit 1
  fi
else
  if ! grep -qE '^[^[:space:]]+[[:space:]]+device$' <<< "${devices_output}"; then
    echo "No online adb device was found. Connect a device or start an emulator first." >&2
    exit 1
  fi
fi

if [[ "${SKIP_BUILD}" -eq 0 ]]; then
  run_checked "${GRADLE_EXECUTABLE}" \
    :lib-exoplayer-cppbridge:assembleDebugAndroidTest \
    :demo-cppbridge:installDebug
fi

for test_class in "${TEST_CLASSES[@]}"; do
  run_checked "${GRADLE_EXECUTABLE}" \
    :lib-exoplayer-cppbridge:connectedDebugAndroidTest \
    "-Pandroid.testInstrumentationRunnerArguments.class=${test_class}"
done

if [[ "${LAUNCH_DEMO}" -eq 1 ]]; then
  run_checked adb "${adb_args[@]}" shell am start -n androidx.media3.demo.cppbridge/.MainActivity
fi

echo
echo "Validation completed."
