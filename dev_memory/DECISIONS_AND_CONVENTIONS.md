# Decisions And Conventions

## 1. Reduced endpoint first, full parity second

The repo is explicitly organized around a reduced bridge surface, not literal one-to-one Java API parity.

Meaning:

- `Done` in current docs often means "done for the reduced endpoint".
- It does not automatically mean "done for every Java object semantic in `api.txt`".

## 2. Smoke-driven closure

The project closes work through dedicated smoke tests and mapping docs, not just implementation presence.

The pattern is:

- add reduced C++ surface
- wire JNI bridge
- wire Java runtime
- add smoke
- document exact mapping and expected markers

## 3. Split JNI architecture

The repo no longer uses one giant JNI file as the main source of truth.

Current layout is split across:

- `exoplayer_cppbridge_jni_common.cpp`
- `exoplayer_cppbridge_jni_bridge.cpp`
- `exoplayer_cppbridge_jni_smoke_tests.cpp`
- `exoplayer_cppbridge_jni_player_tests.cpp`
- `exoplayer_cppbridge_jni_callbacks_demo.cpp`

If a future AI assumes everything is still in one file, it will waste time.

## 4. Opaque-token baseline instead of full arbitrary-object parity

Representative Java object identity is often preserved through opaque-token baselines rather than full deep object transport.

This appears in:

- media item tags
- ads id
- request metadata extras
- timeline window/period ids
- media metadata representative text/extras
- track labels and group ids
- cue text/bitmap baselines

This is an intentional compromise, not an accident.

## 5. Player message is reduced

`PlayerMessage` support is modeled as a reduced transport/request/result layer rather than exposing raw Java `PlayerMessage` directly.

Key conventions:

- request model: `PlayerMessageDescriptor`
- result model: `PlayerMessageResult`
- supports reduced target selection and smokeable outcomes
- not equal to arbitrary Java target parity

## 6. Validation docs matter as much as code

`VALIDATION_GUIDE.md`, `TEST_RESULTS_TEMPLATE.md`, and `VALIDATION_RESULTS_SUMMARY.md` are part of the delivery model.

If future work changes coverage or runtime expectations:

- update mapping docs
- update expected smoke markers
- update validation summary/template if needed

## 7. Current documentation system is table-heavy by design

The current workspace expects maintainers to use:

- quick reference docs for fast lookup
- mapping status docs for implementation routing
- file map / knowledge graph for onboarding

The next AI should lean on those instead of rebuilding a mental index from scratch.
