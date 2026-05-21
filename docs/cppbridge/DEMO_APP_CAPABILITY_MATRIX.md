# CppBridge Demo App Capability Matrix

Last updated: 2026-05-19

## Implemented In `demos/cppbridge`

| Area | Status | Notes |
| --- | --- | --- |
| HTTP progressive playback | Supported | Demo exposes a preset HTTP MP4 sample through C++ APIs and attaches two demo sidecar subtitles for text-track switching. Audio switching is available when the HTTP asset contains multiple audio tracks. |
| DASH playback | Supported | Demo preset now uses a DASH stream with multiple text tracks and multiple audio adaptation sets, then also attaches demo sidecar subtitles through C++ APIs. |
| HLS playback | Supported | Demo preset now uses an Apple advanced multivariant HLS stream with alternate audio groups and subtitle metadata, then also attaches demo sidecar subtitles through C++ APIs. |
| USB / local file playback | Supported with URI handoff | Android Java picks a `content://` or `file://` URI, then playback is started through the C++ player API with demo sidecar subtitles attached. Audio switching works for local files that contain multiple audio tracks. |
| Video rendering | Supported | Demo binds `PlayerView` through `ExoPlayerSdkPlayer::BindPlayerView`. |
| Audio playback | Supported | Standard playback path uses the C++ bridge-backed player. |
| Play / Pause / Stop | Supported | Wired to `Play`, `Pause`, and `Stop`. |
| Seek to custom position | Supported | Wired to `SeekTo`. |
| Seek back / forward increments | Supported | Wired to `SeekBack` and `SeekForward`. |
| Previous / next media item | API available, not exposed in demo UI | C++ SDK/JNI support exists, but the compact demo UI currently exposes seek back/forward rather than playlist previous/next controls. |
| Trick play by speed | Supported | Demo exposes `0.5x`, `1.0x`, `1.5x`, and `2.0x` through `SetPlaybackSpeed`. |
| External subtitle loading | Supported | Demo exposes built-in sidecar subtitles for every HTTP/DASH/HLS/file load plus a `Sub File` picker that reloads the current media with an external subtitle URI. |
| Preferred text language | Supported | Wired to `SetTrackSelectionParameters`. |
| Audio track switching | Supported | Demo cycles audio tracks via `TrackSelectionParametersDescriptor::OverrideDescriptor`. |
| Text track switching | Supported | Demo cycles text tracks via track selection overrides and can disable text after the last track. |
| Playback state inspection | Supported | Demo exposes playback, position, duration, buffering, item count, and selected track state summaries. |
| Stream / track info inspection | Supported | Demo exposes track, current item, timeline, metadata, and cues summaries. |
| Mixed playlist demo | Supported | Demo can load a mixed HTTP + DASH + HLS playlist through C++ APIs. |

## Known Gaps

These are the main items that are not fully solved by the current demo or are not purely C++ API concerns.

| Gap | Type | Notes |
| --- | --- | --- |
| USB device discovery and browsing | Platform gap | Android device enumeration, mount state, and document picking are platform/UI responsibilities. The C++ bridge consumes the final URI but does not discover USB devices by itself. |
| Manual track picker UI | Demo gap | The C++ API supports track overrides, but the demo currently provides cycle-based audio/text switching rather than a full selectable list UI. |
| Bitrate / resolution pinning UI | Demo gap | The C++ API can carry overrides and track constraints, but the demo does not yet expose a dedicated video quality picker. |
| Frame-by-frame trick play | API gap | Current public C++ player controls cover seek navigation and playback speed changes, but not single-frame stepping semantics. |
| Local DASH / HLS folder playback from USB | Platform/integration gap | Picking one manifest URI is easy; resolving a whole relative-segment tree from SAF-backed storage may need additional URI mapping or a custom data source path. |
| Rich stream diagnostics panel | Demo gap | The current demo exposes summaries, but not a structured inspector UI for every field in `TracksSnapshot`, `PlaybackSnapshot`, `TimelineDetailsSnapshot`, and analytics snapshots. |

## Recommended Next Steps

1. Add a full track picker dialog backed by `GetTracks()` so audio, text, and video selections can be made explicitly instead of cycling.
2. Add a quality selection panel that maps supported video groups to `TrackSelectionParametersDescriptor::OverrideDescriptor`.
3. Add a USB/document browser flow if local content needs folder-level navigation rather than single-file picking.
4. Add optional analytics and error panels if the demo is going to be used as an integration validation app rather than only a playback sample.
