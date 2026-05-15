#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

GRADLE_EXECUTABLE="./gradlew"
SERIAL="${ANDROID_SERIAL:-}"
SKIP_INSTALL=0

usage() {
  cat <<'EOF'
Usage:
  ./scripts/cppbridge/launch_demo.sh [options]

Options:
  --gradle <path>      Gradle executable, default: ./gradlew
  --serial <serial>    adb device serial; also exported as ANDROID_SERIAL
  --skip-install       skip Gradle install step
  -h, --help           show this help
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

run_checked adb start-server

if [[ "${SKIP_INSTALL}" -eq 0 ]]; then
  run_checked "${GRADLE_EXECUTABLE}" :demo-cppbridge:installDebug
fi

run_checked adb "${adb_args[@]}" shell am start -n androidx.media3.demo.cppbridge/.MainActivity

echo
echo "Demo launched."
