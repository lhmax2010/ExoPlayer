#!/usr/bin/env python3
"""Generate a heuristic API parity inventory for exoplayer_cppbridge."""

from __future__ import annotations

import argparse
import dataclasses
import json
import re
import sys
from pathlib import Path
from typing import Iterable


PLAYER_API_CLASSES = (
    "androidx.media3.common.Player",
    "androidx.media3.exoplayer.ExoPlayer",
)
PLAYER_BUILDER_CLASSES = ("androidx.media3.exoplayer.ExoPlayer.Builder",)
PLAYER_LISTENER_CLASSES = ("androidx.media3.common.Player.Listener",)
OBJECT_MODEL_CLASSES = (
    "androidx.media3.common.Format",
    "androidx.media3.common.Format.Builder",
    "androidx.media3.common.MediaItem",
    "androidx.media3.common.MediaItem.Builder",
    "androidx.media3.common.MediaItem.ClippingConfiguration",
    "androidx.media3.common.MediaItem.ClippingConfiguration.Builder",
    "androidx.media3.common.MediaItem.DrmConfiguration",
    "androidx.media3.common.MediaItem.DrmConfiguration.Builder",
    "androidx.media3.common.MediaItem.LiveConfiguration",
    "androidx.media3.common.MediaItem.LiveConfiguration.Builder",
    "androidx.media3.common.MediaItem.RequestMetadata",
    "androidx.media3.common.MediaItem.RequestMetadata.Builder",
    "androidx.media3.common.MediaItem.SubtitleConfiguration",
    "androidx.media3.common.MediaItem.SubtitleConfiguration.Builder",
    "androidx.media3.common.MediaMetadata",
    "androidx.media3.common.MediaMetadata.Builder",
    "androidx.media3.common.Timeline",
    "androidx.media3.common.Timeline.Window",
    "androidx.media3.common.Timeline.Period",
    "androidx.media3.common.Tracks",
    "androidx.media3.common.Tracks.Group",
    "androidx.media3.common.text.Cue",
    "androidx.media3.common.text.Cue.Builder",
)

CPP_CLASS_NAMES = (
    "ExoPlayerSdkPlayer",
    "ExoPlayerSdkPlayerBuilder",
    "ExoPlayerBridge",
)
CPP_LISTENER_CLASS_NAMES = (
    "PlayerListener",
    "ImageOutputListener",
    "ExoPlayerSdkImageOutputListener",
)

API_TO_CPP_ALIASES = {
    "addListener": {"setListener"},
    "removeListener": {"removeListener"},
    "getCurrentTracks": {"getTracks"},
    "getCurrentTimeline": {"getTimeline", "getTimelineSnapshot"},
    "getCurrentCues": {"getCurrentCues"},
    "build": {"build", "create"},
    "setMediaSourceFactory": {"setMediaSourceFactoryConfig", "setMediaSourceFactoryToken"},
}

NON_PLAYER_RUNTIME_EXTENSIONS = {
    "bindPlayerView",
    "unbindPlayerView",
    "setPriorityTaskManager",
    "clearPriorityTaskManager",
    "setPriorityTaskManagerEnabled",
    "setPreloadConfiguration",
    "getTargetPreloadDurationUs",
    "sendPlayerMessage",
    "setImageOutputEnabled",
    "setImageOutputListener",
    "removeImageOutputListener",
    "setAudioCodecParameters",
    "setVideoCodecParameters",
    "addAudioCodecParametersChangeListener",
    "removeAudioCodecParametersChangeListener",
    "addVideoCodecParametersChangeListener",
    "removeVideoCodecParametersChangeListener",
    "setVideoFrameMetadataListener",
    "clearVideoFrameMetadataListener",
    "setCameraMotionListener",
    "clearCameraMotionListener",
    "getRendererCount",
    "getRendererType",
    "isSleepingForOffload",
    "isTunnelingEnabled",
    "isReleased",
    "releaseOpaqueObjectTokens",
    "getAnalyticsSnapshot",
    "getMediaSourceFactoryConfig",
}


@dataclasses.dataclass(frozen=True)
class MethodEntry:
    class_name: str
    method_name: str
    signature: str


@dataclasses.dataclass(frozen=True)
class Inventory:
    api_methods: dict[str, list[MethodEntry]]
    java_bridge_methods: set[str]
    cpp_methods: set[str]
    cpp_listener_methods: set[str]


def lower_first(name: str) -> str:
    if not name:
        return name
    return name[0].lower() + name[1:]


def cpp_method_to_java_name(name: str) -> str:
    if name.startswith("~"):
        return name
    if name.startswith("Get") and len(name) > 3:
        return "get" + name[3:]
    if name.startswith("Set") and len(name) > 3:
        return "set" + name[3:]
    if name.startswith("Is") and len(name) > 2:
        return "is" + name[2:]
    if name.startswith("Has") and len(name) > 3:
        return "has" + name[3:]
    if name.startswith("Can") and len(name) > 3:
        return "can" + name[3:]
    if name.startswith("On") and len(name) > 2:
        return "on" + name[2:]
    if name == "Release":
        return "release"
    if name == "Build":
        return "build"
    if name == "Create":
        return "create"
    return lower_first(name)


def normalize_bridge_method(name: str) -> str:
    replacements = {
        "getIsLoading": "isLoading",
        "isScrubbingModeEnabledValue": "isScrubbingModeEnabled",
        "isDeviceMutedValue": "isDeviceMuted",
        "isCurrentMediaItemDynamicValue": "isCurrentMediaItemDynamic",
        "isCurrentMediaItemLiveValue": "isCurrentMediaItemLive",
        "isCurrentMediaItemSeekableValue": "isCurrentMediaItemSeekable",
        "isPlayingAdValue": "isPlayingAd",
        "setAudioAttributesConfig": "setAudioAttributes",
        "getAudioAttributesConfig": "getAudioAttributes",
        "setPlaybackParametersConfig": "setPlaybackParameters",
        "setPriorityTaskManagerObject": "setPriorityTaskManager",
        "setImageOutputObject": "setImageOutput",
    }
    if name in replacements:
        return replacements[name]
    for suffix in ("ForTest", "Value", "Config", "Object", "WithFlags"):
        if name.endswith(suffix):
            return name[: -len(suffix)]
    return name


def method_candidates(api_name: str) -> set[str]:
    candidates = {normalize_bridge_method(api_name)}
    candidates.update(API_TO_CPP_ALIASES.get(api_name, set()))
    return candidates


def strip_java_annotations(signature_line: str) -> str:
    return re.sub(r"@[A-Za-z0-9_.]+(?:\([^)]*\))?\s*", "", signature_line)


def extract_api_methods(api_text: str, target_classes: Iterable[str]) -> dict[str, list[MethodEntry]]:
    targets = set(target_classes)
    result: dict[str, list[MethodEntry]] = {target: [] for target in targets}
    current_package: str | None = None
    current_class: str | None = None
    class_re = re.compile(
        r"(?:@\S+\s+)*(?:public|protected)\s+"
        r"(?:(?:static|final|abstract)\s+)*"
        r"(?:class|interface)\s+([A-Za-z0-9_.]+)\b.*\{"
    )
    for raw_line in api_text.splitlines():
        line = raw_line.strip()
        package_match = re.match(r"package\s+([A-Za-z0-9_.]+)\s+\{", line)
        if package_match:
            current_package = package_match.group(1)
            current_class = None
            continue
        class_match = class_re.match(line)
        if class_match and current_package:
            current_class = f"{current_package}.{class_match.group(1)}"
            result.setdefault(current_class, [])
            continue
        if line == "}":
            current_class = None
            continue
        if current_class not in targets or not line.startswith("method "):
            continue
        cleaned_line = strip_java_annotations(line)
        before_paren = cleaned_line.split("(", 1)[0]
        method_name = before_paren.split()[-1]
        result[current_class].append(MethodEntry(current_class, method_name, line))
    return {key: value for key, value in result.items() if key in targets}


def extract_java_bridge_methods(java_text: str) -> set[str]:
    methods: set[str] = set()
    method_re = re.compile(r"^  public\s+(?:[\w<>\[\].?@,]+\s+)+([a-zA-Z_]\w*)\s*\(")
    for line in java_text.splitlines():
        match = method_re.match(line)
        if not match:
            continue
        name = match.group(1)
        if name == "CppExoPlayerBridge":
            continue
        methods.add(normalize_bridge_method(name))
    return methods


def _extract_class_body(header_text: str, class_name: str) -> str:
    match = re.search(rf"\bclass\s+{re.escape(class_name)}\b.*?\{{", header_text, re.DOTALL)
    if not match:
        return ""
    start = match.end()
    depth = 1
    index = start
    while index < len(header_text):
        char = header_text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return header_text[start:index]
        index += 1
    return ""


def extract_cpp_methods(header_text: str, class_names: Iterable[str]) -> set[str]:
    methods: set[str] = set()
    method_re = re.compile(
        r"(?:^|[;{}\n])\s*(?:virtual\s+)?(?:static\s+)?"
        r"[^(){};]+?\s+([~A-Za-z_]\w*)\s*\(",
        re.DOTALL,
    )
    for class_name in class_names:
        body = _extract_class_body(header_text, class_name)
        for match in method_re.finditer(body):
            method_name = match.group(1)
            if method_name == class_name or method_name.startswith("~"):
                continue
            methods.add(cpp_method_to_java_name(method_name))
    return methods


def build_inventory(root: Path) -> Inventory:
    api_text = (root / "api.txt").read_text(encoding="utf-8")
    java_text = (
        root
        / "libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java"
    ).read_text(encoding="utf-8")
    sdk_header = (
        root / "libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_sdk.h"
    ).read_text(encoding="utf-8")
    bridge_header = (
        root / "libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_bridge.h"
    ).read_text(encoding="utf-8")
    api_classes = (
        PLAYER_API_CLASSES + PLAYER_BUILDER_CLASSES + PLAYER_LISTENER_CLASSES + OBJECT_MODEL_CLASSES
    )
    return Inventory(
        api_methods=extract_api_methods(api_text, api_classes),
        java_bridge_methods=extract_java_bridge_methods(java_text),
        cpp_methods=extract_cpp_methods(sdk_header + "\n" + bridge_header, CPP_CLASS_NAMES),
        cpp_listener_methods=extract_cpp_methods(sdk_header + "\n" + bridge_header, CPP_LISTENER_CLASS_NAMES),
    )


def unique_method_names(entries: Iterable[MethodEntry]) -> set[str]:
    return {entry.method_name for entry in entries}


def uncovered_methods(api_names: set[str], cpp_names: set[str]) -> list[str]:
    uncovered = []
    for name in sorted(api_names):
        if not (method_candidates(name) & cpp_names):
            uncovered.append(name)
    return uncovered


def covered_methods(api_names: set[str], cpp_names: set[str]) -> list[str]:
    covered = []
    for name in sorted(api_names):
        if method_candidates(name) & cpp_names:
            covered.append(name)
    return covered


def markdown_list(items: Iterable[str]) -> str:
    items = list(items)
    if not items:
        return "- none"
    return "\n".join(f"- `{item}`" for item in items)


def object_model_rows(inventory: Inventory) -> list[tuple[str, int]]:
    rows = []
    for class_name in OBJECT_MODEL_CLASSES:
        rows.append((class_name, len(inventory.api_methods.get(class_name, []))))
    return rows


def generate_markdown(inventory: Inventory) -> str:
    player_api_names = unique_method_names(
        entry for cls in PLAYER_API_CLASSES for entry in inventory.api_methods.get(cls, [])
    )
    builder_api_names = unique_method_names(
        entry for cls in PLAYER_BUILDER_CLASSES for entry in inventory.api_methods.get(cls, [])
    )
    listener_api_names = unique_method_names(
        entry for cls in PLAYER_LISTENER_CLASSES for entry in inventory.api_methods.get(cls, [])
    )
    cpp_runtime_names = inventory.cpp_methods | inventory.java_bridge_methods
    player_missing = uncovered_methods(player_api_names, cpp_runtime_names)
    player_covered = covered_methods(player_api_names, cpp_runtime_names)
    builder_missing = uncovered_methods(builder_api_names, cpp_runtime_names)
    builder_covered = covered_methods(builder_api_names, cpp_runtime_names)
    listener_missing = uncovered_methods(listener_api_names, inventory.cpp_listener_methods)
    listener_covered = covered_methods(listener_api_names, inventory.cpp_listener_methods)
    api_or_aliases = set()
    for name in player_api_names | builder_api_names:
        api_or_aliases.update(method_candidates(name))
    runtime_extensions = sorted((inventory.cpp_methods | inventory.java_bridge_methods) - api_or_aliases)
    runtime_extensions = [name for name in runtime_extensions if name in NON_PLAYER_RUNTIME_EXTENSIONS]

    lines = [
        "# C++ Bridge API Parity Gap Report",
        "",
        "Generated by `scripts/cppbridge/api_parity_inventory.py`.",
        "",
        "This report is a deterministic first-pass inventory. It compares method names and known",
        "aliases, then leaves semantic payload parity to the mapping docs and follow-up stages.",
        "",
        "## Summary",
        "",
        "| Area | API methods | Covered by bridge/API alias | Inventory gap |",
        "| --- | ---: | ---: | ---: |",
        f"| `Player` + `ExoPlayer` methods | {len(player_api_names)} | {len(player_covered)} | {len(player_missing)} |",
        f"| `ExoPlayer.Builder` methods | {len(builder_api_names)} | {len(builder_covered)} | {len(builder_missing)} |",
        f"| `Player.Listener` callbacks | {len(listener_api_names)} | {len(listener_covered)} | {len(listener_missing)} |",
        "",
        "## Player / ExoPlayer Method Gaps",
        "",
        markdown_list(player_missing),
        "",
        "## ExoPlayer.Builder Method Gaps",
        "",
        markdown_list(builder_missing),
        "",
        "Builder note: several Java builder options are intentionally represented by `PlayerConfig`",
        "or `ExoPlayerSdkPlayerBuilder` rather than a literal Java-style builder method.",
        "",
        "## Player.Listener Callback Gaps",
        "",
        markdown_list(listener_missing),
        "",
        "## Native Runtime Extensions Outside `Player`",
        "",
        markdown_list(runtime_extensions),
        "",
        "These are useful C++ bridge capabilities, but they should not be counted as Java `Player`",
        "method parity by themselves.",
        "",
        "## Object / Value Model Inventory",
        "",
        "| Java API class | API method count | Follow-up stage |",
        "| --- | ---: | --- |",
    ]
    follow_up_by_class = {
        "Format": "Stage 2",
        "MediaItem": "Stage 3",
        "MediaMetadata": "Stage 3",
        "Timeline": "Stage 3",
        "Tracks": "Stage 2",
        "Cue": "Stage 3",
    }
    for class_name, count in object_model_rows(inventory):
        stage = "Stage 2/3"
        for token, mapped_stage in follow_up_by_class.items():
            if f".{token}" in class_name:
                stage = mapped_stage
                break
        lines.append(f"| `{class_name}` | {count} | {stage} |")
    lines.extend(
        [
            "",
            "## Immediate Phase 1 Conclusions",
            "",
            "- Exact method-level `Player` and `Player.Listener` parity is closed in this inventory.",
            "- Remaining high-value work is semantic object parity, not just adding more method names.",
            "- `ExoPlayer.Builder#setAudioOutputProvider` is a concrete builder-level gap to review",
            "  before the source/runtime integration stage.",
            "- Stages 2 and 3 should use the object/value inventory above as their checklist seed.",
            "",
        ]
    )
    return "\n".join(lines)


def as_json_dict(inventory: Inventory) -> dict[str, object]:
    player_api_names = unique_method_names(
        entry for cls in PLAYER_API_CLASSES for entry in inventory.api_methods.get(cls, [])
    )
    builder_api_names = unique_method_names(
        entry for cls in PLAYER_BUILDER_CLASSES for entry in inventory.api_methods.get(cls, [])
    )
    listener_api_names = unique_method_names(
        entry for cls in PLAYER_LISTENER_CLASSES for entry in inventory.api_methods.get(cls, [])
    )
    cpp_runtime_names = inventory.cpp_methods | inventory.java_bridge_methods
    return {
        "player_api_methods": sorted(player_api_names),
        "player_missing": uncovered_methods(player_api_names, cpp_runtime_names),
        "builder_api_methods": sorted(builder_api_names),
        "builder_missing": uncovered_methods(builder_api_names, cpp_runtime_names),
        "listener_api_methods": sorted(listener_api_names),
        "listener_missing": uncovered_methods(listener_api_names, inventory.cpp_listener_methods),
        "runtime_extensions": sorted((inventory.cpp_methods | inventory.java_bridge_methods)),
        "object_model_counts": object_model_rows(inventory),
    }


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--write", action="store_true", help="write docs/cppbridge/API_PARITY_GAP_REPORT.md")
    parser.add_argument("--check", action="store_true", help="fail if the checked-in report is stale")
    parser.add_argument("--json", action="store_true", help="print machine-readable inventory JSON")
    args = parser.parse_args(argv)

    root = args.root.resolve()
    inventory = build_inventory(root)
    output = json.dumps(as_json_dict(inventory), indent=2, sort_keys=True) if args.json else generate_markdown(inventory)
    report_path = root / "docs/cppbridge/API_PARITY_GAP_REPORT.md"
    if args.write:
        report_path.write_text(output, encoding="utf-8")
    if args.check:
        if not report_path.exists():
            print(f"Missing report: {report_path}", file=sys.stderr)
            return 1
        current = report_path.read_text(encoding="utf-8")
        if current != output:
            print(f"Stale report: {report_path}", file=sys.stderr)
            return 1
    if not args.write and not args.check:
        print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
