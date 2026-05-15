#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

APP_ID="androidx.media3.demo.cppbridge"
ACTIVITY="${APP_ID}/.MainActivity"
GRADLE_EXECUTABLE="./gradlew"
SERIAL="${ANDROID_SERIAL:-}"
WAIT_SECONDS=12
SCENARIO="all"
HTTP_URL=""
HLS_URL=""
DASH_URL=""
HTTP_MIME="video/mp4"
HLS_MIME="application/x-mpegURL"
DASH_MIME="application/dash+xml"
OUTPUT_DIR=""
SKIP_INSTALL=0

FILTER_REGEX='ExoCppBridge|cppbridge|AndroidRuntime|ExoPlayerImpl|ExoPlayerImplInternal|PlaybackException|MediaCodec|ACodec|CCodec|Codec2|NuPlayer|AudioTrack|AudioFlinger|DefaultHttpDataSource|HttpDataSource|ParserException|LoadError|UnknownHost|SSL|SocketTimeout|Cleartext|Surface'

usage() {
  cat <<'EOF'
Usage:
  bash scripts/cppbridge/probe_stream_playback.sh [options]

Options:
  --http-url <url>       HTTP/progressive stream URL
  --hls-url <url>        HLS stream URL
  --dash-url <url>       DASH stream URL
  --scenario <name>      all|http|hls|dash, default: all
  --wait-seconds <sec>   how long to wait after launch, default: 12
  --output-dir <path>    output directory, default: artifacts/cppbridge_stream_probe/<timestamp>
  --gradle <path>        Gradle executable, default: ./gradlew
  --serial <serial>      adb device serial; also exported as ANDROID_SERIAL
  --skip-install         skip Gradle install step
  -h, --help             show this help

Examples:
  bash scripts/cppbridge/probe_stream_playback.sh \
    --http-url http://192.168.1.10/media/test.mp4 \
    --hls-url http://192.168.1.10/hls/stream.m3u8 \
    --dash-url http://192.168.1.10/dash/stream.mpd

  bash scripts/cppbridge/probe_stream_playback.sh \
    --scenario hls \
    --hls-url http://192.168.1.10/hls/stream.m3u8 \
    --skip-install
EOF
}

run_checked() {
  echo ">> $*"
  "$@"
}

adb_args=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --http-url)
      HTTP_URL="$2"
      shift 2
      ;;
    --hls-url)
      HLS_URL="$2"
      shift 2
      ;;
    --dash-url)
      DASH_URL="$2"
      shift 2
      ;;
    --scenario)
      SCENARIO="$2"
      shift 2
      ;;
    --wait-seconds)
      WAIT_SECONDS="$2"
      shift 2
      ;;
    --output-dir)
      OUTPUT_DIR="$2"
      shift 2
      ;;
    --gradle)
      GRADLE_EXECUTABLE="$2"
      shift 2
      ;;
    --serial)
      SERIAL="$2"
      shift 2
      ;;
    --skip-install)
      SKIP_INSTALL=1
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

adb_cmd() {
  adb "${adb_args[@]}" "$@"
}

wait_for_android_ready() {
  adb_cmd wait-for-device
  local boot_completed=""
  until [[ "${boot_completed}" = "1" ]]; do
    boot_completed="$(adb_cmd shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')"
    [[ "${boot_completed}" = "1" ]] && break
    sleep 1
  done
}

capture_ui_snapshot() {
  local scenario_dir="$1"
  local scenario_name="$2"
  local remote_ui_path="/sdcard/${scenario_name}_cppbridge_ui.xml"

  adb_cmd shell uiautomator dump "${remote_ui_path}" >/dev/null 2>&1 || true
  adb_cmd pull "${remote_ui_path}" "${scenario_dir}/ui.xml" >/dev/null 2>&1 || true
  adb_cmd shell rm -f "${remote_ui_path}" >/dev/null 2>&1 || true
  adb_cmd exec-out screencap -p > "${scenario_dir}/screen.png" || true
}

capture_logs() {
  local scenario_dir="$1"
  adb_cmd logcat -d -v threadtime > "${scenario_dir}/logcat_full.log" || true
  grep -aE "${FILTER_REGEX}" "${scenario_dir}/logcat_full.log" > "${scenario_dir}/logcat_filtered.log" || true
}

run_scenario() {
  local scenario_name="$1"
  local media_url="$2"
  local source_type="$3"
  local mime_type="$4"

  if [[ -z "${media_url}" ]]; then
    echo "Skipping ${scenario_name}: no URL provided."
    return 0
  fi

  local scenario_dir="${OUTPUT_DIR}/${scenario_name}"
  mkdir -p "${scenario_dir}"

  echo
  echo "==> Scenario: ${scenario_name}"
  echo "==> URL: ${media_url}"

  adb_cmd logcat -c
  adb_cmd shell am force-stop "${APP_ID}" >/dev/null 2>&1 || true
  run_checked adb "${adb_args[@]}" shell am start -S -n "${ACTIVITY}" \
    --es media_url "${media_url}" \
    --ei source_type "${source_type}" \
    --es mime_type "${mime_type}" \
    --ez auto_play true \
    --ez skip_default_load true

  echo "==> Waiting ${WAIT_SECONDS}s for playback and error callbacks"
  sleep "${WAIT_SECONDS}"

  capture_logs "${scenario_dir}"
  capture_ui_snapshot "${scenario_dir}" "${scenario_name}"

  echo "==> Saved:"
  echo "    ${scenario_dir}/logcat_filtered.log"
  echo "    ${scenario_dir}/logcat_full.log"
  echo "    ${scenario_dir}/ui.xml"
  echo "    ${scenario_dir}/screen.png"
}

case "${SCENARIO}" in
  all|http|hls|dash)
    ;;
  *)
    echo "Unsupported scenario: ${SCENARIO}" >&2
    usage >&2
    exit 1
    ;;
esac

if [[ -z "${OUTPUT_DIR}" ]]; then
  timestamp="$(date +%Y%m%d_%H%M%S)"
  OUTPUT_DIR="${REPO_ROOT}/artifacts/cppbridge_stream_probe/${timestamp}"
fi

mkdir -p "${OUTPUT_DIR}"

run_checked adb start-server
wait_for_android_ready

if [[ "${SKIP_INSTALL}" -eq 0 ]]; then
  run_checked "${GRADLE_EXECUTABLE}" :demo-cppbridge:installDebug
fi

if [[ "${SCENARIO}" = "all" || "${SCENARIO}" = "http" ]]; then
  run_scenario "http" "${HTTP_URL}" 5 "${HTTP_MIME}"
fi

if [[ "${SCENARIO}" = "all" || "${SCENARIO}" = "hls" ]]; then
  run_scenario "hls" "${HLS_URL}" 2 "${HLS_MIME}"
fi

if [[ "${SCENARIO}" = "all" || "${SCENARIO}" = "dash" ]]; then
  run_scenario "dash" "${DASH_URL}" 1 "${DASH_MIME}"
fi

echo
echo "Probe finished."
echo "Artifacts: ${OUTPUT_DIR}"
