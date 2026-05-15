#!/usr/bin/env python3
import argparse
import os
import subprocess
from pathlib import Path


DEFAULT_TEST_CLASSES = [
    "androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest",
    "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest",
]


def default_gradle_command():
    return "gradlew.bat" if os.name == "nt" else "./gradlew"


def run_checked(args, env=None):
    print(">>", " ".join(args))
    subprocess.run(args, check=True, env=env)


def main():
    repo_root = Path(__file__).resolve().parents[2]
    os.chdir(repo_root)

    parser = argparse.ArgumentParser()
    parser.add_argument("--gradle", default=default_gradle_command())
    parser.add_argument("--serial")
    parser.add_argument("--test-class", action="append", dest="test_classes")
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--launch-demo", action="store_true")
    args = parser.parse_args()

    env = os.environ.copy()
    if args.serial:
        env["ANDROID_SERIAL"] = args.serial

    adb_prefix = ["adb"]
    if args.serial:
        adb_prefix += ["-s", args.serial]

    run_checked(["adb", "start-server"], env=env)
    devices = subprocess.run(
        ["adb", "devices"], check=True, capture_output=True, text=True, env=env
    ).stdout.splitlines()
    if not any(line.strip().endswith("\tdevice") or line.strip().endswith(" device") for line in devices):
        raise SystemExit("No online adb device was found. Connect a device or start an emulator first.")

    if not args.skip_build:
        run_checked(
            [args.gradle, ":lib-exoplayer-cppbridge:assembleDebugAndroidTest", ":demo-cppbridge:installDebug"],
            env=env,
        )

    test_classes = args.test_classes or DEFAULT_TEST_CLASSES
    for test_class in test_classes:
        run_checked(
            [
                args.gradle,
                ":lib-exoplayer-cppbridge:connectedDebugAndroidTest",
                f"-Pandroid.testInstrumentationRunnerArguments.class={test_class}",
            ],
            env=env,
        )

    if args.launch_demo:
        run_checked(
            adb_prefix + ["shell", "am", "start", "-n", "androidx.media3.demo.cppbridge/.MainActivity"],
            env=env,
        )

    print("\nValidation completed.")


if __name__ == "__main__":
    main()
