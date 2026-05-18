# Development Plan

Date: 2026-05-18

Goal: keep moving `exoplayer_cppbridge` from a broad, smoke-validated reduced bridge toward fuller
Java API and object parity, without losing the existing validation discipline.

## Operating Rules

- Each stage ends with implementation, focused UT or instrumentation coverage, docs updates, and
  self-review.
- Prefer automated inventory over manual memory when comparing Java API, Java bridge, C++ SDK, and
  JNI bridge surfaces.
- Keep reduced endpoint status separate from full Java `api.txt` parity. A reduced row can be
  smoke-complete while still having full-object parity gaps.
- Do not submit generated/local output such as `artifacts/`.

## Stage 1: Full API Gap Inventory

Status: completed on 2026-05-18.

Objective:

- Generate a repeatable crosswalk from `api.txt`, `CppExoPlayerBridge.java`,
  `exoplayer_sdk.h`, and `exoplayer_bridge.h`.
- Produce a checked-in gap report that separates exact method parity, callback parity, builder
  parity, and object/value-model parity.
- Use the report to choose the concrete implementation order for later stages.

Expected outputs:

- `scripts/cppbridge/api_parity_inventory.py`
- unit tests for the inventory parser
- `docs/cppbridge/API_PARITY_GAP_REPORT.md`
- index/file-map/dev-memory updates
- direct `Player.Listener#onIsLoadingChanged` bridge gap closed with smoke coverage

Exit criteria:

- The inventory script can regenerate the checked-in report.
- Unit tests for the parser pass.
- Existing cppbridge unit tests still pass.

## Stage 2: Format / Tracks Full-Object Payload

Status: completed on 2026-05-18.

Objective:

- Move beyond scalar `TrackInfo` summaries into richer `Format` payload transfer.
- Cover metadata entries, initialization data bytes, DRM scheme data shape, custom data/token
  semantics, and nested `ColorInfo`/HDR fields where practical.

Exit criteria:

- Java converter, JNI create/parse, C++ structs, and native smokes prove round-trip behavior.
- Docs clearly distinguish transferred value fields from token-only or intentionally omitted
  object semantics.

Completed outputs:

- `CppTrackInfo` / `TrackInfo` now preserve richer `Format` payloads: label arrays, metadata and
  custom-data opaque tokens, auxiliary track type, initialization-data byte arrays, projection
  bytes, DRM scheme type / scheme data uuid / license URL / MIME / bytes / has-data, and
  `ColorInfo` HDR static info plus luma/chroma bitdepth.
- `CppBridgeConvertersTest.toCppTrackGroups_mapsSelectionAndSupport` validates Java converter
  coverage.
- `CppBridgeNativeSmokeTest.nativeTracksFullPayloadConversionSmokeTest_roundTripsFormatPayload`
  validates JNI create/parse round-trip coverage.
- `CppBridgeNativePlayerInstrumentationTest.nativeCurrentTracksSmokeTest_returnsTracksSummary`
  keeps query-smoke visibility for the new representative markers.

## Stage 3: MediaItem / Timeline / MediaMetadata Object Parity

Objective:

- Deepen `MediaItem`, `Timeline.Window`, `Timeline.Period`, and `MediaMetadata` beyond current
  representative fields.
- Resolve whether tag, ads id, extras, manifest, uid, and period id remain opaque-token semantics
  or become decoded value models.

Exit criteria:

- New or expanded smoke tests cover the decided semantics.
- Mapping docs call out any remaining deliberate non-parity.

Current progress:

- 2026-05-18: first Stage 3 slice adds a stable decoded `Bundle` value model for
  `MediaItem.RequestMetadata.extras` and `MediaMetadata.extras`. The C++ bridge now transports
  string, integer-like, floating-point, boolean, and byte-array entries through `BundleValueInfo` /
  `CppBundleValue`, while preserving opaque-token fallback for unsupported arbitrary Java values.
- Coverage includes Java converter UTs plus native current-item and playlist-metadata smoke
  assertions for decoded extras values.

## Stage 4: Listener / Analytics Completeness

Objective:

- Audit `Player.Listener`, analytics callbacks, and auxiliary ExoPlayer callbacks against the
  generated gap report.
- Fill remaining callback payload and lifecycle gaps by risk order.

Exit criteria:

- Listener callback table has no unreviewed gaps.
- Tests cover add/remove lifecycle, multi-listener behavior where supported, and representative
  payload fields.

## Stage 5: Playback Source / Runtime Integration

Objective:

- Extend current HTTP/HLS/DASH coverage toward SmoothStreaming, RTSP, custom media/data source
  configuration, cache/DRM integration, and source-factory behavior useful for RPI4 deployment.

Exit criteria:

- Local or deterministic instrumentation smokes cover each supported source family.
- Demo and docs explain what is bridge-owned versus app/platform-owned.

## Stage 6: SDK Stabilization And Delivery

Objective:

- Stabilize public C++ headers, error behavior, token lifetime rules, validation scripts, demo
  flows, and RPI4 deployment notes.

Exit criteria:

- Full validation guide is current.
- Public header/API changes are intentional and reviewed.
- Final PR/branch has clean docs, tests, and handoff memory.
