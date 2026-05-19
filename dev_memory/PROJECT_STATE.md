# Project State

## Snapshot

- Workspace: `/home/linhao/Toolchain/development/ExoPlayer`
- Date of this handoff memory: `2026-05-19`
- Module focus: `libraries/exoplayer_cppbridge`
- Goal: Java-side ExoPlayer usage replaced by a reduced but usable C++ bridge/SDK surface, with smoke coverage and validation docs.
- Latest Stage5 work: custom-cache-key and DRM playback-path preservation completed on
  `codex/cppbridge-stage5-complete`, after PR #1 was merged into `main`; Android 16
  validation is now green for the reduced endpoint.

## Current status from repo docs

According to `docs/cppbridge/API_MAPPING_STATUS.md`:

- `Done`: `99`
- `Partial`: `0`
- `Not started`: `0`

Important nuance:

- These numbers describe the currently exposed reduced endpoint tracker.
- They do not mean full `api.txt` parity is complete.
- Remaining work lives in the "next-phase capability gaps" and "full parity" sections of the docs.

## Validation status

According to `docs/cppbridge/VALIDATION_RESULTS_SUMMARY.md`:

- Build/native link: `Pass`
- JNI/value smoke: `Pass`
- Player/runtime smoke: `Pass`
- Demo manual validation: `Pending`
- Logcat review: `Pending`
- Release recommendation: `Pass with notes`

Interpretation:

- The codebase and docs now have automated validation evidence for the reduced endpoint on
  `cppbridge_android16_api36` (`emulator-5554`).
- Demo manual validation and explicit logcat review remain separate pending checks.
- Full Java `api.txt` parity is still not claimed; next-phase capability gaps remain tracked
  in the docs.

## Smoke footprint

Current androidTest count found in the workspace:

- Total `@Test` count across `CppBridgeNativeSmokeTest.java` and `CppBridgeNativePlayerInstrumentationTest.java`: `118`

This is consistent with a large smoke-first validation strategy.

## High-value implemented areas visible in current workspace

- Core playback lifecycle and seek/navigation
- Playlist mutation and query APIs
- Track selection parameters, tracks snapshots, timeline snapshots, cue snapshots
- Media metadata and playlist metadata round-trip
- Rich analytics reduced coverage, including many concrete event callbacks
- Image output reduced callback path
- PriorityTaskManager reduced wrapper path
- Preload reduced target-duration path
- Player message reduced send/cancel/runtime smoke path
- Opaque token baseline support for representative object identity fields
- Stage5 source integration: `MediaItem.customCacheKey`, progressive source-type inference,
  token-injected fake-source playback smoke for custom cache key and DRM descriptor preservation

## Practical conclusion

For the next AI:

- Treat this project as "reduced endpoint is broad and heavily smoke-documented".
- Treat reduced-endpoint runtime confidence as validated on Android 16 emulator.
- The highest-risk work is no longer basic API exposure; it is preserving behavior while expanding full parity or doing demo/manual release validation.
