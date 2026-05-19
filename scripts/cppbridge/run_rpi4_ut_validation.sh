#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

SERIAL="${ANDROID_SERIAL:-}"
OUT_DIR=""
RUN_LOCAL=1
RUN_CONNECTED=1
LAUNCH_DEMO=0
LOCAL_STATUS=0
CONNECTED_STATUS=0
CONNECTED_MODE="adb"
TEST_PACKAGE="androidx.media3.exoplayer.cppbridge.test"
TEST_RUNNER="androidx.test.runner.AndroidJUnitRunner"
TEST_CLASSES=(
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest"
  "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest"
)

usage() {
  cat <<'EOF'
Usage:
  ./scripts/cppbridge/run_rpi4_ut_validation.sh [options]

Runs the cppbridge validation suite against a connected Raspberry Pi 4 Android board.
The script runs on the host machine that has JDK, Android SDK, Gradle, and adb access.
The connected instrumentation tests execute on the board through adb.

Options:
  --serial <serial>       adb serial from `adb devices`; default: ANDROID_SERIAL or the only online device
  --out <dir>             output directory for logs/results; default: buildout/cppbridge-rpi4-ut-<timestamp>
  --test-class <class>    instrumentation class to run; can be repeated
  --gradle-connected      use Gradle connectedDebugAndroidTest instead of direct adb instrumentation
  --connected-only        skip host-side API/JVM/package checks
  --local-only            skip board-side connected instrumentation checks
  --launch-demo           launch the demo after connected UT passes
  -h, --help              show this help

Examples:
  ./scripts/cppbridge/run_rpi4_ut_validation.sh --serial 192.168.1.20:5555
  ANDROID_SERIAL=192.168.1.20:5555 ./scripts/cppbridge/run_rpi4_ut_validation.sh
EOF
}

run_logged() {
  local log_file="$1"
  shift
  echo ">> $*" | tee -a "${log_file}"
  "$@" 2>&1 | tee -a "${log_file}"
}

detect_single_online_device() {
  adb devices | awk '$2 == "device" { print $1 }'
}

collect_board_facts() {
  local facts_file="${OUT_DIR}/board_facts.txt"
  {
    echo "date=$(date -Iseconds)"
    echo "serial=${SERIAL}"
    echo "repo=$(pwd)"
    echo "git_head=$(git rev-parse --short HEAD 2>/dev/null || true)"
    echo "git_branch=$(git branch --show-current 2>/dev/null || true)"
    echo
    echo "[adb devices]"
    adb devices
    echo
    echo "[getprop]"
    adb -s "${SERIAL}" shell getprop ro.build.version.release
    adb -s "${SERIAL}" shell getprop ro.build.version.sdk
    adb -s "${SERIAL}" shell getprop ro.product.cpu.abi
    adb -s "${SERIAL}" shell getprop ro.product.cpu.abilist
    adb -s "${SERIAL}" shell getprop ro.hardware
    adb -s "${SERIAL}" shell getprop ro.board.platform
    adb -s "${SERIAL}" shell getprop ro.build.fingerprint
    echo
    echo "[display]"
    adb -s "${SERIAL}" shell wm size || true
    adb -s "${SERIAL}" shell wm density || true
  } > "${facts_file}" 2>&1
}

collect_logs() {
  local full_log="${OUT_DIR}/logcat-full.txt"
  local signal_log="${OUT_DIR}/logcat-high-signal.txt"
  adb -s "${SERIAL}" logcat -d > "${full_log}" 2>/dev/null || true
  grep -iE "cppbridge|ExoPlayer|MediaCodec|AndroidRuntime|FATAL|tombstone|UnsatisfiedLinkError|JNI DETECTED ERROR|Fatal signal|ANR|FAILURE|INSTRUMENTATION" \
    "${full_log}" > "${signal_log}" 2>/dev/null || true
  adb -s "${SERIAL}" shell ls -la /data/tombstones > "${OUT_DIR}/tombstones-list.txt" 2>/dev/null || true
}

find_test_apk() {
  find libraries/exoplayer_cppbridge/buildout/outputs/apk/androidTest/debug \
    -type f -name "*-androidTest.apk" | sort | head -n 1
}

install_test_apk() {
  local log_file="$1"
  local test_apk
  test_apk="$(find_test_apk)"
  if [[ -z "${test_apk}" ]]; then
    echo "No androidTest APK found. Expected libraries/exoplayer_cppbridge/buildout/outputs/apk/androidTest/debug/*-androidTest.apk" \
      | tee -a "${log_file}"
    return 1
  fi

  echo ">> adb -s ${SERIAL} install -r -t ${test_apk}" | tee -a "${log_file}"
  if adb -s "${SERIAL}" install -r -t "${test_apk}" 2>&1 | tee -a "${log_file}"; then
    return 0
  fi

  echo "Initial install failed; retrying after uninstalling ${TEST_PACKAGE}." | tee -a "${log_file}"
  adb -s "${SERIAL}" uninstall "${TEST_PACKAGE}" 2>&1 | tee -a "${log_file}" || true
  echo ">> adb -s ${SERIAL} install -r -t ${test_apk}" | tee -a "${log_file}"
  adb -s "${SERIAL}" install -r -t "${test_apk}" 2>&1 | tee -a "${log_file}"
}

run_instrumentation_class() {
  local log_file="$1"
  local test_class="$2"
  local output_file="${OUT_DIR}/instrument-${test_class##*.}.txt"
  local command=(
    adb -s "${SERIAL}" shell am instrument -w -r -e class "${test_class}"
    "${TEST_PACKAGE}/${TEST_RUNNER}"
  )

  echo ">> ${command[*]}" | tee -a "${log_file}" "${output_file}"
  set +e
  "${command[@]}" 2>&1 | tee -a "${log_file}" "${output_file}"
  local command_status=${PIPESTATUS[0]}
  set -e

  if [[ "${command_status}" -ne 0 ]]; then
    return "${command_status}"
  fi
  if grep -qE "FAILURES!!!|INSTRUMENTATION_RESULT: shortMsg=Process crashed|INSTRUMENTATION_RESULT: shortMsg=Native crash|INSTRUMENTATION_RESULT: shortMsg=Timed out|INSTRUMENTATION_CODE: 0" "${output_file}"; then
    return 1
  fi
  if ! grep -qE "OK \\([0-9]+ tests?\\)|INSTRUMENTATION_CODE: -1" "${output_file}"; then
    echo "Could not confirm instrumentation success for ${test_class}; inspect ${output_file}." \
      | tee -a "${log_file}" "${output_file}"
    return 1
  fi
  return 0
}

run_direct_adb_instrumentation() {
  local log_file="${OUT_DIR}/connected_validation.txt"
  local status=0

  run_logged "${log_file}" \
    ./gradlew :lib-exoplayer-cppbridge:assembleDebugAndroidTest --console=plain || status=$?
  if [[ "${status}" -ne 0 ]]; then
    return "${status}"
  fi

  install_test_apk "${log_file}" || status=$?
  if [[ "${status}" -ne 0 ]]; then
    return "${status}"
  fi

  for test_class in "${TEST_CLASSES[@]}"; do
    local class_status=0
    run_instrumentation_class "${log_file}" "${test_class}" || class_status=$?
    if [[ "${class_status}" -ne 0 ]]; then
      status="${class_status}"
    fi
  done

  if [[ "${LAUNCH_DEMO}" -eq 1 ]]; then
    run_logged "${log_file}" ./gradlew :demo-cppbridge:installDebug --console=plain || status=$?
    run_logged "${log_file}" \
      adb -s "${SERIAL}" shell am start -n androidx.media3.demo.cppbridge/.MainActivity || status=$?
  fi

  return "${status}"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --serial)
      SERIAL="$2"
      shift 2
      ;;
    --out)
      OUT_DIR="$2"
      shift 2
      ;;
    --test-class)
      if [[ ${#TEST_CLASSES[@]} -eq 2 && "${TEST_CLASSES[0]}" == "androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest" && "${TEST_CLASSES[1]}" == "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest" ]]; then
        TEST_CLASSES=()
      fi
      TEST_CLASSES+=("$2")
      shift 2
      ;;
    --gradle-connected)
      CONNECTED_MODE="gradle"
      shift
      ;;
    --connected-only)
      RUN_LOCAL=0
      shift
      ;;
    --local-only)
      RUN_CONNECTED=0
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

if [[ "${RUN_LOCAL}" -eq 0 && "${RUN_CONNECTED}" -eq 0 ]]; then
  echo "Nothing to run: --connected-only and --local-only cannot be used together." >&2
  exit 1
fi

if [[ -z "${JAVA_HOME:-}" && -d /usr/lib/jvm/java-17-openjdk-amd64 ]]; then
  export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
fi
if [[ -z "${ANDROID_SDK_ROOT:-}" && -d "${HOME}/Android/Sdk" ]]; then
  export ANDROID_SDK_ROOT="${HOME}/Android/Sdk"
fi
if [[ -n "${JAVA_HOME:-}" ]]; then
  export PATH="${JAVA_HOME}/bin:${PATH}"
fi
if [[ -n "${ANDROID_SDK_ROOT:-}" ]]; then
  export PATH="${ANDROID_SDK_ROOT}/platform-tools:${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin:${ANDROID_SDK_ROOT}/emulator:${PATH}"
fi

if [[ -z "${OUT_DIR}" ]]; then
  OUT_DIR="${REPO_ROOT}/buildout/cppbridge-rpi4-ut-$(date +%Y%m%d-%H%M%S)"
fi
mkdir -p "${OUT_DIR}"

command -v adb >/dev/null 2>&1 || {
  echo "adb not found. Export ANDROID_SDK_ROOT and add platform-tools to PATH." >&2
  exit 1
}

adb start-server >/dev/null

if [[ -z "${SERIAL}" && "${RUN_CONNECTED}" -eq 1 ]]; then
  mapfile -t online_devices < <(detect_single_online_device)
  if [[ "${#online_devices[@]}" -eq 1 ]]; then
    SERIAL="${online_devices[0]}"
  else
    echo "Could not auto-select a board. Use --serial with one of these devices:" >&2
    adb devices >&2
    exit 1
  fi
fi

if [[ "${RUN_CONNECTED}" -eq 1 ]]; then
  if ! adb devices | awk -v serial="${SERIAL}" '$1 == serial && $2 == "device" { found = 1 } END { exit found ? 0 : 1 }'; then
    echo "No online adb device with serial ${SERIAL} was found." >&2
    adb devices >&2
    exit 1
  fi
  export ANDROID_SERIAL="${SERIAL}"
  collect_board_facts
  adb -s "${SERIAL}" logcat -c || true
fi

echo "Output directory: ${OUT_DIR}"

if [[ "${RUN_LOCAL}" -eq 1 ]]; then
  LOCAL_STATUS=0
  run_logged "${OUT_DIR}/local_validation.txt" \
    bash scripts/cppbridge/run_validation.sh --local-only || LOCAL_STATUS=$?
fi

if [[ "${RUN_CONNECTED}" -eq 1 ]]; then
  if [[ "${CONNECTED_MODE}" == "gradle" ]]; then
    connected_args=(bash scripts/cppbridge/run_validation.sh --serial "${SERIAL}")
    for test_class in "${TEST_CLASSES[@]}"; do
      connected_args+=(--test-class "${test_class}")
    done
    if [[ "${LAUNCH_DEMO}" -eq 1 ]]; then
      connected_args+=(--launch-demo)
    fi
    CONNECTED_STATUS=0
    run_logged "${OUT_DIR}/connected_validation.txt" "${connected_args[@]}" || CONNECTED_STATUS=$?
  else
    CONNECTED_STATUS=0
    run_direct_adb_instrumentation || CONNECTED_STATUS=$?
  fi
  collect_logs
fi

local_result="skipped"
connected_result="skipped"
if [[ "${RUN_LOCAL}" -eq 1 ]]; then
  local_result="passed"
  if [[ "${LOCAL_STATUS}" -ne 0 ]]; then
    local_result="failed:${LOCAL_STATUS}"
  fi
fi
if [[ "${RUN_CONNECTED}" -eq 1 ]]; then
  connected_result="passed"
  if [[ "${CONNECTED_STATUS}" -ne 0 ]]; then
    connected_result="failed:${CONNECTED_STATUS}"
  fi
fi

cat <<EOF | tee "${OUT_DIR}/summary.txt"
cppbridge RPI4 UT validation completed.
output_dir=${OUT_DIR}
serial=${SERIAL:-not-used}
local_validation=${local_result}
connected_validation=${connected_result}
connected_mode=$([[ "${RUN_CONNECTED}" -eq 1 ]] && echo "${CONNECTED_MODE}" || echo "skipped")
expected_connected_baseline=CppBridgeNativeSmokeTest 26/26 + CppBridgeNativePlayerInstrumentationTest 112/112 = 138/138
EOF

if [[ "${LOCAL_STATUS}" -ne 0 || "${CONNECTED_STATUS}" -ne 0 ]]; then
  echo "Validation had failures. See ${OUT_DIR}/summary.txt and the phase logs." >&2
  exit 1
fi
