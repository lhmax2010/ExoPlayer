#include "exoplayer_cppbridge_jni_internal.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

namespace androidx::media3::cppbridge::internal {

namespace {

std::string BuildPointerSummary(const void* value) {
  return std::to_string(
      static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(value)));
}

constexpr char kListenerSmokeBridgeBuildMarker[] = "listener-smoke-bridge-v2026-04-02-13";

void LogListenerSmokeBridgeBuildMarkerOnce(const char* source) {
  static std::atomic<bool> logged{false};
  bool expected = false;
  if (!logged.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
    return;
  }
  LogInfo(
      std::string("listenerSmoke bridgeBuildMarker=") + kListenerSmokeBridgeBuildMarker +
      ",source=" + (source != nullptr ? source : "<none>"));
}

bool ShouldTraceSmokeListenerCallback(const char* callback_name) {
  if (callback_name == nullptr) {
    return false;
  }
  static constexpr const char* kCallbackNames[] = {
      "OnTimelineChanged",
      "OnTracksChanged",
      "OnPositionDiscontinuity",
      "OnCues",
      "OnRepeatModeChanged",
      "OnShuffleModeEnabledChanged",
      "OnSeekBackIncrementChanged",
      "OnSeekForwardIncrementChanged",
      "OnMaxSeekToPreviousPositionChanged",
      "OnTrackSelectionParametersChanged",
      "OnPlaybackParametersChanged",
      "OnPlaybackSuppressionReasonChanged",
      "OnAvailableCommandsChanged",
      "OnEvents",
      "OnMediaMetadataChanged",
      "OnPlaylistMetadataChanged",
  };
  for (const char* candidate : kCallbackNames) {
    if (std::strcmp(candidate, callback_name) == 0) {
      return true;
    }
  }
  return false;
}

jobject CreateJavaCommands(JNIEnv* env, const std::vector<int>& command_codes) {
  jintArray values = CreateJavaIntArray(env, command_codes);
  if (values == nullptr) {
    return nullptr;
  }
  jclass clazz = FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppCommands");
  jmethodID ctor = GetMethodChecked(env, clazz, "CppCommands", "<init>", "([I)V");
  jobject object = NewObjectChecked(env, clazz, ctor, "CppCommands", values);
  DeleteLocalRefIfNotNull(env, values);
  DeleteLocalRefIfNotNull(env, clazz);
  return object;
}

jobject CreateJavaPlayerEvents(JNIEnv* env, const std::vector<int>& event_codes) {
  jintArray values = CreateJavaIntArray(env, event_codes);
  if (values == nullptr) {
    return nullptr;
  }
  jclass clazz = FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppPlayerEvents");
  jmethodID ctor = GetMethodChecked(env, clazz, "CppPlayerEvents", "<init>", "([I)V");
  jobject object = NewObjectChecked(env, clazz, ctor, "CppPlayerEvents", values);
  DeleteLocalRefIfNotNull(env, values);
  DeleteLocalRefIfNotNull(env, clazz);
  return object;
}

jobject CreateJavaDeviceInfo(JNIEnv* env, const DeviceInfoDescriptor& device_info) {
  jclass clazz = FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppDeviceInfo");
  jmethodID ctor = GetMethodChecked(
      env,
      clazz,
      "CppDeviceInfo",
      "<init>",
      "(IIILjava/lang/String;)V");
  jstring routing_controller_id =
      device_info.routing_controller_id.empty()
          ? nullptr
          : NewStringUtfChecked(
                env,
                device_info.routing_controller_id,
                "CppDeviceInfo.routingControllerId");
  jobject object = NewObjectChecked(
      env,
      clazz,
      ctor,
      "CppDeviceInfo",
      static_cast<jint>(device_info.playback_type),
      static_cast<jint>(device_info.min_volume),
      static_cast<jint>(device_info.max_volume),
      routing_controller_id);
  DeleteLocalRefIfNotNull(env, routing_controller_id);
  DeleteLocalRefIfNotNull(env, clazz);
  return object;
}

}  // namespace

class JniExoPlayerBridge : public ExoPlayerBridge {
 public:
  JniExoPlayerBridge(JavaVM* java_vm, jobject java_bridge)
      : java_vm_(java_vm), java_bridge_(java_bridge) {}

  ~JniExoPlayerBridge() override = default;

  static std::shared_ptr<JniExoPlayerBridge> Create(
      JNIEnv* env,
      jobject context,
      const PlayerConfig& config) {
    JavaVM* java_vm = nullptr;
    env->GetJavaVM(&java_vm);

    jclass bridge_class =
        FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppExoPlayerBridge");
    jclass config_class =
        FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppPlayerConfig");
    jclass media_source_factory_config_class =
        FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppMediaSourceFactoryConfig");
    if (bridge_class == nullptr || config_class == nullptr ||
        media_source_factory_config_class == nullptr) {
      DeleteLocalRefIfNotNull(env, media_source_factory_config_class);
      DeleteLocalRefIfNotNull(env, config_class);
      DeleteLocalRefIfNotNull(env, bridge_class);
      return nullptr;
    }

    jmethodID media_source_factory_config_ctor = GetMethodChecked(
        env,
        media_source_factory_config_class,
        "CppMediaSourceFactoryConfig",
        "<init>",
        "(Ljava/lang/String;ZZ[Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;IIZJJJFF)V");
    jstring factory_token = NewStringUtfChecked(
        env,
        config.media_source_factory_config.factory_token,
        "CppMediaSourceFactoryConfig.factoryToken");
    jobjectArray header_names = CreateJavaStringArray(
        env, config.media_source_factory_config.default_request_header_names);
    jobjectArray header_values = CreateJavaStringArray(
        env, config.media_source_factory_config.default_request_header_values);
    bool header_names_ok =
        config.media_source_factory_config.default_request_header_names.empty() ||
        header_names != nullptr;
    bool header_values_ok =
        config.media_source_factory_config.default_request_header_values.empty() ||
        header_values != nullptr;
    jstring user_agent = NewStringUtfChecked(
        env, config.media_source_factory_config.user_agent, "CppMediaSourceFactoryConfig.userAgent");
    jobject java_media_source_factory_config = NewObjectChecked(
        env,
        media_source_factory_config_class,
        media_source_factory_config_ctor,
        "CppMediaSourceFactoryConfig",
        factory_token,
        static_cast<jboolean>(
            config.media_source_factory_config.parse_subtitles_during_extraction),
        static_cast<jboolean>(config.media_source_factory_config.load_only_selected_tracks),
        header_names,
        header_values,
        user_agent,
        static_cast<jint>(config.media_source_factory_config.connect_timeout_ms),
        static_cast<jint>(config.media_source_factory_config.read_timeout_ms),
        static_cast<jboolean>(config.media_source_factory_config.allow_cross_protocol_redirects),
        static_cast<jlong>(config.media_source_factory_config.live_target_offset_ms),
        static_cast<jlong>(config.media_source_factory_config.live_min_offset_ms),
        static_cast<jlong>(config.media_source_factory_config.live_max_offset_ms),
        static_cast<jfloat>(config.media_source_factory_config.live_min_speed),
        static_cast<jfloat>(config.media_source_factory_config.live_max_speed));
    if (factory_token == nullptr || user_agent == nullptr || !header_names_ok ||
        !header_values_ok || java_media_source_factory_config == nullptr) {
      DeleteLocalRefIfNotNull(env, factory_token);
      DeleteLocalRefIfNotNull(env, header_names);
      DeleteLocalRefIfNotNull(env, header_values);
      DeleteLocalRefIfNotNull(env, user_agent);
      DeleteLocalRefIfNotNull(env, java_media_source_factory_config);
      env->DeleteLocalRef(media_source_factory_config_class);
      env->DeleteLocalRef(config_class);
      env->DeleteLocalRef(bridge_class);
      return nullptr;
    }

    jmethodID config_ctor = GetMethodChecked(
        env,
        config_class,
        "CppPlayerConfig",
        "<init>",
        "(ZZZLandroidx/media3/exoplayer/cppbridge/CppMediaSourceFactoryConfig;JJIIZJ)V");
    jobject java_config = NewObjectChecked(
        env,
        config_class,
        config_ctor,
        "CppPlayerConfig",
        static_cast<jboolean>(config.handle_audio_focus),
        static_cast<jboolean>(config.handle_audio_becoming_noisy),
        static_cast<jboolean>(config.use_lazy_preparation),
        java_media_source_factory_config,
        static_cast<jlong>(config.seek_back_increment_ms),
        static_cast<jlong>(config.seek_forward_increment_ms),
        static_cast<jint>(config.wake_mode),
        static_cast<jint>(config.priority),
        static_cast<jboolean>(config.use_priority_task_manager),
        static_cast<jlong>(config.target_preload_duration_us));
    if (java_config == nullptr) {
      DeleteLocalRefIfNotNull(env, java_media_source_factory_config);
      DeleteLocalRefIfNotNull(env, factory_token);
      DeleteLocalRefIfNotNull(env, header_names);
      DeleteLocalRefIfNotNull(env, header_values);
      DeleteLocalRefIfNotNull(env, user_agent);
      env->DeleteLocalRef(media_source_factory_config_class);
      env->DeleteLocalRef(config_class);
      env->DeleteLocalRef(bridge_class);
      return nullptr;
    }

    jmethodID bridge_ctor = GetMethodChecked(
        env,
        bridge_class,
        "CppExoPlayerBridge",
        "<init>",
        "(Landroid/content/Context;JLandroidx/media3/exoplayer/cppbridge/CppPlayerConfig;)V");

    auto bridge = std::make_shared<JniExoPlayerBridge>(java_vm, nullptr);
    jobject java_bridge_local = NewObjectChecked(env,
                                                 bridge_class,
                                                 bridge_ctor,
                                                 "CppExoPlayerBridge",
                                                 context,
                                                 reinterpret_cast<jlong>(bridge.get()),
                                                 java_config);
    if (java_bridge_local == nullptr) {
      env->DeleteLocalRef(java_config);
      env->DeleteLocalRef(java_media_source_factory_config);
      DeleteLocalRefIfNotNull(env, factory_token);
      DeleteLocalRefIfNotNull(env, header_names);
      DeleteLocalRefIfNotNull(env, header_values);
      DeleteLocalRefIfNotNull(env, user_agent);
      env->DeleteLocalRef(media_source_factory_config_class);
      env->DeleteLocalRef(config_class);
      env->DeleteLocalRef(bridge_class);
      return nullptr;
    }
    bridge->java_bridge_ = env->NewGlobalRef(java_bridge_local);
    if (ClearJniExceptionIfPresent(env, "NewGlobalRef(CppExoPlayerBridge)") ||
        bridge->java_bridge_ == nullptr) {
      LogError("Failed to create global ref for CppExoPlayerBridge");
      env->DeleteLocalRef(java_bridge_local);
      env->DeleteLocalRef(java_config);
      env->DeleteLocalRef(java_media_source_factory_config);
      DeleteLocalRefIfNotNull(env, factory_token);
      DeleteLocalRefIfNotNull(env, header_names);
      DeleteLocalRefIfNotNull(env, header_values);
      DeleteLocalRefIfNotNull(env, user_agent);
      env->DeleteLocalRef(media_source_factory_config_class);
      env->DeleteLocalRef(config_class);
      env->DeleteLocalRef(bridge_class);
      return nullptr;
    }

    DeleteLocalRefIfNotNull(env, java_bridge_local);
    DeleteLocalRefIfNotNull(env, java_config);
    DeleteLocalRefIfNotNull(env, java_media_source_factory_config);
    DeleteLocalRefIfNotNull(env, factory_token);
    DeleteLocalRefIfNotNull(env, header_names);
    DeleteLocalRefIfNotNull(env, header_values);
    DeleteLocalRefIfNotNull(env, user_agent);
    DeleteLocalRefIfNotNull(env, media_source_factory_config_class);
    DeleteLocalRefIfNotNull(env, config_class);
    DeleteLocalRefIfNotNull(env, bridge_class);
    RegisterBridge(bridge);
    return bridge;
  }

  void SetListener(PlayerListener* listener) override {
    if (releasing_.load(std::memory_order_acquire)) {
      return;
    }
    LogListenerSmokeBridgeBuildMarkerOnce("JniExoPlayerBridge::SetListener");
    std::lock_guard<std::mutex> lock(state_mutex_);
    listener_ = listener;
    LogInfo("JniExoPlayerBridge SetListener listenerPtr=" + BuildPointerSummary(listener) + "," +
            BuildListenerStateLocked());
  }

  void RemoveListener(PlayerListener* listener) override {
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (listener == nullptr || listener == listener_) {
      const bool had_listener = listener_ != nullptr;
      listener_ = nullptr;
      LogInfo(
          "JniExoPlayerBridge RemoveListener listenerPtr=" + BuildPointerSummary(listener) + "," +
          BuildListenerStateLocked());
      if (had_listener) {
        listener_callback_drained_.wait(
            lock, [this]() { return in_flight_listener_callback_count_ == 0; });
        LogInfo("JniExoPlayerBridge RemoveListener drained," + BuildListenerStateLocked());
      }
    }
  }

  void SetImageOutputListener(ImageOutputListener* listener) override {
    if (releasing_.load(std::memory_order_acquire)) {
      return;
    }
    std::lock_guard<std::mutex> lock(state_mutex_);
    image_output_listener_ = listener;
  }

  void RemoveImageOutputListener(ImageOutputListener* listener) override {
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (listener == nullptr || listener == image_output_listener_) {
      const bool had_listener = image_output_listener_ != nullptr;
      image_output_listener_ = nullptr;
      if (had_listener) {
        image_output_callback_drained_.wait(
            lock, [this]() { return in_flight_image_output_callback_count_ == 0; });
      }
    }
  }

  void BindPlayerView(JNIEnv* env, jobject player_view) override {
    CallBridgeVoid(env, "bindPlayerView", "(Landroidx/media3/ui/PlayerView;)V", player_view);
  }

  void UnbindPlayerView(JNIEnv* env, jobject player_view) override {
    CallBridgeVoid(env, "unbindPlayerView", "(Landroidx/media3/ui/PlayerView;)V", player_view);
  }

  void SetVideoSurface(JNIEnv* env, jobject surface) override {
    CallBridgeVoid(env, "setVideoSurface", "(Landroid/view/Surface;)V", surface);
  }

  void ClearVideoSurface(JNIEnv* env) override {
    CallVoidNoArgs(env, "clearVideoSurface");
  }

  void ClearVideoSurface(JNIEnv* env, jobject surface) override {
    CallBridgeVoid(env, "clearVideoSurface", "(Landroid/view/Surface;)V", surface);
  }

  void SetVideoSurfaceHolder(JNIEnv* env, jobject surface_holder) override {
    CallBridgeVoid(
        env,
        "setVideoSurfaceHolder",
        "(Landroid/view/SurfaceHolder;)V",
        surface_holder);
  }

  void ClearVideoSurfaceHolder(JNIEnv* env, jobject surface_holder) override {
    CallBridgeVoid(
        env,
        "clearVideoSurfaceHolder",
        "(Landroid/view/SurfaceHolder;)V",
        surface_holder);
  }

  void SetVideoSurfaceView(JNIEnv* env, jobject surface_view) override {
    CallBridgeVoid(env, "setVideoSurfaceView", "(Landroid/view/SurfaceView;)V", surface_view);
  }

  void ClearVideoSurfaceView(JNIEnv* env, jobject surface_view) override {
    CallBridgeVoid(env, "clearVideoSurfaceView", "(Landroid/view/SurfaceView;)V", surface_view);
  }

  void SetVideoTextureView(JNIEnv* env, jobject texture_view) override {
    CallBridgeVoid(env, "setVideoTextureView", "(Landroid/view/TextureView;)V", texture_view);
  }

  void ClearVideoTextureView(JNIEnv* env, jobject texture_view) override {
    CallBridgeVoid(
        env, "clearVideoTextureView", "(Landroid/view/TextureView;)V", texture_view);
  }

  void SetVideoEffects(
      JNIEnv* env,
      const std::vector<VideoEffectDescriptor>& video_effects) override {
    jobjectArray java_video_effects = CreateJavaVideoEffectArray(env, video_effects);
    if (java_video_effects == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "setVideoEffects",
        "([Landroidx/media3/exoplayer/cppbridge/CppVideoEffect;)V",
        java_video_effects);
    DeleteLocalRefIfNotNull(env, java_video_effects);
  }

  void SetMediaItem(JNIEnv* env, const MediaItemDescriptor& media_item) override {
    jobject item = CreateJavaMediaItem(env, media_item);
    if (item == nullptr) {
      return;
    }
    CallBridgeVoid(
        env, "setMediaItem", "(Landroidx/media3/exoplayer/cppbridge/CppMediaItem;)V", item);
    env->DeleteLocalRef(item);
  }

  void SetMediaItem(
      JNIEnv* env,
      const MediaItemDescriptor& media_item,
      bool reset_position) override {
    jobject item = CreateJavaMediaItem(env, media_item);
    if (item == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "setMediaItem",
        "(Landroidx/media3/exoplayer/cppbridge/CppMediaItem;Z)V",
        item,
        static_cast<jboolean>(reset_position));
    env->DeleteLocalRef(item);
  }

  void SetMediaItem(
      JNIEnv* env,
      const MediaItemDescriptor& media_item,
      int64_t start_position_ms) override {
    jobject item = CreateJavaMediaItem(env, media_item);
    if (item == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "setMediaItem",
        "(Landroidx/media3/exoplayer/cppbridge/CppMediaItem;J)V",
        item,
        static_cast<jlong>(start_position_ms));
    env->DeleteLocalRef(item);
  }

  void SetMediaItems(
      JNIEnv* env,
      const std::vector<MediaItemDescriptor>& media_items,
      int start_index,
      int64_t start_position_ms) override {
    jobjectArray items = CreateJavaMediaItemArray(env, media_items);
    if (items == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "setMediaItems",
        "([Landroidx/media3/exoplayer/cppbridge/CppMediaItem;IJ)V",
        items,
        static_cast<jint>(start_index),
        static_cast<jlong>(start_position_ms));
    env->DeleteLocalRef(items);
  }

  void SetMediaItems(
      JNIEnv* env,
      const std::vector<MediaItemDescriptor>& media_items,
      bool reset_position) override {
    jobjectArray items = CreateJavaMediaItemArray(env, media_items);
    if (items == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "setMediaItems",
        "([Landroidx/media3/exoplayer/cppbridge/CppMediaItem;Z)V",
        items,
        static_cast<jboolean>(reset_position));
    env->DeleteLocalRef(items);
  }

  void AddMediaItem(JNIEnv* env, const MediaItemDescriptor& media_item) override {
    jobject item = CreateJavaMediaItem(env, media_item);
    if (item == nullptr) {
      return;
    }
    CallBridgeVoid(
        env, "addMediaItem", "(Landroidx/media3/exoplayer/cppbridge/CppMediaItem;)V", item);
    env->DeleteLocalRef(item);
  }

  void AddMediaItem(JNIEnv* env, int index, const MediaItemDescriptor& media_item) override {
    jobject item = CreateJavaMediaItem(env, media_item);
    if (item == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "addMediaItem",
        "(ILandroidx/media3/exoplayer/cppbridge/CppMediaItem;)V",
        static_cast<jint>(index),
        item);
    env->DeleteLocalRef(item);
  }

  void AddMediaItems(
      JNIEnv* env,
      const std::vector<MediaItemDescriptor>& media_items) override {
    jobjectArray items = CreateJavaMediaItemArray(env, media_items);
    if (items == nullptr) {
      return;
    }
    CallBridgeVoid(
        env, "addMediaItems", "([Landroidx/media3/exoplayer/cppbridge/CppMediaItem;)V", items);
    env->DeleteLocalRef(items);
  }

  void AddMediaItems(
      JNIEnv* env,
      int index,
      const std::vector<MediaItemDescriptor>& media_items) override {
    jobjectArray items = CreateJavaMediaItemArray(env, media_items);
    if (items == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "addMediaItems",
        "(I[Landroidx/media3/exoplayer/cppbridge/CppMediaItem;)V",
        static_cast<jint>(index),
        items);
    env->DeleteLocalRef(items);
  }

  void RemoveMediaItem(JNIEnv* env, int index) override {
    CallBridgeVoid(env, "removeMediaItem", "(I)V", static_cast<jint>(index));
  }

  void RemoveMediaItems(JNIEnv* env, int from_index, int to_index) override {
    CallBridgeVoid(
        env,
        "removeMediaItems",
        "(II)V",
        static_cast<jint>(from_index),
        static_cast<jint>(to_index));
  }

  void MoveMediaItem(JNIEnv* env, int current_index, int new_index) override {
    CallBridgeVoid(
        env,
        "moveMediaItem",
        "(II)V",
        static_cast<jint>(current_index),
        static_cast<jint>(new_index));
  }

  void MoveMediaItems(JNIEnv* env, int from_index, int to_index, int new_index) override {
    CallBridgeVoid(
        env,
        "moveMediaItems",
        "(III)V",
        static_cast<jint>(from_index),
        static_cast<jint>(to_index),
        static_cast<jint>(new_index));
  }

  void ReplaceMediaItems(
      JNIEnv* env,
      int from_index,
      int to_index,
      const std::vector<MediaItemDescriptor>& media_items) override {
    jobjectArray items = CreateJavaMediaItemArray(env, media_items);
    if (items == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "replaceMediaItems",
        "(II[Landroidx/media3/exoplayer/cppbridge/CppMediaItem;)V",
        static_cast<jint>(from_index),
        static_cast<jint>(to_index),
        items);
    env->DeleteLocalRef(items);
  }

  void ReplaceMediaItem(
      JNIEnv* env,
      int index,
      const MediaItemDescriptor& media_item) override {
    jobject item = CreateJavaMediaItem(env, media_item);
    if (item == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "replaceMediaItem",
        "(ILandroidx/media3/exoplayer/cppbridge/CppMediaItem;)V",
        static_cast<jint>(index),
        item);
    env->DeleteLocalRef(item);
  }

  void ClearMediaItems(JNIEnv* env) override {
    CallVoidNoArgs(env, "clearMediaItems");
  }

  void Prepare(JNIEnv* env) override { CallVoidNoArgs(env, "prepare"); }

  void Play(JNIEnv* env) override { CallVoidNoArgs(env, "play"); }

  void Pause(JNIEnv* env) override { CallVoidNoArgs(env, "pause"); }

  void Stop(JNIEnv* env) override { CallVoidNoArgs(env, "stop"); }

  void SeekTo(JNIEnv* env, int64_t position_ms) override {
    CallBridgeVoid(env, "seekTo", "(J)V", static_cast<jlong>(position_ms));
  }

  void SeekToMediaItem(JNIEnv* env, int media_item_index, int64_t position_ms) override {
    CallBridgeVoid(
        env,
        "seekToMediaItem",
        "(IJ)V",
        static_cast<jint>(media_item_index),
        static_cast<jlong>(position_ms));
  }

  void SeekBack(JNIEnv* env) override { CallVoidNoArgs(env, "seekBack"); }

  void SeekForward(JNIEnv* env) override { CallVoidNoArgs(env, "seekForward"); }

  void SeekToDefaultPosition(JNIEnv* env) override {
    CallVoidNoArgs(env, "seekToDefaultPosition");
  }

  void SeekToDefaultPosition(JNIEnv* env, int media_item_index) override {
    CallBridgeVoid(
        env, "seekToDefaultPosition", "(I)V", static_cast<jint>(media_item_index));
  }

  void SetSeekParameters(
      JNIEnv* env,
      const SeekParametersDescriptor& seek_parameters) override {
    jclass seek_parameters_class =
        FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppSeekParameters");
    jmethodID ctor = GetMethodChecked(
        env,
        seek_parameters_class,
        "CppSeekParameters",
        "<init>",
        "(JJ)V");
    if (seek_parameters_class == nullptr || ctor == nullptr) {
      DeleteLocalRefIfNotNull(env, seek_parameters_class);
      return;
    }
    jobject object = NewObjectChecked(
        env,
        seek_parameters_class,
        ctor,
        "CppSeekParameters",
        static_cast<jlong>(seek_parameters.tolerance_before_us),
        static_cast<jlong>(seek_parameters.tolerance_after_us));
    if (object == nullptr) {
      env->DeleteLocalRef(seek_parameters_class);
      return;
    }
    CallBridgeVoid(
        env,
        "setSeekParameters",
        "(Landroidx/media3/exoplayer/cppbridge/CppSeekParameters;)V",
        object);
    env->DeleteLocalRef(object);
    env->DeleteLocalRef(seek_parameters_class);
  }

  SeekParametersDescriptor GetSeekParameters(JNIEnv* env) override {
    jobject object = CallObjectNoArgs(
        env,
        "getSeekParameters",
        "()Landroidx/media3/exoplayer/cppbridge/CppSeekParameters;");
    SeekParametersDescriptor descriptor = FromJavaSeekParameters(env, object);
    DeleteLocalRefIfNotNull(env, object);
    return descriptor;
  }

  void SeekToNext(JNIEnv* env) override { CallVoidNoArgs(env, "seekToNext"); }

  void SeekToPrevious(JNIEnv* env) override { CallVoidNoArgs(env, "seekToPrevious"); }

  void SeekToNextMediaItem(JNIEnv* env) override {
    CallVoidNoArgs(env, "seekToNextMediaItem");
  }

  void SeekToPreviousMediaItem(JNIEnv* env) override {
    CallVoidNoArgs(env, "seekToPreviousMediaItem");
  }

  void SetWakeMode(JNIEnv* env, int wake_mode) override {
    CallBridgeVoid(env, "setWakeMode", "(I)V", static_cast<jint>(wake_mode));
  }

  void SetPriority(JNIEnv* env, int priority) override {
    CallBridgeVoid(env, "setPriority", "(I)V", static_cast<jint>(priority));
  }

  void SetPriorityTaskManager(JNIEnv* env, jobject priority_task_manager) override {
    CallBridgeVoid(
        env,
        "setPriorityTaskManagerObject",
        "(Landroidx/media3/common/PriorityTaskManager;)V",
        priority_task_manager);
  }

  void SetPriorityTaskManagerEnabled(JNIEnv* env, bool enabled) override {
    CallBridgeVoid(
        env,
        "setPriorityTaskManagerEnabled",
        "(Z)V",
        static_cast<jboolean>(enabled));
  }

  void SetPreloadConfiguration(JNIEnv* env, int64_t target_preload_duration_us) override {
    CallBridgeVoid(
        env,
        "setPreloadConfiguration",
        "(J)V",
        static_cast<jlong>(target_preload_duration_us));
  }

  PlayerMessageResult SendPlayerMessage(
      JNIEnv* env,
      const PlayerMessageDescriptor& message) override {
    jstring payload =
        NewStringUtfChecked(env, message.payload, "sendPlayerMessageForTest.payload");
    if (payload == nullptr) {
      return {};
    }
    jobjectArray values = static_cast<jobjectArray>(CallBridgeObject(
        env,
        "sendPlayerMessageForTest",
        "(IILjava/lang/String;IJZZJ)[Ljava/lang/String;",
        static_cast<jint>(message.target_type),
        static_cast<jint>(message.type),
        payload,
        static_cast<jint>(message.media_item_index),
        static_cast<jlong>(message.position_ms),
        static_cast<jboolean>(message.delete_after_delivery),
        static_cast<jboolean>(message.cancel_after_send),
        static_cast<jlong>(message.block_timeout_ms)));
    DeleteLocalRefIfNotNull(env, payload);
    std::vector<std::string> fields = JStringArrayToVector(env, values);
    DeleteLocalRefIfNotNull(env, values);
    PlayerMessageResult result;
    if (fields.size() >= 10) {
      result.delivered = fields[0] == "1";
      result.timed_out = fields[1] == "1";
      result.canceled = fields[2] == "1";
      result.delivery_count = ParseIntOrDefault(fields[3], 0);
      result.type = ParseIntOrDefault(fields[4], 0);
      result.payload = fields[5];
      result.media_item_index = ParseIntOrDefault(fields[6], -1);
      result.position_ms = ParseLongOrDefault(fields[7], -9223372036854775807LL);
      result.delete_after_delivery = fields[8] == "1";
      result.thread_name = fields[9];
    }
    return result;
  }

  void SetImageOutputEnabled(JNIEnv* env, bool enabled) override {
    CallBridgeVoid(
        env,
        "setImageOutputEnabled",
        "(Z)V",
        static_cast<jboolean>(enabled));
  }

  void SetAudioAttributes(
      JNIEnv* env,
      const AudioAttributesDescriptor& attributes,
      bool handle_audio_focus) override {
    CallBridgeVoid(
        env,
        "setAudioAttributesConfig",
        "(IIIIIZ)V",
        static_cast<jint>(attributes.content_type),
        static_cast<jint>(attributes.usage),
        static_cast<jint>(attributes.flags),
        static_cast<jint>(attributes.allowed_capture_policy),
        static_cast<jint>(attributes.spatialization_behavior),
        static_cast<jboolean>(handle_audio_focus));
  }

  void SetDeviceVolume(JNIEnv* env, int volume, int flags) override {
    CallBridgeVoid(
        env,
        "setDeviceVolumeWithFlags",
        "(II)V",
        static_cast<jint>(volume),
        static_cast<jint>(flags));
  }

  void AdjustDeviceVolume(JNIEnv* env, int direction, int flags) override {
    CallBridgeVoid(
        env,
        "adjustDeviceVolumeWithFlags",
        "(II)V",
        static_cast<jint>(direction),
        static_cast<jint>(flags));
  }

  void IncreaseDeviceVolume(JNIEnv* env, int flags) override {
    CallBridgeVoid(
        env, "increaseDeviceVolumeWithFlags", "(I)V", static_cast<jint>(flags));
  }

  void DecreaseDeviceVolume(JNIEnv* env, int flags) override {
    CallBridgeVoid(
        env, "decreaseDeviceVolumeWithFlags", "(I)V", static_cast<jint>(flags));
  }

  void SetDeviceMuted(JNIEnv* env, bool muted, int flags) override {
    CallBridgeVoid(
        env,
        "setDeviceMutedWithFlags",
        "(ZI)V",
        static_cast<jboolean>(muted),
        static_cast<jint>(flags));
  }

  void SetSkipSilenceEnabled(JNIEnv* env, bool skip_silence_enabled) override {
    CallBridgeVoid(
        env,
        "setSkipSilenceEnabled",
        "(Z)V",
        static_cast<jboolean>(skip_silence_enabled));
  }

  void SetPlayWhenReady(JNIEnv* env, bool play_when_ready) override {
    CallBridgeVoid(
        env, "setPlayWhenReady", "(Z)V", static_cast<jboolean>(play_when_ready));
  }

  void SetRepeatMode(JNIEnv* env, RepeatMode repeat_mode) override {
    CallBridgeVoid(env, "setRepeatMode", "(I)V", static_cast<jint>(repeat_mode));
  }

  void SetShuffleModeEnabled(JNIEnv* env, bool shuffle_mode_enabled) override {
    CallBridgeVoid(
        env,
        "setShuffleModeEnabled",
        "(Z)V",
        static_cast<jboolean>(shuffle_mode_enabled));
  }

  void SetVolume(JNIEnv* env, float volume) override {
    CallBridgeVoid(env, "setVolume", "(F)V", static_cast<jfloat>(volume));
  }

  void SetPlaybackSpeed(JNIEnv* env, float speed) override {
    CallBridgeVoid(env, "setPlaybackSpeed", "(F)V", static_cast<jfloat>(speed));
  }

  void SetPlaybackParameters(
      JNIEnv* env,
      const PlaybackParametersSnapshot& parameters) override {
    CallBridgeVoid(
        env,
        "setPlaybackParametersConfig",
        "(FF)V",
        static_cast<jfloat>(parameters.speed),
        static_cast<jfloat>(parameters.pitch));
  }

  void SetPauseAtEndOfMediaItems(JNIEnv* env, bool pause_at_end_of_media_items) override {
    CallBridgeVoid(
        env,
        "setPauseAtEndOfMediaItems",
        "(Z)V",
        static_cast<jboolean>(pause_at_end_of_media_items));
  }

  void SetTrackSelectionParameters(
      JNIEnv* env,
      const TrackSelectionParametersDescriptor& parameters) override {
    jclass parameters_class =
        FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppTrackSelectionParameters");
    jmethodID ctor = GetMethodChecked(
        env,
        parameters_class,
        "CppTrackSelectionParameters",
        "<init>",
        "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;IIIIIIIIIZZIZZZZZZ[I[Landroidx/media3/exoplayer/cppbridge/CppTrackSelectionOverride;)V");
    if (parameters_class == nullptr || ctor == nullptr) {
      DeleteLocalRefIfNotNull(env, parameters_class);
      return;
    }
    jclass override_class =
        FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppTrackSelectionOverride");
    jmethodID override_ctor = GetMethodChecked(
        env,
        override_class,
        "CppTrackSelectionOverride",
        "<init>",
        "(Ljava/lang/String;I[I)V");
    if (override_class == nullptr || override_ctor == nullptr) {
      DeleteLocalRefIfNotNull(env, parameters_class);
      DeleteLocalRefIfNotNull(env, override_class);
      return;
    }
    jstring preferred_audio_language = parameters.preferred_audio_language.empty()
        ? nullptr
        : NewStringUtfChecked(
              env,
              parameters.preferred_audio_language,
              "CppTrackSelectionParameters.preferredAudioLanguage");
    jstring preferred_text_language = parameters.preferred_text_language.empty()
        ? nullptr
        : NewStringUtfChecked(
              env,
              parameters.preferred_text_language,
              "CppTrackSelectionParameters.preferredTextLanguage");
    jobjectArray preferred_audio_languages =
        CreateJavaStringArray(env, parameters.preferred_audio_languages);
    jobjectArray preferred_text_languages =
        CreateJavaStringArray(env, parameters.preferred_text_languages);
    jintArray disabled_track_types = CreateJavaIntArray(env, parameters.disabled_track_types);
    jobjectArray overrides = env->NewObjectArray(
        static_cast<jsize>(parameters.overrides.size()), override_class, nullptr);
    if (ClearJniExceptionIfPresent(env, "NewObjectArray(CppTrackSelectionOverride)")) {
      DeleteLocalRefIfNotNull(env, preferred_audio_language);
      DeleteLocalRefIfNotNull(env, preferred_text_language);
      DeleteLocalRefIfNotNull(env, preferred_audio_languages);
      DeleteLocalRefIfNotNull(env, preferred_text_languages);
      DeleteLocalRefIfNotNull(env, disabled_track_types);
      DeleteLocalRefIfNotNull(env, parameters_class);
      DeleteLocalRefIfNotNull(env, override_class);
      return;
    }
    for (jsize i = 0; i < static_cast<jsize>(parameters.overrides.size()); ++i) {
      const auto& override_descriptor = parameters.overrides[static_cast<size_t>(i)];
      jstring track_group_id = override_descriptor.track_group_id.empty()
          ? nullptr
          : NewStringUtfChecked(
                env,
                override_descriptor.track_group_id,
                "CppTrackSelectionOverride.trackGroupId");
      jintArray track_indices = CreateJavaIntArray(env, override_descriptor.track_indices);
      jobject override_object = NewObjectChecked(
          env,
          override_class,
          override_ctor,
          "CppTrackSelectionOverride",
          track_group_id,
          static_cast<jint>(override_descriptor.track_type),
          track_indices);
      DeleteLocalRefIfNotNull(env, track_group_id);
      DeleteLocalRefIfNotNull(env, track_indices);
      if (override_object == nullptr) {
        DeleteLocalRefIfNotNull(env, preferred_audio_language);
        DeleteLocalRefIfNotNull(env, preferred_text_language);
        DeleteLocalRefIfNotNull(env, preferred_audio_languages);
        DeleteLocalRefIfNotNull(env, preferred_text_languages);
        DeleteLocalRefIfNotNull(env, disabled_track_types);
        DeleteLocalRefIfNotNull(env, overrides);
        DeleteLocalRefIfNotNull(env, parameters_class);
        DeleteLocalRefIfNotNull(env, override_class);
        return;
      }
      env->SetObjectArrayElement(overrides, i, override_object);
      if (ClearJniExceptionIfPresent(env, "SetObjectArrayElement(CppTrackSelectionOverride)")) {
        DeleteLocalRefIfNotNull(env, override_object);
        DeleteLocalRefIfNotNull(env, preferred_audio_language);
        DeleteLocalRefIfNotNull(env, preferred_text_language);
        DeleteLocalRefIfNotNull(env, preferred_audio_languages);
        DeleteLocalRefIfNotNull(env, preferred_text_languages);
        DeleteLocalRefIfNotNull(env, disabled_track_types);
        DeleteLocalRefIfNotNull(env, overrides);
        DeleteLocalRefIfNotNull(env, parameters_class);
        DeleteLocalRefIfNotNull(env, override_class);
        return;
      }
      DeleteLocalRefIfNotNull(env, override_object);
    }
    jobject object = NewObjectChecked(
        env,
        parameters_class,
        ctor,
        "CppTrackSelectionParameters",
        preferred_audio_language,
        preferred_text_language,
        preferred_audio_languages,
        preferred_text_languages,
        static_cast<jint>(parameters.preferred_audio_role_flags),
        static_cast<jint>(parameters.preferred_text_role_flags),
        static_cast<jint>(parameters.max_audio_channel_count),
        static_cast<jint>(parameters.max_audio_bitrate),
        static_cast<jint>(parameters.max_video_width),
        static_cast<jint>(parameters.max_video_height),
        static_cast<jint>(parameters.max_video_bitrate),
        static_cast<jint>(parameters.viewport_width),
        static_cast<jint>(parameters.viewport_height),
        static_cast<jboolean>(parameters.viewport_orientation_may_change),
        static_cast<jboolean>(parameters.select_text_by_default),
        static_cast<jint>(parameters.ignored_text_selection_flags),
        static_cast<jboolean>(parameters.select_undetermined_text_language),
        static_cast<jboolean>(parameters.force_lowest_bitrate),
        static_cast<jboolean>(parameters.force_highest_supported_bitrate),
        static_cast<jboolean>(parameters.disable_video),
        static_cast<jboolean>(parameters.disable_audio),
        static_cast<jboolean>(parameters.disable_text),
        disabled_track_types,
        overrides);
    if (object == nullptr) {
      DeleteLocalRefIfNotNull(env, preferred_audio_language);
      DeleteLocalRefIfNotNull(env, preferred_text_language);
      DeleteLocalRefIfNotNull(env, preferred_audio_languages);
      DeleteLocalRefIfNotNull(env, preferred_text_languages);
      DeleteLocalRefIfNotNull(env, disabled_track_types);
      DeleteLocalRefIfNotNull(env, overrides);
      env->DeleteLocalRef(parameters_class);
      DeleteLocalRefIfNotNull(env, override_class);
      return;
    }
    CallBridgeVoid(
        env,
        "setTrackSelectionParameters",
        "(Landroidx/media3/exoplayer/cppbridge/CppTrackSelectionParameters;)V",
        object);
    if (preferred_audio_language != nullptr) {
      env->DeleteLocalRef(preferred_audio_language);
    }
    if (preferred_text_language != nullptr) {
      env->DeleteLocalRef(preferred_text_language);
    }
    DeleteLocalRefIfNotNull(env, preferred_audio_languages);
    DeleteLocalRefIfNotNull(env, preferred_text_languages);
    DeleteLocalRefIfNotNull(env, disabled_track_types);
    DeleteLocalRefIfNotNull(env, overrides);
    env->DeleteLocalRef(object);
    env->DeleteLocalRef(parameters_class);
    DeleteLocalRefIfNotNull(env, override_class);
  }

  TrackSelectionParametersDescriptor GetTrackSelectionParameters(JNIEnv* env) override {
    jobject object = CallBridgeObject(
        env,
        "getTrackSelectionParameters",
        "()Landroidx/media3/exoplayer/cppbridge/CppTrackSelectionParameters;");
    TrackSelectionParametersDescriptor result = FromJavaTrackSelectionParameters(env, object);
    if (object != nullptr) {
      env->DeleteLocalRef(object);
    }
    return result;
  }

  MediaItemDescriptor GetMediaItemAt(JNIEnv* env, int index) override {
    jobject media_item = CallBridgeObject(
        env,
        "getMediaItemAt",
        "(I)Landroidx/media3/exoplayer/cppbridge/CppMediaItem;",
        static_cast<jint>(index));
    MediaItemDescriptor descriptor = FromJavaMediaItem(env, media_item);
    DeleteLocalRefIfNotNull(env, media_item);
    return descriptor;
  }

  TracksSnapshot GetTracksSnapshot(JNIEnv* env) override {
    jobject java_tracks = CallBridgeObject(
        env, "getTracks", "()Landroidx/media3/exoplayer/cppbridge/CppTracks;");
    TracksSnapshot snapshot = FromJavaTracks(env, java_tracks);
    DeleteLocalRefIfNotNull(env, java_tracks);
    return snapshot;
  }

  std::vector<TrackGroupSnapshot> GetTrackGroups(JNIEnv* env) override {
    return GetTracksSnapshot(env).groups;
  }

  PlaybackState GetPlaybackState(JNIEnv* env) override {
    return ToPlaybackState(CallIntNoArgs(env, "getPlaybackState"));
  }

  bool GetPlayWhenReady(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "getPlayWhenReady");
  }

  bool IsPlaying(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "isPlaying");
  }

  bool IsLoading(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "getIsLoading");
  }

  PlayerError GetPlayerError(JNIEnv* env) override {
    PlayerError error;
    jobjectArray values = static_cast<jobjectArray>(
        CallObjectNoArgs(env, "getPlayerErrorData", "()[Ljava/lang/String;"));
    std::vector<std::string> fields = JStringArrayToVector(env, values);
    if (values != nullptr) {
      env->DeleteLocalRef(values);
    }
    if (fields.size() >= 2) {
      error.error_code = ParseIntOrDefault(fields[0], 0);
      error.message = fields[1];
    }
    return error;
  }

  int64_t GetCurrentPosition(JNIEnv* env) override {
    return CallLongNoArgs(env, "getCurrentPosition");
  }

  int64_t GetBufferedPosition(JNIEnv* env) override {
    return CallLongNoArgs(env, "getBufferedPosition");
  }

  int64_t GetDuration(JNIEnv* env) override {
    return CallLongNoArgs(env, "getDuration");
  }

  int GetCurrentMediaItemIndex(JNIEnv* env) override {
    return CallIntNoArgs(env, "getCurrentMediaItemIndex");
  }

  int GetMediaItemCount(JNIEnv* env) override {
    return CallIntNoArgs(env, "getMediaItemCount");
  }

  RepeatMode GetRepeatMode(JNIEnv* env) override {
    return static_cast<RepeatMode>(CallIntNoArgs(env, "getRepeatMode"));
  }

  bool GetShuffleModeEnabled(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "getShuffleModeEnabled");
  }

  float GetVolume(JNIEnv* env) override {
    return CallFloatNoArgs(env, "getVolume");
  }

  AudioAttributesDescriptor GetAudioAttributes(JNIEnv* env) override {
    jintArray values = static_cast<jintArray>(
        CallObjectNoArgs(env, "getAudioAttributesConfig", "()[I"));
    AudioAttributesDescriptor descriptor = FromJavaAudioAttributes(env, values);
    if (values != nullptr) {
      env->DeleteLocalRef(values);
    }
    return descriptor;
  }

  DeviceInfoDescriptor GetDeviceInfo(JNIEnv* env) override {
    jobject value = CallObjectNoArgs(
        env, "getDeviceInfo", "()Landroidx/media3/exoplayer/cppbridge/CppDeviceInfo;");
    DeviceInfoDescriptor descriptor = FromJavaDeviceInfo(env, value);
    DeleteLocalRefIfNotNull(env, value);
    return descriptor;
  }

  int GetDeviceVolume(JNIEnv* env) override {
    return CallIntNoArgs(env, "getDeviceVolumeValue");
  }

  bool IsDeviceMuted(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "isDeviceMutedValue");
  }

  bool GetSkipSilenceEnabled(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "getSkipSilenceEnabled");
  }

  VideoSizeSnapshot GetVideoSize(JNIEnv* env) override {
    jobject value = CallObjectNoArgs(
        env, "getVideoSize", "()Landroidx/media3/exoplayer/cppbridge/CppVideoSize;");
    VideoSizeSnapshot snapshot = FromJavaVideoSize(env, value);
    DeleteLocalRefIfNotNull(env, value);
    return snapshot;
  }

  int GetNextMediaItemIndex(JNIEnv* env) override {
    return CallIntNoArgs(env, "getNextMediaItemIndex");
  }

  int GetPreviousMediaItemIndex(JNIEnv* env) override {
    return CallIntNoArgs(env, "getPreviousMediaItemIndex");
  }

  bool HasNextMediaItem(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "hasNextMediaItem");
  }

  bool HasPreviousMediaItem(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "hasPreviousMediaItem");
  }

  int GetBufferedPercentage(JNIEnv* env) override {
    return CallIntNoArgs(env, "getBufferedPercentage");
  }

  int64_t GetContentBufferedPosition(JNIEnv* env) override {
    return CallLongNoArgs(env, "getContentBufferedPosition");
  }

  int64_t GetContentDuration(JNIEnv* env) override {
    return CallLongNoArgs(env, "getContentDuration");
  }

  int64_t GetContentPosition(JNIEnv* env) override {
    return CallLongNoArgs(env, "getContentPosition");
  }

  int64_t GetCurrentLiveOffset(JNIEnv* env) override {
    return CallLongNoArgs(env, "getCurrentLiveOffset");
  }

  int GetCurrentPeriodIndex(JNIEnv* env) override {
    return CallIntNoArgs(env, "getCurrentPeriodIndex");
  }

  int64_t GetMaxSeekToPreviousPosition(JNIEnv* env) override {
    return CallLongNoArgs(env, "getMaxSeekToPreviousPosition");
  }

  PlaybackSuppressionReason GetPlaybackSuppressionReason(JNIEnv* env) override {
    return ToPlaybackSuppressionReason(CallIntNoArgs(env, "getPlaybackSuppressionReason"));
  }

  int64_t GetSeekBackIncrement(JNIEnv* env) override {
    return CallLongNoArgs(env, "getSeekBackIncrement");
  }

  int64_t GetSeekForwardIncrement(JNIEnv* env) override {
    return CallLongNoArgs(env, "getSeekForwardIncrement");
  }

  int64_t GetTotalBufferedDuration(JNIEnv* env) override {
    return CallLongNoArgs(env, "getTotalBufferedDuration");
  }

  int64_t GetTargetPreloadDurationUs(JNIEnv* env) override {
    return CallLongNoArgs(env, "getTargetPreloadDurationUs");
  }

  bool IsCommandAvailable(JNIEnv* env, int command_code) override {
    return CallBridgeBoolean(
        env, "isCommandAvailable", "(I)Z", static_cast<jint>(command_code));
  }

  bool CanAdvertiseSession(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "canAdvertiseSession");
  }

  ApplicationLooperDescriptor GetApplicationLooper(JNIEnv* env) override {
    jobject value = CallObjectNoArgs(
        env,
        "getApplicationLooper",
        "()Landroidx/media3/exoplayer/cppbridge/CppApplicationLooper;");
    ApplicationLooperDescriptor descriptor = FromJavaApplicationLooper(env, value);
    DeleteLocalRefIfNotNull(env, value);
    return descriptor;
  }

  int GetCurrentAdGroupIndex(JNIEnv* env) override {
    return CallIntNoArgs(env, "getCurrentAdGroupIndex");
  }

  int GetCurrentAdIndexInAdGroup(JNIEnv* env) override {
    return CallIntNoArgs(env, "getCurrentAdIndexInAdGroup");
  }

  bool IsCurrentMediaItemDynamic(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "isCurrentMediaItemDynamicValue");
  }

  bool IsCurrentMediaItemLive(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "isCurrentMediaItemLiveValue");
  }

  bool IsCurrentMediaItemSeekable(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "isCurrentMediaItemSeekableValue");
  }

  bool IsPlayingAd(JNIEnv* env) override {
    return CallBooleanNoArgs(env, "isPlayingAdValue");
  }

  void SetPlaylistMetadata(
      JNIEnv* env,
      const MediaMetadataSnapshot& metadata) override {
    jobject object = CreateJavaMediaMetadata(env, metadata);
    if (object == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "setPlaylistMetadata",
        "(Landroidx/media3/exoplayer/cppbridge/CppMediaMetadata;)V",
        object);
    env->DeleteLocalRef(object);
  }

  MediaMetadataSnapshot GetMediaMetadata(JNIEnv* env) override {
    jobject value = CallObjectNoArgs(
        env, "getMediaMetadata", "()Landroidx/media3/exoplayer/cppbridge/CppMediaMetadata;");
    MediaMetadataSnapshot snapshot = FromJavaMediaMetadata(env, value);
    DeleteLocalRefIfNotNull(env, value);
    return snapshot;
  }

  MediaMetadataSnapshot GetPlaylistMetadata(JNIEnv* env) override {
    jobject value = CallObjectNoArgs(
        env, "getPlaylistMetadata", "()Landroidx/media3/exoplayer/cppbridge/CppMediaMetadata;");
    MediaMetadataSnapshot snapshot = FromJavaMediaMetadata(env, value);
    DeleteLocalRefIfNotNull(env, value);
    return snapshot;
  }

  PlaybackParametersSnapshot GetPlaybackParameters(JNIEnv* env) override {
    jobject value = CallObjectNoArgs(
        env,
        "getPlaybackParameters",
        "()Landroidx/media3/exoplayer/cppbridge/CppPlaybackParameters;");
    PlaybackParametersSnapshot snapshot = FromJavaPlaybackParameters(env, value);
    DeleteLocalRefIfNotNull(env, value);
    return snapshot;
  }

  MediaItemDescriptor GetCurrentMediaItem(JNIEnv* env) override {
    jobject media_item = CallObjectNoArgs(
        env,
        "getCurrentMediaItem",
        "()Landroidx/media3/exoplayer/cppbridge/CppMediaItem;");
    MediaItemDescriptor descriptor = FromJavaMediaItem(env, media_item);
    DeleteLocalRefIfNotNull(env, media_item);
    return descriptor;
  }

  void ReleaseOpaqueObjectTokens(
      JNIEnv* env,
      const std::vector<std::string>& tokens) override {
    ReleaseOpaqueObjectTokensInternal(env, tokens);
  }

  AnalyticsSnapshot GetAnalyticsSnapshot(JNIEnv* env) override {
    AnalyticsSnapshot snapshot;
    jobjectArray values = static_cast<jobjectArray>(
        CallObjectNoArgs(env, "getAnalyticsStrings", "()[Ljava/lang/String;"));
    std::vector<std::string> fields = JStringArrayToVector(env, values);
    if (values != nullptr) {
      env->DeleteLocalRef(values);
    }
    if (fields.size() >= 6) {
      snapshot.bitrate_estimate = ParseLongOrDefault(fields[0], 0);
      snapshot.dropped_video_frames = ParseIntOrDefault(fields[1], 0);
      snapshot.load_started_count = ParseIntOrDefault(fields[2], 0);
      snapshot.load_completed_count = ParseIntOrDefault(fields[3], 0);
      snapshot.audio_sample_mime_type = fields[4];
      snapshot.video_sample_mime_type = fields[5];
    }
    return snapshot;
  }

  void SimulateAnalyticsUpdateForTest(
      JNIEnv* env,
      const AnalyticsSnapshot& analytics) override {
    SimulateAnalyticsUpdateForTest(env,
                                   analytics.bitrate_estimate,
                                   analytics.dropped_video_frames,
                                   analytics.load_started_count,
                                   analytics.load_completed_count,
                                   analytics.audio_sample_mime_type,
                                   analytics.video_sample_mime_type);
  }

  void SimulateAudioUnderrunForTest(
      JNIEnv* env,
      const AudioUnderrunEvent& audio_underrun) override {
    CallBridgeVoid(
        env,
        "simulateAudioUnderrunForTest",
        "(IJJ)V",
        static_cast<jint>(audio_underrun.buffer_size),
        static_cast<jlong>(audio_underrun.buffer_size_ms),
        static_cast<jlong>(audio_underrun.elapsed_since_last_feed_ms));
  }

  void SimulateDroppedVideoFramesForTest(
      JNIEnv* env,
      const DroppedVideoFramesEvent& dropped_video_frames) override {
    CallBridgeVoid(
        env,
        "simulateDroppedVideoFramesForTest",
        "(IJ)V",
        static_cast<jint>(dropped_video_frames.dropped_frames),
        static_cast<jlong>(dropped_video_frames.elapsed_ms));
  }

  void SimulateBandwidthEstimateForTest(
      JNIEnv* env,
      const BandwidthEstimateEvent& bandwidth_estimate) override {
    CallBridgeVoid(
        env,
        "simulateBandwidthEstimateForTest",
        "(IJJ)V",
        static_cast<jint>(bandwidth_estimate.elapsed_ms),
        static_cast<jlong>(bandwidth_estimate.bytes_transferred),
        static_cast<jlong>(bandwidth_estimate.bitrate_estimate));
  }

  void SimulateLoadStartedForTest(
      JNIEnv* env,
      const LoadStartedEvent& load_started) override {
    jstring uri = NewStringUtfChecked(env, load_started.uri, "simulateLoadStartedForTest.uri");
    if (uri == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateLoadStartedForTest",
        "(Ljava/lang/String;III)V",
        uri,
        static_cast<jint>(load_started.data_type),
        static_cast<jint>(load_started.track_type),
        static_cast<jint>(load_started.retry_count));
    env->DeleteLocalRef(uri);
  }

  void SimulateLoadCompletedForTest(
      JNIEnv* env,
      const LoadCompletedEvent& load_completed) override {
    jstring uri = NewStringUtfChecked(env, load_completed.uri, "simulateLoadCompletedForTest.uri");
    if (uri == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateLoadCompletedForTest",
        "(Ljava/lang/String;II)V",
        uri,
        static_cast<jint>(load_completed.data_type),
        static_cast<jint>(load_completed.track_type));
    env->DeleteLocalRef(uri);
  }

  void SimulateAnalyticsLoadErrorForTest(
      JNIEnv* env,
      const AnalyticsLoadErrorEvent& load_error) override {
    jstring uri = NewStringUtfChecked(env, load_error.uri, "simulateAnalyticsLoadErrorForTest.uri");
    jstring message = NewStringUtfChecked(
        env,
        load_error.message,
        "simulateAnalyticsLoadErrorForTest.message");
    if (uri == nullptr || message == nullptr) {
      DeleteLocalRefIfNotNull(env, uri);
      DeleteLocalRefIfNotNull(env, message);
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAnalyticsLoadErrorForTest",
        "(Ljava/lang/String;IILjava/lang/String;Z)V",
        uri,
        static_cast<jint>(load_error.data_type),
        static_cast<jint>(load_error.track_type),
        message,
        static_cast<jboolean>(load_error.was_canceled));
    DeleteLocalRefIfNotNull(env, uri);
    DeleteLocalRefIfNotNull(env, message);
  }

  void SimulateAudioInputFormatChangedForTest(
      JNIEnv* env,
      const AudioInputFormatChangedEvent& audio_input_format_changed) override {
    jstring sample_mime_type = NewStringUtfChecked(
        env,
        audio_input_format_changed.sample_mime_type,
        "simulateAudioInputFormatChangedForTest.sampleMimeType");
    jstring codecs = NewStringUtfChecked(
        env,
        audio_input_format_changed.codecs,
        "simulateAudioInputFormatChangedForTest.codecs");
    if (sample_mime_type == nullptr || codecs == nullptr) {
      DeleteLocalRefIfNotNull(env, sample_mime_type);
      DeleteLocalRefIfNotNull(env, codecs);
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAudioInputFormatChangedForTest",
        "(Ljava/lang/String;Ljava/lang/String;II)V",
        sample_mime_type,
        codecs,
        static_cast<jint>(audio_input_format_changed.channel_count),
        static_cast<jint>(audio_input_format_changed.sample_rate));
    env->DeleteLocalRef(sample_mime_type);
    env->DeleteLocalRef(codecs);
  }

  void SimulateAudioDecoderInitializedForTest(
      JNIEnv* env,
      const AudioDecoderInitializedEvent& audio_decoder_initialized) override {
    jstring decoder_name = NewStringUtfChecked(
        env,
        audio_decoder_initialized.decoder_name,
        "simulateAudioDecoderInitializedForTest.decoderName");
    if (decoder_name == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAudioDecoderInitializedForTest",
        "(Ljava/lang/String;JJ)V",
        decoder_name,
        static_cast<jlong>(audio_decoder_initialized.initialized_timestamp_ms),
        static_cast<jlong>(audio_decoder_initialized.initialization_duration_ms));
    env->DeleteLocalRef(decoder_name);
  }

  void SimulateVideoDecoderInitializedForTest(
      JNIEnv* env,
      const VideoDecoderInitializedEvent& video_decoder_initialized) override {
    jstring decoder_name = NewStringUtfChecked(
        env,
        video_decoder_initialized.decoder_name,
        "simulateVideoDecoderInitializedForTest.decoderName");
    if (decoder_name == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateVideoDecoderInitializedForTest",
        "(Ljava/lang/String;JJ)V",
        decoder_name,
        static_cast<jlong>(video_decoder_initialized.initialized_timestamp_ms),
        static_cast<jlong>(video_decoder_initialized.initialization_duration_ms));
    env->DeleteLocalRef(decoder_name);
  }

  void SimulateAudioDecoderReleasedForTest(
      JNIEnv* env,
      const AudioDecoderReleasedEvent& audio_decoder_released) override {
    jstring decoder_name = NewStringUtfChecked(
        env,
        audio_decoder_released.decoder_name,
        "simulateAudioDecoderReleasedForTest.decoderName");
    if (decoder_name == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAudioDecoderReleasedForTest",
        "(Ljava/lang/String;)V",
        decoder_name);
    env->DeleteLocalRef(decoder_name);
  }

  void SimulateVideoDecoderReleasedForTest(
      JNIEnv* env,
      const VideoDecoderReleasedEvent& video_decoder_released) override {
    jstring decoder_name = NewStringUtfChecked(
        env,
        video_decoder_released.decoder_name,
        "simulateVideoDecoderReleasedForTest.decoderName");
    if (decoder_name == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateVideoDecoderReleasedForTest",
        "(Ljava/lang/String;)V",
        decoder_name);
    env->DeleteLocalRef(decoder_name);
  }

  void SimulateAnalyticsRenderedFirstFrameForTest(
      JNIEnv* env,
      const AnalyticsRenderedFirstFrameEvent& rendered_first_frame) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsRenderedFirstFrameForTest",
        "(J)V",
        static_cast<jlong>(rendered_first_frame.render_time_ms));
  }

  void SimulateAnalyticsVideoSizeChangedForTest(
      JNIEnv* env,
      const AnalyticsVideoSizeChangedEvent& analytics_video_size) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsVideoSizeChangedForTest",
        "(IIF)V",
        static_cast<jint>(analytics_video_size.width),
        static_cast<jint>(analytics_video_size.height),
        static_cast<jfloat>(analytics_video_size.pixel_width_height_ratio));
  }

  void SimulateAudioPositionAdvancingForTest(
      JNIEnv* env,
      const AudioPositionAdvancingEvent& audio_position_advancing) override {
    CallBridgeVoid(
        env,
        "simulateAudioPositionAdvancingForTest",
        "(J)V",
        static_cast<jlong>(audio_position_advancing.playout_start_system_time_ms));
  }

  void SimulateVideoFrameProcessingOffsetForTest(
      JNIEnv* env,
      const VideoFrameProcessingOffsetEvent& video_frame_processing_offset) override {
    CallBridgeVoid(
        env,
        "simulateVideoFrameProcessingOffsetForTest",
        "(JI)V",
        static_cast<jlong>(video_frame_processing_offset.total_processing_offset_us),
        static_cast<jint>(video_frame_processing_offset.frame_count));
  }

  void SimulateVolumeChangedForTest(
      JNIEnv* env,
      const VolumeChangedEvent& volume_changed) override {
    CallBridgeVoid(
        env,
        "simulateVolumeChangedForTest",
        "(F)V",
        static_cast<jfloat>(volume_changed.volume));
  }

  void SimulateAudioSessionIdChangedForTest(
      JNIEnv* env,
      const AudioSessionIdChangedEvent& audio_session_id_changed) override {
    CallBridgeVoid(
        env,
        "simulateAudioSessionIdChangedForTest",
        "(I)V",
        static_cast<jint>(audio_session_id_changed.audio_session_id));
  }

  void SimulateAnalyticsSkipSilenceEnabledChangedForTest(
      JNIEnv* env,
      const AnalyticsSkipSilenceEnabledChangedEvent& skip_silence_enabled_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsSkipSilenceEnabledChangedForTest",
        "(Z)V",
        static_cast<jboolean>(skip_silence_enabled_changed.skip_silence_enabled));
  }

  void SimulateAnalyticsDeviceVolumeChangedForTest(
      JNIEnv* env,
      const AnalyticsDeviceVolumeChangedEvent& device_volume_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsDeviceVolumeChangedForTest",
        "(IZ)V",
        static_cast<jint>(device_volume_changed.volume),
        static_cast<jboolean>(device_volume_changed.muted));
  }

  void SimulateAnalyticsPlaybackStateChangedForTest(
      JNIEnv* env,
      const AnalyticsPlaybackStateChangedEvent& playback_state_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsPlaybackStateChangedForTest",
        "(I)V",
        static_cast<jint>(playback_state_changed.playback_state));
  }

  void SimulateAnalyticsIsPlayingChangedForTest(
      JNIEnv* env,
      const AnalyticsIsPlayingChangedEvent& is_playing_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsIsPlayingChangedForTest",
        "(Z)V",
        static_cast<jboolean>(is_playing_changed.is_playing));
  }

  void SimulateAnalyticsPlayWhenReadyChangedForTest(
      JNIEnv* env,
      const AnalyticsPlayWhenReadyChangedEvent& play_when_ready_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsPlayWhenReadyChangedForTest",
        "(ZI)V",
        static_cast<jboolean>(play_when_ready_changed.play_when_ready),
        static_cast<jint>(play_when_ready_changed.reason));
  }

  void SimulateAnalyticsPlaybackSuppressionReasonChangedForTest(
      JNIEnv* env,
      const AnalyticsPlaybackSuppressionReasonChangedEvent& suppression_reason_changed)
      override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsPlaybackSuppressionReasonChangedForTest",
        "(I)V",
        static_cast<jint>(suppression_reason_changed.playback_suppression_reason));
  }

  void SimulateAnalyticsIsLoadingChangedForTest(
      JNIEnv* env,
      const AnalyticsIsLoadingChangedEvent& is_loading_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsIsLoadingChangedForTest",
        "(Z)V",
        static_cast<jboolean>(is_loading_changed.is_loading));
  }

  void SimulateAnalyticsRepeatModeChangedForTest(
      JNIEnv* env,
      const AnalyticsRepeatModeChangedEvent& repeat_mode_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsRepeatModeChangedForTest",
        "(I)V",
        static_cast<jint>(repeat_mode_changed.repeat_mode));
  }

  void SimulateAnalyticsShuffleModeChangedForTest(
      JNIEnv* env,
      const AnalyticsShuffleModeChangedEvent& shuffle_mode_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsShuffleModeChangedForTest",
        "(Z)V",
        static_cast<jboolean>(shuffle_mode_changed.shuffle_mode_enabled));
  }

  void SimulateAnalyticsPlaybackParametersChangedForTest(
      JNIEnv* env,
      const AnalyticsPlaybackParametersChangedEvent& playback_parameters_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsPlaybackParametersChangedForTest",
        "(FF)V",
        static_cast<jfloat>(playback_parameters_changed.speed),
        static_cast<jfloat>(playback_parameters_changed.pitch));
  }

  void SimulateAnalyticsAvailableCommandsChangedForTest(
      JNIEnv* env,
      const AnalyticsAvailableCommandsChangedEvent& available_commands_changed) override {
    jobject commands = CreateJavaCommands(env, available_commands_changed.commands);
    if (commands == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAnalyticsAvailableCommandsChangedForTest",
        "(Landroidx/media3/exoplayer/cppbridge/CppCommands;)V",
        commands);
    env->DeleteLocalRef(commands);
  }

  void SimulateAnalyticsEventsForTest(
      JNIEnv* env,
      const AnalyticsEventsEvent& analytics_events) override {
    jobject events = CreateJavaPlayerEvents(env, analytics_events.event_codes);
    if (events == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAnalyticsEventsForTest",
        "(Landroidx/media3/exoplayer/cppbridge/CppPlayerEvents;)V",
        events);
    env->DeleteLocalRef(events);
  }

  void SimulateSeekBackIncrementChangedForTest(JNIEnv* env, int64_t seek_back_increment_ms) override {
    CallBridgeVoid(
        env,
        "simulateSeekBackIncrementChangedForTest",
        "(J)V",
        static_cast<jlong>(seek_back_increment_ms));
  }

  void SimulateSeekForwardIncrementChangedForTest(
      JNIEnv* env,
      int64_t seek_forward_increment_ms) override {
    CallBridgeVoid(
        env,
        "simulateSeekForwardIncrementChangedForTest",
        "(J)V",
        static_cast<jlong>(seek_forward_increment_ms));
  }

  void SimulateMaxSeekToPreviousPositionChangedForTest(
      JNIEnv* env,
      int64_t max_seek_to_previous_position_ms) override {
    CallBridgeVoid(
        env,
        "simulateMaxSeekToPreviousPositionChangedForTest",
        "(J)V",
        static_cast<jlong>(max_seek_to_previous_position_ms));
  }

  void SimulateAnalyticsSeekBackIncrementChangedForTest(
      JNIEnv* env,
      const AnalyticsSeekBackIncrementChangedEvent& seek_back_increment_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsSeekBackIncrementChangedForTest",
        "(J)V",
        static_cast<jlong>(seek_back_increment_changed.seek_back_increment_ms));
  }

  void SimulateAnalyticsSeekForwardIncrementChangedForTest(
      JNIEnv* env,
      const AnalyticsSeekForwardIncrementChangedEvent& seek_forward_increment_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsSeekForwardIncrementChangedForTest",
        "(J)V",
        static_cast<jlong>(seek_forward_increment_changed.seek_forward_increment_ms));
  }

  void SimulateAnalyticsMaxSeekToPreviousPositionChangedForTest(
      JNIEnv* env,
      const AnalyticsMaxSeekToPreviousPositionChangedEvent&
          max_seek_to_previous_position_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsMaxSeekToPreviousPositionChangedForTest",
        "(J)V",
        static_cast<jlong>(
            max_seek_to_previous_position_changed.max_seek_to_previous_position_ms));
  }

  void SimulateAnalyticsTimelineChangedForTest(
      JNIEnv* env,
      const AnalyticsTimelineChangedEvent& timeline_changed) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsTimelineChangedForTest",
        "(I)V",
        static_cast<jint>(timeline_changed.reason));
  }

  void SimulateAnalyticsPositionDiscontinuityForTest(
      JNIEnv* env,
      const AnalyticsPositionDiscontinuityEvent& position_discontinuity) override {
    CallBridgeVoid(
        env,
        "simulateAnalyticsPositionDiscontinuityForTest",
        "(I)V",
        static_cast<jint>(position_discontinuity.reason));
  }

  void SimulateAnalyticsSeekStartedForTest(
      JNIEnv* env,
      const AnalyticsSeekStartedEvent&) override {
    CallBridgeVoid(env, "simulateAnalyticsSeekStartedForTest", "()V");
  }

  void SimulateAnalyticsPlayerErrorForTest(
      JNIEnv* env,
      const PlayerError& error) override {
    jstring message = NewStringUtfChecked(env, error.message, "simulateAnalyticsPlayerErrorForTest.message");
    if (message == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAnalyticsPlayerErrorForTest",
        "(ILjava/lang/String;)V",
        static_cast<jint>(error.error_code),
        message);
    env->DeleteLocalRef(message);
  }

  void SimulateAnalyticsPlayerErrorChangedForTest(
      JNIEnv* env,
      const PlayerError& error) override {
    jstring message = NewStringUtfChecked(
        env,
        error.message,
        "simulateAnalyticsPlayerErrorChangedForTest.message");
    if (message == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAnalyticsPlayerErrorChangedForTest",
        "(ILjava/lang/String;)V",
        static_cast<jint>(error.error_code),
        message);
    env->DeleteLocalRef(message);
  }

  void SimulateAnalyticsTracksChangedForTest(
      JNIEnv* env,
      const TracksSnapshot& tracks) override {
    jobject java_tracks = CreateJavaTracks(env, tracks);
    if (java_tracks == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAnalyticsTracksChangedForTest",
        "(Landroidx/media3/exoplayer/cppbridge/CppTracks;)V",
        java_tracks);
    env->DeleteLocalRef(java_tracks);
  }

  void SimulateAnalyticsMediaItemTransitionForTest(
      JNIEnv* env,
      const AnalyticsMediaItemTransitionEvent& media_item_transition) override {
    jobject java_media_item = CreateJavaMediaItem(env, media_item_transition.media_item);
    if (java_media_item == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAnalyticsMediaItemTransitionForTest",
        "(Landroidx/media3/exoplayer/cppbridge/CppMediaItem;I)V",
        java_media_item,
        static_cast<jint>(media_item_transition.reason));
    DeleteLocalRefIfNotNull(env, java_media_item);
  }

  void SimulateAnalyticsCuesForTest(
      JNIEnv* env,
      const CueSnapshot& cues) override {
    jobjectArray java_cues = CreateJavaCueArray(env, cues);
    if (java_cues == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAnalyticsCuesForTest",
        "([Landroidx/media3/exoplayer/cppbridge/CppCue;J)V",
        java_cues,
        static_cast<jlong>(cues.presentation_time_us));
    DeleteLocalRefIfNotNull(env, java_cues);
  }

  void SimulateCurrentCuesForTest(
      JNIEnv* env,
      const CueSnapshot& cues) override {
    jobjectArray java_cues = CreateJavaCueArray(env, cues);
    if (java_cues == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateCurrentCuesForTest",
        "([Landroidx/media3/exoplayer/cppbridge/CppCue;J)V",
        java_cues,
        static_cast<jlong>(cues.presentation_time_us));
    DeleteLocalRefIfNotNull(env, java_cues);
  }

  void SimulateAnalyticsMetadataForTest(
      JNIEnv* env,
      const AnalyticsMetadataEvent& metadata) override {
    jstring first_entry_type = NewStringUtfChecked(
        env,
        metadata.first_entry_type,
        "simulateAnalyticsMetadataForTest.firstEntryType");
    jstring first_entry_text = NewStringUtfChecked(
        env,
        metadata.first_entry_text,
        "simulateAnalyticsMetadataForTest.firstEntryText");
    if (first_entry_type == nullptr || first_entry_text == nullptr) {
      DeleteLocalRefIfNotNull(env, first_entry_type);
      DeleteLocalRefIfNotNull(env, first_entry_text);
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAnalyticsMetadataForTest",
        "(ILjava/lang/String;Ljava/lang/String;)V",
        static_cast<jint>(metadata.entry_count),
        first_entry_type,
        first_entry_text);
    DeleteLocalRefIfNotNull(env, first_entry_type);
    DeleteLocalRefIfNotNull(env, first_entry_text);
  }

  void SimulateAnalyticsDeviceInfoChangedForTest(
      JNIEnv* env,
      const DeviceInfoDescriptor& device_info) override {
    jobject object = CreateJavaDeviceInfo(env, device_info);
    if (object == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAnalyticsDeviceInfoChangedForTest",
        "(Landroidx/media3/exoplayer/cppbridge/CppDeviceInfo;)V",
        object);
    env->DeleteLocalRef(object);
  }

  void SimulateAnalyticsMediaMetadataChangedForTest(
      JNIEnv* env,
      const MediaMetadataSnapshot& metadata) override {
    jobject object = CreateJavaMediaMetadata(env, metadata);
    if (object == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAnalyticsMediaMetadataChangedForTest",
        "(Landroidx/media3/exoplayer/cppbridge/CppMediaMetadata;)V",
        object);
    env->DeleteLocalRef(object);
  }

  void SimulateAnalyticsPlaylistMetadataChangedForTest(
      JNIEnv* env,
      const MediaMetadataSnapshot& metadata) override {
    jobject object = CreateJavaMediaMetadata(env, metadata);
    if (object == nullptr) {
      return;
    }
    CallBridgeVoid(
        env,
        "simulateAnalyticsPlaylistMetadataChangedForTest",
        "(Landroidx/media3/exoplayer/cppbridge/CppMediaMetadata;)V",
        object);
    env->DeleteLocalRef(object);
  }

  void SimulateVideoInputFormatChangedForTest(
      JNIEnv* env,
      const VideoInputFormatChangedEvent& video_input_format_changed) override {
    jstring sample_mime_type = NewStringUtfChecked(
        env,
        video_input_format_changed.sample_mime_type,
        "simulateVideoInputFormatChangedForTest.sampleMimeType");
    jstring codecs = NewStringUtfChecked(
        env,
        video_input_format_changed.codecs,
        "simulateVideoInputFormatChangedForTest.codecs");
    if (sample_mime_type == nullptr || codecs == nullptr) {
      DeleteLocalRefIfNotNull(env, sample_mime_type);
      DeleteLocalRefIfNotNull(env, codecs);
      return;
    }
    CallBridgeVoid(
        env,
        "simulateVideoInputFormatChangedForTest",
        "(Ljava/lang/String;Ljava/lang/String;IIF)V",
        sample_mime_type,
        codecs,
        static_cast<jint>(video_input_format_changed.width),
        static_cast<jint>(video_input_format_changed.height),
        static_cast<jfloat>(video_input_format_changed.frame_rate));
    env->DeleteLocalRef(sample_mime_type);
    env->DeleteLocalRef(codecs);
  }

  void SimulateImageOutputForTest(
      JNIEnv* env,
      const ImageFrameSnapshot& image_frame) override {
    CallBridgeVoid(
        env,
        "simulateImageOutputForTest",
        "(JII)V",
        static_cast<jlong>(image_frame.presentation_time_us),
        static_cast<jint>(image_frame.width),
        static_cast<jint>(image_frame.height));
  }

  void SimulateAnalyticsUpdateForTest(
      JNIEnv* env,
      int64_t bitrate_estimate,
      int dropped_frames,
      int load_started_count,
      int load_completed_count,
      const std::string& audio_sample_mime_type,
      const std::string& video_sample_mime_type) {
    jstring audio_mime = NewStringUtfChecked(
        env, audio_sample_mime_type, "simulateAnalyticsUpdateForTest.audioMime");
    jstring video_mime = NewStringUtfChecked(
        env, video_sample_mime_type, "simulateAnalyticsUpdateForTest.videoMime");
    if (audio_mime == nullptr || video_mime == nullptr) {
      DeleteLocalRefIfNotNull(env, audio_mime);
      DeleteLocalRefIfNotNull(env, video_mime);
      return;
    }
    CallBridgeVoid(env,
                   "simulateAnalyticsUpdateForTest",
                   "(JIIILjava/lang/String;Ljava/lang/String;)V",
                   static_cast<jlong>(bitrate_estimate),
                   static_cast<jint>(dropped_frames),
                   static_cast<jint>(load_started_count),
                   static_cast<jint>(load_completed_count),
                   audio_mime,
                   video_mime);
    env->DeleteLocalRef(audio_mime);
    env->DeleteLocalRef(video_mime);
  }

  PlayerConfig::MediaSourceFactoryConfig GetMediaSourceFactoryConfig(JNIEnv* env) override {
    PlayerConfig::MediaSourceFactoryConfig config;
    jstring summary_value = static_cast<jstring>(
        CallObjectNoArgs(env, "getMediaSourceFactoryDebugSummary", "()Ljava/lang/String;"));
    std::string summary = JStringToString(env, summary_value);
    DeleteLocalRefIfNotNull(env, summary_value);
    size_t injected_marker = summary.find("injectedFactoryUsed=");
    if (injected_marker != std::string::npos) {
      size_t injected_start = injected_marker + std::string("injectedFactoryUsed=").size();
      size_t injected_end = summary.find(',', injected_start);
      std::string injected_value = summary.substr(
          injected_start,
          injected_end == std::string::npos ? std::string::npos : injected_end - injected_start);
      config.injected_factory_used_for_test = injected_value == "1";
    }
    size_t token_marker = summary.find("factoryToken=");
    if (token_marker != std::string::npos) {
      size_t token_start = token_marker + std::string("factoryToken=").size();
      size_t token_end = summary.find(',', token_start);
      config.factory_token =
          summary.substr(token_start, token_end == std::string::npos ? std::string::npos : token_end - token_start);
    }
    size_t identity_marker = summary.find("factoryIdentity=");
    if (identity_marker != std::string::npos) {
      size_t identity_start = identity_marker + std::string("factoryIdentity=").size();
      size_t identity_end = summary.find(',', identity_start);
      std::string identity_value = summary.substr(
          identity_start,
          identity_end == std::string::npos ? std::string::npos : identity_end - identity_start);
      config.injected_factory_identity_for_test = ParseIntOrDefault(identity_value, 0);
    }
    jobjectArray values = static_cast<jobjectArray>(
        CallObjectNoArgs(env, "getMediaSourceFactoryConfigStrings", "()[Ljava/lang/String;"));
    std::vector<std::string> fields = JStringArrayToVector(env, values);
    if (values != nullptr) {
      env->DeleteLocalRef(values);
    }
    if (fields.size() >= 11) {
      config.parse_subtitles_during_extraction = fields[0] == "1";
      config.load_only_selected_tracks = fields[1] == "1";
      config.user_agent = fields[2];
      config.connect_timeout_ms = ParseIntOrDefault(fields[3], -1);
      config.read_timeout_ms = ParseIntOrDefault(fields[4], -1);
      config.allow_cross_protocol_redirects = fields[5] == "1";
      config.live_target_offset_ms =
          ParseLongOrDefault(fields[6], -9223372036854775807LL);
      config.live_min_offset_ms =
          ParseLongOrDefault(fields[7], -9223372036854775807LL);
      config.live_max_offset_ms =
          ParseLongOrDefault(fields[8], -9223372036854775807LL);
      config.live_min_speed = fields[9].empty() ? -3.4028235e38f : std::stof(fields[9]);
      config.live_max_speed = fields[10].empty() ? -3.4028235e38f : std::stof(fields[10]);
    }
    jobjectArray header_names = static_cast<jobjectArray>(
        CallObjectNoArgs(env, "getMediaSourceFactoryHeaderNames", "()[Ljava/lang/String;"));
    jobjectArray header_values = static_cast<jobjectArray>(
        CallObjectNoArgs(env, "getMediaSourceFactoryHeaderValues", "()[Ljava/lang/String;"));
    config.default_request_header_names = JStringArrayToVector(env, header_names);
    config.default_request_header_values = JStringArrayToVector(env, header_values);
    if (header_names != nullptr) {
      env->DeleteLocalRef(header_names);
    }
    if (header_values != nullptr) {
      env->DeleteLocalRef(header_values);
    }
    return config;
  }

  std::vector<std::string> GetPlayerConfigFlagsForTest(JNIEnv* env) {
    jobjectArray values = static_cast<jobjectArray>(
        CallObjectNoArgs(env, "getPlayerConfigFlagsForTest", "()[Ljava/lang/String;"));
    std::vector<std::string> flags = JStringArrayToVector(env, values);
    DeleteLocalRefIfNotNull(env, values);
    return flags;
  }

  std::vector<std::string> GetPriorityTaskManagerStateForTest(JNIEnv* env) {
    jobjectArray values = static_cast<jobjectArray>(
        CallObjectNoArgs(env, "getPriorityTaskManagerStateForTest", "()[Ljava/lang/String;"));
    std::vector<std::string> flags = JStringArrayToVector(env, values);
    DeleteLocalRefIfNotNull(env, values);
    return flags;
  }

  std::string SummarizeVideoEffectsForTest(
      JNIEnv* env,
      const std::vector<VideoEffectDescriptor>& video_effects) {
    jobjectArray java_video_effects = CreateJavaVideoEffectArray(env, video_effects);
    if (java_video_effects == nullptr) {
      return "";
    }
    jstring summary = static_cast<jstring>(CallBridgeObject(
        env,
        "summarizeVideoEffectsForTest",
        "([Landroidx/media3/exoplayer/cppbridge/CppVideoEffect;)Ljava/lang/String;",
        java_video_effects));
    std::string result = JStringToString(env, summary);
    DeleteLocalRefIfNotNull(env, summary);
    DeleteLocalRefIfNotNull(env, java_video_effects);
    return result;
  }

  int GetAvailableCommandCount(JNIEnv* env) override {
    return CallIntNoArgs(env, "getAvailableCommandCount");
  }

  AvailableCommandsSnapshot GetAvailableCommands(JNIEnv* env) override {
    jobject value = CallObjectNoArgs(
        env, "getAvailableCommands", "()Landroidx/media3/exoplayer/cppbridge/CppCommands;");
    AvailableCommandsSnapshot snapshot = FromJavaCommands(env, value);
    DeleteLocalRefIfNotNull(env, value);
    return snapshot;
  }

  TimelineSnapshot GetTimelineSnapshot(JNIEnv* env) override {
    TimelineSnapshot snapshot;
    jintArray values = static_cast<jintArray>(
        CallObjectNoArgs(env, "getTimelineSnapshotData", "()[I"));
    if (values == nullptr || env->GetArrayLength(values) < 3) {
      if (values != nullptr) {
        env->DeleteLocalRef(values);
      }
      return snapshot;
    }
    jint* raw = env->GetIntArrayElements(values, nullptr);
    if (ClearJniExceptionIfPresent(env, "GetIntArrayElements(timelineSnapshot)") ||
        raw == nullptr) {
      env->DeleteLocalRef(values);
      return snapshot;
    }
    snapshot.window_count = raw[0];
    snapshot.period_count = raw[1];
    snapshot.empty = raw[2] != 0;
    if (env->GetArrayLength(values) >= 11) {
      snapshot.current_media_item_index = raw[3];
      snapshot.next_media_item_index = raw[4];
      snapshot.previous_media_item_index = raw[5];
      snapshot.has_next_media_item = raw[6] != 0;
      snapshot.has_previous_media_item = raw[7] != 0;
      snapshot.current_media_item_dynamic = raw[8] != 0;
      snapshot.current_media_item_live = raw[9] != 0;
      snapshot.current_media_item_seekable = raw[10] != 0;
    }
    env->ReleaseIntArrayElements(values, raw, JNI_ABORT);
    env->DeleteLocalRef(values);
    return snapshot;
  }

  std::vector<TimelineWindowSnapshot> GetTimelineWindows(JNIEnv* env) override {
    std::vector<TimelineWindowSnapshot> windows;
    jobjectArray rows = static_cast<jobjectArray>(
        CallObjectNoArgs(env, "getTimelineWindowRows", "()[Ljava/lang/String;"));
    std::vector<std::string> values = JStringArrayToVector(env, rows);
    if (rows != nullptr) {
      env->DeleteLocalRef(rows);
    }
    windows.reserve(values.size());
    for (const std::string& row : values) {
      std::vector<std::string> fields = SplitString(row, '|');
      if (fields.size() < 31) {
        continue;
      }
      TimelineWindowSnapshot window;
      window.media_item_index = ParseIntOrDefault(fields[0], -1);
      window.media_item_id = fields[1];
      window.media_item_uri = fields[2];
      window.media_item_tag_present = fields[3] == "1";
      window.media_item_tag_string = fields[4];
      window.media_item_tag_token = fields[5];
      window.uid = fields[6];
      window.uid_token = fields[7];
      window.live_configuration_present = fields[8] == "1";
      window.live_target_offset_ms = ParseLongOrDefault(fields[9], -9223372036854775807LL);
      window.live_min_offset_ms = ParseLongOrDefault(fields[10], -9223372036854775807LL);
      window.live_max_offset_ms = ParseLongOrDefault(fields[11], -9223372036854775807LL);
      window.live_min_playback_speed =
          ParseFloatOrDefault(fields[12], -3.4028235e38f);
      window.live_max_playback_speed =
          ParseFloatOrDefault(fields[13], -3.4028235e38f);
      window.manifest_present = fields[14] == "1";
      window.manifest_string = fields[15];
      window.manifest_token = fields[16];
      window.first_period_index = ParseIntOrDefault(fields[17], -1);
      window.last_period_index = ParseIntOrDefault(fields[18], -1);
      window.presentation_start_time_ms =
          ParseLongOrDefault(fields[19], -9223372036854775807LL);
      window.window_start_time_ms = ParseLongOrDefault(fields[20], -9223372036854775807LL);
      window.elapsed_realtime_epoch_offset_ms =
          ParseLongOrDefault(fields[21], -9223372036854775807LL);
      window.duration_ms = ParseLongOrDefault(fields[22], -9223372036854775807LL);
      window.duration_us = ParseLongOrDefault(fields[23], -9223372036854775807LL);
      window.default_position_ms = ParseLongOrDefault(fields[24], -9223372036854775807LL);
      window.default_position_us = ParseLongOrDefault(fields[25], -9223372036854775807LL);
      window.position_in_first_period_ms = ParseLongOrDefault(fields[26], 0);
      window.position_in_first_period_us = ParseLongOrDefault(fields[27], 0);
      window.is_seekable = fields[28] == "1";
      window.is_dynamic = fields[29] == "1";
      window.is_live = fields[30] == "1";
      if (fields.size() >= 32) {
        window.is_placeholder = fields[31] == "1";
      }
      windows.push_back(window);
    }
    return windows;
  }

  std::vector<TimelinePeriodSnapshot> GetTimelinePeriods(JNIEnv* env) override {
    std::vector<TimelinePeriodSnapshot> periods;
    jobjectArray rows = static_cast<jobjectArray>(
        CallObjectNoArgs(env, "getTimelinePeriodRows", "()[Ljava/lang/String;"));
    std::vector<std::string> values = JStringArrayToVector(env, rows);
    if (rows != nullptr) {
      env->DeleteLocalRef(rows);
    }
    periods.reserve(values.size());
    for (const std::string& row : values) {
      std::vector<std::string> fields = SplitString(row, '|');
      if (fields.size() < 13) {
        continue;
      }
      TimelinePeriodSnapshot period;
      period.id = fields[0];
      period.id_token = fields[1];
      period.uid = fields[2];
      period.uid_token = fields[3];
      period.ads_id = fields[4];
      period.ads_id_token = fields[5];
      period.window_index = ParseIntOrDefault(fields[6], -1);
      period.ad_group_count = ParseIntOrDefault(fields[7], 0);
      period.duration_ms = ParseLongOrDefault(fields[8], -9223372036854775807LL);
      period.duration_us = ParseLongOrDefault(fields[9], -9223372036854775807LL);
      period.position_in_window_ms = ParseLongOrDefault(fields[10], 0);
      period.position_in_window_us = ParseLongOrDefault(fields[11], 0);
      period.is_placeholder = fields[12] == "1";
      periods.push_back(period);
    }
    return periods;
  }

  CueSnapshot GetCurrentCues(JNIEnv* env) override {
    jobjectArray cues = static_cast<jobjectArray>(
        CallObjectNoArgs(env, "getCurrentCues", "()[Landroidx/media3/exoplayer/cppbridge/CppCue;"));
    CueSnapshot snapshot =
        FromJavaCues(env, cues, CallLongNoArgs(env, "getCurrentCuesPresentationTimeUs"));
    DeleteLocalRefIfNotNull(env, cues);
    return snapshot;
  }

  std::string GetCurrentMediaItemDebugSummary(JNIEnv* env) override {
    jstring summary = static_cast<jstring>(
        CallObjectNoArgs(env, "getCurrentMediaItemDebugSummary", "()Ljava/lang/String;"));
    std::string result = JStringToString(env, summary);
    if (summary != nullptr) {
      env->DeleteLocalRef(summary);
    }
    return result;
  }

  PlaybackSnapshot GetSnapshot(JNIEnv* env) override {
    PlaybackSnapshot snapshot;
    snapshot.playback_state = ToPlaybackState(CallIntNoArgs(env, "getPlaybackState"));
    snapshot.play_when_ready = CallBooleanNoArgs(env, "getPlayWhenReady");
    snapshot.is_playing = CallBooleanNoArgs(env, "isPlaying");
    snapshot.is_loading = CallBooleanNoArgs(env, "getIsLoading");
    snapshot.shuffle_mode_enabled = CallBooleanNoArgs(env, "getShuffleModeEnabled");
    snapshot.current_media_item_index = CallIntNoArgs(env, "getCurrentMediaItemIndex");
    snapshot.media_item_count = CallIntNoArgs(env, "getMediaItemCount");
    snapshot.repeat_mode = static_cast<RepeatMode>(CallIntNoArgs(env, "getRepeatMode"));
    snapshot.current_position_ms = CallLongNoArgs(env, "getCurrentPosition");
    snapshot.buffered_position_ms = CallLongNoArgs(env, "getBufferedPosition");
    snapshot.duration_ms = CallLongNoArgs(env, "getDuration");
    snapshot.volume = CallFloatNoArgs(env, "getVolume");
    snapshot.playback_speed = CallFloatNoArgs(env, "getPlaybackSpeed");
    snapshot.last_error = GetLastErrorSnapshot();
    return snapshot;
  }

  void Release(JNIEnv* env) override {
    LogInfo("ExoPlayerBridge::Release start");
    releasing_.store(true, std::memory_order_release);
    jobject java_bridge = nullptr;
    {
      std::unique_lock<std::mutex> lock(state_mutex_);
      if (java_bridge_ == nullptr) {
        LogInfo("ExoPlayerBridge::Release skip missingBridge");
        return;
      }
      listener_ = nullptr;
      image_output_listener_ = nullptr;
      listener_callback_drained_.wait(
          lock, [this]() { return in_flight_listener_callback_count_ == 0; });
      image_output_callback_drained_.wait(
          lock, [this]() { return in_flight_image_output_callback_count_ == 0; });
      java_bridge = java_bridge_;
      java_bridge_ = nullptr;
    }
    LogInfo("ExoPlayerBridge::Release java release begin");
    CallVoidNoArgsOnBridge(env, java_bridge, "release");
    LogInfo("ExoPlayerBridge::Release java release end");
    env->DeleteGlobalRef(java_bridge);
    LogInfo("ExoPlayerBridge::Release delete global ref");
    UnregisterBridge(this);
    LogInfo("ExoPlayerBridge::Release done");
  }

  void OnPlaybackStateChanged(int playback_state) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      PlaybackSnapshot snapshot = GetSnapshot(env);
      snapshot.playback_state = ToPlaybackState(playback_state);
      listener->OnPlaybackStateChanged(snapshot);
    });
  }

  void OnPlayWhenReadyChanged(bool play_when_ready, int reason) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      PlaybackSnapshot snapshot = GetSnapshot(env);
      snapshot.play_when_ready = play_when_ready;
      listener->OnPlayWhenReadyChanged(snapshot, reason);
    });
  }

  void OnIsPlayingChanged(bool is_playing) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      PlaybackSnapshot snapshot = GetSnapshot(env);
      snapshot.is_playing = is_playing;
      listener->OnIsPlayingChanged(snapshot);
    });
  }

  void OnMediaItemTransition(int media_item_index, int reason) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      PlaybackSnapshot snapshot = GetSnapshot(env);
      snapshot.current_media_item_index = media_item_index;
      listener->OnMediaItemTransition(snapshot, reason);
    });
  }

  void OnPlayerError(int error_code, const std::string& message) {
    SetLastError(error_code, message);
    WithListenerEnv(
        [&](PlayerListener* listener, JNIEnv* env) { listener->OnPlayerError(GetSnapshot(env)); });
  }

  void OnPlayerErrorChanged(int error_code, const std::string& message) {
    SetLastError(error_code, message);
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnPlayerErrorChanged(GetSnapshot(env));
    });
  }

  void OnTimelineChanged(int window_count, int period_count, int reason) {
    WithListenerEnv("OnTimelineChanged", [&](PlayerListener* listener, JNIEnv* env) {
      PlaybackSnapshot snapshot = GetSnapshot(env);
      TimelineDetailsSnapshot timeline;
      timeline.summary = GetTimelineSnapshot(env);
      timeline.summary.window_count = window_count;
      timeline.summary.period_count = period_count;
      timeline.summary.empty = window_count == 0;
      timeline.windows = GetTimelineWindows(env);
      timeline.periods = GetTimelinePeriods(env);
      listener->OnTimelineChanged(snapshot, timeline, reason);
    });
  }

  void OnTracksChanged() {
    WithListenerEnv("OnTracksChanged", [&](PlayerListener* listener, JNIEnv* env) {
      listener->OnTracksChanged(GetSnapshot(env), GetTracksSnapshot(env));
    });
  }

  void OnPositionDiscontinuity(
      const PositionInfoSnapshot& old_position,
      const PositionInfoSnapshot& new_position,
      int reason) {
    WithListenerEnv("OnPositionDiscontinuity", [&](PlayerListener* listener, JNIEnv* env) {
      PlaybackSnapshot snapshot = GetSnapshot(env);
      snapshot.current_media_item_index = new_position.media_item_index;
      snapshot.current_position_ms = new_position.position_ms;
      listener->OnPositionDiscontinuity(snapshot, old_position, new_position, reason);
    });
  }

  void OnAudioAttributesChanged(const AudioAttributesDescriptor& attributes) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnAudioAttributesChanged(GetSnapshot(env), attributes);
    });
  }

  void OnCues(int cue_count, int64_t presentation_time_us) {
    WithListenerEnv("OnCues", [&](PlayerListener* listener, JNIEnv* env) {
      CueSnapshot cues = GetCurrentCues(env);
      cues.cue_count = cue_count;
      cues.presentation_time_us = presentation_time_us;
      listener->OnCues(GetSnapshot(env), cues);
    });
  }

  void OnRepeatModeChanged(int repeat_mode) {
    WithListenerEnv("OnRepeatModeChanged", [&](PlayerListener* listener, JNIEnv* env) {
      PlaybackSnapshot snapshot = GetSnapshot(env);
      snapshot.repeat_mode = static_cast<RepeatMode>(repeat_mode);
      listener->OnRepeatModeChanged(snapshot);
    });
  }

  void OnShuffleModeEnabledChanged(bool shuffle_mode_enabled) {
    WithListenerEnv("OnShuffleModeEnabledChanged", [&](PlayerListener* listener, JNIEnv* env) {
      PlaybackSnapshot snapshot = GetSnapshot(env);
      snapshot.shuffle_mode_enabled = shuffle_mode_enabled;
      listener->OnShuffleModeEnabledChanged(snapshot);
    });
  }

  void OnSeekBackIncrementChanged(int64_t seek_back_increment_ms) {
    WithListenerEnv("OnSeekBackIncrementChanged", [&](PlayerListener* listener, JNIEnv* env) {
      PlaybackSnapshot snapshot = GetSnapshot(env);
      snapshot.seek_back_increment_ms = seek_back_increment_ms;
      listener->OnSeekBackIncrementChanged(snapshot, seek_back_increment_ms);
    });
  }

  void OnSeekForwardIncrementChanged(int64_t seek_forward_increment_ms) {
    WithListenerEnv("OnSeekForwardIncrementChanged", [&](PlayerListener* listener, JNIEnv* env) {
      PlaybackSnapshot snapshot = GetSnapshot(env);
      snapshot.seek_forward_increment_ms = seek_forward_increment_ms;
      listener->OnSeekForwardIncrementChanged(snapshot, seek_forward_increment_ms);
    });
  }

  void OnMaxSeekToPreviousPositionChanged(int64_t max_seek_to_previous_position_ms) {
    WithListenerEnv(
        "OnMaxSeekToPreviousPositionChanged",
        [&](PlayerListener* listener, JNIEnv* env) {
      PlaybackSnapshot snapshot = GetSnapshot(env);
      snapshot.max_seek_to_previous_position_ms = max_seek_to_previous_position_ms;
      listener->OnMaxSeekToPreviousPositionChanged(
          snapshot, max_seek_to_previous_position_ms);
    });
  }

  void OnTrackSelectionParametersChanged() {
    WithListenerEnv("OnTrackSelectionParametersChanged", [&](PlayerListener* listener, JNIEnv* env) {
      listener->OnTrackSelectionParametersChanged(GetSnapshot(env), GetTrackSelectionParameters(env));
    });
  }

  void OnPlaybackParametersChanged(float speed, float pitch) {
    WithListenerEnv("OnPlaybackParametersChanged", [&](PlayerListener* listener, JNIEnv* env) {
      PlaybackParametersSnapshot parameters;
      parameters.speed = speed;
      parameters.pitch = pitch;
      PlaybackSnapshot snapshot = GetSnapshot(env);
      snapshot.playback_speed = speed;
      listener->OnPlaybackParametersChanged(snapshot, parameters);
    });
  }

  void OnPlaybackSuppressionReasonChanged(int suppression_reason) {
    WithListenerEnv(
        "OnPlaybackSuppressionReasonChanged",
        [&](PlayerListener* listener, JNIEnv* env) {
      listener->OnPlaybackSuppressionReasonChanged(
          GetSnapshot(env), ToPlaybackSuppressionReason(suppression_reason));
    });
  }

  void OnAvailableCommandsChanged(const std::vector<int>& command_codes) {
    WithListenerEnv("OnAvailableCommandsChanged", [&](PlayerListener* listener, JNIEnv* env) {
      AvailableCommandsSnapshot commands;
      commands.command_codes = command_codes;
      listener->OnAvailableCommandsChanged(GetSnapshot(env), commands);
    });
  }

  void OnEvents(const std::vector<int>& event_codes) {
    WithListenerEnv("OnEvents", [&](PlayerListener* listener, JNIEnv* env) {
      PlayerEventsSnapshot events;
      events.event_codes = event_codes;
      listener->OnEvents(GetSnapshot(env), events);
    });
  }

  void OnDeviceInfoChanged(const DeviceInfoDescriptor& device_info) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnDeviceInfoChanged(GetSnapshot(env), device_info);
    });
  }

  void OnDeviceVolumeChanged(int device_volume, bool muted) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnDeviceVolumeChanged(GetSnapshot(env), device_volume, muted);
    });
  }

  void OnSkipSilenceEnabledChanged(bool skip_silence_enabled) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnSkipSilenceEnabledChanged(GetSnapshot(env), skip_silence_enabled);
    });
  }

  void OnVideoSizeChanged(const VideoSizeSnapshot& video_size) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnVideoSizeChanged(GetSnapshot(env), video_size);
    });
  }

  void OnSurfaceSizeChanged(int width, int height) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnSurfaceSizeChanged(GetSnapshot(env), width, height);
    });
  }

  void OnRenderedFirstFrame() {
    WithListenerEnv(
        [&](PlayerListener* listener, JNIEnv* env) { listener->OnRenderedFirstFrame(GetSnapshot(env)); });
  }

  void OnMediaMetadataChanged(const MediaMetadataSnapshot& metadata) {
    WithListenerEnv("OnMediaMetadataChanged", [&](PlayerListener* listener, JNIEnv* env) {
      listener->OnMediaMetadataChanged(GetSnapshot(env), metadata);
    });
  }

  void OnPlaylistMetadataChanged(const MediaMetadataSnapshot& metadata) {
    WithListenerEnv("OnPlaylistMetadataChanged", [&](PlayerListener* listener, JNIEnv* env) {
      listener->OnPlaylistMetadataChanged(GetSnapshot(env), metadata);
    });
  }

  void OnAnalyticsUpdated() {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnAnalyticsUpdated(GetSnapshot(env), GetAnalyticsSnapshot(env));
    });
  }

  void OnAudioUnderrun(
      int buffer_size,
      int64_t buffer_size_ms,
      int64_t elapsed_since_last_feed_ms) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AudioUnderrunEvent audio_underrun;
      audio_underrun.buffer_size = buffer_size;
      audio_underrun.buffer_size_ms = buffer_size_ms;
      audio_underrun.elapsed_since_last_feed_ms = elapsed_since_last_feed_ms;
      listener->OnAudioUnderrun(GetSnapshot(env), audio_underrun);
    });
  }

  void OnDroppedVideoFrames(int dropped_frames, int64_t elapsed_ms) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      DroppedVideoFramesEvent dropped_video_frames;
      dropped_video_frames.dropped_frames = dropped_frames;
      dropped_video_frames.elapsed_ms = elapsed_ms;
      listener->OnDroppedVideoFrames(GetSnapshot(env), dropped_video_frames);
    });
  }

  void OnBandwidthEstimate(
      int elapsed_ms,
      int64_t bytes_transferred,
      int64_t bitrate_estimate) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      BandwidthEstimateEvent bandwidth_estimate_event;
      bandwidth_estimate_event.elapsed_ms = elapsed_ms;
      bandwidth_estimate_event.bytes_transferred = bytes_transferred;
      bandwidth_estimate_event.bitrate_estimate = bitrate_estimate;
      listener->OnBandwidthEstimate(GetSnapshot(env), bandwidth_estimate_event);
    });
  }

  void OnLoadStarted(
      const std::string& uri,
      int data_type,
      int track_type,
      int retry_count) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      LoadStartedEvent load_started;
      load_started.uri = uri;
      load_started.data_type = data_type;
      load_started.track_type = track_type;
      load_started.retry_count = retry_count;
      listener->OnLoadStarted(GetSnapshot(env), load_started);
    });
  }

  void OnLoadCompleted(
      const std::string& uri,
      int data_type,
      int track_type) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      LoadCompletedEvent load_completed;
      load_completed.uri = uri;
      load_completed.data_type = data_type;
      load_completed.track_type = track_type;
      listener->OnLoadCompleted(GetSnapshot(env), load_completed);
    });
  }

  void OnAudioInputFormatChanged(
      const std::string& sample_mime_type,
      const std::string& codecs,
      int channel_count,
      int sample_rate) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AudioInputFormatChangedEvent audio_input_format_changed;
      audio_input_format_changed.sample_mime_type = sample_mime_type;
      audio_input_format_changed.codecs = codecs;
      audio_input_format_changed.channel_count = channel_count;
      audio_input_format_changed.sample_rate = sample_rate;
      listener->OnAudioInputFormatChanged(GetSnapshot(env), audio_input_format_changed);
    });
  }

  void OnAudioDecoderInitialized(
      const std::string& decoder_name,
      int64_t initialized_timestamp_ms,
      int64_t initialization_duration_ms) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AudioDecoderInitializedEvent audio_decoder_initialized;
      audio_decoder_initialized.decoder_name = decoder_name;
      audio_decoder_initialized.initialized_timestamp_ms = initialized_timestamp_ms;
      audio_decoder_initialized.initialization_duration_ms = initialization_duration_ms;
      listener->OnAudioDecoderInitialized(GetSnapshot(env), audio_decoder_initialized);
    });
  }

  void OnVideoDecoderInitialized(
      const std::string& decoder_name,
      int64_t initialized_timestamp_ms,
      int64_t initialization_duration_ms) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      VideoDecoderInitializedEvent video_decoder_initialized;
      video_decoder_initialized.decoder_name = decoder_name;
      video_decoder_initialized.initialized_timestamp_ms = initialized_timestamp_ms;
      video_decoder_initialized.initialization_duration_ms = initialization_duration_ms;
      listener->OnVideoDecoderInitialized(GetSnapshot(env), video_decoder_initialized);
    });
  }

  void OnAudioDecoderReleased(const std::string& decoder_name) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AudioDecoderReleasedEvent audio_decoder_released;
      audio_decoder_released.decoder_name = decoder_name;
      listener->OnAudioDecoderReleased(GetSnapshot(env), audio_decoder_released);
    });
  }

  void OnVideoDecoderReleased(const std::string& decoder_name) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      VideoDecoderReleasedEvent video_decoder_released;
      video_decoder_released.decoder_name = decoder_name;
      listener->OnVideoDecoderReleased(GetSnapshot(env), video_decoder_released);
    });
  }

  void OnAnalyticsRenderedFirstFrame(int64_t render_time_ms) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsRenderedFirstFrameEvent rendered_first_frame;
      rendered_first_frame.render_time_ms = render_time_ms;
      listener->OnAnalyticsRenderedFirstFrame(GetSnapshot(env), rendered_first_frame);
    });
  }

  void OnAnalyticsVideoSizeChanged(
      int width,
      int height,
      float pixel_width_height_ratio) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsVideoSizeChangedEvent analytics_video_size;
      analytics_video_size.width = width;
      analytics_video_size.height = height;
      analytics_video_size.pixel_width_height_ratio = pixel_width_height_ratio;
      listener->OnAnalyticsVideoSizeChanged(GetSnapshot(env), analytics_video_size);
    });
  }

  void OnAudioPositionAdvancing(int64_t playout_start_system_time_ms) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AudioPositionAdvancingEvent audio_position_advancing;
      audio_position_advancing.playout_start_system_time_ms = playout_start_system_time_ms;
      listener->OnAudioPositionAdvancing(GetSnapshot(env), audio_position_advancing);
    });
  }

  void OnVideoFrameProcessingOffset(
      int64_t total_processing_offset_us,
      int frame_count) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      VideoFrameProcessingOffsetEvent video_frame_processing_offset;
      video_frame_processing_offset.total_processing_offset_us = total_processing_offset_us;
      video_frame_processing_offset.frame_count = frame_count;
      listener->OnVideoFrameProcessingOffset(GetSnapshot(env), video_frame_processing_offset);
    });
  }

  void OnVolumeChanged(float volume) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      VolumeChangedEvent volume_changed;
      volume_changed.volume = volume;
      listener->OnVolumeChanged(GetSnapshot(env), volume_changed);
    });
  }

  void OnAudioSessionIdChanged(int audio_session_id) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AudioSessionIdChangedEvent audio_session_id_changed;
      audio_session_id_changed.audio_session_id = audio_session_id;
      listener->OnAudioSessionIdChanged(GetSnapshot(env), audio_session_id_changed);
    });
  }

  void OnAnalyticsSkipSilenceEnabledChanged(bool skip_silence_enabled) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsSkipSilenceEnabledChangedEvent skip_silence_enabled_changed;
      skip_silence_enabled_changed.skip_silence_enabled = skip_silence_enabled;
      listener->OnAnalyticsSkipSilenceEnabledChanged(
          GetSnapshot(env), skip_silence_enabled_changed);
    });
  }

  void OnAnalyticsDeviceVolumeChanged(int volume, bool muted) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsDeviceVolumeChangedEvent device_volume_changed;
      device_volume_changed.volume = volume;
      device_volume_changed.muted = muted;
      listener->OnAnalyticsDeviceVolumeChanged(GetSnapshot(env), device_volume_changed);
    });
  }

  void OnAnalyticsPlaybackStateChanged(int playback_state) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsPlaybackStateChangedEvent playback_state_changed;
      playback_state_changed.playback_state = playback_state;
      listener->OnAnalyticsPlaybackStateChanged(GetSnapshot(env), playback_state_changed);
    });
  }

  void OnAnalyticsIsPlayingChanged(bool is_playing) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsIsPlayingChangedEvent is_playing_changed;
      is_playing_changed.is_playing = is_playing;
      listener->OnAnalyticsIsPlayingChanged(GetSnapshot(env), is_playing_changed);
    });
  }

  void OnAnalyticsPlayWhenReadyChanged(bool play_when_ready, int reason) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsPlayWhenReadyChangedEvent play_when_ready_changed;
      play_when_ready_changed.play_when_ready = play_when_ready;
      play_when_ready_changed.reason = reason;
      listener->OnAnalyticsPlayWhenReadyChanged(GetSnapshot(env), play_when_ready_changed);
    });
  }

  void OnAnalyticsPlaybackSuppressionReasonChanged(int playback_suppression_reason) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsPlaybackSuppressionReasonChangedEvent suppression_reason_changed;
      suppression_reason_changed.playback_suppression_reason = playback_suppression_reason;
      listener->OnAnalyticsPlaybackSuppressionReasonChanged(
          GetSnapshot(env), suppression_reason_changed);
    });
  }

  void OnAnalyticsIsLoadingChanged(bool is_loading) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsIsLoadingChangedEvent is_loading_changed;
      is_loading_changed.is_loading = is_loading;
      listener->OnAnalyticsIsLoadingChanged(GetSnapshot(env), is_loading_changed);
    });
  }

  void OnAnalyticsRepeatModeChanged(int repeat_mode) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsRepeatModeChangedEvent repeat_mode_changed;
      repeat_mode_changed.repeat_mode = repeat_mode;
      listener->OnAnalyticsRepeatModeChanged(GetSnapshot(env), repeat_mode_changed);
    });
  }

  void OnAnalyticsShuffleModeChanged(bool shuffle_mode_enabled) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsShuffleModeChangedEvent shuffle_mode_changed;
      shuffle_mode_changed.shuffle_mode_enabled = shuffle_mode_enabled;
      listener->OnAnalyticsShuffleModeChanged(GetSnapshot(env), shuffle_mode_changed);
    });
  }

  void OnAnalyticsPlaybackParametersChanged(float speed, float pitch) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsPlaybackParametersChangedEvent playback_parameters_changed;
      playback_parameters_changed.speed = speed;
      playback_parameters_changed.pitch = pitch;
      listener->OnAnalyticsPlaybackParametersChanged(
          GetSnapshot(env), playback_parameters_changed);
    });
  }

  void OnAnalyticsAvailableCommandsChanged(const std::vector<int>& command_codes) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsAvailableCommandsChangedEvent available_commands_changed;
      available_commands_changed.commands = command_codes;
      listener->OnAnalyticsAvailableCommandsChanged(
          GetSnapshot(env), available_commands_changed);
    });
  }

  void OnAnalyticsEvents(const std::vector<int>& event_codes) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsEventsEvent analytics_events;
      analytics_events.event_codes = event_codes;
      listener->OnAnalyticsEvents(GetSnapshot(env), analytics_events);
    });
  }

  void OnAnalyticsSeekBackIncrementChanged(int64_t seek_back_increment_ms) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsSeekBackIncrementChangedEvent seek_back_increment_changed;
      seek_back_increment_changed.seek_back_increment_ms = seek_back_increment_ms;
      listener->OnAnalyticsSeekBackIncrementChanged(
          GetSnapshot(env), seek_back_increment_changed);
    });
  }

  void OnAnalyticsSeekForwardIncrementChanged(int64_t seek_forward_increment_ms) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsSeekForwardIncrementChangedEvent seek_forward_increment_changed;
      seek_forward_increment_changed.seek_forward_increment_ms = seek_forward_increment_ms;
      listener->OnAnalyticsSeekForwardIncrementChanged(
          GetSnapshot(env), seek_forward_increment_changed);
    });
  }

  void OnAnalyticsMaxSeekToPreviousPositionChanged(int64_t max_seek_to_previous_position_ms) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsMaxSeekToPreviousPositionChangedEvent max_seek_to_previous_position_changed;
      max_seek_to_previous_position_changed.max_seek_to_previous_position_ms =
          max_seek_to_previous_position_ms;
      listener->OnAnalyticsMaxSeekToPreviousPositionChanged(
          GetSnapshot(env), max_seek_to_previous_position_changed);
    });
  }

  void OnAnalyticsTimelineChanged(int reason) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsTimelineChangedEvent timeline_changed;
      timeline_changed.reason = reason;
      listener->OnAnalyticsTimelineChanged(GetSnapshot(env), timeline_changed);
    });
  }

  void OnAnalyticsPositionDiscontinuity(int reason) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsPositionDiscontinuityEvent position_discontinuity;
      position_discontinuity.reason = reason;
      listener->OnAnalyticsPositionDiscontinuity(GetSnapshot(env), position_discontinuity);
    });
  }

  void OnAnalyticsLoadError(const AnalyticsLoadErrorEvent& load_error) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnAnalyticsLoadError(GetSnapshot(env), load_error);
    });
  }

  void OnAnalyticsSeekStarted() {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      AnalyticsSeekStartedEvent seek_started;
      listener->OnAnalyticsSeekStarted(GetSnapshot(env), seek_started);
    });
  }

  void OnAnalyticsPlayerError(const PlayerError& error) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnAnalyticsPlayerError(GetSnapshot(env), error);
    });
  }

  void OnAnalyticsPlayerErrorChanged(const PlayerError& error) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnAnalyticsPlayerErrorChanged(GetSnapshot(env), error);
    });
  }

  void OnAnalyticsTracksChanged(const TracksSnapshot& tracks) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnAnalyticsTracksChanged(GetSnapshot(env), tracks);
    });
  }

  void OnAnalyticsMediaItemTransition(
      const AnalyticsMediaItemTransitionEvent& media_item_transition) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnAnalyticsMediaItemTransition(GetSnapshot(env), media_item_transition);
    });
  }

  void OnAnalyticsCues(const CueSnapshot& cues) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnAnalyticsCues(GetSnapshot(env), cues);
    });
  }

  void OnAnalyticsMetadata(const AnalyticsMetadataEvent& metadata) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnAnalyticsMetadata(GetSnapshot(env), metadata);
    });
  }

  void OnAnalyticsDeviceInfoChanged(const DeviceInfoDescriptor& device_info) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnAnalyticsDeviceInfoChanged(GetSnapshot(env), device_info);
    });
  }

  void OnAnalyticsMediaMetadataChanged(const MediaMetadataSnapshot& metadata) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnAnalyticsMediaMetadataChanged(GetSnapshot(env), metadata);
    });
  }

  void OnAnalyticsPlaylistMetadataChanged(const MediaMetadataSnapshot& metadata) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      listener->OnAnalyticsPlaylistMetadataChanged(GetSnapshot(env), metadata);
    });
  }

  void OnVideoInputFormatChanged(
      const std::string& sample_mime_type,
      const std::string& codecs,
      int width,
      int height,
      float frame_rate) {
    WithListenerEnv([&](PlayerListener* listener, JNIEnv* env) {
      VideoInputFormatChangedEvent video_input_format_changed;
      video_input_format_changed.sample_mime_type = sample_mime_type;
      video_input_format_changed.codecs = codecs;
      video_input_format_changed.width = width;
      video_input_format_changed.height = height;
      video_input_format_changed.frame_rate = frame_rate;
      listener->OnVideoInputFormatChanged(GetSnapshot(env), video_input_format_changed);
    });
  }

  void OnImageOutputAvailable(const ImageFrameSnapshot& image_frame) {
    WithImageOutputListener(
        [&](ImageOutputListener* listener) { listener->OnImageAvailable(image_frame); });
  }

  void OnImageOutputDisabled() {
    WithImageOutputListener([](ImageOutputListener* listener) { listener->OnDisabled(); });
  }

 private:
  class ScopedListenerCallback {
   public:
    ScopedListenerCallback() = default;
    ScopedListenerCallback(JniExoPlayerBridge* owner_in, PlayerListener* listener_in)
        : owner(owner_in), listener(listener_in) {}

    ScopedListenerCallback(const ScopedListenerCallback&) = delete;
    ScopedListenerCallback& operator=(const ScopedListenerCallback&) = delete;

    ScopedListenerCallback(ScopedListenerCallback&& other) noexcept
        : owner(other.owner), listener(other.listener) {
      other.owner = nullptr;
      other.listener = nullptr;
    }

    ScopedListenerCallback& operator=(ScopedListenerCallback&& other) noexcept {
      if (this != &other) {
        Release();
        owner = other.owner;
        listener = other.listener;
        other.owner = nullptr;
        other.listener = nullptr;
      }
      return *this;
    }

    ~ScopedListenerCallback() { Release(); }

    void Release() {
      if (owner != nullptr) {
        owner->OnListenerCallbackReleased();
        owner = nullptr;
      }
    }

    JniExoPlayerBridge* owner = nullptr;
    PlayerListener* listener = nullptr;
  };

  class ScopedImageOutputCallback {
   public:
    ScopedImageOutputCallback() = default;
    ScopedImageOutputCallback(JniExoPlayerBridge* owner_in, ImageOutputListener* listener_in)
        : owner(owner_in), listener(listener_in) {}

    ScopedImageOutputCallback(const ScopedImageOutputCallback&) = delete;
    ScopedImageOutputCallback& operator=(const ScopedImageOutputCallback&) = delete;

    ScopedImageOutputCallback(ScopedImageOutputCallback&& other) noexcept
        : owner(other.owner), listener(other.listener) {
      other.owner = nullptr;
      other.listener = nullptr;
    }

    ScopedImageOutputCallback& operator=(ScopedImageOutputCallback&& other) noexcept {
      if (this != &other) {
        Release();
        owner = other.owner;
        listener = other.listener;
        other.owner = nullptr;
        other.listener = nullptr;
      }
      return *this;
    }

    ~ScopedImageOutputCallback() { Release(); }

    void Release() {
      if (owner != nullptr) {
        owner->OnImageOutputCallbackReleased();
        owner = nullptr;
      }
    }

    JniExoPlayerBridge* owner = nullptr;
    ImageOutputListener* listener = nullptr;
  };

  template <typename Fn>
  void WithListenerEnv(const char* callback_name, Fn&& fn) {
    ScopedListenerCallback listener_callback = AcquireListenerCallback(callback_name);
    if (listener_callback.listener == nullptr) {
      if (ShouldTraceSmokeListenerCallback(callback_name)) {
        LogInfo(
            std::string("JniExoPlayerBridge WithListenerEnv callback=") + callback_name +
            ",phase=skip-no-listener," + BuildListenerState());
      }
      return;
    }
    ScopedEnv env(
        java_vm_,
        "GetEnv failed while resolving callback JNIEnv",
        "AttachCurrentThread failed while resolving callback JNIEnv");
    if (!env.ok()) {
      if (ShouldTraceSmokeListenerCallback(callback_name)) {
        LogError(
            std::string("JniExoPlayerBridge WithListenerEnv callback=") + callback_name +
            ",phase=skip-no-env");
      }
      return;
    }
    if (releasing_.load(std::memory_order_acquire)) {
      if (ShouldTraceSmokeListenerCallback(callback_name)) {
        LogInfo(
            std::string("JniExoPlayerBridge WithListenerEnv callback=") + callback_name +
            ",phase=skip-releasing," + BuildListenerState());
      }
      return;
    }
    if (ShouldTraceSmokeListenerCallback(callback_name)) {
      LogInfo(
          std::string("JniExoPlayerBridge WithListenerEnv callback=") + callback_name +
          ",phase=begin,listenerPtr=" + BuildPointerSummary(listener_callback.listener));
    }
    fn(listener_callback.listener, env.env());
    if (ShouldTraceSmokeListenerCallback(callback_name)) {
      LogInfo(
          std::string("JniExoPlayerBridge WithListenerEnv callback=") + callback_name +
          ",phase=end,listenerPtr=" + BuildPointerSummary(listener_callback.listener));
    }
  }

  template <typename Fn>
  void WithListenerEnv(Fn&& fn) {
    WithListenerEnv(nullptr, std::forward<Fn>(fn));
  }

  template <typename Fn>
  void WithImageOutputListener(Fn&& fn) {
    ScopedImageOutputCallback listener_callback = AcquireImageOutputCallback();
    if (listener_callback.listener != nullptr) {
      fn(listener_callback.listener);
    }
  }

  ScopedListenerCallback AcquireListenerCallback(const char* callback_name = nullptr) {
    if (releasing_.load(std::memory_order_acquire)) {
      if (ShouldTraceSmokeListenerCallback(callback_name)) {
        LogInfo(
            std::string("JniExoPlayerBridge AcquireListenerCallback callback=") + callback_name +
            ",phase=skip-releasing," + BuildListenerState());
      }
      return ScopedListenerCallback();
    }
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (releasing_.load(std::memory_order_acquire) || listener_ == nullptr) {
      if (ShouldTraceSmokeListenerCallback(callback_name)) {
        LogInfo(
            std::string("JniExoPlayerBridge AcquireListenerCallback callback=") + callback_name +
            ",phase=skip-no-listener," + BuildListenerStateLocked());
      }
      return ScopedListenerCallback();
    }
    ++in_flight_listener_callback_count_;
    if (ShouldTraceSmokeListenerCallback(callback_name)) {
      LogInfo(
          std::string("JniExoPlayerBridge AcquireListenerCallback callback=") + callback_name +
          ",phase=acquired," + BuildListenerStateLocked());
    }
    return ScopedListenerCallback(this, listener_);
  }

  ScopedImageOutputCallback AcquireImageOutputCallback() {
    if (releasing_.load(std::memory_order_acquire)) {
      return ScopedImageOutputCallback();
    }
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (releasing_.load(std::memory_order_acquire) || image_output_listener_ == nullptr) {
      return ScopedImageOutputCallback();
    }
    ++in_flight_image_output_callback_count_;
    return ScopedImageOutputCallback(this, image_output_listener_);
  }

  void OnListenerCallbackReleased() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (--in_flight_listener_callback_count_ == 0) {
      listener_callback_drained_.notify_all();
    }
  }

  std::string BuildListenerStateLocked() const {
    return "listenerPtr=" + BuildPointerSummary(listener_) +
        ",imageOutputPtr=" + BuildPointerSummary(image_output_listener_) +
        ",inFlightListener=" + std::to_string(in_flight_listener_callback_count_) +
        ",inFlightImageOutput=" + std::to_string(in_flight_image_output_callback_count_) +
        ",releasing=" + std::to_string(releasing_.load(std::memory_order_acquire) ? 1 : 0);
  }

  std::string BuildListenerState() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return BuildListenerStateLocked();
  }

  void OnImageOutputCallbackReleased() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (--in_flight_image_output_callback_count_ == 0) {
      image_output_callback_drained_.notify_all();
    }
  }

  void SetLastError(int error_code, const std::string& message) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    last_error_.error_code = error_code;
    last_error_.message = message;
  }

  PlayerError GetLastErrorSnapshot() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return last_error_;
  }

  jobject GetJavaBridgeLocalRef(JNIEnv* env) {
    jobject java_bridge = nullptr;
    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      java_bridge = java_bridge_;
    }
    if (java_bridge == nullptr) {
      LogError("GetJavaBridgeLocalRef called after bridge release");
      return nullptr;
    }
    jobject local_ref = env->NewLocalRef(java_bridge);
    if (ClearJniExceptionIfPresent(env, "NewLocalRef(java_bridge_)")) {
      return nullptr;
    }
    if (local_ref == nullptr) {
      LogError("Failed to create local ref for java_bridge_");
    }
    return local_ref;
  }

  jclass GetBridgeClass(JNIEnv* env, jobject bridge_object) const {
    if (bridge_object == nullptr) {
      return nullptr;
    }
    jclass bridge_class = env->GetObjectClass(bridge_object);
    if (ClearJniExceptionIfPresent(env, "GetObjectClass(java_bridge_)")) {
      return nullptr;
    }
    return bridge_class;
  }

  jmethodID GetBridgeMethod(
      JNIEnv* env,
      jclass bridge_class,
      const char* method_name,
      const char* signature) const {
    if (bridge_class == nullptr) {
      return nullptr;
    }
    jmethodID method = env->GetMethodID(bridge_class, method_name, signature);
    if (ClearJniExceptionIfPresent(env, std::string("GetMethodID(") + method_name + ")")) {
      return nullptr;
    }
    if (method == nullptr) {
      LogError(std::string("Missing JNI method: ") + method_name + " " + signature);
      return nullptr;
    }
    return method;
  }

  void ReleaseOpaqueObjectTokensInternal(JNIEnv* env, const std::vector<std::string>& tokens) {
    if (env == nullptr || tokens.empty()) {
      return;
    }
    jobjectArray java_tokens = CreateJavaStringArray(env, tokens);
    if (java_tokens == nullptr) {
      return;
    }
    CallBridgeVoid(env, "releaseOpaqueObjectTokens", "([Ljava/lang/String;)V", java_tokens);
    DeleteLocalRefIfNotNull(env, java_tokens);
  }

  template <typename... Args>
  void CallBridgeVoid(JNIEnv* env, const char* method_name, const char* signature, Args... args) {
    if (env == nullptr) {
      return;
    }
    jobject bridge_object = GetJavaBridgeLocalRef(env);
    jclass bridge_class = GetBridgeClass(env, bridge_object);
    if (bridge_class == nullptr) {
      DeleteLocalRefIfNotNull(env, bridge_object);
      return;
    }
    jmethodID method = GetBridgeMethod(env, bridge_class, method_name, signature);
    if (method == nullptr) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return;
    }
    env->CallVoidMethod(bridge_object, method, args...);
    ClearJniExceptionIfPresent(env, std::string("CallVoidMethod(") + method_name + ")");
    env->DeleteLocalRef(bridge_class);
    env->DeleteLocalRef(bridge_object);
  }

  template <typename... Args>
  jobject CallBridgeObject(
      JNIEnv* env,
      const char* method_name,
      const char* signature,
      Args... args) {
    if (env == nullptr) {
      return nullptr;
    }
    jobject bridge_object = GetJavaBridgeLocalRef(env);
    jclass bridge_class = GetBridgeClass(env, bridge_object);
    if (bridge_class == nullptr) {
      DeleteLocalRefIfNotNull(env, bridge_object);
      return nullptr;
    }
    jmethodID method = GetBridgeMethod(env, bridge_class, method_name, signature);
    if (method == nullptr) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return nullptr;
    }
    jobject result = env->CallObjectMethod(bridge_object, method, args...);
    if (ClearJniExceptionIfPresent(env, std::string("CallObjectMethod(") + method_name + ")")) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return nullptr;
    }
    env->DeleteLocalRef(bridge_class);
    env->DeleteLocalRef(bridge_object);
    return result;
  }

  template <typename... Args>
  jboolean CallBridgeBoolean(
      JNIEnv* env,
      const char* method_name,
      const char* signature,
      Args... args) {
    if (env == nullptr) {
      return JNI_FALSE;
    }
    jobject bridge_object = GetJavaBridgeLocalRef(env);
    jclass bridge_class = GetBridgeClass(env, bridge_object);
    if (bridge_class == nullptr) {
      DeleteLocalRefIfNotNull(env, bridge_object);
      return JNI_FALSE;
    }
    jmethodID method = GetBridgeMethod(env, bridge_class, method_name, signature);
    if (method == nullptr) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return JNI_FALSE;
    }
    jboolean result = env->CallBooleanMethod(bridge_object, method, args...);
    if (ClearJniExceptionIfPresent(env, std::string("CallBooleanMethod(") + method_name + ")")) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return JNI_FALSE;
    }
    env->DeleteLocalRef(bridge_class);
    env->DeleteLocalRef(bridge_object);
    return JNI_FALSE != result;
  }

  void CallVoidNoArgsOnBridge(JNIEnv* env, jobject bridge_object, const char* method_name) {
    if (env == nullptr || bridge_object == nullptr) {
      return;
    }
    jclass bridge_class = GetBridgeClass(env, bridge_object);
    if (bridge_class == nullptr) {
      return;
    }
    jmethodID method = GetBridgeMethod(env, bridge_class, method_name, "()V");
    if (method == nullptr) {
      env->DeleteLocalRef(bridge_class);
      return;
    }
    env->CallVoidMethod(bridge_object, method);
    ClearJniExceptionIfPresent(env, std::string("CallVoidMethod(") + method_name + ")");
    env->DeleteLocalRef(bridge_class);
  }

  void CallVoidNoArgs(JNIEnv* env, const char* method_name) {
    if (env == nullptr) {
      return;
    }
    jobject bridge_object = GetJavaBridgeLocalRef(env);
    if (bridge_object == nullptr) {
      return;
    }
    CallVoidNoArgsOnBridge(env, bridge_object, method_name);
    env->DeleteLocalRef(bridge_object);
  }

  jint CallIntNoArgs(JNIEnv* env, const char* method_name) {
    if (env == nullptr) {
      return 0;
    }
    jobject bridge_object = GetJavaBridgeLocalRef(env);
    jclass bridge_class = GetBridgeClass(env, bridge_object);
    if (bridge_class == nullptr) {
      DeleteLocalRefIfNotNull(env, bridge_object);
      return 0;
    }
    jmethodID method = GetBridgeMethod(env, bridge_class, method_name, "()I");
    if (method == nullptr) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return 0;
    }
    jint result = env->CallIntMethod(bridge_object, method);
    if (ClearJniExceptionIfPresent(env, std::string("CallIntMethod(") + method_name + ")")) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return 0;
    }
    env->DeleteLocalRef(bridge_class);
    env->DeleteLocalRef(bridge_object);
    return result;
  }

  jobject CallObjectNoArgs(JNIEnv* env, const char* method_name, const char* signature) {
    if (env == nullptr) {
      return nullptr;
    }
    jobject bridge_object = GetJavaBridgeLocalRef(env);
    jclass bridge_class = GetBridgeClass(env, bridge_object);
    if (bridge_class == nullptr) {
      DeleteLocalRefIfNotNull(env, bridge_object);
      return nullptr;
    }
    jmethodID method = GetBridgeMethod(env, bridge_class, method_name, signature);
    if (method == nullptr) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return nullptr;
    }
    jobject result = env->CallObjectMethod(bridge_object, method);
    if (ClearJniExceptionIfPresent(env, std::string("CallObjectMethod(") + method_name + ")")) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return nullptr;
    }
    env->DeleteLocalRef(bridge_class);
    env->DeleteLocalRef(bridge_object);
    return result;
  }

  jlong CallLongNoArgs(JNIEnv* env, const char* method_name) {
    if (env == nullptr) {
      return 0;
    }
    jobject bridge_object = GetJavaBridgeLocalRef(env);
    jclass bridge_class = GetBridgeClass(env, bridge_object);
    if (bridge_class == nullptr) {
      DeleteLocalRefIfNotNull(env, bridge_object);
      return 0;
    }
    jmethodID method = GetBridgeMethod(env, bridge_class, method_name, "()J");
    if (method == nullptr) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return 0;
    }
    jlong result = env->CallLongMethod(bridge_object, method);
    if (ClearJniExceptionIfPresent(env, std::string("CallLongMethod(") + method_name + ")")) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return 0;
    }
    env->DeleteLocalRef(bridge_class);
    env->DeleteLocalRef(bridge_object);
    return result;
  }

  jboolean CallBooleanNoArgs(JNIEnv* env, const char* method_name) {
    if (env == nullptr) {
      return JNI_FALSE;
    }
    jobject bridge_object = GetJavaBridgeLocalRef(env);
    jclass bridge_class = GetBridgeClass(env, bridge_object);
    if (bridge_class == nullptr) {
      DeleteLocalRefIfNotNull(env, bridge_object);
      return JNI_FALSE;
    }
    jmethodID method = GetBridgeMethod(env, bridge_class, method_name, "()Z");
    if (method == nullptr) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return JNI_FALSE;
    }
    jboolean result = env->CallBooleanMethod(bridge_object, method);
    if (ClearJniExceptionIfPresent(env, std::string("CallBooleanMethod(") + method_name + ")")) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return JNI_FALSE;
    }
    env->DeleteLocalRef(bridge_class);
    env->DeleteLocalRef(bridge_object);
    return result;
  }

  jfloat CallFloatNoArgs(JNIEnv* env, const char* method_name) {
    if (env == nullptr) {
      return 0.0f;
    }
    jobject bridge_object = GetJavaBridgeLocalRef(env);
    jclass bridge_class = GetBridgeClass(env, bridge_object);
    if (bridge_class == nullptr) {
      DeleteLocalRefIfNotNull(env, bridge_object);
      return 0.0f;
    }
    jmethodID method = GetBridgeMethod(env, bridge_class, method_name, "()F");
    if (method == nullptr) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return 0.0f;
    }
    jfloat result = env->CallFloatMethod(bridge_object, method);
    if (ClearJniExceptionIfPresent(env, std::string("CallFloatMethod(") + method_name + ")")) {
      env->DeleteLocalRef(bridge_class);
      env->DeleteLocalRef(bridge_object);
      return 0.0f;
    }
    env->DeleteLocalRef(bridge_class);
    env->DeleteLocalRef(bridge_object);
    return result;
  }

  JavaVM* java_vm_;
  mutable std::mutex state_mutex_;
  jobject java_bridge_;
  std::atomic<bool> releasing_{false};
  std::condition_variable listener_callback_drained_;
  std::condition_variable image_output_callback_drained_;
  PlayerListener* listener_ = nullptr;
  ImageOutputListener* image_output_listener_ = nullptr;
  int in_flight_listener_callback_count_ = 0;
  int in_flight_image_output_callback_count_ = 0;
  PlayerError last_error_;
};

class LoggingPlayerListener : public PlayerListener {
 public:
  void OnPlaybackStateChanged(const PlaybackSnapshot& snapshot) override {
    LogInfo("state=" + std::to_string(static_cast<int>(snapshot.playback_state)) +
            " positionMs=" + std::to_string(snapshot.current_position_ms));
  }

  void OnPlayWhenReadyChanged(const PlaybackSnapshot& snapshot, int reason) override {
    LogInfo("playWhenReady=" + std::to_string(snapshot.play_when_ready) +
            " reason=" + std::to_string(reason));
  }

  void OnIsPlayingChanged(const PlaybackSnapshot& snapshot) override {
    LogInfo("isPlaying=" + std::to_string(snapshot.is_playing));
  }

  void OnMediaItemTransition(const PlaybackSnapshot& snapshot, int reason) override {
    LogInfo("mediaItemIndex=" + std::to_string(snapshot.current_media_item_index) +
            " reason=" + std::to_string(reason));
  }

  void OnPlayerError(const PlaybackSnapshot& snapshot) override {
    LogInfo("errorCode=" + std::to_string(snapshot.last_error.error_code) +
            " message=" + snapshot.last_error.message);
  }
};

PlayerListener& GetDemoLoggingPlayerListener() {
  static LoggingPlayerListener listener;
  return listener;
}

namespace {

template <typename Fn>
void WithBridgeHandle(jlong native_handle, Fn&& fn) {
  std::shared_ptr<JniExoPlayerBridge> bridge = AcquireBridge(native_handle);
  if (bridge != nullptr) {
    fn(*bridge);
  }
}

}  // namespace

void BridgeOnPlaybackStateChanged(jlong native_handle, int playback_state) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnPlaybackStateChanged(playback_state);
  });
}

void BridgeOnPlayWhenReadyChanged(jlong native_handle, bool play_when_ready, int reason) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnPlayWhenReadyChanged(play_when_ready, reason);
  });
}

void BridgeOnIsPlayingChanged(jlong native_handle, bool is_playing) {
  WithBridgeHandle(native_handle,
                   [&](JniExoPlayerBridge& bridge) { bridge.OnIsPlayingChanged(is_playing); });
}

void BridgeOnMediaItemTransition(jlong native_handle, int media_item_index, int reason) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnMediaItemTransition(media_item_index, reason);
  });
}

void BridgeOnPlayerError(JNIEnv* env, jlong native_handle, int error_code, jstring message) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnPlayerError(error_code, JStringToString(env, message));
  });
}

void BridgeOnPlayerErrorChanged(JNIEnv* env, jlong native_handle, int error_code, jstring message) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnPlayerErrorChanged(error_code, JStringToString(env, message));
  });
}

void BridgeOnTimelineChanged(jlong native_handle, int window_count, int period_count, int reason) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnTimelineChanged(window_count, period_count, reason);
  });
}

void BridgeOnTracksChanged(jlong native_handle) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) { bridge.OnTracksChanged(); });
}

void BridgeOnPositionDiscontinuity(
    JNIEnv* env,
    jlong native_handle,
    jobject old_position,
    jobject new_position,
    int reason) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    PositionInfoSnapshot old_position_snapshot = FromJavaPositionInfo(env, old_position);
    PositionInfoSnapshot new_position_snapshot = FromJavaPositionInfo(env, new_position);
    bridge.OnPositionDiscontinuity(old_position_snapshot, new_position_snapshot, reason);
  });
}

void BridgeOnAudioAttributesChanged(
    jlong native_handle,
    int content_type,
    int usage,
    int flags,
    int allowed_capture_policy,
    int spatialization_behavior) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    AudioAttributesDescriptor attributes;
    attributes.content_type = content_type;
    attributes.usage = usage;
    attributes.flags = flags;
    attributes.allowed_capture_policy = allowed_capture_policy;
    attributes.spatialization_behavior = spatialization_behavior;
    bridge.OnAudioAttributesChanged(attributes);
  });
}

void BridgeOnCues(jlong native_handle, int cue_count, int64_t presentation_time_us) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnCues(cue_count, presentation_time_us);
  });
}

void BridgeOnRepeatModeChanged(jlong native_handle, int repeat_mode) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnRepeatModeChanged(repeat_mode);
  });
}

void BridgeOnShuffleModeEnabledChanged(jlong native_handle, bool shuffle_mode_enabled) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnShuffleModeEnabledChanged(shuffle_mode_enabled);
  });
}

void BridgeOnSeekBackIncrementChanged(jlong native_handle, int64_t seek_back_increment_ms) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnSeekBackIncrementChanged(seek_back_increment_ms);
  });
}

void BridgeOnSeekForwardIncrementChanged(jlong native_handle, int64_t seek_forward_increment_ms) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnSeekForwardIncrementChanged(seek_forward_increment_ms);
  });
}

void BridgeOnMaxSeekToPreviousPositionChanged(
    jlong native_handle,
    int64_t max_seek_to_previous_position_ms) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnMaxSeekToPreviousPositionChanged(max_seek_to_previous_position_ms);
  });
}

void BridgeOnTrackSelectionParametersChanged(jlong native_handle) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnTrackSelectionParametersChanged();
  });
}

void BridgeOnPlaybackParametersChanged(jlong native_handle, float speed, float pitch) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnPlaybackParametersChanged(speed, pitch);
  });
}

void BridgeOnPlaybackSuppressionReasonChanged(jlong native_handle, int playback_suppression_reason) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnPlaybackSuppressionReasonChanged(playback_suppression_reason);
  });
}

void BridgeOnAvailableCommandsChanged(JNIEnv* env, jlong native_handle, jobject commands_object) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAvailableCommandsChanged(FromJavaCommands(env, commands_object).command_codes);
  });
}

void BridgeOnEvents(JNIEnv* env, jlong native_handle, jobject events_object) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnEvents(FromJavaPlayerEvents(env, events_object).event_codes);
  });
}

void BridgeOnDeviceInfoChanged(JNIEnv* env, jlong native_handle, jobject device_info_object) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnDeviceInfoChanged(FromJavaDeviceInfo(env, device_info_object));
  });
}

void BridgeOnDeviceVolumeChanged(jlong native_handle, int volume, bool muted) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnDeviceVolumeChanged(volume, muted);
  });
}

void BridgeOnSkipSilenceEnabledChanged(jlong native_handle, bool skip_silence_enabled) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnSkipSilenceEnabledChanged(skip_silence_enabled);
  });
}

void BridgeOnVideoSizeChanged(JNIEnv* env, jlong native_handle, jobject video_size_object) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnVideoSizeChanged(FromJavaVideoSize(env, video_size_object));
  });
}

void BridgeOnSurfaceSizeChanged(jlong native_handle, int width, int height) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnSurfaceSizeChanged(width, height);
  });
}

void BridgeOnRenderedFirstFrame(jlong native_handle) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) { bridge.OnRenderedFirstFrame(); });
}

void BridgeOnMediaMetadataChanged(JNIEnv* env, jlong native_handle, jobject metadata_object) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnMediaMetadataChanged(FromJavaMediaMetadata(env, metadata_object));
  });
}

void BridgeOnPlaylistMetadataChanged(JNIEnv* env, jlong native_handle, jobject metadata_object) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnPlaylistMetadataChanged(FromJavaMediaMetadata(env, metadata_object));
  });
}

void BridgeOnAnalyticsUpdated(jlong native_handle) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) { bridge.OnAnalyticsUpdated(); });
}

void BridgeOnAudioUnderrun(
    jlong native_handle,
    int buffer_size,
    int64_t buffer_size_ms,
    int64_t elapsed_since_last_feed_ms) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAudioUnderrun(buffer_size, buffer_size_ms, elapsed_since_last_feed_ms);
  });
}

void BridgeOnDroppedVideoFrames(
    jlong native_handle,
    int dropped_frames,
    int64_t elapsed_ms) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnDroppedVideoFrames(dropped_frames, elapsed_ms);
  });
}

void BridgeOnBandwidthEstimate(
    jlong native_handle,
    int elapsed_ms,
    int64_t bytes_transferred,
    int64_t bitrate_estimate) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnBandwidthEstimate(elapsed_ms, bytes_transferred, bitrate_estimate);
  });
}

void BridgeOnLoadStarted(
    JNIEnv* env,
    jlong native_handle,
    jstring uri,
    int data_type,
    int track_type,
    int retry_count) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnLoadStarted(JStringToString(env, uri), data_type, track_type, retry_count);
  });
}

void BridgeOnLoadCompleted(
    JNIEnv* env,
    jlong native_handle,
    jstring uri,
    int data_type,
    int track_type) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnLoadCompleted(JStringToString(env, uri), data_type, track_type);
  });
}

void BridgeOnAudioInputFormatChanged(
    JNIEnv* env,
    jlong native_handle,
    jstring sample_mime_type,
    jstring codecs,
    int channel_count,
    int sample_rate) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAudioInputFormatChanged(
        JStringToString(env, sample_mime_type),
        JStringToString(env, codecs),
        channel_count,
        sample_rate);
  });
}

void BridgeOnAudioDecoderInitialized(
    JNIEnv* env,
    jlong native_handle,
    jstring decoder_name,
    int64_t initialized_timestamp_ms,
    int64_t initialization_duration_ms) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAudioDecoderInitialized(
        JStringToString(env, decoder_name),
        initialized_timestamp_ms,
        initialization_duration_ms);
  });
}

void BridgeOnVideoDecoderInitialized(
    JNIEnv* env,
    jlong native_handle,
    jstring decoder_name,
    int64_t initialized_timestamp_ms,
    int64_t initialization_duration_ms) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnVideoDecoderInitialized(
        JStringToString(env, decoder_name),
        initialized_timestamp_ms,
        initialization_duration_ms);
  });
}

void BridgeOnAudioDecoderReleased(
    JNIEnv* env,
    jlong native_handle,
    jstring decoder_name) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAudioDecoderReleased(JStringToString(env, decoder_name));
  });
}

void BridgeOnVideoDecoderReleased(
    JNIEnv* env,
    jlong native_handle,
    jstring decoder_name) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnVideoDecoderReleased(JStringToString(env, decoder_name));
  });
}

void BridgeOnAnalyticsRenderedFirstFrame(jlong native_handle, int64_t render_time_ms) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsRenderedFirstFrame(render_time_ms);
  });
}

void BridgeOnAnalyticsVideoSizeChanged(
    jlong native_handle,
    int width,
    int height,
    float pixel_width_height_ratio) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsVideoSizeChanged(width, height, pixel_width_height_ratio);
  });
}

void BridgeOnAudioPositionAdvancing(
    jlong native_handle,
    int64_t playout_start_system_time_ms) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAudioPositionAdvancing(playout_start_system_time_ms);
  });
}

void BridgeOnVideoFrameProcessingOffset(
    jlong native_handle,
    int64_t total_processing_offset_us,
    int frame_count) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnVideoFrameProcessingOffset(total_processing_offset_us, frame_count);
  });
}

void BridgeOnVolumeChanged(
    jlong native_handle,
    float volume) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnVolumeChanged(volume);
  });
}

void BridgeOnAudioSessionIdChanged(
    jlong native_handle,
    int audio_session_id) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAudioSessionIdChanged(audio_session_id);
  });
}

void BridgeOnAnalyticsSkipSilenceEnabledChanged(
    jlong native_handle,
    bool skip_silence_enabled) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsSkipSilenceEnabledChanged(skip_silence_enabled);
  });
}

void BridgeOnAnalyticsDeviceVolumeChanged(
    jlong native_handle,
    int volume,
    bool muted) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsDeviceVolumeChanged(volume, muted);
  });
}

void BridgeOnAnalyticsPlaybackStateChanged(
    jlong native_handle,
    int playback_state) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsPlaybackStateChanged(playback_state);
  });
}

void BridgeOnAnalyticsIsPlayingChanged(
    jlong native_handle,
    bool is_playing) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsIsPlayingChanged(is_playing);
  });
}

void BridgeOnAnalyticsPlayWhenReadyChanged(
    jlong native_handle,
    bool play_when_ready,
    int reason) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsPlayWhenReadyChanged(play_when_ready, reason);
  });
}

void BridgeOnAnalyticsPlaybackSuppressionReasonChanged(
    jlong native_handle,
    int playback_suppression_reason) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsPlaybackSuppressionReasonChanged(playback_suppression_reason);
  });
}

void BridgeOnAnalyticsIsLoadingChanged(
    jlong native_handle,
    bool is_loading) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsIsLoadingChanged(is_loading);
  });
}

void BridgeOnAnalyticsRepeatModeChanged(
    jlong native_handle,
    int repeat_mode) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsRepeatModeChanged(repeat_mode);
  });
}

void BridgeOnAnalyticsShuffleModeChanged(
    jlong native_handle,
    bool shuffle_mode_enabled) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsShuffleModeChanged(shuffle_mode_enabled);
  });
}

void BridgeOnAnalyticsAvailableCommandsChanged(
    JNIEnv* env,
    jlong native_handle,
    jobject commands) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsAvailableCommandsChanged(
        FromJavaCommands(env, commands).command_codes);
  });
}

void BridgeOnAnalyticsEvents(
    JNIEnv* env,
    jlong native_handle,
    jobject events) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsEvents(FromJavaPlayerEvents(env, events).event_codes);
  });
}

void BridgeOnAnalyticsPlaybackParametersChanged(
    jlong native_handle,
    float speed,
    float pitch) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsPlaybackParametersChanged(speed, pitch);
  });
}

void BridgeOnAnalyticsSeekBackIncrementChanged(
    jlong native_handle,
    int64_t seek_back_increment_ms) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsSeekBackIncrementChanged(seek_back_increment_ms);
  });
}

void BridgeOnAnalyticsSeekForwardIncrementChanged(
    jlong native_handle,
    int64_t seek_forward_increment_ms) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsSeekForwardIncrementChanged(seek_forward_increment_ms);
  });
}

void BridgeOnAnalyticsMaxSeekToPreviousPositionChanged(
    jlong native_handle,
    int64_t max_seek_to_previous_position_ms) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsMaxSeekToPreviousPositionChanged(max_seek_to_previous_position_ms);
  });
}

void BridgeOnAnalyticsTimelineChanged(
    jlong native_handle,
    int reason) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsTimelineChanged(reason);
  });
}

void BridgeOnAnalyticsPositionDiscontinuity(
    jlong native_handle,
    int reason) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsPositionDiscontinuity(reason);
  });
}

void BridgeOnAnalyticsSeekStarted(jlong native_handle) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsSeekStarted();
  });
}

void BridgeOnAnalyticsPlayerError(
    JNIEnv* env,
    jlong native_handle,
    int error_code,
    jstring message) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    PlayerError error;
    error.error_code = error_code;
    error.message = JStringToString(env, message);
    bridge.OnAnalyticsPlayerError(error);
  });
}

void BridgeOnAnalyticsPlayerErrorChanged(
    JNIEnv* env,
    jlong native_handle,
    int error_code,
    jstring message) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    PlayerError error;
    error.error_code = error_code;
    error.message = JStringToString(env, message);
    bridge.OnAnalyticsPlayerErrorChanged(error);
  });
}

void BridgeOnAnalyticsTracksChanged(
    JNIEnv* env,
    jlong native_handle,
    jobject tracks) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsTracksChanged(FromJavaTracks(env, tracks));
  });
}

void BridgeOnAnalyticsMediaItemTransition(
    JNIEnv* env,
    jlong native_handle,
    jobject media_item,
    int reason) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    AnalyticsMediaItemTransitionEvent media_item_transition;
    media_item_transition.media_item = FromJavaMediaItem(env, media_item);
    media_item_transition.reason = reason;
    bridge.OnAnalyticsMediaItemTransition(media_item_transition);
  });
}

void BridgeOnAnalyticsCues(
    JNIEnv* env,
    jlong native_handle,
    jobjectArray cues,
    int64_t presentation_time_us) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsCues(FromJavaCues(env, cues, presentation_time_us));
  });
}

void BridgeOnAnalyticsMetadata(
    JNIEnv* env,
    jlong native_handle,
    jint entry_count,
    jstring first_entry_type,
    jstring first_entry_text) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    AnalyticsMetadataEvent metadata;
    metadata.entry_count = entry_count;
    metadata.first_entry_type = JStringToString(env, first_entry_type);
    metadata.first_entry_text = JStringToString(env, first_entry_text);
    bridge.OnAnalyticsMetadata(metadata);
  });
}

void BridgeOnAnalyticsLoadError(
    JNIEnv* env,
    jlong native_handle,
    jstring uri,
    jint data_type,
    jint track_type,
    jstring message,
    jboolean was_canceled) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    AnalyticsLoadErrorEvent load_error;
    load_error.uri = JStringToString(env, uri);
    load_error.data_type = data_type;
    load_error.track_type = track_type;
    load_error.message = JStringToString(env, message);
    load_error.was_canceled = JNI_FALSE != was_canceled;
    bridge.OnAnalyticsLoadError(load_error);
  });
}

void BridgeOnAnalyticsDeviceInfoChanged(
    JNIEnv* env,
    jlong native_handle,
    jobject device_info) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsDeviceInfoChanged(FromJavaDeviceInfo(env, device_info));
  });
}

void BridgeOnAnalyticsMediaMetadataChanged(
    JNIEnv* env,
    jlong native_handle,
    jobject metadata) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsMediaMetadataChanged(FromJavaMediaMetadata(env, metadata));
  });
}

void BridgeOnAnalyticsPlaylistMetadataChanged(
    JNIEnv* env,
    jlong native_handle,
    jobject metadata) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnAnalyticsPlaylistMetadataChanged(FromJavaMediaMetadata(env, metadata));
  });
}

void BridgeOnVideoInputFormatChanged(
    JNIEnv* env,
    jlong native_handle,
    jstring sample_mime_type,
    jstring codecs,
    int width,
    int height,
    float frame_rate) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    bridge.OnVideoInputFormatChanged(
        JStringToString(env, sample_mime_type),
        JStringToString(env, codecs),
        width,
        height,
        frame_rate);
  });
}

void BridgeOnImageOutputAvailable(
    JNIEnv* env,
    jlong native_handle,
    int64_t presentation_time_us,
    int width,
    int height,
    int byte_count,
    int allocation_byte_count,
    int row_bytes,
    bool has_alpha,
    bool is_premultiplied,
    bool is_mutable,
    jstring bitmap_config) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) {
    ImageFrameSnapshot image_frame;
    image_frame.presentation_time_us = presentation_time_us;
    image_frame.width = width;
    image_frame.height = height;
    image_frame.byte_count = byte_count;
    image_frame.allocation_byte_count = allocation_byte_count;
    image_frame.row_bytes = row_bytes;
    image_frame.has_alpha = has_alpha;
    image_frame.is_premultiplied = is_premultiplied;
    image_frame.is_mutable = is_mutable;
    image_frame.bitmap_config = JStringToString(env, bitmap_config);
    bridge.OnImageOutputAvailable(image_frame);
  });
}

void BridgeOnImageOutputDisabled(jlong native_handle) {
  WithBridgeHandle(native_handle, [&](JniExoPlayerBridge& bridge) { bridge.OnImageOutputDisabled(); });
}

std::vector<std::string> BridgeGetPlayerConfigFlagsForTest(
    JNIEnv* env,
    const std::shared_ptr<ExoPlayerBridge>& bridge) {
  auto typed_bridge = std::static_pointer_cast<JniExoPlayerBridge>(bridge);
  return typed_bridge != nullptr ? typed_bridge->GetPlayerConfigFlagsForTest(env)
                                 : std::vector<std::string>{};
}

std::vector<std::string> BridgeGetPriorityTaskManagerStateForTest(
    JNIEnv* env,
    const std::shared_ptr<ExoPlayerBridge>& bridge) {
  auto typed_bridge = std::static_pointer_cast<JniExoPlayerBridge>(bridge);
  return typed_bridge != nullptr ? typed_bridge->GetPriorityTaskManagerStateForTest(env)
                                 : std::vector<std::string>{};
}

std::string BridgeSummarizeVideoEffectsForTest(
    JNIEnv* env,
    const std::shared_ptr<ExoPlayerBridge>& bridge,
    const std::vector<VideoEffectDescriptor>& video_effects) {
  auto typed_bridge = std::static_pointer_cast<JniExoPlayerBridge>(bridge);
  return typed_bridge != nullptr ? typed_bridge->SummarizeVideoEffectsForTest(env, video_effects)
                                 : "";
}

}  // namespace androidx::media3::cppbridge::internal

namespace androidx::media3::cppbridge {

std::shared_ptr<ExoPlayerBridge> ExoPlayerBridge::Create(
    JNIEnv* env,
    jobject context,
    const PlayerConfig& config) {
  return internal::JniExoPlayerBridge::Create(env, context, config);
}

}  // namespace androidx::media3::cppbridge


