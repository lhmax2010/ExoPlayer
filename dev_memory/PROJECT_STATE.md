# Project State

## Snapshot

- Workspace: `c:\Users\hao.lin\Downloads\media-release`
- Date of this handoff memory: `2026-05-15`
- Module focus: `libraries/exoplayer_cppbridge`
- Goal: Java-side ExoPlayer usage replaced by a reduced but usable C++ bridge/SDK surface, with smoke coverage and validation docs.

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

- Build/native link: `Pending`
- JNI/value smoke: `Pending`
- Player/runtime smoke: `Pending`
- Demo manual validation: `Pending`
- Logcat review: `Pending`
- Release recommendation: `Pending`

Interpretation:

- The codebase and docs claim very broad reduced coverage.
- The validation summary still treats final environment validation as not yet closed.
- Do not assume everything is runtime-verified on this machine just because mapping docs are detailed.

## Smoke footprint

Current androidTest count found in the workspace:

- Total `@Test` count across `CppBridgeNativeSmokeTest.java` and `CppBridgeNativePlayerInstrumentationTest.java`: `116`

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

## Practical conclusion

For the next AI:

- Treat this project as "reduced endpoint is broad and heavily smoke-documented".
- Treat final runtime confidence as "still requires environment validation".
- The highest-risk work is no longer basic API exposure; it is preserving behavior while expanding full parity or validating on a healthy Android build/device setup.
