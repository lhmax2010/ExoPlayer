cmake_minimum_required(VERSION 3.21.0)

project(exoplayer_cppbridge_jni)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(
    EXOPLAYER_CPPBRIDGE_INCLUDE_TEST_ENTRYPOINTS
    "Build smoke test and demo JNI entrypoints into the shared library"
    ON)

set(EXOPLAYER_CPPBRIDGE_CORE_SOURCES
    exoplayer_cppbridge_jni_common.cpp
    exoplayer_cppbridge_jni_playlist_helpers.cpp
    exoplayer_cppbridge_jni_bridge.cpp
    exoplayer_cppbridge_jni_callbacks_demo.cpp
    exoplayer_sdk.cpp)

if (EXOPLAYER_CPPBRIDGE_INCLUDE_TEST_ENTRYPOINTS)
  list(APPEND EXOPLAYER_CPPBRIDGE_CORE_SOURCES
      exoplayer_cppbridge_jni_player_tests.cpp)
endif()

add_library(
    exoplayer_cppbridge_jni
    SHARED
    ${EXOPLAYER_CPPBRIDGE_CORE_SOURCES})

target_include_directories(
    exoplayer_cppbridge_jni
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_library(log-lib log)

target_link_libraries(
    exoplayer_cppbridge_jni
    ${log-lib})

if (EXOPLAYER_CPPBRIDGE_INCLUDE_TEST_ENTRYPOINTS)
  add_library(
      exoplayer_cppbridge_jni_testhooks
      SHARED
      exoplayer_cppbridge_jni_smoke_tests.cpp)

  target_include_directories(
      exoplayer_cppbridge_jni_testhooks
      PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include)

  target_link_libraries(
      exoplayer_cppbridge_jni_testhooks
      exoplayer_cppbridge_jni
      ${log-lib})
endif()
