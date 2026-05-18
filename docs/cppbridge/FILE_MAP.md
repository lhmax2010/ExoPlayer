# File Map

Last updated: 2026-05-18

This is the current file-to-responsibility map for the C++ bridge work. Use it to find the right
place to read or modify code.

## 0. Fast Lookup Rules

Use these rules when a mapping document names an API or data type and you want to jump straight to
the implementation instead of reading the whole bridge.

If you have a C++ public API name such as `SetMediaItem`, `GetTracks`, or `SetSeekParameters`:

1. start in `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_sdk.h`
2. jump to the matching `JniExoPlayerBridge` method in
   `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_bridge.cpp`
3. then find the matching Java runtime method in
   `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java`

If you have a reduced C++ struct such as `MediaItemDescriptor`, `TracksSnapshot`, or
`MediaMetadataSnapshot`:

1. start in `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_bridge.h`
2. find the JNI conversion helper in
   `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_common.cpp`
3. then find the Java transport DTO or converter in
   `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/Cpp*.java`
   or `CppBridgeConverters.java`

If you have a Java DTO name such as `CppMediaItem`, `CppMediaMetadata`, or `CppTrackSelectionParameters`:

1. start in `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge`
2. find the converter method in `CppBridgeConverters.java`
3. then find the JNI create/parse helper in
   `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_common.cpp`

Special current exception:

- `CppCodecParameter.java` is created from C++ by
  `JniExoPlayerBridge::CreateJavaCodecParameterArray` in
  `exoplayer_cppbridge_jni_bridge.cpp`, because it is only used for C++ to Java codec parameter
  setter transport.
- `CppBundleValue.java` is shared by `MediaItem.RequestMetadata.extras` and
  `MediaMetadata.extras`; its C++ partner is `BundleValueInfo`, and the JNI helpers are
  `CreateJavaBundleValueArray` / `FromJavaBundleValueArray` in
  `exoplayer_cppbridge_jni_common.cpp`.
- `CppObjectValue.java` is the reusable Java DTO for reduced Java `Object` metadata when an object
  travels through a DTO path. It is currently used by `MediaItem` tag/adsId and representative
  `MediaMetadata` text/`CharSequence` fields. Its C++ partner is `ObjectValueInfo`, and the JNI
  helpers are `CreateJavaObjectValueInfo` / `FromJavaObjectValueInfo` in
  `exoplayer_cppbridge_jni_common.cpp`.
- Timeline window/period Java object identity fields use row-string transport rather than a Java
  DTO. Their C++ partner is `ObjectValueInfo`, parsed by `ParseObjectValueInfo` in
  `exoplayer_cppbridge_jni_bridge.cpp`.

If you have a callback name such as `onTracksChanged`, `onMediaMetadataChanged`, or
`onDroppedVideoFrames`:

1. start in `CppExoPlayerBridge.java`
2. find the paired `nativeOn*` method call
3. jump to the matching JNI callback entry in
   `exoplayer_cppbridge_jni_bridge.cpp`
4. if needed, continue into listener forwarding in `exoplayer_sdk.cpp`

If you are checking full Java API parity:

1. read `docs/cppbridge/API_PARITY_GAP_REPORT.md`
2. regenerate it with `python3 scripts/cppbridge/api_parity_inventory.py --write`
3. verify it is current with `python3 scripts/cppbridge/api_parity_inventory.py --check`

### Common Search Patterns

These are the fastest exact-text searches when you already know what kind of symbol you have.

| If you have... | Search pattern |
| --- | --- |
| C++ public method name | `ExoPlayerSdkPlayer::MethodName(` or just `MethodName(` in `include/exoplayer_sdk.h` |
| bridge runtime method | `JniExoPlayerBridge::MethodName(` |
| Java runtime method | `methodName(` in `CppExoPlayerBridge.java` |
| Java DTO class | `class CppName` |
| C++ reduced struct | `struct Name` in `include/exoplayer_bridge.h` |
| JNI Java-to-C++ conversion | `FromJavaName(` |
| JNI C++-to-Java conversion | `CreateJavaName(` |
| listener callback from Java into JNI | `Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOn...` |
| listener callback from JNI into C++ listener | `BridgeOn...` or `On...Changed(` in `exoplayer_sdk.cpp` |
| analytics concrete test hook | `SimulateAnalytics...ForTest(` |

## 1. Where To Start

If you need the public native API shape:

- `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_sdk.h`
- `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_bridge.h`

If you need the Java bridge runtime:

- `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java`
- `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBridgeConverters.java`

If you need JNI implementation:

- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_bridge.cpp`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_common.cpp`
- `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_callbacks_demo.cpp`

## 2. Native File Responsibilities

| File | Responsibility | How to read it |
| --- | --- | --- |
| `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_bridge.h` | reduced bridge interfaces, DTOs, listener contracts | read first to understand model and API surface |
| `libraries/exoplayer_cppbridge/src/main/jni/include/exoplayer_sdk.h` | public native SDK wrapper API used by tests/demo/native callers | read after `exoplayer_bridge.h` |
| `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_sdk.cpp` | concrete SDK wrapper around bridge object, env attachment, listener forwarding | read for public runtime behavior |
| `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_internal.h` | shared internal declarations across split JNI files | read only after high-level flow is clear |
| `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_common.cpp` | shared JNI helper layer, DTO parsing/creation, registry management, logging | read when changing DTO fields or handle lifecycle |
| `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_playlist_helpers.cpp` | helper summaries used by smoke/demo outputs | read when summary format changes |
| `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_bridge.cpp` | `JniExoPlayerBridge` implementation, Java method calls, callback hardening | read for runtime bridge logic |
| `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_callbacks_demo.cpp` | callback JNI entrypoints and demo JNI functions | read for JNI boundary entry flow |
| `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_smoke_tests.cpp` | JNI/value conversion smoke helpers | read when updating DTO validation |
| `libraries/exoplayer_cppbridge/src/main/jni/exoplayer_cppbridge_jni_player_tests.cpp` | player/runtime smoke helpers | read when updating runtime parity or smoke summaries |
| `libraries/exoplayer_cppbridge/src/main/jni/CMakeLists.txt` | native translation unit wiring | read when adding/removing JNI files |

## 3. Java File Responsibilities

| File | Responsibility | How to read it |
| --- | --- | --- |
| `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge.java` | Java runtime bridge, Media3 player ownership, JNI callback emission | first Java runtime file to inspect |
| `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBridgeConverters.java` | Java <-> reduced DTO conversion rules | inspect whenever value parity changes |
| `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativeSmokeTestHelper.java` | Java loader for JNI/value smoke helpers | maps directly to `exoplayer_cppbridge_jni_smoke_tests.cpp` |
| `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerTestHelper.java` | Java loader for player/runtime smoke helpers | maps directly to `exoplayer_cppbridge_jni_player_tests.cpp` |
| `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/Cpp*.java` DTO files | Java transport/value objects used across JNI | inspect when adding/removing fields |
| `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppCodecParameter.java` | Java transport object for typed `CodecParameters` entries | inspect with `CodecParameterDescriptor`, `CodecParametersDescriptor`, and `CreateJavaCodecParameterArray` |
| `libraries/exoplayer_cppbridge/src/main/java/androidx/media3/exoplayer/cppbridge/CppBundleValue.java` | Java transport object for decoded stable `Bundle` extras entries | inspect with `BundleValueInfo`, `MediaItem.RequestMetadata`, `MediaMetadataSnapshot`, and `CreateJavaBundleValueArray` |

## 4. Validation And Demo Files

| File | Responsibility | How to read it |
| --- | --- | --- |
| `libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativeSmokeTest.java` | asserts JNI/value smoke outputs, including full `TrackInfo` payload round-trip markers | read for low-level bridge expectations |
| `libraries/exoplayer_cppbridge/src/androidTest/java/androidx/media3/exoplayer/cppbridge/CppBridgeNativePlayerInstrumentationTest.java` | asserts player/runtime smoke outputs | read for functional bridge expectations |
| `demos/cppbridge/src/main/java/androidx/media3/demo/cppbridge/MainActivity.java` | manual end-to-end demo using C++ bridge APIs | read for demo workflow and manual QA |
| `scripts/cppbridge/run_validation.sh` | one-command Linux validation runner | use first on a healthy machine |
| `scripts/cppbridge/run_validation.py` | Python validation runner for Linux environments | use when Python entrypoint is preferred |
| `scripts/cppbridge/api_parity_inventory.py` | deterministic API parity inventory and report generator | run after changing public Java/C++ bridge surfaces |
| `scripts/cppbridge/launch_demo.sh` | Linux demo install/launch helper | use for manual demo QA |
| `scripts/cppbridge/launch_demo.py` | Python demo install/launch helper | use when Python entrypoint is preferred |
| `docs/cppbridge/API_PARITY_GAP_REPORT.md` | generated method/callback/builder/object parity report | read before choosing the next full-parity slice |
| `docs/cppbridge/API_MAPPING_QUICK_REFERENCE.md` | direct lookup sheet in `Java API / C++ API / JNI API / SmokeTest / 功能描述` format | use first when you already know the API name |
| `docs/cppbridge/DATA_STRUCTURE_QUICK_REFERENCE.md` | direct lookup sheet in `Java 数据结构 / C++ 数据结构 / JNI Create API / JNI Parse API / SmokeTest / 功能描述` format | use first when you already know the DTO or reduced struct name |
| `docs/cppbridge/TEST_RESULTS_TEMPLATE.md` | validation result write-back template | fill after running in the new environment |
| `docs/cppbridge/VALIDATION_RESULTS_SUMMARY.md` | final validation rollup page | summarize the final pass/fail decision after filling the raw template |

## 5. Reading Order By Task

### Add a new reduced DTO field

1. `exoplayer_bridge.h`
2. Java `Cpp*` DTO class
3. `CppBridgeConverters.java`
4. `exoplayer_cppbridge_jni_common.cpp`
5. `exoplayer_cppbridge_jni_bridge.cpp`
6. smoke helper `.cpp`
7. instrumentation test `.java`

### Fix runtime callback/release bug

1. `exoplayer_sdk.cpp`
2. `exoplayer_cppbridge_jni_bridge.cpp`
3. `exoplayer_cppbridge_jni_callbacks_demo.cpp`
4. player instrumentation test

### Fix demo behavior

1. `demos/cppbridge/.../MainActivity.java`
2. `exoplayer_cppbridge_jni_callbacks_demo.cpp`
3. `exoplayer_sdk.cpp`

### Add scalar runtime setter/getter parity

1. `include/exoplayer_bridge.h`
2. `include/exoplayer_sdk.h`
3. `CppExoPlayerBridge.java`
4. `exoplayer_cppbridge_jni_bridge.cpp`
5. `exoplayer_sdk.cpp`
6. `CppBridgeNativePlayerTestHelper.java`
7. `exoplayer_cppbridge_jni_player_tests.cpp`
8. `CppBridgeNativePlayerInstrumentationTest.java`

### Add callback-style ExoPlayer parity

1. Java listener registration/lifecycle in `CppExoPlayerBridge.java`
2. native callback contracts in `include/exoplayer_bridge.h`
3. SDK listener forwarding in `exoplayer_sdk.cpp`
4. JNI callback entrypoints in `exoplayer_cppbridge_jni_bridge.cpp` or
   `exoplayer_cppbridge_jni_callbacks_demo.cpp`
5. targeted instrumentation tests before broadening the surface

## 6. Current File Size Notes

Still above the original preferred threshold:

- `exoplayer_cppbridge_jni_common.cpp`
- `exoplayer_cppbridge_jni_bridge.cpp`

Current judgment:

- structure is acceptable for continued work
- further splitting is optional, not a blocker
