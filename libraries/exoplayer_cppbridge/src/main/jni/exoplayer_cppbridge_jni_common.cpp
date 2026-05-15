#include "exoplayer_cppbridge_jni_internal.h"

#include <android/log.h>

#include <algorithm>
#include <cstring>
#include <exception>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace androidx::media3::cppbridge::internal {

constexpr char kLogTag[] = "ExoCppBridge";

std::string JStringToString(JNIEnv* env, jstring value);

class JniExoPlayerBridge;

std::mutex& GetBridgeRegistryMutex() {
  static std::mutex mutex;
  return mutex;
}

std::unordered_map<jlong, std::shared_ptr<JniExoPlayerBridge>>& GetBridgeRegistry() {
  static auto* registry =
      new std::unordered_map<jlong, std::shared_ptr<JniExoPlayerBridge>>();
  return *registry;
}

std::mutex& GetDemoPlayerRegistryMutex() {
  static std::mutex mutex;
  return mutex;
}

std::unordered_map<jlong, ExoPlayerSdkPlayer*>& GetDemoPlayerRegistry() {
  static auto* registry = new std::unordered_map<jlong, ExoPlayerSdkPlayer*>();
  return *registry;
}

void RegisterBridge(const std::shared_ptr<JniExoPlayerBridge>& bridge) {
  std::lock_guard<std::mutex> lock(GetBridgeRegistryMutex());
  GetBridgeRegistry()[reinterpret_cast<jlong>(bridge.get())] = bridge;
}

std::shared_ptr<JniExoPlayerBridge> AcquireBridge(jlong native_handle) {
  if (native_handle == 0) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(GetBridgeRegistryMutex());
  auto it = GetBridgeRegistry().find(native_handle);
  return it != GetBridgeRegistry().end() ? it->second : nullptr;
}

void UnregisterBridge(const JniExoPlayerBridge* bridge) {
  std::lock_guard<std::mutex> lock(GetBridgeRegistryMutex());
  GetBridgeRegistry().erase(reinterpret_cast<jlong>(bridge));
}

void RegisterDemoPlayer(ExoPlayerSdkPlayer* player) {
  if (player == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(GetDemoPlayerRegistryMutex());
  GetDemoPlayerRegistry()[reinterpret_cast<jlong>(player)] = player;
}

ExoPlayerSdkPlayer* AcquireDemoPlayer(jlong native_handle) {
  if (native_handle == 0) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(GetDemoPlayerRegistryMutex());
  auto it = GetDemoPlayerRegistry().find(native_handle);
  return it != GetDemoPlayerRegistry().end() ? it->second : nullptr;
}

void UnregisterDemoPlayer(ExoPlayerSdkPlayer* player) {
  if (player == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(GetDemoPlayerRegistryMutex());
  GetDemoPlayerRegistry().erase(reinterpret_cast<jlong>(player));
}

void LogInfo(const std::string& message) {
  __android_log_print(ANDROID_LOG_INFO, kLogTag, "%s", message.c_str());
}

void LogError(const std::string& message) {
  __android_log_print(ANDROID_LOG_ERROR, kLogTag, "%s", message.c_str());
}

ScopedEnv::ScopedEnv(
    JavaVM* java_vm,
    const char* get_env_error_context,
    const char* attach_error_context)
    : java_vm_(java_vm) {
  if (java_vm_ == nullptr) {
    return;
  }
  void* raw_env = nullptr;
  jint get_env_result = java_vm_->GetEnv(&raw_env, JNI_VERSION_1_6);
  if (get_env_result == JNI_OK) {
    env_ = static_cast<JNIEnv*>(raw_env);
    return;
  }
  if (get_env_result != JNI_EDETACHED) {
    if (get_env_error_context != nullptr) {
      LogError(get_env_error_context);
    }
    return;
  }
  if (java_vm_->AttachCurrentThread(&env_, nullptr) == JNI_OK) {
    attached_by_scope_ = true;
    return;
  }
  env_ = nullptr;
  if (attach_error_context != nullptr) {
    LogError(attach_error_context);
  }
}

ScopedEnv::~ScopedEnv() {
  if (attached_by_scope_ && java_vm_ != nullptr) {
    java_vm_->DetachCurrentThread();
  }
}

bool ClearJniExceptionIfPresent(JNIEnv* env, const std::string& context) {
  if (!env->ExceptionCheck()) {
    return false;
  }
  LogError("JNI exception during " + context);
  env->ExceptionDescribe();
  env->ExceptionClear();
  return true;
}

void DeleteLocalRefIfNotNull(JNIEnv* env, jobject object) {
  if (object != nullptr) {
    env->DeleteLocalRef(object);
  }
}

jclass FindClassChecked(JNIEnv* env, const char* class_name) {
  jclass clazz = env->FindClass(class_name);
  if (ClearJniExceptionIfPresent(env, std::string("FindClass(") + class_name + ")")) {
    return nullptr;
  }
  if (clazz == nullptr) {
    LogError(std::string("Missing JNI class: ") + class_name);
  }
  return clazz;
}

jclass GetObjectClassChecked(JNIEnv* env, jobject object, const std::string& context) {
  if (object == nullptr) {
    return nullptr;
  }
  jclass clazz = env->GetObjectClass(object);
  if (ClearJniExceptionIfPresent(env, "GetObjectClass(" + context + ")")) {
    return nullptr;
  }
  if (clazz == nullptr) {
    LogError("Missing JNI object class for " + context);
  }
  return clazz;
}

jstring NewStringUtfChecked(JNIEnv* env, const std::string& value, const std::string& context) {
  jstring result = env->NewStringUTF(value.c_str());
  if (ClearJniExceptionIfPresent(env, "NewStringUTF(" + context + ")")) {
    return nullptr;
  }
  if (result == nullptr) {
    LogError("Failed to allocate jstring for " + context);
  }
  return result;
}

jmethodID GetMethodChecked(
    JNIEnv* env,
    jclass clazz,
    const char* class_name,
    const char* method_name,
    const char* signature) {
  if (clazz == nullptr) {
    return nullptr;
  }
  jmethodID method = env->GetMethodID(clazz, method_name, signature);
  if (ClearJniExceptionIfPresent(
          env, std::string("GetMethodID(") + class_name + "." + method_name + ")")) {
    return nullptr;
  }
  if (method == nullptr) {
    LogError(std::string("Missing JNI method: ") + class_name + "." + method_name + " " +
             signature);
  }
  return method;
}

jfieldID GetFieldChecked(
    JNIEnv* env,
    jclass clazz,
    const char* class_name,
    const char* field_name,
    const char* signature) {
  if (clazz == nullptr) {
    return nullptr;
  }
  jfieldID field = env->GetFieldID(clazz, field_name, signature);
  if (ClearJniExceptionIfPresent(
          env, std::string("GetFieldID(") + class_name + "." + field_name + ")")) {
    return nullptr;
  }
  if (field == nullptr) {
    LogError(std::string("Missing JNI field: ") + class_name + "." + field_name + " " +
             signature);
  }
  return field;
}

std::string GetStringFieldValue(
    JNIEnv* env,
    jobject object,
    jclass clazz,
    const char* class_name,
    const char* field_name) {
  jfieldID field = GetFieldChecked(env, clazz, class_name, field_name, "Ljava/lang/String;");
  if (field == nullptr) {
    return "";
  }
  jstring value = static_cast<jstring>(env->GetObjectField(object, field));
  if (ClearJniExceptionIfPresent(
          env, std::string("GetObjectField(") + class_name + "." + field_name + ")")) {
    return "";
  }
  std::string result = JStringToString(env, value);
  DeleteLocalRefIfNotNull(env, value);
  return result;
}

int GetIntFieldValue(
    JNIEnv* env,
    jobject object,
    jclass clazz,
    const char* class_name,
    const char* field_name) {
  jfieldID field = GetFieldChecked(env, clazz, class_name, field_name, "I");
  if (field == nullptr) {
    return 0;
  }
  jint value = env->GetIntField(object, field);
  if (ClearJniExceptionIfPresent(
          env, std::string("GetIntField(") + class_name + "." + field_name + ")")) {
    return 0;
  }
  return static_cast<int>(value);
}

int64_t GetLongFieldValue(
    JNIEnv* env,
    jobject object,
    jclass clazz,
    const char* class_name,
    const char* field_name) {
  jfieldID field = GetFieldChecked(env, clazz, class_name, field_name, "J");
  if (field == nullptr) {
    return 0;
  }
  jlong value = env->GetLongField(object, field);
  if (ClearJniExceptionIfPresent(
          env, std::string("GetLongField(") + class_name + "." + field_name + ")")) {
    return 0;
  }
  return static_cast<int64_t>(value);
}

float GetFloatFieldValue(
    JNIEnv* env,
    jobject object,
    jclass clazz,
    const char* class_name,
    const char* field_name) {
  jfieldID field = GetFieldChecked(env, clazz, class_name, field_name, "F");
  if (field == nullptr) {
    return 0.0f;
  }
  jfloat value = env->GetFloatField(object, field);
  if (ClearJniExceptionIfPresent(
          env, std::string("GetFloatField(") + class_name + "." + field_name + ")")) {
    return 0.0f;
  }
  return static_cast<float>(value);
}

bool GetBooleanFieldValue(
    JNIEnv* env,
    jobject object,
    jclass clazz,
    const char* class_name,
    const char* field_name) {
  jfieldID field = GetFieldChecked(env, clazz, class_name, field_name, "Z");
  if (field == nullptr) {
    return false;
  }
  jboolean value = env->GetBooleanField(object, field);
  if (ClearJniExceptionIfPresent(
          env, std::string("GetBooleanField(") + class_name + "." + field_name + ")")) {
    return false;
  }
  return JNI_FALSE != value;
}

jobject GetObjectFieldValue(
    JNIEnv* env,
    jobject object,
    jclass clazz,
    const char* class_name,
    const char* field_name,
    const char* signature) {
  jfieldID field = GetFieldChecked(env, clazz, class_name, field_name, signature);
  if (field == nullptr) {
    return nullptr;
  }
  jobject value = env->GetObjectField(object, field);
  if (ClearJniExceptionIfPresent(
          env, std::string("GetObjectField(") + class_name + "." + field_name + ")")) {
    return nullptr;
  }
  return value;
}

std::string JStringToString(JNIEnv* env, jstring value) {
  if (value == nullptr) {
    return "";
  }
  const char* chars = env->GetStringUTFChars(value, nullptr);
  if (ClearJniExceptionIfPresent(env, "GetStringUTFChars(java/lang/String)") || chars == nullptr) {
    return "";
  }
  std::string result = chars != nullptr ? chars : "";
  env->ReleaseStringUTFChars(value, chars);
  if (ClearJniExceptionIfPresent(env, "ReleaseStringUTFChars(java/lang/String)")) {
    return "";
  }
  return result;
}

std::string GetTrackTypeName(int type) {
  switch (type) {
    case 1:
      return "audio";
    case 2:
      return "video";
    case 3:
      return "text";
    case 4:
      return "image";
    default:
      return "type-" + std::to_string(type);
  }
}

AudioAttributesDescriptor FromJavaAudioAttributes(JNIEnv* env, jintArray values) {
  AudioAttributesDescriptor descriptor;
  if (values == nullptr || env->GetArrayLength(values) < 5) {
    return descriptor;
  }
  jint* raw = env->GetIntArrayElements(values, nullptr);
  if (ClearJniExceptionIfPresent(env, "GetIntArrayElements(AudioAttributes)") || raw == nullptr) {
    return descriptor;
  }
  descriptor.content_type = raw[0];
  descriptor.usage = raw[1];
  descriptor.flags = raw[2];
  descriptor.allowed_capture_policy = raw[3];
  descriptor.spatialization_behavior = raw[4];
  env->ReleaseIntArrayElements(values, raw, JNI_ABORT);
  return descriptor;
}

std::vector<int> JIntArrayToVector(JNIEnv* env, jintArray values) {
  std::vector<int> result;
  if (values == nullptr) {
    return result;
  }
  jsize length = env->GetArrayLength(values);
  jint* raw = env->GetIntArrayElements(values, nullptr);
  if (ClearJniExceptionIfPresent(env, "GetIntArrayElements(int[])") || raw == nullptr) {
    return result;
  }
  result.reserve(static_cast<size_t>(length));
  for (jsize i = 0; i < length; ++i) {
    result.push_back(raw[i]);
  }
  env->ReleaseIntArrayElements(values, raw, JNI_ABORT);
  return result;
}

std::vector<uint8_t> JByteArrayToVector(JNIEnv* env, jbyteArray values) {
  std::vector<uint8_t> result;
  if (values == nullptr) {
    return result;
  }
  jsize length = env->GetArrayLength(values);
  jbyte* raw = env->GetByteArrayElements(values, nullptr);
  if (ClearJniExceptionIfPresent(env, "GetByteArrayElements(byte[])") || raw == nullptr) {
    return result;
  }
  result.reserve(static_cast<size_t>(length));
  for (jsize i = 0; i < length; ++i) {
    result.push_back(static_cast<uint8_t>(raw[i]));
  }
  env->ReleaseByteArrayElements(values, raw, JNI_ABORT);
  return result;
}

std::vector<float> JFloatArrayToVector(JNIEnv* env, jfloatArray values) {
  std::vector<float> result;
  if (values == nullptr) {
    return result;
  }
  jsize length = env->GetArrayLength(values);
  jfloat* raw = env->GetFloatArrayElements(values, nullptr);
  if (ClearJniExceptionIfPresent(env, "GetFloatArrayElements(float[])") || raw == nullptr) {
    return result;
  }
  result.reserve(static_cast<size_t>(length));
  for (jsize i = 0; i < length; ++i) {
    result.push_back(static_cast<float>(raw[i]));
  }
  env->ReleaseFloatArrayElements(values, raw, JNI_ABORT);
  return result;
}

jintArray CreateJavaIntArray(JNIEnv* env, const std::vector<int>& values) {
  jintArray array = env->NewIntArray(static_cast<jsize>(values.size()));
  if (ClearJniExceptionIfPresent(env, "NewIntArray(int[])") || array == nullptr) {
    return nullptr;
  }
  if (!values.empty()) {
    env->SetIntArrayRegion(
        array, 0, static_cast<jsize>(values.size()), reinterpret_cast<const jint*>(values.data()));
    if (ClearJniExceptionIfPresent(env, "SetIntArrayRegion(int[])")) {
      env->DeleteLocalRef(array);
      return nullptr;
    }
  }
  return array;
}

jfloatArray CreateJavaFloatArray(JNIEnv* env, const std::vector<float>& values) {
  jfloatArray array = env->NewFloatArray(static_cast<jsize>(values.size()));
  if (ClearJniExceptionIfPresent(env, "NewFloatArray(float[])") || array == nullptr) {
    return nullptr;
  }
  if (!values.empty()) {
    env->SetFloatArrayRegion(
        array,
        0,
        static_cast<jsize>(values.size()),
        reinterpret_cast<const jfloat*>(values.data()));
    if (ClearJniExceptionIfPresent(env, "SetFloatArrayRegion(float[])")) {
      env->DeleteLocalRef(array);
      return nullptr;
    }
  }
  return array;
}

std::vector<std::string> JStringArrayToVector(JNIEnv* env, jobjectArray values) {
  std::vector<std::string> result;
  if (values == nullptr) {
    return result;
  }
  jsize length = env->GetArrayLength(values);
  result.reserve(static_cast<size_t>(length));
  for (jsize i = 0; i < length; ++i) {
    jstring value = static_cast<jstring>(env->GetObjectArrayElement(values, i));
    if (ClearJniExceptionIfPresent(env, "GetObjectArrayElement(java/lang/String)") ||
        value == nullptr) {
      DeleteLocalRefIfNotNull(env, value);
      continue;
    }
    result.push_back(JStringToString(env, value));
    env->DeleteLocalRef(value);
  }
  return result;
}

jobjectArray CreateJavaStringArray(JNIEnv* env, const std::vector<std::string>& values) {
  jclass string_class = FindClassChecked(env, "java/lang/String");
  if (string_class == nullptr) {
    return nullptr;
  }
  jobjectArray array =
      env->NewObjectArray(static_cast<jsize>(values.size()), string_class, nullptr);
  if (ClearJniExceptionIfPresent(env, "NewObjectArray(java/lang/String)")) {
    env->DeleteLocalRef(string_class);
    return nullptr;
  }
  for (size_t i = 0; i < values.size(); ++i) {
    jstring value = NewStringUtfChecked(env, values[i], "java/lang/String array item");
    if (value == nullptr) {
      env->DeleteLocalRef(string_class);
      DeleteLocalRefIfNotNull(env, array);
      return nullptr;
    }
    env->SetObjectArrayElement(array, static_cast<jsize>(i), value);
    if (ClearJniExceptionIfPresent(env, "SetObjectArrayElement(java/lang/String array item)")) {
      env->DeleteLocalRef(value);
      env->DeleteLocalRef(string_class);
      DeleteLocalRefIfNotNull(env, array);
      return nullptr;
    }
    env->DeleteLocalRef(value);
  }
  env->DeleteLocalRef(string_class);
  return array;
}

jobjectArray CreateJavaVideoEffectArray(
    JNIEnv* env,
    const std::vector<VideoEffectDescriptor>& video_effects) {
  jclass effect_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppVideoEffect");
  if (effect_class == nullptr) {
    return nullptr;
  }
    jmethodID constructor = GetMethodChecked(
        env,
        effect_class,
        "CppVideoEffect",
        "<init>",
        "(IFFFFFFIII)V");
  if (constructor == nullptr) {
    env->DeleteLocalRef(effect_class);
    return nullptr;
  }
  jobjectArray array =
      env->NewObjectArray(static_cast<jsize>(video_effects.size()), effect_class, nullptr);
  if (ClearJniExceptionIfPresent(env, "NewObjectArray(CppVideoEffect)") || array == nullptr) {
    DeleteLocalRefIfNotNull(env, effect_class);
    DeleteLocalRefIfNotNull(env, array);
    return nullptr;
  }
  for (jsize i = 0; i < static_cast<jsize>(video_effects.size()); ++i) {
    const auto& effect = video_effects[static_cast<size_t>(i)];
      jobject java_effect = NewObjectChecked(
          env,
          effect_class,
          constructor,
          "CppVideoEffect",
          static_cast<jint>(
              effect.type == VideoEffectDescriptor::Type::kScaleAndRotate
                  ? 1
                  : effect.type == VideoEffectDescriptor::Type::kRgbAdjustment ? 2 : 3),
          static_cast<jfloat>(effect.scale_x),
          static_cast<jfloat>(effect.scale_y),
          static_cast<jfloat>(effect.rotation_degrees),
          static_cast<jfloat>(effect.red_scale),
          static_cast<jfloat>(effect.green_scale),
          static_cast<jfloat>(effect.blue_scale),
          static_cast<jint>(effect.presentation_width),
          static_cast<jint>(effect.presentation_height),
          static_cast<jint>(effect.presentation_layout));
    if (java_effect == nullptr) {
      DeleteLocalRefIfNotNull(env, array);
      env->DeleteLocalRef(effect_class);
      return nullptr;
    }
    env->SetObjectArrayElement(array, i, java_effect);
    if (ClearJniExceptionIfPresent(env, "SetObjectArrayElement(CppVideoEffect)")) {
      env->DeleteLocalRef(java_effect);
      DeleteLocalRefIfNotNull(env, array);
      env->DeleteLocalRef(effect_class);
      return nullptr;
    }
    env->DeleteLocalRef(java_effect);
  }
  env->DeleteLocalRef(effect_class);
  return array;
}

jbyteArray CreateJavaByteArray(JNIEnv* env, const std::vector<uint8_t>& values) {
  jbyteArray array = env->NewByteArray(static_cast<jsize>(values.size()));
  if (ClearJniExceptionIfPresent(env, "NewByteArray(byte[])") || array == nullptr) {
    return nullptr;
  }
  if (!values.empty()) {
    env->SetByteArrayRegion(
        array,
        0,
        static_cast<jsize>(values.size()),
        reinterpret_cast<const jbyte*>(values.data()));
    if (ClearJniExceptionIfPresent(env, "SetByteArrayRegion(byte[])")) {
      env->DeleteLocalRef(array);
      return nullptr;
    }
  }
  return array;
}

CodecParametersDescriptor FromJavaCodecParameterArray(JNIEnv* env, jobjectArray values) {
  CodecParametersDescriptor descriptor;
  if (values == nullptr) {
    return descriptor;
  }
  jsize length = env->GetArrayLength(values);
  if (ClearJniExceptionIfPresent(env, "GetArrayLength(CppCodecParameter[])")) {
    return descriptor;
  }
  descriptor.parameters.reserve(static_cast<size_t>(length));
  for (jsize i = 0; i < length; ++i) {
    jobject object = env->GetObjectArrayElement(values, i);
    if (ClearJniExceptionIfPresent(env, "GetObjectArrayElement(CppCodecParameter)") ||
        object == nullptr) {
      DeleteLocalRefIfNotNull(env, object);
      continue;
    }
    jclass clazz = GetObjectClassChecked(env, object, "CppCodecParameter");
    if (clazz == nullptr) {
      env->DeleteLocalRef(object);
      continue;
    }
    CodecParameterDescriptor parameter;
    parameter.key =
        GetStringFieldValue(env, object, clazz, "CppCodecParameter", "key");
    int type = GetIntFieldValue(env, object, clazz, "CppCodecParameter", "type");
    switch (type) {
      case 0:
        parameter.value_type = CodecParameterDescriptor::ValueType::kInteger;
        break;
      case 1:
        parameter.value_type = CodecParameterDescriptor::ValueType::kLong;
        break;
      case 2:
        parameter.value_type = CodecParameterDescriptor::ValueType::kFloat;
        break;
      case 3:
        parameter.value_type = CodecParameterDescriptor::ValueType::kString;
        break;
      case 4:
        parameter.value_type = CodecParameterDescriptor::ValueType::kByteBuffer;
        break;
      case 5:
      default:
        parameter.value_type = CodecParameterDescriptor::ValueType::kNull;
        break;
    }
    parameter.int_value =
        GetIntFieldValue(env, object, clazz, "CppCodecParameter", "intValue");
    parameter.long_value =
        GetLongFieldValue(env, object, clazz, "CppCodecParameter", "longValue");
    parameter.float_value =
        GetFloatFieldValue(env, object, clazz, "CppCodecParameter", "floatValue");
    parameter.string_value =
        GetStringFieldValue(env, object, clazz, "CppCodecParameter", "stringValue");
    jbyteArray byte_buffer_value = static_cast<jbyteArray>(GetObjectFieldValue(
        env,
        object,
        clazz,
        "CppCodecParameter",
        "byteBufferValue",
        "[B"));
    parameter.byte_buffer_value = JByteArrayToVector(env, byte_buffer_value);
    DeleteLocalRefIfNotNull(env, byte_buffer_value);
    env->DeleteLocalRef(clazz);
    env->DeleteLocalRef(object);
    descriptor.parameters.push_back(std::move(parameter));
  }
  return descriptor;
}

int ParseIntOrDefault(const std::string& value, int fallback) {
  if (value.empty()) {
    return fallback;
  }
  try {
    size_t parsed_length = 0;
    int parsed_value = std::stoi(value, &parsed_length);
    return parsed_length == value.size() ? parsed_value : fallback;
  } catch (const std::exception&) {
    return fallback;
  }
}

int64_t ParseLongOrDefault(const std::string& value, int64_t fallback) {
  if (value.empty()) {
    return fallback;
  }
  try {
    size_t parsed_length = 0;
    int64_t parsed_value = std::stoll(value, &parsed_length);
    return parsed_length == value.size() ? parsed_value : fallback;
  } catch (const std::exception&) {
    return fallback;
  }
}

float ParseFloatOrDefault(const std::string& value, float fallback) {
  if (value.empty()) {
    return fallback;
  }
  try {
    size_t parsed_length = 0;
    float parsed_value = std::stof(value, &parsed_length);
    return parsed_length == value.size() ? parsed_value : fallback;
  } catch (const std::exception&) {
    return fallback;
  }
}

std::vector<std::string> SplitString(const std::string& value, char delimiter) {
  std::vector<std::string> parts;
  std::string part;
  bool escaping = false;
  for (char c : value) {
    if (escaping) {
      part.push_back(c);
      escaping = false;
    } else if (c == '\\') {
      escaping = true;
    } else if (c == delimiter) {
      parts.push_back(part);
      part.clear();
    } else {
      part.push_back(c);
    }
  }
  if (escaping) {
    part.push_back('\\');
  }
  parts.push_back(part);
  return parts;
}


PlaybackState ToPlaybackState(int state) {
  switch (state) {
    case 1:
      return PlaybackState::kIdle;
    case 2:
      return PlaybackState::kBuffering;
    case 3:
      return PlaybackState::kReady;
    case 4:
      return PlaybackState::kEnded;
    default:
      return PlaybackState::kIdle;
  }
}

PlaybackSuppressionReason ToPlaybackSuppressionReason(int reason) {
  switch (reason) {
    case 1:
      return PlaybackSuppressionReason::kTransientAudioFocusLoss;
    case 2:
      return PlaybackSuppressionReason::kUnsuitableAudioRoute;
    case 3:
      return PlaybackSuppressionReason::kUnsuitableAudioOutput;
    case 4:
      return PlaybackSuppressionReason::kScrubbing;
    case 0:
    default:
      return PlaybackSuppressionReason::kNone;
  }
}

jint ToJavaSourceType(androidx::media3::cppbridge::MediaSourceType source_type) {
  return static_cast<jint>(source_type);
}

jobject CreateJavaMediaItem(JNIEnv* env, const MediaItemDescriptor& media_item) {
  jclass item_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppMediaItem");
  jclass subtitle_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppSubtitleConfiguration");
  jclass clipping_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppClippingConfiguration");
  jclass live_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppLiveConfiguration");
  jclass drm_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppDrmConfiguration");
  jclass ads_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppAdsConfiguration");
  jclass request_metadata_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppRequestMetadata");
  jclass metadata_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppMediaMetadata");
  if (item_class == nullptr || subtitle_class == nullptr || clipping_class == nullptr ||
      live_class == nullptr || drm_class == nullptr || ads_class == nullptr ||
      request_metadata_class == nullptr ||
      metadata_class == nullptr) {
    DeleteLocalRefIfNotNull(env, item_class);
    DeleteLocalRefIfNotNull(env, subtitle_class);
    DeleteLocalRefIfNotNull(env, clipping_class);
    DeleteLocalRefIfNotNull(env, live_class);
    DeleteLocalRefIfNotNull(env, drm_class);
    DeleteLocalRefIfNotNull(env, ads_class);
    DeleteLocalRefIfNotNull(env, request_metadata_class);
    DeleteLocalRefIfNotNull(env, metadata_class);
    return nullptr;
  }
  jmethodID item_ctor = GetMethodChecked(
      env,
      item_class,
      "CppMediaItem",
      "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IZLjava/lang/String;Ljava/lang/String;"
      "Landroidx/media3/exoplayer/cppbridge/CppMediaMetadata;"
      "Landroidx/media3/exoplayer/cppbridge/CppRequestMetadata;"
      "Landroidx/media3/exoplayer/cppbridge/CppAdsConfiguration;"
      "[Landroidx/media3/exoplayer/cppbridge/CppSubtitleConfiguration;"
      "Landroidx/media3/exoplayer/cppbridge/CppClippingConfiguration;"
      "Landroidx/media3/exoplayer/cppbridge/CppLiveConfiguration;"
      "Landroidx/media3/exoplayer/cppbridge/CppDrmConfiguration;)V");
  jmethodID request_metadata_ctor = GetMethodChecked(
      env,
      request_metadata_class,
      "CppRequestMetadata",
      "<init>",
      "(Ljava/lang/String;Ljava/lang/String;ZILjava/lang/String;)V");
  jmethodID ads_ctor = GetMethodChecked(
      env,
      ads_class,
      "CppAdsConfiguration",
      "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  jmethodID subtitle_ctor = GetMethodChecked(
      env,
      subtitle_class,
      "CppSubtitleConfiguration",
      "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V");
  jmethodID clipping_ctor =
      GetMethodChecked(env, clipping_class, "CppClippingConfiguration", "<init>", "(JJZZZZ)V");
  jmethodID live_ctor =
      GetMethodChecked(env, live_class, "CppLiveConfiguration", "<init>", "(JJJFF)V");
  jmethodID drm_ctor = GetMethodChecked(
      env,
          drm_class,
          "CppDrmConfiguration",
          "<init>",
          "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;[I[BZZZ)V");
  if (item_ctor == nullptr || request_metadata_ctor == nullptr || subtitle_ctor == nullptr ||
      clipping_ctor == nullptr || live_ctor == nullptr || drm_ctor == nullptr ||
      ads_ctor == nullptr) {
    env->DeleteLocalRef(subtitle_class);
    env->DeleteLocalRef(clipping_class);
    env->DeleteLocalRef(live_class);
    env->DeleteLocalRef(drm_class);
    env->DeleteLocalRef(ads_class);
    env->DeleteLocalRef(request_metadata_class);
    env->DeleteLocalRef(metadata_class);
    env->DeleteLocalRef(item_class);
    return nullptr;
  }

  jstring uri = NewStringUtfChecked(env, media_item.uri, "CppMediaItem.uri");
  jstring media_id =
      media_item.media_id.empty()
          ? nullptr
          : NewStringUtfChecked(env, media_item.media_id, "CppMediaItem.mediaId");
  jstring mime_type =
      media_item.mime_type.empty()
          ? nullptr
          : NewStringUtfChecked(env, media_item.mime_type, "CppMediaItem.mimeType");
  jstring tag_string =
      media_item.tag_string.empty()
          ? nullptr
          : NewStringUtfChecked(env, media_item.tag_string, "CppMediaItem.tagString");
  jstring tag_token =
      media_item.tag_token.empty()
          ? nullptr
          : NewStringUtfChecked(env, media_item.tag_token, "CppMediaItem.tagToken");
  if (uri == nullptr || (!media_item.media_id.empty() && media_id == nullptr) ||
      (!media_item.mime_type.empty() && mime_type == nullptr) ||
      (!media_item.tag_string.empty() && tag_string == nullptr) ||
      (!media_item.tag_token.empty() && tag_token == nullptr)) {
    DeleteLocalRefIfNotNull(env, uri);
    DeleteLocalRefIfNotNull(env, media_id);
    DeleteLocalRefIfNotNull(env, mime_type);
    DeleteLocalRefIfNotNull(env, tag_string);
    DeleteLocalRefIfNotNull(env, tag_token);
    env->DeleteLocalRef(subtitle_class);
    env->DeleteLocalRef(clipping_class);
    env->DeleteLocalRef(live_class);
    env->DeleteLocalRef(drm_class);
    env->DeleteLocalRef(ads_class);
    env->DeleteLocalRef(request_metadata_class);
    env->DeleteLocalRef(metadata_class);
    env->DeleteLocalRef(item_class);
    return nullptr;
  }
  jobject metadata = CreateJavaMediaMetadata(env, media_item.media_metadata);
  jobject request_metadata = nullptr;
  if (!media_item.request_metadata.media_uri.empty() ||
      !media_item.request_metadata.search_query.empty() ||
      media_item.request_metadata.extras_present) {
    jstring request_media_uri =
        media_item.request_metadata.media_uri.empty()
            ? nullptr
            : NewStringUtfChecked(
                  env, media_item.request_metadata.media_uri, "CppRequestMetadata.mediaUri");
    jstring request_search_query =
        media_item.request_metadata.search_query.empty()
            ? nullptr
            : NewStringUtfChecked(
                  env,
                  media_item.request_metadata.search_query,
                  "CppRequestMetadata.searchQuery");
    jstring request_extras_token =
        media_item.request_metadata.extras_token.empty()
            ? nullptr
            : NewStringUtfChecked(
                  env,
                  media_item.request_metadata.extras_token,
                  "CppRequestMetadata.extrasToken");
    if ((!media_item.request_metadata.media_uri.empty() && request_media_uri == nullptr) ||
        (!media_item.request_metadata.search_query.empty() &&
         request_search_query == nullptr) ||
        (!media_item.request_metadata.extras_token.empty() &&
         request_extras_token == nullptr)) {
      DeleteLocalRefIfNotNull(env, request_media_uri);
      DeleteLocalRefIfNotNull(env, request_search_query);
      DeleteLocalRefIfNotNull(env, request_extras_token);
      DeleteLocalRefIfNotNull(env, uri);
      DeleteLocalRefIfNotNull(env, media_id);
      DeleteLocalRefIfNotNull(env, mime_type);
      DeleteLocalRefIfNotNull(env, metadata);
      env->DeleteLocalRef(subtitle_class);
      env->DeleteLocalRef(clipping_class);
      env->DeleteLocalRef(live_class);
      env->DeleteLocalRef(drm_class);
      env->DeleteLocalRef(ads_class);
      env->DeleteLocalRef(request_metadata_class);
      env->DeleteLocalRef(metadata_class);
      env->DeleteLocalRef(item_class);
      return nullptr;
    }
    request_metadata = NewObjectChecked(
        env,
        request_metadata_class,
        request_metadata_ctor,
        "CppRequestMetadata",
        request_media_uri,
        request_search_query,
        static_cast<jboolean>(media_item.request_metadata.extras_present),
        static_cast<jint>(media_item.request_metadata.extras_key_count),
        request_extras_token);
    DeleteLocalRefIfNotNull(env, request_media_uri);
    DeleteLocalRefIfNotNull(env, request_search_query);
    DeleteLocalRefIfNotNull(env, request_extras_token);
    if (request_metadata == nullptr) {
      DeleteLocalRefIfNotNull(env, uri);
      DeleteLocalRefIfNotNull(env, media_id);
      DeleteLocalRefIfNotNull(env, mime_type);
      DeleteLocalRefIfNotNull(env, metadata);
      env->DeleteLocalRef(subtitle_class);
      env->DeleteLocalRef(clipping_class);
      env->DeleteLocalRef(live_class);
      env->DeleteLocalRef(drm_class);
      env->DeleteLocalRef(ads_class);
      env->DeleteLocalRef(request_metadata_class);
      env->DeleteLocalRef(metadata_class);
      env->DeleteLocalRef(item_class);
      return nullptr;
    }
  }
  jobject ads = nullptr;
  if (!media_item.ads_configuration.ad_tag_uri.empty()) {
    jstring ad_tag_uri = NewStringUtfChecked(
        env, media_item.ads_configuration.ad_tag_uri, "CppAdsConfiguration.adTagUri");
    jstring ads_id =
        media_item.ads_configuration.ads_id.empty()
            ? nullptr
            : NewStringUtfChecked(
                  env, media_item.ads_configuration.ads_id, "CppAdsConfiguration.adsId");
    jstring ads_id_token =
        media_item.ads_configuration.ads_id_token.empty()
            ? nullptr
            : NewStringUtfChecked(
                  env, media_item.ads_configuration.ads_id_token, "CppAdsConfiguration.adsIdToken");
    if (ad_tag_uri == nullptr ||
        (!media_item.ads_configuration.ads_id.empty() && ads_id == nullptr) ||
        (!media_item.ads_configuration.ads_id_token.empty() && ads_id_token == nullptr)) {
      DeleteLocalRefIfNotNull(env, ad_tag_uri);
      DeleteLocalRefIfNotNull(env, ads_id);
      DeleteLocalRefIfNotNull(env, ads_id_token);
      DeleteLocalRefIfNotNull(env, uri);
      DeleteLocalRefIfNotNull(env, media_id);
      DeleteLocalRefIfNotNull(env, mime_type);
      DeleteLocalRefIfNotNull(env, metadata);
      DeleteLocalRefIfNotNull(env, request_metadata);
      env->DeleteLocalRef(subtitle_class);
      env->DeleteLocalRef(clipping_class);
      env->DeleteLocalRef(live_class);
      env->DeleteLocalRef(drm_class);
      env->DeleteLocalRef(ads_class);
      env->DeleteLocalRef(request_metadata_class);
      env->DeleteLocalRef(metadata_class);
      env->DeleteLocalRef(item_class);
      return nullptr;
    }
    ads = NewObjectChecked(
        env, ads_class, ads_ctor, "CppAdsConfiguration", ad_tag_uri, ads_id, ads_id_token);
    DeleteLocalRefIfNotNull(env, ad_tag_uri);
    DeleteLocalRefIfNotNull(env, ads_id);
    DeleteLocalRefIfNotNull(env, ads_id_token);
    if (ads == nullptr) {
      DeleteLocalRefIfNotNull(env, uri);
      DeleteLocalRefIfNotNull(env, media_id);
      DeleteLocalRefIfNotNull(env, mime_type);
      DeleteLocalRefIfNotNull(env, metadata);
      DeleteLocalRefIfNotNull(env, request_metadata);
      env->DeleteLocalRef(subtitle_class);
      env->DeleteLocalRef(clipping_class);
      env->DeleteLocalRef(live_class);
      env->DeleteLocalRef(drm_class);
      env->DeleteLocalRef(ads_class);
      env->DeleteLocalRef(request_metadata_class);
      env->DeleteLocalRef(metadata_class);
      env->DeleteLocalRef(item_class);
      return nullptr;
    }
  }
  jobjectArray subtitles = env->NewObjectArray(
      static_cast<jsize>(media_item.subtitle_configurations.size()), subtitle_class, nullptr);
  if (ClearJniExceptionIfPresent(env, "NewObjectArray(CppSubtitleConfiguration)")) {
    DeleteLocalRefIfNotNull(env, uri);
    DeleteLocalRefIfNotNull(env, media_id);
    DeleteLocalRefIfNotNull(env, mime_type);
    DeleteLocalRefIfNotNull(env, metadata);
    DeleteLocalRefIfNotNull(env, request_metadata);
    DeleteLocalRefIfNotNull(env, ads);
    env->DeleteLocalRef(subtitle_class);
    env->DeleteLocalRef(clipping_class);
    env->DeleteLocalRef(live_class);
    env->DeleteLocalRef(drm_class);
    env->DeleteLocalRef(ads_class);
    env->DeleteLocalRef(request_metadata_class);
    env->DeleteLocalRef(metadata_class);
    env->DeleteLocalRef(item_class);
    return nullptr;
  }
  for (jsize i = 0; i < static_cast<jsize>(media_item.subtitle_configurations.size()); ++i) {
    const auto& subtitle = media_item.subtitle_configurations[static_cast<size_t>(i)];
    jstring subtitle_uri = NewStringUtfChecked(env, subtitle.uri, "CppSubtitleConfiguration.uri");
    jstring subtitle_mime_type =
        subtitle.mime_type.empty()
            ? nullptr
            : NewStringUtfChecked(env, subtitle.mime_type, "CppSubtitleConfiguration.mimeType");
    jstring subtitle_language =
        subtitle.language.empty()
            ? nullptr
            : NewStringUtfChecked(env, subtitle.language, "CppSubtitleConfiguration.language");
    jstring subtitle_label =
        subtitle.label.empty()
            ? nullptr
            : NewStringUtfChecked(env, subtitle.label, "CppSubtitleConfiguration.label");
    jstring subtitle_id =
        subtitle.id.empty()
            ? nullptr
            : NewStringUtfChecked(env, subtitle.id, "CppSubtitleConfiguration.id");
    if (subtitle_uri == nullptr ||
        (!subtitle.mime_type.empty() && subtitle_mime_type == nullptr) ||
        (!subtitle.language.empty() && subtitle_language == nullptr) ||
        (!subtitle.label.empty() && subtitle_label == nullptr) ||
        (!subtitle.id.empty() && subtitle_id == nullptr)) {
      DeleteLocalRefIfNotNull(env, subtitle_uri);
      DeleteLocalRefIfNotNull(env, subtitle_mime_type);
      DeleteLocalRefIfNotNull(env, subtitle_language);
      DeleteLocalRefIfNotNull(env, subtitle_label);
      DeleteLocalRefIfNotNull(env, subtitle_id);
      DeleteLocalRefIfNotNull(env, uri);
      DeleteLocalRefIfNotNull(env, media_id);
      DeleteLocalRefIfNotNull(env, mime_type);
      DeleteLocalRefIfNotNull(env, metadata);
      DeleteLocalRefIfNotNull(env, request_metadata);
      DeleteLocalRefIfNotNull(env, ads);
      DeleteLocalRefIfNotNull(env, subtitles);
      env->DeleteLocalRef(subtitle_class);
      env->DeleteLocalRef(clipping_class);
      env->DeleteLocalRef(live_class);
      env->DeleteLocalRef(drm_class);
      env->DeleteLocalRef(ads_class);
      env->DeleteLocalRef(request_metadata_class);
      env->DeleteLocalRef(metadata_class);
      env->DeleteLocalRef(item_class);
      return nullptr;
    }
    jobject subtitle_object = NewObjectChecked(
        env,
        subtitle_class,
        subtitle_ctor,
        "CppSubtitleConfiguration",
        subtitle_uri,
        subtitle_mime_type,
        subtitle_language,
        subtitle_label,
        subtitle_id,
        static_cast<jint>(subtitle.selection_flags),
        static_cast<jint>(subtitle.role_flags));
    if (subtitle_object == nullptr) {
      DeleteLocalRefIfNotNull(env, subtitle_uri);
      DeleteLocalRefIfNotNull(env, subtitle_mime_type);
      DeleteLocalRefIfNotNull(env, subtitle_language);
      DeleteLocalRefIfNotNull(env, subtitle_label);
      DeleteLocalRefIfNotNull(env, subtitle_id);
      DeleteLocalRefIfNotNull(env, uri);
      DeleteLocalRefIfNotNull(env, media_id);
      DeleteLocalRefIfNotNull(env, mime_type);
      DeleteLocalRefIfNotNull(env, metadata);
      DeleteLocalRefIfNotNull(env, request_metadata);
      DeleteLocalRefIfNotNull(env, ads);
      DeleteLocalRefIfNotNull(env, subtitles);
      env->DeleteLocalRef(subtitle_class);
      env->DeleteLocalRef(clipping_class);
      env->DeleteLocalRef(live_class);
      env->DeleteLocalRef(drm_class);
      env->DeleteLocalRef(ads_class);
      env->DeleteLocalRef(request_metadata_class);
      env->DeleteLocalRef(metadata_class);
      env->DeleteLocalRef(item_class);
      return nullptr;
    }
    env->SetObjectArrayElement(subtitles, i, subtitle_object);
    if (ClearJniExceptionIfPresent(env, "SetObjectArrayElement(CppSubtitleConfiguration)")) {
      env->DeleteLocalRef(subtitle_object);
      DeleteLocalRefIfNotNull(env, subtitle_uri);
      DeleteLocalRefIfNotNull(env, subtitle_mime_type);
      DeleteLocalRefIfNotNull(env, subtitle_language);
      DeleteLocalRefIfNotNull(env, subtitle_label);
      DeleteLocalRefIfNotNull(env, subtitle_id);
      DeleteLocalRefIfNotNull(env, uri);
      DeleteLocalRefIfNotNull(env, media_id);
      DeleteLocalRefIfNotNull(env, mime_type);
      DeleteLocalRefIfNotNull(env, metadata);
      DeleteLocalRefIfNotNull(env, request_metadata);
      DeleteLocalRefIfNotNull(env, ads);
      DeleteLocalRefIfNotNull(env, subtitles);
      env->DeleteLocalRef(subtitle_class);
      env->DeleteLocalRef(clipping_class);
      env->DeleteLocalRef(live_class);
      env->DeleteLocalRef(drm_class);
      env->DeleteLocalRef(ads_class);
      env->DeleteLocalRef(request_metadata_class);
      env->DeleteLocalRef(metadata_class);
      env->DeleteLocalRef(item_class);
      return nullptr;
    }
    env->DeleteLocalRef(subtitle_object);
    env->DeleteLocalRef(subtitle_uri);
    if (subtitle_mime_type != nullptr) {
      env->DeleteLocalRef(subtitle_mime_type);
    }
    if (subtitle_language != nullptr) {
      env->DeleteLocalRef(subtitle_language);
    }
    if (subtitle_label != nullptr) {
      env->DeleteLocalRef(subtitle_label);
    }
    if (subtitle_id != nullptr) {
      env->DeleteLocalRef(subtitle_id);
    }
  }
  jobject clipping = nullptr;
  if (media_item.clipping_configuration.start_position_ms != 0 ||
      media_item.clipping_configuration.end_position_ms != -9223372036854775807LL ||
      media_item.clipping_configuration.relative_to_live_window ||
      media_item.clipping_configuration.relative_to_default_position ||
      media_item.clipping_configuration.starts_at_key_frame ||
      media_item.clipping_configuration.allow_unseekable_media) {
    clipping = NewObjectChecked(
        env,
        clipping_class,
        clipping_ctor,
        "CppClippingConfiguration",
        static_cast<jlong>(media_item.clipping_configuration.start_position_ms),
        static_cast<jlong>(media_item.clipping_configuration.end_position_ms),
        static_cast<jboolean>(media_item.clipping_configuration.relative_to_live_window),
        static_cast<jboolean>(media_item.clipping_configuration.relative_to_default_position),
        static_cast<jboolean>(media_item.clipping_configuration.starts_at_key_frame),
        static_cast<jboolean>(media_item.clipping_configuration.allow_unseekable_media));
    if (clipping == nullptr) {
      DeleteLocalRefIfNotNull(env, uri);
      DeleteLocalRefIfNotNull(env, media_id);
      DeleteLocalRefIfNotNull(env, mime_type);
      DeleteLocalRefIfNotNull(env, subtitles);
      DeleteLocalRefIfNotNull(env, metadata);
      DeleteLocalRefIfNotNull(env, request_metadata);
      DeleteLocalRefIfNotNull(env, ads);
      env->DeleteLocalRef(subtitle_class);
      env->DeleteLocalRef(clipping_class);
      env->DeleteLocalRef(live_class);
      env->DeleteLocalRef(drm_class);
      env->DeleteLocalRef(ads_class);
      env->DeleteLocalRef(request_metadata_class);
      env->DeleteLocalRef(metadata_class);
      env->DeleteLocalRef(item_class);
      return nullptr;
    }
  }
  jobject live = nullptr;
  if (media_item.live_configuration.target_offset_ms != -9223372036854775807LL ||
      media_item.live_configuration.min_offset_ms != -9223372036854775807LL ||
      media_item.live_configuration.max_offset_ms != -9223372036854775807LL ||
      media_item.live_configuration.min_playback_speed != -3.4028235e38f ||
      media_item.live_configuration.max_playback_speed != -3.4028235e38f) {
    live = NewObjectChecked(
        env,
        live_class,
        live_ctor,
        "CppLiveConfiguration",
        static_cast<jlong>(media_item.live_configuration.target_offset_ms),
        static_cast<jlong>(media_item.live_configuration.min_offset_ms),
        static_cast<jlong>(media_item.live_configuration.max_offset_ms),
        static_cast<jfloat>(media_item.live_configuration.min_playback_speed),
        static_cast<jfloat>(media_item.live_configuration.max_playback_speed));
    if (live == nullptr) {
      DeleteLocalRefIfNotNull(env, uri);
      DeleteLocalRefIfNotNull(env, media_id);
      DeleteLocalRefIfNotNull(env, mime_type);
      DeleteLocalRefIfNotNull(env, metadata);
      DeleteLocalRefIfNotNull(env, request_metadata);
      DeleteLocalRefIfNotNull(env, ads);
      DeleteLocalRefIfNotNull(env, subtitles);
      DeleteLocalRefIfNotNull(env, clipping);
      env->DeleteLocalRef(subtitle_class);
      env->DeleteLocalRef(clipping_class);
      env->DeleteLocalRef(live_class);
      env->DeleteLocalRef(drm_class);
      env->DeleteLocalRef(ads_class);
      env->DeleteLocalRef(request_metadata_class);
      env->DeleteLocalRef(metadata_class);
      env->DeleteLocalRef(item_class);
      return nullptr;
    }
  }
  jobject drm = nullptr;
  if (!media_item.drm_configuration.scheme_uuid.empty()) {
    jstring scheme_uuid = NewStringUtfChecked(
        env, media_item.drm_configuration.scheme_uuid, "CppDrmConfiguration.schemeUuid");
    jstring license_uri = media_item.drm_configuration.license_uri.empty()
        ? nullptr
        : NewStringUtfChecked(
              env, media_item.drm_configuration.license_uri, "CppDrmConfiguration.licenseUri");
    if (scheme_uuid == nullptr ||
        (!media_item.drm_configuration.license_uri.empty() && license_uri == nullptr)) {
      DeleteLocalRefIfNotNull(env, scheme_uuid);
      DeleteLocalRefIfNotNull(env, license_uri);
      DeleteLocalRefIfNotNull(env, uri);
      DeleteLocalRefIfNotNull(env, media_id);
      DeleteLocalRefIfNotNull(env, mime_type);
      DeleteLocalRefIfNotNull(env, metadata);
      DeleteLocalRefIfNotNull(env, request_metadata);
      DeleteLocalRefIfNotNull(env, ads);
      DeleteLocalRefIfNotNull(env, subtitles);
      DeleteLocalRefIfNotNull(env, clipping);
      DeleteLocalRefIfNotNull(env, live);
      env->DeleteLocalRef(subtitle_class);
      env->DeleteLocalRef(clipping_class);
      env->DeleteLocalRef(live_class);
      env->DeleteLocalRef(drm_class);
      env->DeleteLocalRef(ads_class);
      env->DeleteLocalRef(request_metadata_class);
      env->DeleteLocalRef(metadata_class);
      env->DeleteLocalRef(item_class);
      return nullptr;
    }
    jobjectArray header_names = CreateJavaStringArray(
        env, media_item.drm_configuration.license_request_header_names);
    jobjectArray header_values = CreateJavaStringArray(
        env, media_item.drm_configuration.license_request_header_values);
    jintArray forced_session_track_types = CreateJavaIntArray(
        env, media_item.drm_configuration.forced_session_track_types);
    jbyteArray key_set_id = CreateJavaByteArray(env, media_item.drm_configuration.key_set_id);
    drm = NewObjectChecked(
        env,
        drm_class,
        drm_ctor,
        "CppDrmConfiguration",
        scheme_uuid,
        license_uri,
        header_names,
        header_values,
        forced_session_track_types,
        key_set_id,
        static_cast<jboolean>(media_item.drm_configuration.multi_session),
        static_cast<jboolean>(media_item.drm_configuration.force_default_license_uri),
        static_cast<jboolean>(media_item.drm_configuration.play_clear_content_without_key));
    if (drm == nullptr) {
      DeleteLocalRefIfNotNull(env, scheme_uuid);
      DeleteLocalRefIfNotNull(env, license_uri);
      DeleteLocalRefIfNotNull(env, header_names);
      DeleteLocalRefIfNotNull(env, header_values);
      DeleteLocalRefIfNotNull(env, forced_session_track_types);
      DeleteLocalRefIfNotNull(env, key_set_id);
      DeleteLocalRefIfNotNull(env, uri);
      DeleteLocalRefIfNotNull(env, media_id);
      DeleteLocalRefIfNotNull(env, mime_type);
      DeleteLocalRefIfNotNull(env, metadata);
      DeleteLocalRefIfNotNull(env, request_metadata);
      DeleteLocalRefIfNotNull(env, ads);
      DeleteLocalRefIfNotNull(env, subtitles);
      DeleteLocalRefIfNotNull(env, clipping);
      DeleteLocalRefIfNotNull(env, live);
      env->DeleteLocalRef(subtitle_class);
      env->DeleteLocalRef(clipping_class);
      env->DeleteLocalRef(live_class);
      env->DeleteLocalRef(drm_class);
      env->DeleteLocalRef(ads_class);
      env->DeleteLocalRef(request_metadata_class);
      env->DeleteLocalRef(metadata_class);
      env->DeleteLocalRef(item_class);
      return nullptr;
    }
    env->DeleteLocalRef(scheme_uuid);
    DeleteLocalRefIfNotNull(env, license_uri);
    DeleteLocalRefIfNotNull(env, header_names);
    DeleteLocalRefIfNotNull(env, header_values);
    DeleteLocalRefIfNotNull(env, forced_session_track_types);
    DeleteLocalRefIfNotNull(env, key_set_id);
  }
  jobject item = NewObjectChecked(env,
                                  item_class,
                                  item_ctor,
                                  "CppMediaItem",
                                  uri,
                                  media_id,
                                  mime_type,
                                  ToJavaSourceType(media_item.source_type),
                                  static_cast<jboolean>(media_item.tag_present),
                                  tag_string,
                                  tag_token,
                                  metadata,
                                  request_metadata,
                                  ads,
                                  subtitles,
                                  clipping,
                                  live,
                                  drm);

  env->DeleteLocalRef(uri);
  if (media_id != nullptr) {
    env->DeleteLocalRef(media_id);
  }
  if (mime_type != nullptr) {
    env->DeleteLocalRef(mime_type);
  }
  DeleteLocalRefIfNotNull(env, tag_string);
  DeleteLocalRefIfNotNull(env, tag_token);
  DeleteLocalRefIfNotNull(env, metadata);
  DeleteLocalRefIfNotNull(env, request_metadata);
  DeleteLocalRefIfNotNull(env, ads);
  env->DeleteLocalRef(subtitles);
  if (clipping != nullptr) {
    env->DeleteLocalRef(clipping);
  }
  if (live != nullptr) {
    env->DeleteLocalRef(live);
  }
  if (drm != nullptr) {
    env->DeleteLocalRef(drm);
  }
  env->DeleteLocalRef(subtitle_class);
  env->DeleteLocalRef(clipping_class);
  env->DeleteLocalRef(live_class);
  env->DeleteLocalRef(drm_class);
  env->DeleteLocalRef(ads_class);
  env->DeleteLocalRef(request_metadata_class);
  env->DeleteLocalRef(metadata_class);
  env->DeleteLocalRef(item_class);
  return item;
}

jobjectArray CreateJavaMediaItemArray(
    JNIEnv* env,
    const std::vector<MediaItemDescriptor>& media_items) {
  jclass item_class = FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppMediaItem");
  if (item_class == nullptr) {
    return nullptr;
  }
  // Empty native input intentionally maps to an empty Java array so callers can
  // distinguish "clear playlist" from "ignore request".
  jobjectArray array =
      env->NewObjectArray(static_cast<jsize>(media_items.size()), item_class, nullptr);
  if (ClearJniExceptionIfPresent(env, "NewObjectArray(CppMediaItem)") || array == nullptr) {
    DeleteLocalRefIfNotNull(env, item_class);
    DeleteLocalRefIfNotNull(env, array);
    return nullptr;
  }
  for (jsize i = 0; i < static_cast<jsize>(media_items.size()); ++i) {
    jobject item = CreateJavaMediaItem(env, media_items[static_cast<size_t>(i)]);
    if (item == nullptr) {
      env->DeleteLocalRef(item_class);
      env->DeleteLocalRef(array);
      return nullptr;
    }
    env->SetObjectArrayElement(array, i, item);
    if (ClearJniExceptionIfPresent(env, "SetObjectArrayElement(CppMediaItem)")) {
      env->DeleteLocalRef(item);
      env->DeleteLocalRef(item_class);
      env->DeleteLocalRef(array);
      return nullptr;
    }
    env->DeleteLocalRef(item);
  }
  env->DeleteLocalRef(item_class);
  return array;
}

TrackSelectionParametersDescriptor FromJavaTrackSelectionParameters(JNIEnv* env, jobject object) {
  constexpr int kTrackTypeAudio = 1;
  constexpr int kTrackTypeVideo = 2;
  constexpr int kTrackTypeText = 3;
  TrackSelectionParametersDescriptor descriptor;
  if (object == nullptr) {
    return descriptor;
  }
  jclass clazz = GetObjectClassChecked(env, object, "CppTrackSelectionParameters");
  if (clazz == nullptr) {
    DeleteLocalRefIfNotNull(env, clazz);
    return descriptor;
  }
  descriptor.preferred_audio_language =
      GetStringFieldValue(env, object, clazz, "CppTrackSelectionParameters", "preferredAudioLanguage");
  descriptor.preferred_text_language =
      GetStringFieldValue(env, object, clazz, "CppTrackSelectionParameters", "preferredTextLanguage");
  jobjectArray preferred_audio_languages = static_cast<jobjectArray>(GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppTrackSelectionParameters",
      "preferredAudioLanguages",
      "[Ljava/lang/String;"));
  jobjectArray preferred_text_languages = static_cast<jobjectArray>(GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppTrackSelectionParameters",
      "preferredTextLanguages",
      "[Ljava/lang/String;"));
  descriptor.preferred_audio_languages = JStringArrayToVector(env, preferred_audio_languages);
  descriptor.preferred_text_languages = JStringArrayToVector(env, preferred_text_languages);
  DeleteLocalRefIfNotNull(env, preferred_audio_languages);
  DeleteLocalRefIfNotNull(env, preferred_text_languages);
  if (!descriptor.preferred_audio_languages.empty()) {
    descriptor.preferred_audio_language = descriptor.preferred_audio_languages.front();
  }
  if (!descriptor.preferred_text_languages.empty()) {
    descriptor.preferred_text_language = descriptor.preferred_text_languages.front();
  }
  descriptor.preferred_audio_role_flags =
      GetIntFieldValue(env, object, clazz, "CppTrackSelectionParameters", "preferredAudioRoleFlags");
  descriptor.preferred_text_role_flags =
      GetIntFieldValue(env, object, clazz, "CppTrackSelectionParameters", "preferredTextRoleFlags");
  descriptor.max_audio_channel_count =
      GetIntFieldValue(env, object, clazz, "CppTrackSelectionParameters", "maxAudioChannelCount");
  descriptor.max_audio_bitrate =
      GetIntFieldValue(env, object, clazz, "CppTrackSelectionParameters", "maxAudioBitrate");
  descriptor.max_video_width =
      GetIntFieldValue(env, object, clazz, "CppTrackSelectionParameters", "maxVideoWidth");
  descriptor.max_video_height =
      GetIntFieldValue(env, object, clazz, "CppTrackSelectionParameters", "maxVideoHeight");
  descriptor.max_video_bitrate =
      GetIntFieldValue(env, object, clazz, "CppTrackSelectionParameters", "maxVideoBitrate");
  descriptor.viewport_width =
      GetIntFieldValue(env, object, clazz, "CppTrackSelectionParameters", "viewportWidth");
  descriptor.viewport_height =
      GetIntFieldValue(env, object, clazz, "CppTrackSelectionParameters", "viewportHeight");
  descriptor.viewport_orientation_may_change = GetBooleanFieldValue(
      env, object, clazz, "CppTrackSelectionParameters", "viewportOrientationMayChange");
  descriptor.select_text_by_default =
      GetBooleanFieldValue(env, object, clazz, "CppTrackSelectionParameters", "selectTextByDefault");
  descriptor.ignored_text_selection_flags = GetIntFieldValue(
      env, object, clazz, "CppTrackSelectionParameters", "ignoredTextSelectionFlags");
  descriptor.select_undetermined_text_language = GetBooleanFieldValue(
      env,
      object,
      clazz,
      "CppTrackSelectionParameters",
      "selectUndeterminedTextLanguage");
  descriptor.force_lowest_bitrate =
      GetBooleanFieldValue(env, object, clazz, "CppTrackSelectionParameters", "forceLowestBitrate");
  descriptor.force_highest_supported_bitrate = GetBooleanFieldValue(
      env, object, clazz, "CppTrackSelectionParameters", "forceHighestSupportedBitrate");
  descriptor.disable_video =
      GetBooleanFieldValue(env, object, clazz, "CppTrackSelectionParameters", "disableVideo");
  descriptor.disable_audio =
      GetBooleanFieldValue(env, object, clazz, "CppTrackSelectionParameters", "disableAudio");
  descriptor.disable_text =
      GetBooleanFieldValue(env, object, clazz, "CppTrackSelectionParameters", "disableText");
  jintArray disabled_track_types = static_cast<jintArray>(GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppTrackSelectionParameters",
      "disabledTrackTypes",
      "[I"));
  descriptor.disabled_track_types = JIntArrayToVector(env, disabled_track_types);
  DeleteLocalRefIfNotNull(env, disabled_track_types);
  if (descriptor.disable_video &&
      std::find(
          descriptor.disabled_track_types.begin(),
          descriptor.disabled_track_types.end(),
          kTrackTypeVideo) == descriptor.disabled_track_types.end()) {
    descriptor.disabled_track_types.push_back(kTrackTypeVideo);
  }
  if (descriptor.disable_audio &&
      std::find(
          descriptor.disabled_track_types.begin(),
          descriptor.disabled_track_types.end(),
          kTrackTypeAudio) == descriptor.disabled_track_types.end()) {
    descriptor.disabled_track_types.push_back(kTrackTypeAudio);
  }
  if (descriptor.disable_text &&
      std::find(
          descriptor.disabled_track_types.begin(),
          descriptor.disabled_track_types.end(),
          kTrackTypeText) == descriptor.disabled_track_types.end()) {
    descriptor.disabled_track_types.push_back(kTrackTypeText);
  }
  jobjectArray overrides = static_cast<jobjectArray>(GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppTrackSelectionParameters",
      "overrides",
      "[Landroidx/media3/exoplayer/cppbridge/CppTrackSelectionOverride;"));
  if (overrides != nullptr) {
    jsize override_count = env->GetArrayLength(overrides);
    descriptor.overrides.reserve(static_cast<size_t>(override_count));
    for (jsize i = 0; i < override_count; ++i) {
      jobject override_object = env->GetObjectArrayElement(overrides, i);
      if (ClearJniExceptionIfPresent(env, "GetObjectArrayElement(CppTrackSelectionOverride)") ||
          override_object == nullptr) {
        continue;
      }
      jclass override_class =
          GetObjectClassChecked(env, override_object, "CppTrackSelectionOverride");
      if (override_class == nullptr) {
        DeleteLocalRefIfNotNull(env, override_object);
        continue;
      }
      TrackSelectionParametersDescriptor::OverrideDescriptor override_descriptor;
      override_descriptor.track_group_id = GetStringFieldValue(
          env, override_object, override_class, "CppTrackSelectionOverride", "trackGroupId");
      override_descriptor.track_type = GetIntFieldValue(
          env, override_object, override_class, "CppTrackSelectionOverride", "trackType");
      jintArray track_indices = static_cast<jintArray>(GetObjectFieldValue(
          env,
          override_object,
          override_class,
          "CppTrackSelectionOverride",
          "trackIndices",
          "[I"));
      override_descriptor.track_indices = JIntArrayToVector(env, track_indices);
      DeleteLocalRefIfNotNull(env, track_indices);
      descriptor.overrides.push_back(override_descriptor);
      env->DeleteLocalRef(override_class);
      env->DeleteLocalRef(override_object);
    }
    DeleteLocalRefIfNotNull(env, overrides);
  }
  env->DeleteLocalRef(clazz);
  return descriptor;
}

SeekParametersDescriptor FromJavaSeekParameters(JNIEnv* env, jobject object) {
  SeekParametersDescriptor descriptor;
  if (object == nullptr) {
    return descriptor;
  }
  jclass clazz = GetObjectClassChecked(env, object, "CppSeekParameters");
  if (clazz == nullptr) {
    DeleteLocalRefIfNotNull(env, clazz);
    return descriptor;
  }
  descriptor.tolerance_before_us =
      GetLongFieldValue(env, object, clazz, "CppSeekParameters", "toleranceBeforeUs");
  descriptor.tolerance_after_us =
      GetLongFieldValue(env, object, clazz, "CppSeekParameters", "toleranceAfterUs");
  env->DeleteLocalRef(clazz);
  return descriptor;
}

MediaMetadataSnapshot FromJavaMediaMetadata(JNIEnv* env, jobject object) {
  MediaMetadataSnapshot snapshot;
  if (object == nullptr) {
    return snapshot;
  }
  jclass clazz = GetObjectClassChecked(env, object, "CppMediaMetadata");
  if (clazz == nullptr) {
    DeleteLocalRefIfNotNull(env, clazz);
    return snapshot;
  }
  snapshot.title = GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "title");
  snapshot.title_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "titleToken");
  snapshot.artist = GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "artist");
  snapshot.artist_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "artistToken");
  snapshot.album_title =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "albumTitle");
  snapshot.album_title_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "albumTitleToken");
  snapshot.album_artist =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "albumArtist");
  snapshot.album_artist_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "albumArtistToken");
  snapshot.display_title =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "displayTitle");
  snapshot.display_title_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "displayTitleToken");
  snapshot.subtitle = GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "subtitle");
  snapshot.subtitle_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "subtitleToken");
  snapshot.description =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "description");
  snapshot.description_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "descriptionToken");
  snapshot.artwork_uri =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "artworkUri");
  jbyteArray artwork_data = static_cast<jbyteArray>(
      GetObjectFieldValue(env, object, clazz, "CppMediaMetadata", "artworkData", "[B"));
  snapshot.artwork_data = JByteArrayToVector(env, artwork_data);
  DeleteLocalRefIfNotNull(env, artwork_data);
  snapshot.artwork_data_type =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "artworkDataType");
  snapshot.duration_ms = GetLongFieldValue(env, object, clazz, "CppMediaMetadata", "durationMs");
  snapshot.track_number =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "trackNumber");
  snapshot.total_track_count =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "totalTrackCount");
  snapshot.is_browsable =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "isBrowsable");
  snapshot.is_playable =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "isPlayable");
  snapshot.folder_type = GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "folderType");
  snapshot.recording_year =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "recordingYear");
  snapshot.recording_month =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "recordingMonth");
  snapshot.recording_day =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "recordingDay");
  snapshot.release_year =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "releaseYear");
  snapshot.release_month =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "releaseMonth");
  snapshot.release_day =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "releaseDay");
  snapshot.writer = GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "writer");
  snapshot.writer_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "writerToken");
  snapshot.author = GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "author");
  snapshot.author_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "authorToken");
  snapshot.composer = GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "composer");
  snapshot.composer_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "composerToken");
  snapshot.conductor =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "conductor");
  snapshot.conductor_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "conductorToken");
  snapshot.disc_number =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "discNumber");
  snapshot.total_disc_count =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "totalDiscCount");
  snapshot.genre = GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "genre");
  snapshot.genre_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "genreToken");
  snapshot.compilation =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "compilation");
  snapshot.compilation_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "compilationToken");
  snapshot.media_type = GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "mediaType");
  snapshot.station = GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "station");
  snapshot.station_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "stationToken");
  snapshot.extras_present =
      GetBooleanFieldValue(env, object, clazz, "CppMediaMetadata", "extrasPresent");
  snapshot.extras_key_count =
      GetIntFieldValue(env, object, clazz, "CppMediaMetadata", "extrasKeyCount");
  snapshot.extras_token =
      GetStringFieldValue(env, object, clazz, "CppMediaMetadata", "extrasToken");
  env->DeleteLocalRef(clazz);
  return snapshot;
}

DeviceInfoDescriptor FromJavaDeviceInfo(JNIEnv* env, jobject object) {
  DeviceInfoDescriptor descriptor;
  if (object == nullptr) {
    return descriptor;
  }
  jclass clazz = GetObjectClassChecked(env, object, "CppDeviceInfo");
  if (clazz == nullptr) {
    DeleteLocalRefIfNotNull(env, clazz);
    return descriptor;
  }
  descriptor.playback_type =
      GetIntFieldValue(env, object, clazz, "CppDeviceInfo", "playbackType");
  descriptor.min_volume = GetIntFieldValue(env, object, clazz, "CppDeviceInfo", "minVolume");
  descriptor.max_volume = GetIntFieldValue(env, object, clazz, "CppDeviceInfo", "maxVolume");
  descriptor.routing_controller_id =
      GetStringFieldValue(env, object, clazz, "CppDeviceInfo", "routingControllerId");
  env->DeleteLocalRef(clazz);
  return descriptor;
}

VideoSizeSnapshot FromJavaVideoSize(JNIEnv* env, jobject object) {
  VideoSizeSnapshot snapshot;
  if (object == nullptr) {
    return snapshot;
  }
  jclass clazz = GetObjectClassChecked(env, object, "CppVideoSize");
  if (clazz == nullptr) {
    DeleteLocalRefIfNotNull(env, clazz);
    return snapshot;
  }
  snapshot.width = GetIntFieldValue(env, object, clazz, "CppVideoSize", "width");
  snapshot.height = GetIntFieldValue(env, object, clazz, "CppVideoSize", "height");
  snapshot.unapplied_rotation_degrees =
      GetIntFieldValue(env, object, clazz, "CppVideoSize", "unappliedRotationDegrees");
  snapshot.pixel_width_height_ratio =
      GetFloatFieldValue(env, object, clazz, "CppVideoSize", "pixelWidthHeightRatio");
  env->DeleteLocalRef(clazz);
  return snapshot;
}

PlaybackParametersSnapshot FromJavaPlaybackParameters(JNIEnv* env, jobject object) {
  PlaybackParametersSnapshot snapshot;
  if (object == nullptr) {
    return snapshot;
  }
  jclass clazz = GetObjectClassChecked(env, object, "CppPlaybackParameters");
  if (clazz == nullptr) {
    DeleteLocalRefIfNotNull(env, clazz);
    return snapshot;
  }
  snapshot.speed = GetFloatFieldValue(env, object, clazz, "CppPlaybackParameters", "speed");
  snapshot.pitch = GetFloatFieldValue(env, object, clazz, "CppPlaybackParameters", "pitch");
  env->DeleteLocalRef(clazz);
  return snapshot;
}

ApplicationLooperDescriptor FromJavaApplicationLooper(JNIEnv* env, jobject object) {
  ApplicationLooperDescriptor descriptor;
  if (object == nullptr) {
    return descriptor;
  }
  jclass clazz = GetObjectClassChecked(env, object, "CppApplicationLooper");
  if (clazz == nullptr) {
    DeleteLocalRefIfNotNull(env, clazz);
    return descriptor;
  }
  descriptor.thread_name =
      GetStringFieldValue(env, object, clazz, "CppApplicationLooper", "threadName");
  descriptor.thread_id =
      GetLongFieldValue(env, object, clazz, "CppApplicationLooper", "threadId");
  descriptor.is_current_thread =
      GetBooleanFieldValue(env, object, clazz, "CppApplicationLooper", "isCurrentThread");
  env->DeleteLocalRef(clazz);
  return descriptor;
}

jobject CreateJavaTracks(JNIEnv* env, const TracksSnapshot& tracks) {
  jclass track_info_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppTrackInfo");
  jclass track_group_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppTrackGroup");
  jclass tracks_class = FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppTracks");
  if (track_info_class == nullptr || track_group_class == nullptr || tracks_class == nullptr) {
    DeleteLocalRefIfNotNull(env, track_info_class);
    DeleteLocalRefIfNotNull(env, track_group_class);
    DeleteLocalRefIfNotNull(env, tracks_class);
    return nullptr;
  }
  jmethodID track_info_ctor = GetMethodChecked(
      env,
      track_info_class,
      "CppTrackInfo",
      "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
      "Ljava/lang/String;IIIIIFIFIIIIIIIIIZZZ)V");
  jmethodID track_group_ctor = GetMethodChecked(
      env,
      track_group_class,
      "CppTrackGroup",
      "<init>",
      "(Ljava/lang/String;Ljava/lang/String;IZZZZ[Landroidx/media3/exoplayer/cppbridge/CppTrackInfo;)V");
  jmethodID tracks_ctor = GetMethodChecked(
      env,
      tracks_class,
      "CppTracks",
      "<init>",
      "([Landroidx/media3/exoplayer/cppbridge/CppTrackGroup;ZZZZZZZZZZZZZZZZ)V");
  if (track_info_ctor == nullptr || track_group_ctor == nullptr || tracks_ctor == nullptr) {
    DeleteLocalRefIfNotNull(env, track_info_class);
    DeleteLocalRefIfNotNull(env, track_group_class);
    DeleteLocalRefIfNotNull(env, tracks_class);
    return nullptr;
  }

  jobjectArray groups =
      env->NewObjectArray(static_cast<jsize>(tracks.groups.size()), track_group_class, nullptr);
  if (ClearJniExceptionIfPresent(env, "NewObjectArray(CppTrackGroup)") || groups == nullptr) {
    DeleteLocalRefIfNotNull(env, groups);
    DeleteLocalRefIfNotNull(env, track_info_class);
    DeleteLocalRefIfNotNull(env, track_group_class);
    DeleteLocalRefIfNotNull(env, tracks_class);
    return nullptr;
  }

  for (jsize i = 0; i < static_cast<jsize>(tracks.groups.size()); ++i) {
    const auto& group = tracks.groups[static_cast<size_t>(i)];
    jobjectArray track_array =
        env->NewObjectArray(static_cast<jsize>(group.tracks.size()), track_info_class, nullptr);
    if (ClearJniExceptionIfPresent(env, "NewObjectArray(CppTrackInfo)") || track_array == nullptr) {
      DeleteLocalRefIfNotNull(env, track_array);
      DeleteLocalRefIfNotNull(env, groups);
      DeleteLocalRefIfNotNull(env, track_info_class);
      DeleteLocalRefIfNotNull(env, track_group_class);
      DeleteLocalRefIfNotNull(env, tracks_class);
      return nullptr;
    }

    for (jsize j = 0; j < static_cast<jsize>(group.tracks.size()); ++j) {
      const auto& track = group.tracks[static_cast<size_t>(j)];
      jstring id = track.id.empty() ? nullptr : NewStringUtfChecked(env, track.id, "CppTrackInfo.id");
      jstring language =
          track.language.empty() ? nullptr : NewStringUtfChecked(env, track.language, "CppTrackInfo.language");
      jstring label =
          track.label.empty() ? nullptr : NewStringUtfChecked(env, track.label, "CppTrackInfo.label");
      jstring label_token =
          track.label_token.empty()
              ? nullptr
              : NewStringUtfChecked(env, track.label_token, "CppTrackInfo.labelToken");
      jstring mime_type =
          track.mime_type.empty() ? nullptr : NewStringUtfChecked(env, track.mime_type, "CppTrackInfo.mimeType");
      jstring container_mime_type = track.container_mime_type.empty()
          ? nullptr
          : NewStringUtfChecked(env, track.container_mime_type, "CppTrackInfo.containerMimeType");
      jstring codecs =
          track.codecs.empty() ? nullptr : NewStringUtfChecked(env, track.codecs, "CppTrackInfo.codecs");
      jobject track_object = NewObjectChecked(
          env,
          track_info_class,
          track_info_ctor,
          "CppTrackInfo",
          id,
          language,
          label,
          label_token,
          mime_type,
          container_mime_type,
          codecs,
          static_cast<jint>(track.bitrate),
          static_cast<jint>(track.average_bitrate),
          static_cast<jint>(track.peak_bitrate),
          static_cast<jint>(track.width),
          static_cast<jint>(track.height),
          static_cast<jfloat>(track.frame_rate),
          static_cast<jint>(track.rotation_degrees),
          static_cast<jfloat>(track.pixel_width_height_ratio),
          static_cast<jint>(track.color_standard),
          static_cast<jint>(track.color_range),
          static_cast<jint>(track.color_transfer),
          static_cast<jint>(track.sample_rate),
          static_cast<jint>(track.channel_count),
          static_cast<jint>(track.accessibility_channel),
          static_cast<jint>(track.role_flags),
          static_cast<jint>(track.selection_flags),
          static_cast<jint>(track.format_support),
          static_cast<jboolean>(track.selected),
          static_cast<jboolean>(track.supported),
          static_cast<jboolean>(track.supported_within_capabilities));
      DeleteLocalRefIfNotNull(env, id);
      DeleteLocalRefIfNotNull(env, language);
      DeleteLocalRefIfNotNull(env, label);
      DeleteLocalRefIfNotNull(env, label_token);
      DeleteLocalRefIfNotNull(env, mime_type);
      DeleteLocalRefIfNotNull(env, container_mime_type);
      DeleteLocalRefIfNotNull(env, codecs);
      if (track_object == nullptr) {
        DeleteLocalRefIfNotNull(env, track_array);
        DeleteLocalRefIfNotNull(env, groups);
        DeleteLocalRefIfNotNull(env, track_info_class);
        DeleteLocalRefIfNotNull(env, track_group_class);
        DeleteLocalRefIfNotNull(env, tracks_class);
        return nullptr;
      }
      env->SetObjectArrayElement(track_array, j, track_object);
      if (ClearJniExceptionIfPresent(env, "SetObjectArrayElement(CppTrackInfo)")) {
        DeleteLocalRefIfNotNull(env, track_object);
        DeleteLocalRefIfNotNull(env, track_array);
        DeleteLocalRefIfNotNull(env, groups);
        DeleteLocalRefIfNotNull(env, track_info_class);
        DeleteLocalRefIfNotNull(env, track_group_class);
        DeleteLocalRefIfNotNull(env, tracks_class);
        return nullptr;
      }
      DeleteLocalRefIfNotNull(env, track_object);
    }

    jstring group_id = group.id.empty() ? nullptr : NewStringUtfChecked(env, group.id, "CppTrackGroup.id");
    jstring group_token =
        group.group_token.empty()
            ? nullptr
            : NewStringUtfChecked(env, group.group_token, "CppTrackGroup.groupToken");
    jobject group_object = NewObjectChecked(
        env,
        track_group_class,
        track_group_ctor,
        "CppTrackGroup",
        group_id,
        group_token,
        static_cast<jint>(group.type),
        static_cast<jboolean>(group.adaptive_supported),
        static_cast<jboolean>(group.selected),
        static_cast<jboolean>(group.supported),
        static_cast<jboolean>(group.supported_allowing_exceeds_capabilities),
        track_array);
    DeleteLocalRefIfNotNull(env, group_id);
    DeleteLocalRefIfNotNull(env, group_token);
    DeleteLocalRefIfNotNull(env, track_array);
    if (group_object == nullptr) {
      DeleteLocalRefIfNotNull(env, groups);
      DeleteLocalRefIfNotNull(env, track_info_class);
      DeleteLocalRefIfNotNull(env, track_group_class);
      DeleteLocalRefIfNotNull(env, tracks_class);
      return nullptr;
    }
    env->SetObjectArrayElement(groups, i, group_object);
    if (ClearJniExceptionIfPresent(env, "SetObjectArrayElement(CppTrackGroup)")) {
      DeleteLocalRefIfNotNull(env, group_object);
      DeleteLocalRefIfNotNull(env, groups);
      DeleteLocalRefIfNotNull(env, track_info_class);
      DeleteLocalRefIfNotNull(env, track_group_class);
      DeleteLocalRefIfNotNull(env, tracks_class);
      return nullptr;
    }
    DeleteLocalRefIfNotNull(env, group_object);
  }

  jobject result = NewObjectChecked(
      env,
      tracks_class,
      tracks_ctor,
      "CppTracks",
      groups,
      static_cast<jboolean>(tracks.contains_audio),
      static_cast<jboolean>(tracks.contains_video),
      static_cast<jboolean>(tracks.contains_text),
      static_cast<jboolean>(tracks.contains_image),
      static_cast<jboolean>(tracks.audio_selected),
      static_cast<jboolean>(tracks.video_selected),
      static_cast<jboolean>(tracks.text_selected),
      static_cast<jboolean>(tracks.image_selected),
      static_cast<jboolean>(tracks.audio_supported),
      static_cast<jboolean>(tracks.video_supported),
      static_cast<jboolean>(tracks.text_supported),
      static_cast<jboolean>(tracks.image_supported),
      static_cast<jboolean>(tracks.audio_supported_allowing_exceeds_capabilities),
      static_cast<jboolean>(tracks.video_supported_allowing_exceeds_capabilities),
      static_cast<jboolean>(tracks.text_supported_allowing_exceeds_capabilities),
      static_cast<jboolean>(tracks.image_supported_allowing_exceeds_capabilities));
  DeleteLocalRefIfNotNull(env, groups);
  DeleteLocalRefIfNotNull(env, track_info_class);
  DeleteLocalRefIfNotNull(env, track_group_class);
  DeleteLocalRefIfNotNull(env, tracks_class);
  return result;
}

TracksSnapshot FromJavaTracks(JNIEnv* env, jobject object) {
  TracksSnapshot snapshot;
  if (object == nullptr) {
    return snapshot;
  }
  jclass clazz = GetObjectClassChecked(env, object, "CppTracks");
  if (clazz == nullptr) {
    DeleteLocalRefIfNotNull(env, clazz);
    return snapshot;
  }
  jobjectArray groups = static_cast<jobjectArray>(GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppTracks",
      "groups",
      "[Landroidx/media3/exoplayer/cppbridge/CppTrackGroup;"));
  if (groups != nullptr) {
    jsize group_count = env->GetArrayLength(groups);
    snapshot.groups.reserve(static_cast<size_t>(group_count));
    for (jsize i = 0; i < group_count; ++i) {
      jobject group = env->GetObjectArrayElement(groups, i);
      if (ClearJniExceptionIfPresent(env, "GetObjectArrayElement(CppTrackGroup)") ||
          group == nullptr) {
        continue;
      }
      jclass group_class = GetObjectClassChecked(env, group, "CppTrackGroup");
      if (group_class == nullptr) {
        DeleteLocalRefIfNotNull(env, group);
        continue;
      }
      TrackGroupSnapshot group_snapshot;
      group_snapshot.id = GetStringFieldValue(env, group, group_class, "CppTrackGroup", "id");
      group_snapshot.group_token =
          GetStringFieldValue(env, group, group_class, "CppTrackGroup", "groupToken");
      group_snapshot.type = GetIntFieldValue(env, group, group_class, "CppTrackGroup", "type");
      group_snapshot.adaptive_supported =
          GetBooleanFieldValue(env, group, group_class, "CppTrackGroup", "adaptiveSupported");
      group_snapshot.selected =
          GetBooleanFieldValue(env, group, group_class, "CppTrackGroup", "selected");
      group_snapshot.supported =
          GetBooleanFieldValue(env, group, group_class, "CppTrackGroup", "supported");
      group_snapshot.supported_allowing_exceeds_capabilities = GetBooleanFieldValue(
          env,
          group,
          group_class,
          "CppTrackGroup",
          "supportedAllowingExceedsCapabilities");
      jobjectArray tracks = static_cast<jobjectArray>(GetObjectFieldValue(
          env,
          group,
          group_class,
          "CppTrackGroup",
          "tracks",
          "[Landroidx/media3/exoplayer/cppbridge/CppTrackInfo;"));
      if (tracks != nullptr) {
        jsize track_count = env->GetArrayLength(tracks);
        group_snapshot.tracks.reserve(static_cast<size_t>(track_count));
        for (jsize j = 0; j < track_count; ++j) {
          jobject track = env->GetObjectArrayElement(tracks, j);
          if (ClearJniExceptionIfPresent(env, "GetObjectArrayElement(CppTrackInfo)") ||
              track == nullptr) {
            continue;
          }
          jclass track_class = GetObjectClassChecked(env, track, "CppTrackInfo");
          if (track_class == nullptr) {
            DeleteLocalRefIfNotNull(env, track);
            continue;
          }
          TrackInfo track_info;
          track_info.id = GetStringFieldValue(env, track, track_class, "CppTrackInfo", "id");
          track_info.language =
              GetStringFieldValue(env, track, track_class, "CppTrackInfo", "language");
          track_info.label =
              GetStringFieldValue(env, track, track_class, "CppTrackInfo", "label");
          track_info.label_token =
              GetStringFieldValue(env, track, track_class, "CppTrackInfo", "labelToken");
          track_info.mime_type =
              GetStringFieldValue(env, track, track_class, "CppTrackInfo", "mimeType");
          track_info.container_mime_type = GetStringFieldValue(
              env, track, track_class, "CppTrackInfo", "containerMimeType");
          track_info.codecs =
              GetStringFieldValue(env, track, track_class, "CppTrackInfo", "codecs");
          track_info.bitrate =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "bitrate");
          track_info.average_bitrate =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "averageBitrate");
          track_info.peak_bitrate =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "peakBitrate");
          track_info.width = GetIntFieldValue(env, track, track_class, "CppTrackInfo", "width");
          track_info.height =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "height");
          track_info.frame_rate =
              GetFloatFieldValue(env, track, track_class, "CppTrackInfo", "frameRate");
          track_info.rotation_degrees =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "rotationDegrees");
          track_info.pixel_width_height_ratio = GetFloatFieldValue(
              env, track, track_class, "CppTrackInfo", "pixelWidthHeightRatio");
          track_info.color_standard =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "colorStandard");
          track_info.color_range =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "colorRange");
          track_info.color_transfer =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "colorTransfer");
          track_info.sample_rate =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "sampleRate");
          track_info.channel_count =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "channelCount");
          track_info.accessibility_channel = GetIntFieldValue(
              env, track, track_class, "CppTrackInfo", "accessibilityChannel");
          track_info.role_flags =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "roleFlags");
          track_info.selection_flags =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "selectionFlags");
          track_info.format_support =
              GetIntFieldValue(env, track, track_class, "CppTrackInfo", "formatSupport");
          track_info.selected =
              GetBooleanFieldValue(env, track, track_class, "CppTrackInfo", "selected");
          track_info.supported =
              GetBooleanFieldValue(env, track, track_class, "CppTrackInfo", "supported");
          track_info.supported_within_capabilities = GetBooleanFieldValue(
              env,
              track,
              track_class,
              "CppTrackInfo",
              "supportedWithinCapabilities");
          group_snapshot.tracks.push_back(track_info);
          env->DeleteLocalRef(track_class);
          env->DeleteLocalRef(track);
        }
        env->DeleteLocalRef(tracks);
      }
      snapshot.groups.push_back(group_snapshot);
      env->DeleteLocalRef(group_class);
      env->DeleteLocalRef(group);
    }
    env->DeleteLocalRef(groups);
  }
  snapshot.contains_audio =
      GetBooleanFieldValue(env, object, clazz, "CppTracks", "containsAudio");
  snapshot.contains_video =
      GetBooleanFieldValue(env, object, clazz, "CppTracks", "containsVideo");
  snapshot.contains_text =
      GetBooleanFieldValue(env, object, clazz, "CppTracks", "containsText");
  snapshot.contains_image =
      GetBooleanFieldValue(env, object, clazz, "CppTracks", "containsImage");
  snapshot.audio_selected =
      GetBooleanFieldValue(env, object, clazz, "CppTracks", "audioSelected");
  snapshot.video_selected =
      GetBooleanFieldValue(env, object, clazz, "CppTracks", "videoSelected");
  snapshot.text_selected =
      GetBooleanFieldValue(env, object, clazz, "CppTracks", "textSelected");
  snapshot.image_selected =
      GetBooleanFieldValue(env, object, clazz, "CppTracks", "imageSelected");
  snapshot.audio_supported =
      GetBooleanFieldValue(env, object, clazz, "CppTracks", "audioSupported");
  snapshot.video_supported =
      GetBooleanFieldValue(env, object, clazz, "CppTracks", "videoSupported");
  snapshot.text_supported =
      GetBooleanFieldValue(env, object, clazz, "CppTracks", "textSupported");
  snapshot.image_supported =
      GetBooleanFieldValue(env, object, clazz, "CppTracks", "imageSupported");
  snapshot.audio_supported_allowing_exceeds_capabilities = GetBooleanFieldValue(
      env,
      object,
      clazz,
      "CppTracks",
      "audioSupportedAllowingExceedsCapabilities");
  snapshot.video_supported_allowing_exceeds_capabilities = GetBooleanFieldValue(
      env,
      object,
      clazz,
      "CppTracks",
      "videoSupportedAllowingExceedsCapabilities");
  snapshot.text_supported_allowing_exceeds_capabilities = GetBooleanFieldValue(
      env,
      object,
      clazz,
      "CppTracks",
      "textSupportedAllowingExceedsCapabilities");
  snapshot.image_supported_allowing_exceeds_capabilities = GetBooleanFieldValue(
      env,
      object,
      clazz,
      "CppTracks",
      "imageSupportedAllowingExceedsCapabilities");
  env->DeleteLocalRef(clazz);
  return snapshot;
}

CueSnapshot FromJavaCues(JNIEnv* env, jobjectArray cues_array, int64_t presentation_time_us) {
  CueSnapshot snapshot;
  snapshot.presentation_time_us = presentation_time_us;
  if (cues_array == nullptr) {
    return snapshot;
  }
  jsize length = env->GetArrayLength(cues_array);
  snapshot.cue_count = static_cast<int>(length);
  snapshot.texts.reserve(static_cast<size_t>(length));
  snapshot.text_tokens.reserve(static_cast<size_t>(length));
  snapshot.bitmap_tokens.reserve(static_cast<size_t>(length));
  snapshot.cues.reserve(static_cast<size_t>(length));
  for (jsize i = 0; i < length; ++i) {
    jobject cue_object = env->GetObjectArrayElement(cues_array, i);
    if (ClearJniExceptionIfPresent(env, "GetObjectArrayElement(CppCue)") ||
        cue_object == nullptr) {
      continue;
    }
    jclass cue_class = GetObjectClassChecked(env, cue_object, "CppCue");
    if (cue_class == nullptr) {
      DeleteLocalRefIfNotNull(env, cue_object);
      continue;
    }
    CueSnapshot::CueInfo cue_info;
    cue_info.text = GetStringFieldValue(env, cue_object, cue_class, "CppCue", "text");
    cue_info.text_token =
        GetStringFieldValue(env, cue_object, cue_class, "CppCue", "textToken");
    cue_info.bitmap_token =
        GetStringFieldValue(env, cue_object, cue_class, "CppCue", "bitmapToken");
    cue_info.text_alignment =
        GetIntFieldValue(env, cue_object, cue_class, "CppCue", "textAlignment");
    cue_info.multi_row_alignment =
        GetIntFieldValue(env, cue_object, cue_class, "CppCue", "multiRowAlignment");
    cue_info.line = GetFloatFieldValue(env, cue_object, cue_class, "CppCue", "line");
    cue_info.line_type = GetIntFieldValue(env, cue_object, cue_class, "CppCue", "lineType");
    cue_info.line_anchor =
        GetIntFieldValue(env, cue_object, cue_class, "CppCue", "lineAnchor");
    cue_info.position = GetFloatFieldValue(env, cue_object, cue_class, "CppCue", "position");
    cue_info.position_anchor =
        GetIntFieldValue(env, cue_object, cue_class, "CppCue", "positionAnchor");
    cue_info.size = GetFloatFieldValue(env, cue_object, cue_class, "CppCue", "size");
    cue_info.bitmap_height =
        GetFloatFieldValue(env, cue_object, cue_class, "CppCue", "bitmapHeight");
    cue_info.text_size = GetFloatFieldValue(env, cue_object, cue_class, "CppCue", "textSize");
    cue_info.text_size_type =
        GetIntFieldValue(env, cue_object, cue_class, "CppCue", "textSizeType");
    cue_info.vertical_type =
        GetIntFieldValue(env, cue_object, cue_class, "CppCue", "verticalType");
    cue_info.shear_degrees =
        GetFloatFieldValue(env, cue_object, cue_class, "CppCue", "shearDegrees");
    cue_info.z_index = GetIntFieldValue(env, cue_object, cue_class, "CppCue", "zIndex");
    cue_info.window_color_set =
        GetBooleanFieldValue(env, cue_object, cue_class, "CppCue", "windowColorSet");
    cue_info.window_color =
        GetIntFieldValue(env, cue_object, cue_class, "CppCue", "windowColor");
    cue_info.has_bitmap =
        GetBooleanFieldValue(env, cue_object, cue_class, "CppCue", "hasBitmap");
    snapshot.texts.push_back(cue_info.text);
    snapshot.text_tokens.push_back(cue_info.text_token);
    snapshot.bitmap_tokens.push_back(cue_info.bitmap_token);
    snapshot.cues.push_back(cue_info);
    env->DeleteLocalRef(cue_class);
    env->DeleteLocalRef(cue_object);
  }
  return snapshot;
}

AvailableCommandsSnapshot FromJavaCommands(JNIEnv* env, jobject object) {
  AvailableCommandsSnapshot snapshot;
  if (object == nullptr) {
    return snapshot;
  }
  jclass clazz = GetObjectClassChecked(env, object, "CppCommands");
  if (clazz == nullptr) {
    DeleteLocalRefIfNotNull(env, clazz);
    return snapshot;
  }
  jintArray values = static_cast<jintArray>(GetObjectFieldValue(
      env, object, clazz, "CppCommands", "commandCodes", "[I"));
  snapshot.command_codes = JIntArrayToVector(env, values);
  DeleteLocalRefIfNotNull(env, values);
  env->DeleteLocalRef(clazz);
  return snapshot;
}

PlayerEventsSnapshot FromJavaPlayerEvents(JNIEnv* env, jobject object) {
  PlayerEventsSnapshot snapshot;
  if (object == nullptr) {
    return snapshot;
  }
  jclass clazz = GetObjectClassChecked(env, object, "CppPlayerEvents");
  if (clazz == nullptr) {
    DeleteLocalRefIfNotNull(env, clazz);
    return snapshot;
  }
  jintArray values = static_cast<jintArray>(GetObjectFieldValue(
      env, object, clazz, "CppPlayerEvents", "eventCodes", "[I"));
  snapshot.event_codes = JIntArrayToVector(env, values);
  DeleteLocalRefIfNotNull(env, values);
  env->DeleteLocalRef(clazz);
  return snapshot;
}

jobject CreateJavaMediaMetadata(JNIEnv* env, const MediaMetadataSnapshot& metadata) {
  jclass metadata_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppMediaMetadata");
  jmethodID ctor = GetMethodChecked(
      env,
      metadata_class,
      "CppMediaMetadata",
      "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[BIJIIIIIIIIIIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;ZILjava/lang/String;)V");
  if (metadata_class == nullptr || ctor == nullptr) {
    DeleteLocalRefIfNotNull(env, metadata_class);
    return nullptr;
  }
  jstring title =
      metadata.title.empty() ? nullptr : NewStringUtfChecked(env, metadata.title, "CppMediaMetadata.title");
  jstring title_token =
      metadata.title_token.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.title_token, "CppMediaMetadata.titleToken");
  jstring artist =
      metadata.artist.empty() ? nullptr : NewStringUtfChecked(env, metadata.artist, "CppMediaMetadata.artist");
  jstring artist_token =
      metadata.artist_token.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.artist_token, "CppMediaMetadata.artistToken");
  jstring album_title =
      metadata.album_title.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.album_title, "CppMediaMetadata.albumTitle");
  jstring album_title_token =
      metadata.album_title_token.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.album_title_token, "CppMediaMetadata.albumTitleToken");
  jstring album_artist =
      metadata.album_artist.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.album_artist, "CppMediaMetadata.albumArtist");
  jstring album_artist_token =
      metadata.album_artist_token.empty()
          ? nullptr
          : NewStringUtfChecked(
                env, metadata.album_artist_token, "CppMediaMetadata.albumArtistToken");
  jstring display_title =
      metadata.display_title.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.display_title, "CppMediaMetadata.displayTitle");
  jstring display_title_token =
      metadata.display_title_token.empty()
          ? nullptr
          : NewStringUtfChecked(
                env, metadata.display_title_token, "CppMediaMetadata.displayTitleToken");
  jstring subtitle =
      metadata.subtitle.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.subtitle, "CppMediaMetadata.subtitle");
  jstring subtitle_token =
      metadata.subtitle_token.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.subtitle_token, "CppMediaMetadata.subtitleToken");
  jstring description =
      metadata.description.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.description, "CppMediaMetadata.description");
  jstring description_token =
      metadata.description_token.empty()
          ? nullptr
          : NewStringUtfChecked(
                env, metadata.description_token, "CppMediaMetadata.descriptionToken");
  jstring artwork_uri =
      metadata.artwork_uri.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.artwork_uri, "CppMediaMetadata.artworkUri");
  jbyteArray artwork_data = CreateJavaByteArray(env, metadata.artwork_data);
  jstring writer =
      metadata.writer.empty() ? nullptr : NewStringUtfChecked(env, metadata.writer, "CppMediaMetadata.writer");
  jstring writer_token =
      metadata.writer_token.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.writer_token, "CppMediaMetadata.writerToken");
  jstring author =
      metadata.author.empty() ? nullptr : NewStringUtfChecked(env, metadata.author, "CppMediaMetadata.author");
  jstring author_token =
      metadata.author_token.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.author_token, "CppMediaMetadata.authorToken");
  jstring composer =
      metadata.composer.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.composer, "CppMediaMetadata.composer");
  jstring composer_token =
      metadata.composer_token.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.composer_token, "CppMediaMetadata.composerToken");
  jstring conductor =
      metadata.conductor.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.conductor, "CppMediaMetadata.conductor");
  jstring conductor_token =
      metadata.conductor_token.empty()
          ? nullptr
          : NewStringUtfChecked(
                env, metadata.conductor_token, "CppMediaMetadata.conductorToken");
  jstring genre =
      metadata.genre.empty() ? nullptr : NewStringUtfChecked(env, metadata.genre, "CppMediaMetadata.genre");
  jstring genre_token =
      metadata.genre_token.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.genre_token, "CppMediaMetadata.genreToken");
  jstring compilation =
      metadata.compilation.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.compilation, "CppMediaMetadata.compilation");
  jstring compilation_token =
      metadata.compilation_token.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.compilation_token, "CppMediaMetadata.compilationToken");
  jstring station =
      metadata.station.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.station, "CppMediaMetadata.station");
  jstring station_token =
      metadata.station_token.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.station_token, "CppMediaMetadata.stationToken");
  jstring extras_token =
      metadata.extras_token.empty()
          ? nullptr
          : NewStringUtfChecked(env, metadata.extras_token, "CppMediaMetadata.extrasToken");
  jobject object = NewObjectChecked(
      env,
      metadata_class,
      ctor,
      "CppMediaMetadata",
      title,
      title_token,
      artist,
      artist_token,
      album_title,
      album_title_token,
      album_artist,
      album_artist_token,
      display_title,
      display_title_token,
      subtitle,
      subtitle_token,
      description,
      description_token,
      artwork_uri,
      artwork_data,
      static_cast<jint>(metadata.artwork_data_type),
      static_cast<jlong>(metadata.duration_ms),
      static_cast<jint>(metadata.track_number),
      static_cast<jint>(metadata.total_track_count),
      static_cast<jint>(metadata.is_browsable),
      static_cast<jint>(metadata.is_playable),
      static_cast<jint>(metadata.folder_type),
      static_cast<jint>(metadata.recording_year),
      static_cast<jint>(metadata.recording_month),
      static_cast<jint>(metadata.recording_day),
      static_cast<jint>(metadata.release_year),
      static_cast<jint>(metadata.release_month),
      static_cast<jint>(metadata.release_day),
      writer,
      writer_token,
      author,
      author_token,
      composer,
      composer_token,
      conductor,
      conductor_token,
      static_cast<jint>(metadata.disc_number),
      static_cast<jint>(metadata.total_disc_count),
      genre,
      genre_token,
      compilation,
      compilation_token,
      static_cast<jint>(metadata.media_type),
      station,
      station_token,
      static_cast<jboolean>(metadata.extras_present),
      static_cast<jint>(metadata.extras_key_count),
      extras_token);
  DeleteLocalRefIfNotNull(env, title);
  DeleteLocalRefIfNotNull(env, title_token);
  DeleteLocalRefIfNotNull(env, artist);
  DeleteLocalRefIfNotNull(env, artist_token);
  DeleteLocalRefIfNotNull(env, album_title);
  DeleteLocalRefIfNotNull(env, album_title_token);
  DeleteLocalRefIfNotNull(env, album_artist);
  DeleteLocalRefIfNotNull(env, album_artist_token);
  DeleteLocalRefIfNotNull(env, display_title);
  DeleteLocalRefIfNotNull(env, display_title_token);
  DeleteLocalRefIfNotNull(env, subtitle);
  DeleteLocalRefIfNotNull(env, subtitle_token);
  DeleteLocalRefIfNotNull(env, description);
  DeleteLocalRefIfNotNull(env, description_token);
  DeleteLocalRefIfNotNull(env, artwork_uri);
  DeleteLocalRefIfNotNull(env, artwork_data);
  DeleteLocalRefIfNotNull(env, writer);
  DeleteLocalRefIfNotNull(env, writer_token);
  DeleteLocalRefIfNotNull(env, author);
  DeleteLocalRefIfNotNull(env, author_token);
  DeleteLocalRefIfNotNull(env, composer);
  DeleteLocalRefIfNotNull(env, composer_token);
  DeleteLocalRefIfNotNull(env, conductor);
  DeleteLocalRefIfNotNull(env, conductor_token);
  DeleteLocalRefIfNotNull(env, genre);
  DeleteLocalRefIfNotNull(env, genre_token);
  DeleteLocalRefIfNotNull(env, compilation);
  DeleteLocalRefIfNotNull(env, compilation_token);
  DeleteLocalRefIfNotNull(env, station);
  DeleteLocalRefIfNotNull(env, station_token);
  DeleteLocalRefIfNotNull(env, extras_token);
  env->DeleteLocalRef(metadata_class);
  return object;
}

jobjectArray CreateJavaCueArray(JNIEnv* env, const CueSnapshot& cues) {
  jclass cue_class = FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppCue");
  jmethodID cue_ctor = GetMethodChecked(
      env,
      cue_class,
      "CppCue",
      "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IIFIIFIFFFIIFIZIZ)V");
  if (cue_class == nullptr || cue_ctor == nullptr) {
    DeleteLocalRefIfNotNull(env, cue_class);
    return nullptr;
  }

  size_t cue_count = cues.cues.size();
  if (cue_count == 0) {
    cue_count = static_cast<size_t>(std::max(cues.cue_count, 0));
    cue_count = std::max(cue_count, cues.texts.size());
    cue_count = std::max(cue_count, cues.text_tokens.size());
    cue_count = std::max(cue_count, cues.bitmap_tokens.size());
  }
  jobjectArray result =
      env->NewObjectArray(static_cast<jsize>(cue_count), cue_class, nullptr);
  if (ClearJniExceptionIfPresent(env, "NewObjectArray(CppCue)") || result == nullptr) {
    DeleteLocalRefIfNotNull(env, cue_class);
    DeleteLocalRefIfNotNull(env, result);
    return nullptr;
  }

  for (jsize i = 0; i < static_cast<jsize>(cue_count); ++i) {
    const size_t cue_index = static_cast<size_t>(i);
    CueSnapshot::CueInfo empty_cue;
    const CueSnapshot::CueInfo& cue =
        cue_index < cues.cues.size() ? cues.cues[cue_index] : empty_cue;
    std::string cue_text =
        !cue.text.empty()
            ? cue.text
            : cue_index < cues.texts.size() ? cues.texts[cue_index] : "";
    std::string cue_text_token =
        !cue.text_token.empty()
            ? cue.text_token
            : cue_index < cues.text_tokens.size() ? cues.text_tokens[cue_index] : "";
    std::string cue_bitmap_token =
        !cue.bitmap_token.empty()
            ? cue.bitmap_token
            : cue_index < cues.bitmap_tokens.size() ? cues.bitmap_tokens[cue_index] : "";
    jstring text =
        cue_text.empty() ? nullptr : NewStringUtfChecked(env, cue_text, "CppCue.text");
    jstring text_token =
        cue_text_token.empty()
            ? nullptr
            : NewStringUtfChecked(env, cue_text_token, "CppCue.textToken");
    jstring bitmap_token =
        cue_bitmap_token.empty()
            ? nullptr
            : NewStringUtfChecked(env, cue_bitmap_token, "CppCue.bitmapToken");
    jobject cue_object = NewObjectChecked(
        env,
        cue_class,
        cue_ctor,
        "CppCue",
        text,
        text_token,
        bitmap_token,
        static_cast<jint>(cue.text_alignment),
        static_cast<jint>(cue.multi_row_alignment),
        static_cast<jfloat>(cue.line),
        static_cast<jint>(cue.line_type),
        static_cast<jint>(cue.line_anchor),
        static_cast<jfloat>(cue.position),
        static_cast<jint>(cue.position_anchor),
        static_cast<jfloat>(cue.size),
        static_cast<jfloat>(cue.bitmap_height),
        static_cast<jfloat>(cue.text_size),
        static_cast<jint>(cue.text_size_type),
        static_cast<jint>(cue.vertical_type),
        static_cast<jfloat>(cue.shear_degrees),
        static_cast<jint>(cue.z_index),
        static_cast<jboolean>(cue.window_color_set),
        static_cast<jint>(cue.window_color),
        static_cast<jboolean>(cue.has_bitmap || !cue_bitmap_token.empty()));
    DeleteLocalRefIfNotNull(env, text);
    DeleteLocalRefIfNotNull(env, text_token);
    DeleteLocalRefIfNotNull(env, bitmap_token);
    if (cue_object == nullptr) {
      DeleteLocalRefIfNotNull(env, result);
      DeleteLocalRefIfNotNull(env, cue_class);
      return nullptr;
    }
    env->SetObjectArrayElement(result, i, cue_object);
    if (ClearJniExceptionIfPresent(env, "SetObjectArrayElement(CppCue)")) {
      DeleteLocalRefIfNotNull(env, cue_object);
      DeleteLocalRefIfNotNull(env, result);
      DeleteLocalRefIfNotNull(env, cue_class);
      return nullptr;
    }
    DeleteLocalRefIfNotNull(env, cue_object);
  }

  DeleteLocalRefIfNotNull(env, cue_class);
  return result;
}

MediaItemDescriptor FromJavaMediaItem(JNIEnv* env, jobject object) {
  MediaItemDescriptor descriptor;
  if (object == nullptr) {
    return descriptor;
  }
  jclass clazz = GetObjectClassChecked(env, object, "CppMediaItem");
  if (clazz == nullptr) {
    DeleteLocalRefIfNotNull(env, clazz);
    return descriptor;
  }
  descriptor.uri = GetStringFieldValue(env, object, clazz, "CppMediaItem", "uri");
  descriptor.media_id = GetStringFieldValue(env, object, clazz, "CppMediaItem", "mediaId");
  descriptor.mime_type = GetStringFieldValue(env, object, clazz, "CppMediaItem", "mimeType");
  descriptor.source_type =
      static_cast<MediaSourceType>(GetIntFieldValue(env, object, clazz, "CppMediaItem", "sourceType"));
  descriptor.tag_present = GetBooleanFieldValue(env, object, clazz, "CppMediaItem", "tagPresent");
  descriptor.tag_string = GetStringFieldValue(env, object, clazz, "CppMediaItem", "tagString");
  descriptor.tag_token = GetStringFieldValue(env, object, clazz, "CppMediaItem", "tagToken");
  jobject metadata = GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppMediaItem",
      "mediaMetadata",
      "Landroidx/media3/exoplayer/cppbridge/CppMediaMetadata;");
  descriptor.media_metadata = FromJavaMediaMetadata(env, metadata);
  DeleteLocalRefIfNotNull(env, metadata);
  jobject request_metadata = GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppMediaItem",
      "requestMetadata",
      "Landroidx/media3/exoplayer/cppbridge/CppRequestMetadata;");
  if (request_metadata != nullptr) {
    jclass request_metadata_class =
        GetObjectClassChecked(env, request_metadata, "CppRequestMetadata");
    if (request_metadata_class != nullptr) {
      descriptor.request_metadata.media_uri = GetStringFieldValue(
          env, request_metadata, request_metadata_class, "CppRequestMetadata", "mediaUri");
      descriptor.request_metadata.search_query = GetStringFieldValue(
          env, request_metadata, request_metadata_class, "CppRequestMetadata", "searchQuery");
      descriptor.request_metadata.extras_present = GetBooleanFieldValue(
          env, request_metadata, request_metadata_class, "CppRequestMetadata", "extrasPresent");
      descriptor.request_metadata.extras_key_count = GetIntFieldValue(
          env, request_metadata, request_metadata_class, "CppRequestMetadata", "extrasKeyCount");
      descriptor.request_metadata.extras_token = GetStringFieldValue(
          env, request_metadata, request_metadata_class, "CppRequestMetadata", "extrasToken");
      env->DeleteLocalRef(request_metadata_class);
    }
    DeleteLocalRefIfNotNull(env, request_metadata);
  }

  jobject ads = GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppMediaItem",
      "adsConfiguration",
      "Landroidx/media3/exoplayer/cppbridge/CppAdsConfiguration;");
  if (ads != nullptr) {
    jclass ads_class = GetObjectClassChecked(env, ads, "CppAdsConfiguration");
    if (ads_class != nullptr) {
      descriptor.ads_configuration.ad_tag_uri =
          GetStringFieldValue(env, ads, ads_class, "CppAdsConfiguration", "adTagUri");
      descriptor.ads_configuration.ads_id =
          GetStringFieldValue(env, ads, ads_class, "CppAdsConfiguration", "adsId");
      descriptor.ads_configuration.ads_id_token =
          GetStringFieldValue(env, ads, ads_class, "CppAdsConfiguration", "adsIdToken");
      env->DeleteLocalRef(ads_class);
    }
    env->DeleteLocalRef(ads);
  }

  jobjectArray subtitles = static_cast<jobjectArray>(GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppMediaItem",
      "subtitleConfigurations",
      "[Landroidx/media3/exoplayer/cppbridge/CppSubtitleConfiguration;"));
  if (subtitles != nullptr) {
    jsize subtitle_count = env->GetArrayLength(subtitles);
    descriptor.subtitle_configurations.reserve(static_cast<size_t>(subtitle_count));
    for (jsize i = 0; i < subtitle_count; ++i) {
      jobject subtitle = env->GetObjectArrayElement(subtitles, i);
      if (ClearJniExceptionIfPresent(env, "GetObjectArrayElement(CppSubtitleConfiguration)") ||
          subtitle == nullptr) {
        continue;
      }
      jclass subtitle_class =
          GetObjectClassChecked(env, subtitle, "CppSubtitleConfiguration");
      if (subtitle_class == nullptr) {
        DeleteLocalRefIfNotNull(env, subtitle);
        continue;
      }
      MediaItemDescriptor::SubtitleConfigurationDescriptor subtitle_descriptor;
      subtitle_descriptor.uri =
          GetStringFieldValue(env, subtitle, subtitle_class, "CppSubtitleConfiguration", "uri");
      subtitle_descriptor.mime_type = GetStringFieldValue(
          env, subtitle, subtitle_class, "CppSubtitleConfiguration", "mimeType");
      subtitle_descriptor.language = GetStringFieldValue(
          env, subtitle, subtitle_class, "CppSubtitleConfiguration", "language");
      subtitle_descriptor.label = GetStringFieldValue(
          env, subtitle, subtitle_class, "CppSubtitleConfiguration", "label");
      subtitle_descriptor.id =
          GetStringFieldValue(env, subtitle, subtitle_class, "CppSubtitleConfiguration", "id");
      subtitle_descriptor.selection_flags = GetIntFieldValue(
          env, subtitle, subtitle_class, "CppSubtitleConfiguration", "selectionFlags");
      subtitle_descriptor.role_flags = GetIntFieldValue(
          env, subtitle, subtitle_class, "CppSubtitleConfiguration", "roleFlags");
      descriptor.subtitle_configurations.push_back(subtitle_descriptor);
      env->DeleteLocalRef(subtitle_class);
      env->DeleteLocalRef(subtitle);
    }
    env->DeleteLocalRef(subtitles);
  }

  jobject clipping = GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppMediaItem",
      "clippingConfiguration",
      "Landroidx/media3/exoplayer/cppbridge/CppClippingConfiguration;");
  if (clipping != nullptr) {
    jclass clipping_class = GetObjectClassChecked(env, clipping, "CppClippingConfiguration");
    if (clipping_class != nullptr) {
      descriptor.clipping_configuration.start_position_ms = GetLongFieldValue(
          env, clipping, clipping_class, "CppClippingConfiguration", "startPositionMs");
      descriptor.clipping_configuration.end_position_ms = GetLongFieldValue(
          env, clipping, clipping_class, "CppClippingConfiguration", "endPositionMs");
      descriptor.clipping_configuration.relative_to_live_window = GetBooleanFieldValue(
          env,
          clipping,
          clipping_class,
          "CppClippingConfiguration",
          "relativeToLiveWindow");
      descriptor.clipping_configuration.relative_to_default_position = GetBooleanFieldValue(
          env,
          clipping,
          clipping_class,
          "CppClippingConfiguration",
          "relativeToDefaultPosition");
      descriptor.clipping_configuration.starts_at_key_frame = GetBooleanFieldValue(
          env, clipping, clipping_class, "CppClippingConfiguration", "startsAtKeyFrame");
      descriptor.clipping_configuration.allow_unseekable_media = GetBooleanFieldValue(
          env, clipping, clipping_class, "CppClippingConfiguration", "allowUnseekableMedia");
      env->DeleteLocalRef(clipping_class);
    }
    env->DeleteLocalRef(clipping);
  }

  jobject live = GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppMediaItem",
      "liveConfiguration",
      "Landroidx/media3/exoplayer/cppbridge/CppLiveConfiguration;");
  if (live != nullptr) {
    jclass live_class = GetObjectClassChecked(env, live, "CppLiveConfiguration");
    if (live_class != nullptr) {
      descriptor.live_configuration.target_offset_ms =
          GetLongFieldValue(env, live, live_class, "CppLiveConfiguration", "targetOffsetMs");
      descriptor.live_configuration.min_offset_ms =
          GetLongFieldValue(env, live, live_class, "CppLiveConfiguration", "minOffsetMs");
      descriptor.live_configuration.max_offset_ms =
          GetLongFieldValue(env, live, live_class, "CppLiveConfiguration", "maxOffsetMs");
      descriptor.live_configuration.min_playback_speed =
          GetFloatFieldValue(env, live, live_class, "CppLiveConfiguration", "minPlaybackSpeed");
      descriptor.live_configuration.max_playback_speed =
          GetFloatFieldValue(env, live, live_class, "CppLiveConfiguration", "maxPlaybackSpeed");
      env->DeleteLocalRef(live_class);
    }
    env->DeleteLocalRef(live);
  }

  jobject drm = GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppMediaItem",
      "drmConfiguration",
      "Landroidx/media3/exoplayer/cppbridge/CppDrmConfiguration;");
  if (drm != nullptr) {
    jclass drm_class = GetObjectClassChecked(env, drm, "CppDrmConfiguration");
    if (drm_class != nullptr) {
      descriptor.drm_configuration.scheme_uuid =
          GetStringFieldValue(env, drm, drm_class, "CppDrmConfiguration", "schemeUuid");
      descriptor.drm_configuration.license_uri =
          GetStringFieldValue(env, drm, drm_class, "CppDrmConfiguration", "licenseUri");
      jobjectArray header_names = static_cast<jobjectArray>(GetObjectFieldValue(
          env,
          drm,
          drm_class,
          "CppDrmConfiguration",
          "licenseRequestHeaderNames",
          "[Ljava/lang/String;"));
      jobjectArray header_values = static_cast<jobjectArray>(GetObjectFieldValue(
          env,
          drm,
          drm_class,
          "CppDrmConfiguration",
          "licenseRequestHeaderValues",
          "[Ljava/lang/String;"));
      jintArray forced_session_track_types = static_cast<jintArray>(GetObjectFieldValue(
          env, drm, drm_class, "CppDrmConfiguration", "forcedSessionTrackTypes", "[I"));
      jbyteArray key_set_id = static_cast<jbyteArray>(
          GetObjectFieldValue(env, drm, drm_class, "CppDrmConfiguration", "keySetId", "[B"));
      descriptor.drm_configuration.license_request_header_names =
          JStringArrayToVector(env, header_names);
      descriptor.drm_configuration.license_request_header_values =
          JStringArrayToVector(env, header_values);
      descriptor.drm_configuration.forced_session_track_types =
          JIntArrayToVector(env, forced_session_track_types);
      descriptor.drm_configuration.key_set_id = JByteArrayToVector(env, key_set_id);
      descriptor.drm_configuration.multi_session =
          GetBooleanFieldValue(env, drm, drm_class, "CppDrmConfiguration", "multiSession");
      descriptor.drm_configuration.force_default_license_uri = GetBooleanFieldValue(
          env, drm, drm_class, "CppDrmConfiguration", "forceDefaultLicenseUri");
      descriptor.drm_configuration.play_clear_content_without_key = GetBooleanFieldValue(
          env, drm, drm_class, "CppDrmConfiguration", "playClearContentWithoutKey");
      DeleteLocalRefIfNotNull(env, header_names);
      DeleteLocalRefIfNotNull(env, header_values);
      DeleteLocalRefIfNotNull(env, forced_session_track_types);
      DeleteLocalRefIfNotNull(env, key_set_id);
      env->DeleteLocalRef(drm_class);
    }
    env->DeleteLocalRef(drm);
  }
  env->DeleteLocalRef(clazz);
  return descriptor;
}

PositionInfoSnapshot FromJavaPositionInfo(JNIEnv* env, jobject object) {
  PositionInfoSnapshot snapshot;
  if (object == nullptr) {
    return snapshot;
  }
  jclass clazz = GetObjectClassChecked(env, object, "CppPositionInfo");
  if (clazz == nullptr) {
    DeleteLocalRefIfNotNull(env, clazz);
    return snapshot;
  }
  snapshot.media_item_index =
      GetIntFieldValue(env, object, clazz, "CppPositionInfo", "mediaItemIndex");
  jobject media_item = GetObjectFieldValue(
      env,
      object,
      clazz,
      "CppPositionInfo",
      "mediaItem",
      "Landroidx/media3/exoplayer/cppbridge/CppMediaItem;");
  snapshot.media_item = FromJavaMediaItem(env, media_item);
  DeleteLocalRefIfNotNull(env, media_item);
  snapshot.period_index =
      GetIntFieldValue(env, object, clazz, "CppPositionInfo", "periodIndex");
  snapshot.position_ms =
      GetLongFieldValue(env, object, clazz, "CppPositionInfo", "positionMs");
  snapshot.content_position_ms =
      GetLongFieldValue(env, object, clazz, "CppPositionInfo", "contentPositionMs");
  snapshot.ad_group_index =
      GetIntFieldValue(env, object, clazz, "CppPositionInfo", "adGroupIndex");
  snapshot.ad_index_in_ad_group =
      GetIntFieldValue(env, object, clazz, "CppPositionInfo", "adIndexInAdGroup");
  env->DeleteLocalRef(clazz);
  return snapshot;
}

}  // namespace androidx::media3::cppbridge::internal
