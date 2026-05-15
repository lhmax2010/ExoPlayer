#!/usr/bin/env python3
import argparse
import os
import subprocess
from pathlib import Path


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
    parser.add_argument("--skip-install", action="store_true")
    args = parser.parse_args()

    env = os.environ.copy()
    if args.serial:
        env["ANDROID_SERIAL"] = args.serial

    adb_prefix = ["adb"]
    if args.serial:
        adb_prefix += ["-s", args.serial]

    run_checked(["adb", "start-server"], env=env)

    if not args.skip_install:
        run_checked([args.gradle, ":demo-cppbridge:installDebug"], env=env)

    run_checked(
        adb_prefix + ["shell", "am", "start", "-n", "androidx.media3.demo.cppbridge/.MainActivity"],
        env=env,
    )

    print("\nDemo launched.")


if __name__ == "__main__":
    main()
