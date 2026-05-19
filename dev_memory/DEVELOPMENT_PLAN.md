# Development Plan

Date: 2026-05-19

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
- 2026-05-18: second Stage 3 slice adds reduced `ObjectValueInfo` metadata for timeline object
  identity fields: `Timeline.Window.uid`, `Timeline.Window.manifest`, `Timeline.Period.id`,
  `Timeline.Period.uid`, and `Timeline.Period.adsId`. The bridge now distinguishes null, class
  name, reduced value type, and stable string/number/boolean payloads while keeping existing
  opaque-token baselines.
- Coverage includes a native `ObjectValueInfo` parser smoke for string/long/double/boolean/null/
  other/truncated rows, expanded native current-timeline assertions, and query-smoke markers for
  real runtime timeline rows.
- 2026-05-18: third Stage 3 slice reuses the reduced object-value model for
  `MediaItem.LocalConfiguration.tag` and `MediaItem.AdsConfiguration.adsId`. The bridge now
  carries `ObjectValueInfo`-style metadata through Java `CppObjectValue`, C++
  `MediaItemDescriptor.tag_value`, and `AdsConfigurationDescriptor.ads_id_value`, while preserving
  existing string fallback and opaque-token object identity.
- Coverage includes Java converter UTs for C++-to-Java scalar fallback and Java-to-C++ metadata
  extraction, a JNI media-item object-value DTO round-trip smoke, and expanded runtime assertions
  for current item, indexed item, and opaque-token media-item flows.
- 2026-05-18: fourth Stage 3 slice adds reduced `ObjectValueInfo` metadata for representative
  `MediaMetadata` text/`CharSequence` fields: title, artist, album title/artist, display title,
  subtitle, description, writer, author, composer, conductor, genre, compilation, and station.
  Java `CppMediaMetadata` now carries `CppObjectValue` fields, the C++ `MediaMetadataSnapshot`
  mirrors them, and JNI create/parse paths round-trip the reduced class/type/scalar payload while
  preserving token-first and string fallback behavior.
- Coverage includes converter UTs for all metadata text object values plus scalar C++ fallback,
  a JNI media-metadata object-value DTO round-trip smoke, expanded current-item / playlist-metadata
  runtime assertions, and listener metadata object-value markers. Full Android 16 connected
  validation passed with `128/128` tests.
- Stage 3 reduced object/value-model work is complete for the planned slices. Remaining
  `MediaItem`, `Timeline`, and `MediaMetadata` gaps are full Java object-graph parity beyond the
  reduced descriptors and should be tracked as later-stage full-support work.

## Stage 4: Listener / Analytics Completeness

Objective:

- Audit `Player.Listener`, analytics callbacks, and auxiliary ExoPlayer callbacks against the
  generated gap report.
- Fill remaining callback payload and lifecycle gaps by risk order.

Exit criteria:

- Listener callback table has no unreviewed gaps.
- Tests cover add/remove lifecycle, multi-listener behavior where supported, and representative
  payload fields.

Current progress:

- 2026-05-18: first Stage 4 slice adds an independent reduced
  `AnalyticsListener#onAudioAttributesChanged` C++ callback:
  `OnAnalyticsAudioAttributesChanged` carries `AudioAttributesDescriptor` content type, usage,
  flags, allowed-capture policy, and spatialization behavior. Java dispatch, JNI entrypoint,
  bridge forwarding, SDK forwarding, and test simulation APIs are wired end to end.
- Coverage: `nativeAnalyticsAudioAttributesChangedSmokeTest_reportsConcreteAnalyticsEvent`
  validates multi-update last-value behavior plus remove-listener stop delivery on Android 16.
- 2026-05-18: Stage 4 remaining `AnalyticsListener` method-name coverage is complete for the
  reduced bridge surface. The bridge now exposes C++ callbacks for the remaining Java analytics
  events: player-state/loading aliases, track-selection parameters, load canceled, downstream /
  upstream format events, decoder-counters enabled/disabled events, audio/video codec and sink
  errors, audio-track init/release, surface size, DRM lifecycle/key events, renderer ready,
  dropped seeks while scrubbing, and player released.
- Coverage: `nativeAnalyticsStage4RemainingCallbacksSmokeTest_reportsConcreteAnalyticsEvents`
  validates 25 newly bridged callbacks, representative payload fields, and remove-listener stop
  delivery on Android 16. The Stage 4 reduced callback table has no unreviewed Java
  `AnalyticsListener` method-name gaps; remaining analytics work is richer/full-object payload
  fidelity beyond the reduced descriptors.

## Stage 5: Playback Source / Runtime Integration

Objective:

- Extend current HTTP/HLS/DASH coverage toward SmoothStreaming, RTSP, custom media/data source
  configuration, cache/DRM integration, and source-factory behavior useful for RPI4 deployment.

Exit criteria:

- Local or deterministic instrumentation smokes cover each supported source family.
- Demo and docs explain what is bridge-owned versus app/platform-owned.

Current progress:

- 2026-05-19: first Stage 5 slice adds
  `nativeCustomMediaSourceFactoryPlaybackSmokeTest_preparesSmoothAndRtspViaCppConfig`. The test
  registers a Java `FakeMediaSourceFactory`, selects it through C++ `PlayerConfig` factory-token
  injection, then prepares SmoothStreaming and RTSP `MediaItemDescriptor` source types through the
  C++ `SetMediaItem` / `Prepare` / `Play` path. This validates the custom factory integration path
  for source families that need app/platform-owned media-source handling.
- 2026-05-19: second Stage 5 slice adds
  `nativeHttpDataSourceConfigPlaybackSmokeTest_sendsHeadersThroughCppConfig`. The test configures
  HTTP headers, user agent, timeouts, and redirect behavior from C++, plays a local progressive
  stream, and verifies MockWebServer received the expected request headers and UA.

## Stage 6: SDK Stabilization And Delivery

Objective:

- Stabilize public C++ headers, error behavior, token lifetime rules, validation scripts, demo
  flows, and RPI4 deployment notes.

Exit criteria:

- Full validation guide is current.
- Public header/API changes are intentional and reviewed.
- Final PR/branch has clean docs, tests, and handoff memory.
