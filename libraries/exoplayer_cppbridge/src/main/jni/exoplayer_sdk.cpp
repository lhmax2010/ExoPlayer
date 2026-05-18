#include "exoplayer_cppbridge_jni_internal.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <utility>

namespace androidx::media3::cppbridge {
namespace {

using internal::ScopedEnv;

std::string BuildPointerSummary(const void* value) {
  return std::to_string(
      static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(value)));
}

constexpr char kListenerSmokeCoreBuildMarker[] = "listener-smoke-core-v2026-04-02-13";

void LogListenerSmokeCoreBuildMarkerOnce(const char* source) {
  static std::atomic<bool> logged{false};
  bool expected = false;
  if (!logged.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
    return;
  }
  internal::LogInfo(
      std::string("listenerSmoke coreBuildMarker=") + kListenerSmokeCoreBuildMarker +
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

struct CodecParametersDelegateRegistration {
  PlayerListener* delegate = nullptr;
  std::vector<std::string> keys;
};

CodecParametersDescriptor FilterCodecParametersByKeys(
    const CodecParametersDescriptor& codec_parameters,
    const std::vector<std::string>& keys) {
  CodecParametersDescriptor filtered;
  if (keys.empty()) {
    return filtered;
  }
  for (const CodecParameterDescriptor& parameter : codec_parameters.parameters) {
    if (std::find(keys.begin(), keys.end(), parameter.key) != keys.end()) {
      filtered.parameters.push_back(parameter);
    }
  }
  return filtered;
}

class ExoPlayerSdkPriorityTaskManagerImpl : public ExoPlayerSdkPriorityTaskManager {
 public:
  ExoPlayerSdkPriorityTaskManagerImpl(JavaVM* java_vm, jobject priority_task_manager)
      : java_vm_(java_vm), priority_task_manager_(priority_task_manager) {}

  ~ExoPlayerSdkPriorityTaskManagerImpl() override { Release(); }

  void Release() override {
    jobject priority_task_manager = priority_task_manager_.exchange(nullptr);
    if (priority_task_manager == nullptr || java_vm_ == nullptr) {
      return;
    }
    ScopedEnv env(java_vm_);
    if (!env.ok()) {
      return;
    }
    env.env()->DeleteGlobalRef(priority_task_manager);
  }

  void Add(int priority) override {
    WithLocalManager([&](JNIEnv* env, jobject manager) {
      jclass manager_class = env->GetObjectClass(manager);
      if (manager_class == nullptr) {
        return;
      }
      jmethodID add_method = env->GetMethodID(manager_class, "add", "(I)V");
      if (add_method != nullptr) {
        env->CallVoidMethod(manager, add_method, static_cast<jint>(priority));
      }
      env->DeleteLocalRef(manager_class);
    });
  }

  void Remove(int priority) override {
    WithLocalManager([&](JNIEnv* env, jobject manager) {
      jclass manager_class = env->GetObjectClass(manager);
      if (manager_class == nullptr) {
        return;
      }
      jmethodID remove_method = env->GetMethodID(manager_class, "remove", "(I)V");
      if (remove_method != nullptr) {
        env->CallVoidMethod(manager, remove_method, static_cast<jint>(priority));
      }
      env->DeleteLocalRef(manager_class);
    });
  }

  bool ProceedNonBlocking(int priority) override {
    return WithLocalManagerOrDefault<bool>(
        [&](JNIEnv* env, jobject manager) {
          jclass manager_class = env->GetObjectClass(manager);
          if (manager_class == nullptr) {
            return false;
          }
          jmethodID method =
              env->GetMethodID(manager_class, "proceedNonBlocking", "(I)Z");
          jboolean result = JNI_FALSE;
          if (method != nullptr) {
            result = env->CallBooleanMethod(manager, method, static_cast<jint>(priority));
          }
          env->DeleteLocalRef(manager_class);
          return result != JNI_FALSE;
        },
        false);
  }

  jobject GetJavaObjectLocalRef(JNIEnv* env) override {
    jobject priority_task_manager = priority_task_manager_.load();
    if (env == nullptr || priority_task_manager == nullptr) {
      return nullptr;
    }
    return env->NewLocalRef(priority_task_manager);
  }

 private:
  template <typename Fn>
  void WithLocalManager(Fn&& fn) {
    ScopedEnv env(java_vm_);
    if (!env.ok()) {
      return;
    }
    jobject manager = GetJavaObjectLocalRef(env.env());
    if (manager == nullptr) {
      return;
    }
    fn(env.env(), manager);
    env.env()->DeleteLocalRef(manager);
  }

  template <typename T, typename Fn>
  T WithLocalManagerOrDefault(Fn&& fn, T fallback) {
    ScopedEnv env(java_vm_);
    if (!env.ok()) {
      return fallback;
    }
    jobject manager = GetJavaObjectLocalRef(env.env());
    if (manager == nullptr) {
      return fallback;
    }
    T result = fn(env.env(), manager);
    env.env()->DeleteLocalRef(manager);
    return result;
  }

  JavaVM* java_vm_;
  std::atomic<jobject> priority_task_manager_{nullptr};
};

class ForwardingPlayerListener : public PlayerListener {
 public:
  void SetDelegate(PlayerListener* delegate) {
    LogListenerSmokeCoreBuildMarkerOnce("ForwardingPlayerListener::SetDelegate");
    std::unique_lock<std::mutex> lock(mutex_);
    delegate_ = delegate;
    internal::LogInfo("ForwardingPlayerListener SetDelegate delegatePtr=" +
                      BuildPointerSummary(delegate) + "," + BuildStateLocked());
    if (delegate == nullptr) {
      callback_drained_.wait(lock, [this]() { return in_flight_callback_count_ == 0; });
      internal::LogInfo("ForwardingPlayerListener SetDelegate drained," + BuildStateLocked());
    }
  }
  PlayerListener* delegate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return delegate_;
  }
  void AddAnalyticsDelegate(PlayerListener* delegate) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (delegate == nullptr ||
        std::find(analytics_delegates_.begin(), analytics_delegates_.end(), delegate) !=
            analytics_delegates_.end()) {
      return;
    }
    analytics_delegates_.push_back(delegate);
    internal::LogInfo("ForwardingPlayerListener AddAnalyticsDelegate delegatePtr=" +
                      BuildPointerSummary(delegate) + "," + BuildStateLocked());
  }
  void RemoveAnalyticsDelegate(PlayerListener* delegate) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (delegate == nullptr) {
      const bool had_delegates = !analytics_delegates_.empty();
      analytics_delegates_.clear();
      internal::LogInfo("ForwardingPlayerListener RemoveAnalyticsDelegate clearAll," +
                        BuildStateLocked());
      if (had_delegates) {
        callback_drained_.wait(lock, [this]() { return in_flight_callback_count_ == 0; });
        internal::LogInfo(
            "ForwardingPlayerListener RemoveAnalyticsDelegate clearAll drained," +
            BuildStateLocked());
      }
      return;
    }
    const size_t previous_size = analytics_delegates_.size();
    analytics_delegates_.erase(
        std::remove(analytics_delegates_.begin(), analytics_delegates_.end(), delegate),
        analytics_delegates_.end());
    if (analytics_delegates_.size() != previous_size) {
      internal::LogInfo("ForwardingPlayerListener RemoveAnalyticsDelegate delegatePtr=" +
                        BuildPointerSummary(delegate) + "," + BuildStateLocked());
      callback_drained_.wait(lock, [this]() { return in_flight_callback_count_ == 0; });
      internal::LogInfo(
          "ForwardingPlayerListener RemoveAnalyticsDelegate drained delegatePtr=" +
          BuildPointerSummary(delegate) + "," + BuildStateLocked());
    }
  }

  void AddAudioCodecParametersDelegate(
      PlayerListener* delegate,
      const std::vector<std::string>& keys) {
    AddCodecParametersDelegate(
        delegate, keys, &audio_codec_parameter_delegates_, "AddAudioCodecParametersDelegate");
  }

  void RemoveAudioCodecParametersDelegate(PlayerListener* delegate) {
    RemoveCodecParametersDelegate(
        delegate, &audio_codec_parameter_delegates_, "RemoveAudioCodecParametersDelegate");
  }

  bool HasAudioCodecParametersDelegates() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !audio_codec_parameter_delegates_.empty();
  }

  std::vector<std::string> GetAudioCodecParameterKeys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return CollectCodecParameterKeysLocked(audio_codec_parameter_delegates_);
  }

  void RouteNextAudioCodecParametersCallbackTo(PlayerListener* delegate) {
    std::lock_guard<std::mutex> lock(mutex_);
    next_audio_codec_parameter_callback_delegate_ = delegate;
    suppress_next_audio_codec_parameter_callback_ = false;
  }

  void SuppressNextAudioCodecParametersCallback() {
    std::lock_guard<std::mutex> lock(mutex_);
    next_audio_codec_parameter_callback_delegate_ = nullptr;
    suppress_next_audio_codec_parameter_callback_ = true;
  }

  void ClearNextAudioCodecParametersCallbackRouting() {
    std::lock_guard<std::mutex> lock(mutex_);
    next_audio_codec_parameter_callback_delegate_ = nullptr;
    suppress_next_audio_codec_parameter_callback_ = false;
  }

  void AddVideoCodecParametersDelegate(
      PlayerListener* delegate,
      const std::vector<std::string>& keys) {
    AddCodecParametersDelegate(
        delegate, keys, &video_codec_parameter_delegates_, "AddVideoCodecParametersDelegate");
  }

  void RemoveVideoCodecParametersDelegate(PlayerListener* delegate) {
    RemoveCodecParametersDelegate(
        delegate, &video_codec_parameter_delegates_, "RemoveVideoCodecParametersDelegate");
  }

  bool HasVideoCodecParametersDelegates() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !video_codec_parameter_delegates_.empty();
  }

  std::vector<std::string> GetVideoCodecParameterKeys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return CollectCodecParameterKeysLocked(video_codec_parameter_delegates_);
  }

  void RouteNextVideoCodecParametersCallbackTo(PlayerListener* delegate) {
    std::lock_guard<std::mutex> lock(mutex_);
    next_video_codec_parameter_callback_delegate_ = delegate;
    suppress_next_video_codec_parameter_callback_ = false;
  }

  void SuppressNextVideoCodecParametersCallback() {
    std::lock_guard<std::mutex> lock(mutex_);
    next_video_codec_parameter_callback_delegate_ = nullptr;
    suppress_next_video_codec_parameter_callback_ = true;
  }

  void ClearNextVideoCodecParametersCallbackRouting() {
    std::lock_guard<std::mutex> lock(mutex_);
    next_video_codec_parameter_callback_delegate_ = nullptr;
    suppress_next_video_codec_parameter_callback_ = false;
  }

  void SetVideoFrameMetadataDelegate(PlayerListener* delegate) {
    std::unique_lock<std::mutex> lock(mutex_);
    video_frame_metadata_delegate_ = delegate;
    if (delegate == nullptr) {
      callback_drained_.wait(lock, [this]() { return in_flight_callback_count_ == 0; });
    }
  }

  bool RemoveVideoFrameMetadataDelegate(PlayerListener* delegate) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (delegate != nullptr && video_frame_metadata_delegate_ != delegate) {
      return false;
    }
    const bool had_delegate = video_frame_metadata_delegate_ != nullptr;
    video_frame_metadata_delegate_ = nullptr;
    if (had_delegate) {
      callback_drained_.wait(lock, [this]() { return in_flight_callback_count_ == 0; });
    }
    return had_delegate;
  }

  void SetCameraMotionDelegate(PlayerListener* delegate) {
    std::unique_lock<std::mutex> lock(mutex_);
    camera_motion_delegate_ = delegate;
    if (delegate == nullptr) {
      callback_drained_.wait(lock, [this]() { return in_flight_callback_count_ == 0; });
    }
  }

  bool RemoveCameraMotionDelegate(PlayerListener* delegate) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (delegate != nullptr && camera_motion_delegate_ != delegate) {
      return false;
    }
    const bool had_delegate = camera_motion_delegate_ != nullptr;
    camera_motion_delegate_ = nullptr;
    if (had_delegate) {
      callback_drained_.wait(lock, [this]() { return in_flight_callback_count_ == 0; });
    }
    return had_delegate;
  }

  void OnPlaybackStateChanged(const PlaybackSnapshot& snapshot) override {
    NotifyDelegate([&](PlayerListener* delegate) { delegate->OnPlaybackStateChanged(snapshot); });
  }

  void OnPlayWhenReadyChanged(const PlaybackSnapshot& snapshot, int reason) override {
    NotifyDelegate(
        [&](PlayerListener* delegate) { delegate->OnPlayWhenReadyChanged(snapshot, reason); });
  }

  void OnIsPlayingChanged(const PlaybackSnapshot& snapshot) override {
    NotifyDelegate([&](PlayerListener* delegate) { delegate->OnIsPlayingChanged(snapshot); });
  }

  void OnIsLoadingChanged(const PlaybackSnapshot& snapshot) override {
    NotifyDelegate([&](PlayerListener* delegate) { delegate->OnIsLoadingChanged(snapshot); });
  }

  void OnMediaItemTransition(const PlaybackSnapshot& snapshot, int reason) override {
    NotifyDelegate(
        [&](PlayerListener* delegate) { delegate->OnMediaItemTransition(snapshot, reason); });
  }

  void OnPlayerError(const PlaybackSnapshot& snapshot) override {
    NotifyDelegate([&](PlayerListener* delegate) { delegate->OnPlayerError(snapshot); });
  }

  void OnPlayerErrorChanged(const PlaybackSnapshot& snapshot) override {
    NotifyDelegate([&](PlayerListener* delegate) { delegate->OnPlayerErrorChanged(snapshot); });
  }

  void OnTimelineChanged(
      const PlaybackSnapshot& snapshot,
      const TimelineDetailsSnapshot& timeline,
      int reason) override {
    NotifyDelegate("OnTimelineChanged", [&](PlayerListener* delegate) {
      delegate->OnTimelineChanged(snapshot, timeline, reason);
    });
  }

  void OnTracksChanged(
      const PlaybackSnapshot& snapshot,
      const TracksSnapshot& tracks) override {
    NotifyDelegate("OnTracksChanged",
                   [&](PlayerListener* delegate) { delegate->OnTracksChanged(snapshot, tracks); });
  }

  void OnPositionDiscontinuity(
      const PlaybackSnapshot& snapshot,
      const PositionInfoSnapshot& old_position,
      const PositionInfoSnapshot& new_position,
      int reason) override {
    NotifyDelegate("OnPositionDiscontinuity", [&](PlayerListener* delegate) {
      delegate->OnPositionDiscontinuity(snapshot, old_position, new_position, reason);
    });
  }

  void OnAudioAttributesChanged(
      const PlaybackSnapshot& snapshot,
      const AudioAttributesDescriptor& attributes) override {
    NotifyDelegate([&](PlayerListener* delegate) {
      delegate->OnAudioAttributesChanged(snapshot, attributes);
    });
  }

  void OnCues(const PlaybackSnapshot& snapshot, const CueSnapshot& cues) override {
    NotifyDelegate("OnCues",
                   [&](PlayerListener* delegate) { delegate->OnCues(snapshot, cues); });
  }

  void OnRepeatModeChanged(const PlaybackSnapshot& snapshot) override {
    NotifyDelegate(
        "OnRepeatModeChanged",
        [&](PlayerListener* delegate) { delegate->OnRepeatModeChanged(snapshot); });
  }

  void OnShuffleModeEnabledChanged(const PlaybackSnapshot& snapshot) override {
    NotifyDelegate("OnShuffleModeEnabledChanged", [&](PlayerListener* delegate) {
      delegate->OnShuffleModeEnabledChanged(snapshot);
    });
  }

  void OnSeekBackIncrementChanged(
      const PlaybackSnapshot& snapshot,
      int64_t seek_back_increment_ms) override {
    NotifyDelegate("OnSeekBackIncrementChanged", [&](PlayerListener* delegate) {
      delegate->OnSeekBackIncrementChanged(snapshot, seek_back_increment_ms);
    });
  }

  void OnSeekForwardIncrementChanged(
      const PlaybackSnapshot& snapshot,
      int64_t seek_forward_increment_ms) override {
    NotifyDelegate("OnSeekForwardIncrementChanged", [&](PlayerListener* delegate) {
      delegate->OnSeekForwardIncrementChanged(snapshot, seek_forward_increment_ms);
    });
  }

  void OnMaxSeekToPreviousPositionChanged(
      const PlaybackSnapshot& snapshot,
      int64_t max_seek_to_previous_position_ms) override {
    NotifyDelegate("OnMaxSeekToPreviousPositionChanged", [&](PlayerListener* delegate) {
      delegate->OnMaxSeekToPreviousPositionChanged(
          snapshot, max_seek_to_previous_position_ms);
    });
  }

  void OnTrackSelectionParametersChanged(
      const PlaybackSnapshot& snapshot,
      const TrackSelectionParametersDescriptor& parameters) override {
    NotifyDelegate("OnTrackSelectionParametersChanged", [&](PlayerListener* delegate) {
      delegate->OnTrackSelectionParametersChanged(snapshot, parameters);
    });
  }

  void OnPlaybackParametersChanged(
      const PlaybackSnapshot& snapshot,
      const PlaybackParametersSnapshot& parameters) override {
    NotifyDelegate("OnPlaybackParametersChanged", [&](PlayerListener* delegate) {
      delegate->OnPlaybackParametersChanged(snapshot, parameters);
    });
  }

  void OnPlaybackSuppressionReasonChanged(
      const PlaybackSnapshot& snapshot,
      PlaybackSuppressionReason suppression_reason) override {
    NotifyDelegate("OnPlaybackSuppressionReasonChanged", [&](PlayerListener* delegate) {
      delegate->OnPlaybackSuppressionReasonChanged(snapshot, suppression_reason);
    });
  }

  void OnAvailableCommandsChanged(
      const PlaybackSnapshot& snapshot,
      const AvailableCommandsSnapshot& commands) override {
    NotifyDelegate("OnAvailableCommandsChanged", [&](PlayerListener* delegate) {
      delegate->OnAvailableCommandsChanged(snapshot, commands);
    });
  }

  void OnEvents(
      const PlaybackSnapshot& snapshot,
      const PlayerEventsSnapshot& events) override {
    NotifyDelegate(
        "OnEvents", [&](PlayerListener* delegate) { delegate->OnEvents(snapshot, events); });
  }

  void OnDeviceInfoChanged(
      const PlaybackSnapshot& snapshot,
      const DeviceInfoDescriptor& device_info) override {
    NotifyDelegate([&](PlayerListener* delegate) {
      delegate->OnDeviceInfoChanged(snapshot, device_info);
    });
  }

  void OnDeviceVolumeChanged(
      const PlaybackSnapshot& snapshot,
      int device_volume,
      bool muted) override {
    NotifyDelegate([&](PlayerListener* delegate) {
      delegate->OnDeviceVolumeChanged(snapshot, device_volume, muted);
    });
  }

  void OnSkipSilenceEnabledChanged(
      const PlaybackSnapshot& snapshot,
      bool skip_silence_enabled) override {
    NotifyDelegate([&](PlayerListener* delegate) {
      delegate->OnSkipSilenceEnabledChanged(snapshot, skip_silence_enabled);
    });
  }

  void OnVideoSizeChanged(
      const PlaybackSnapshot& snapshot,
      const VideoSizeSnapshot& video_size) override {
    NotifyDelegate([&](PlayerListener* delegate) {
      delegate->OnVideoSizeChanged(snapshot, video_size);
    });
  }

  void OnSurfaceSizeChanged(
      const PlaybackSnapshot& snapshot,
      int width,
      int height) override {
    NotifyDelegate([&](PlayerListener* delegate) {
      delegate->OnSurfaceSizeChanged(snapshot, width, height);
    });
  }

  void OnRenderedFirstFrame(const PlaybackSnapshot& snapshot) override {
    NotifyDelegate([&](PlayerListener* delegate) { delegate->OnRenderedFirstFrame(snapshot); });
  }

  void OnMediaMetadataChanged(
      const PlaybackSnapshot& snapshot,
      const MediaMetadataSnapshot& metadata) override {
    NotifyDelegate("OnMediaMetadataChanged", [&](PlayerListener* delegate) {
      delegate->OnMediaMetadataChanged(snapshot, metadata);
    });
  }

  void OnPlaylistMetadataChanged(
      const PlaybackSnapshot& snapshot,
      const MediaMetadataSnapshot& metadata) override {
    NotifyDelegate("OnPlaylistMetadataChanged", [&](PlayerListener* delegate) {
      delegate->OnPlaylistMetadataChanged(snapshot, metadata);
    });
  }

  void OnAnalyticsUpdated(
      const PlaybackSnapshot& snapshot,
      const AnalyticsSnapshot& analytics) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsUpdated(snapshot, analytics);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsUpdated(snapshot, analytics);
      }
    }
  }

  void OnAudioUnderrun(
      const PlaybackSnapshot& snapshot,
      const AudioUnderrunEvent& audio_underrun) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAudioUnderrun(snapshot, audio_underrun);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAudioUnderrun(snapshot, audio_underrun);
      }
    }
  }

  void OnDroppedVideoFrames(
      const PlaybackSnapshot& snapshot,
      const DroppedVideoFramesEvent& dropped_video_frames) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnDroppedVideoFrames(snapshot, dropped_video_frames);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnDroppedVideoFrames(snapshot, dropped_video_frames);
      }
    }
  }

  void OnBandwidthEstimate(
      const PlaybackSnapshot& snapshot,
      const BandwidthEstimateEvent& bandwidth_estimate) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnBandwidthEstimate(snapshot, bandwidth_estimate);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnBandwidthEstimate(snapshot, bandwidth_estimate);
      }
    }
  }

  void OnLoadStarted(
      const PlaybackSnapshot& snapshot,
      const LoadStartedEvent& load_started) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnLoadStarted(snapshot, load_started);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnLoadStarted(snapshot, load_started);
      }
    }
  }

  void OnLoadCompleted(
      const PlaybackSnapshot& snapshot,
      const LoadCompletedEvent& load_completed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnLoadCompleted(snapshot, load_completed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnLoadCompleted(snapshot, load_completed);
      }
    }
  }

  void OnAnalyticsLoadError(
      const PlaybackSnapshot& snapshot,
      const AnalyticsLoadErrorEvent& load_error) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsLoadError(snapshot, load_error);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsLoadError(snapshot, load_error);
      }
    }
  }

  void OnAudioInputFormatChanged(
      const PlaybackSnapshot& snapshot,
      const AudioInputFormatChangedEvent& audio_input_format_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAudioInputFormatChanged(snapshot, audio_input_format_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAudioInputFormatChanged(snapshot, audio_input_format_changed);
      }
    }
  }

  void OnAudioDecoderInitialized(
      const PlaybackSnapshot& snapshot,
      const AudioDecoderInitializedEvent& audio_decoder_initialized) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAudioDecoderInitialized(snapshot, audio_decoder_initialized);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAudioDecoderInitialized(snapshot, audio_decoder_initialized);
      }
    }
  }

  void OnVideoDecoderInitialized(
      const PlaybackSnapshot& snapshot,
      const VideoDecoderInitializedEvent& video_decoder_initialized) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnVideoDecoderInitialized(snapshot, video_decoder_initialized);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnVideoDecoderInitialized(snapshot, video_decoder_initialized);
      }
    }
  }

  void OnAudioDecoderReleased(
      const PlaybackSnapshot& snapshot,
      const AudioDecoderReleasedEvent& audio_decoder_released) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAudioDecoderReleased(snapshot, audio_decoder_released);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAudioDecoderReleased(snapshot, audio_decoder_released);
      }
    }
  }

  void OnVideoDecoderReleased(
      const PlaybackSnapshot& snapshot,
      const VideoDecoderReleasedEvent& video_decoder_released) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnVideoDecoderReleased(snapshot, video_decoder_released);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnVideoDecoderReleased(snapshot, video_decoder_released);
      }
    }
  }

  void OnAnalyticsRenderedFirstFrame(
      const PlaybackSnapshot& snapshot,
      const AnalyticsRenderedFirstFrameEvent& rendered_first_frame) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsRenderedFirstFrame(snapshot, rendered_first_frame);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsRenderedFirstFrame(snapshot, rendered_first_frame);
      }
    }
  }

  void OnAnalyticsVideoSizeChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsVideoSizeChangedEvent& analytics_video_size) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsVideoSizeChanged(snapshot, analytics_video_size);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsVideoSizeChanged(snapshot, analytics_video_size);
      }
    }
  }

  void OnAudioPositionAdvancing(
      const PlaybackSnapshot& snapshot,
      const AudioPositionAdvancingEvent& audio_position_advancing) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAudioPositionAdvancing(snapshot, audio_position_advancing);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAudioPositionAdvancing(snapshot, audio_position_advancing);
      }
    }
  }

  void OnVideoFrameProcessingOffset(
      const PlaybackSnapshot& snapshot,
      const VideoFrameProcessingOffsetEvent& video_frame_processing_offset) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnVideoFrameProcessingOffset(
          snapshot, video_frame_processing_offset);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnVideoFrameProcessingOffset(
            snapshot, video_frame_processing_offset);
      }
    }
  }

  void OnVolumeChanged(
      const PlaybackSnapshot& snapshot,
      const VolumeChangedEvent& volume_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnVolumeChanged(snapshot, volume_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnVolumeChanged(snapshot, volume_changed);
      }
    }
  }

  void OnAudioSessionIdChanged(
      const PlaybackSnapshot& snapshot,
      const AudioSessionIdChangedEvent& audio_session_id_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAudioSessionIdChanged(snapshot, audio_session_id_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAudioSessionIdChanged(snapshot, audio_session_id_changed);
      }
    }
  }

  void OnAnalyticsSkipSilenceEnabledChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsSkipSilenceEnabledChangedEvent& skip_silence_enabled_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsSkipSilenceEnabledChanged(
          snapshot, skip_silence_enabled_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsSkipSilenceEnabledChanged(
            snapshot, skip_silence_enabled_changed);
      }
    }
  }

  void OnAnalyticsDeviceVolumeChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsDeviceVolumeChangedEvent& device_volume_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsDeviceVolumeChanged(snapshot, device_volume_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsDeviceVolumeChanged(snapshot, device_volume_changed);
      }
    }
  }

  void OnAnalyticsPlaybackStateChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsPlaybackStateChangedEvent& playback_state_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsPlaybackStateChanged(snapshot, playback_state_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsPlaybackStateChanged(snapshot, playback_state_changed);
      }
    }
  }

  void OnAnalyticsIsPlayingChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsIsPlayingChangedEvent& is_playing_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsIsPlayingChanged(snapshot, is_playing_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsIsPlayingChanged(snapshot, is_playing_changed);
      }
    }
  }

  void OnAnalyticsPlayWhenReadyChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsPlayWhenReadyChangedEvent& play_when_ready_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsPlayWhenReadyChanged(snapshot, play_when_ready_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsPlayWhenReadyChanged(snapshot, play_when_ready_changed);
      }
    }
  }

  void OnAnalyticsPlaybackSuppressionReasonChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsPlaybackSuppressionReasonChangedEvent& suppression_reason_changed)
      override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsPlaybackSuppressionReasonChanged(
          snapshot, suppression_reason_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsPlaybackSuppressionReasonChanged(
            snapshot, suppression_reason_changed);
      }
    }
  }

  void OnAnalyticsIsLoadingChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsIsLoadingChangedEvent& is_loading_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsIsLoadingChanged(snapshot, is_loading_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsIsLoadingChanged(snapshot, is_loading_changed);
      }
    }
  }

  void OnAnalyticsRepeatModeChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsRepeatModeChangedEvent& repeat_mode_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsRepeatModeChanged(snapshot, repeat_mode_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsRepeatModeChanged(snapshot, repeat_mode_changed);
      }
    }
  }

  void OnAnalyticsShuffleModeChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsShuffleModeChangedEvent& shuffle_mode_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsShuffleModeChanged(snapshot, shuffle_mode_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsShuffleModeChanged(snapshot, shuffle_mode_changed);
      }
    }
  }

  void OnAnalyticsPlaybackParametersChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsPlaybackParametersChangedEvent& playback_parameters_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsPlaybackParametersChanged(snapshot, playback_parameters_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsPlaybackParametersChanged(
            snapshot, playback_parameters_changed);
      }
    }
  }

  void OnAnalyticsAvailableCommandsChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsAvailableCommandsChangedEvent& available_commands_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsAvailableCommandsChanged(snapshot, available_commands_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsAvailableCommandsChanged(
            snapshot, available_commands_changed);
      }
    }
  }

  void OnAnalyticsEvents(
      const PlaybackSnapshot& snapshot,
      const AnalyticsEventsEvent& analytics_events) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsEvents(snapshot, analytics_events);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsEvents(snapshot, analytics_events);
      }
    }
  }

  void OnAnalyticsSeekBackIncrementChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsSeekBackIncrementChangedEvent& seek_back_increment_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsSeekBackIncrementChanged(
          snapshot, seek_back_increment_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsSeekBackIncrementChanged(
            snapshot, seek_back_increment_changed);
      }
    }
  }

  void OnAnalyticsSeekForwardIncrementChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsSeekForwardIncrementChangedEvent& seek_forward_increment_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsSeekForwardIncrementChanged(
          snapshot, seek_forward_increment_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsSeekForwardIncrementChanged(
            snapshot, seek_forward_increment_changed);
      }
    }
  }

  void OnAnalyticsMaxSeekToPreviousPositionChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsMaxSeekToPreviousPositionChangedEvent&
          max_seek_to_previous_position_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsMaxSeekToPreviousPositionChanged(
          snapshot, max_seek_to_previous_position_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsMaxSeekToPreviousPositionChanged(
            snapshot, max_seek_to_previous_position_changed);
      }
    }
  }

  void OnAnalyticsTimelineChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsTimelineChangedEvent& timeline_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsTimelineChanged(snapshot, timeline_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsTimelineChanged(snapshot, timeline_changed);
      }
    }
  }

  void OnAnalyticsPositionDiscontinuity(
      const PlaybackSnapshot& snapshot,
      const AnalyticsPositionDiscontinuityEvent& position_discontinuity) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsPositionDiscontinuity(snapshot, position_discontinuity);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsPositionDiscontinuity(snapshot, position_discontinuity);
      }
    }
  }

  void OnAnalyticsSeekStarted(
      const PlaybackSnapshot& snapshot,
      const AnalyticsSeekStartedEvent& seek_started) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsSeekStarted(snapshot, seek_started);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsSeekStarted(snapshot, seek_started);
      }
    }
  }

  void OnAnalyticsPlayerError(
      const PlaybackSnapshot& snapshot,
      const PlayerError& error) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsPlayerError(snapshot, error);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsPlayerError(snapshot, error);
      }
    }
  }

  void OnAnalyticsPlayerErrorChanged(
      const PlaybackSnapshot& snapshot,
      const PlayerError& error) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsPlayerErrorChanged(snapshot, error);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsPlayerErrorChanged(snapshot, error);
      }
    }
  }

  void OnAnalyticsTracksChanged(
      const PlaybackSnapshot& snapshot,
      const TracksSnapshot& tracks) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsTracksChanged(snapshot, tracks);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsTracksChanged(snapshot, tracks);
      }
    }
  }

  void OnAnalyticsMediaItemTransition(
      const PlaybackSnapshot& snapshot,
      const AnalyticsMediaItemTransitionEvent& media_item_transition) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsMediaItemTransition(snapshot, media_item_transition);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsMediaItemTransition(snapshot, media_item_transition);
      }
    }
  }

  void OnAnalyticsCues(
      const PlaybackSnapshot& snapshot,
      const CueSnapshot& cues) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsCues(snapshot, cues);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsCues(snapshot, cues);
      }
    }
  }

  void OnAnalyticsMetadata(
      const PlaybackSnapshot& snapshot,
      const AnalyticsMetadataEvent& metadata) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsMetadata(snapshot, metadata);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsMetadata(snapshot, metadata);
      }
    }
  }

  void OnAnalyticsDeviceInfoChanged(
      const PlaybackSnapshot& snapshot,
      const DeviceInfoDescriptor& device_info) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsDeviceInfoChanged(snapshot, device_info);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsDeviceInfoChanged(snapshot, device_info);
      }
    }
  }

  void OnAnalyticsMediaMetadataChanged(
      const PlaybackSnapshot& snapshot,
      const MediaMetadataSnapshot& metadata) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsMediaMetadataChanged(snapshot, metadata);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsMediaMetadataChanged(snapshot, metadata);
      }
    }
  }

  void OnAnalyticsPlaylistMetadataChanged(
      const PlaybackSnapshot& snapshot,
      const MediaMetadataSnapshot& metadata) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnAnalyticsPlaylistMetadataChanged(snapshot, metadata);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnAnalyticsPlaylistMetadataChanged(snapshot, metadata);
      }
    }
  }

  void OnVideoInputFormatChanged(
      const PlaybackSnapshot& snapshot,
      const VideoInputFormatChangedEvent& video_input_format_changed) override {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      listeners.delegate->OnVideoInputFormatChanged(snapshot, video_input_format_changed);
    }
    for (PlayerListener* analytics_delegate : listeners.analytics_delegates) {
      if (analytics_delegate != nullptr && analytics_delegate != listeners.delegate) {
        analytics_delegate->OnVideoInputFormatChanged(snapshot, video_input_format_changed);
      }
    }
  }

  void OnAudioCodecParametersChanged(
      const PlaybackSnapshot& snapshot,
      const CodecParametersDescriptor& codec_parameters) override {
    auto listeners =
        SnapshotListeners(nullptr, ListenerSnapshotKind::kAudioCodecParameters);
    if (listeners.suppress_audio_codec_parameter_callback) {
      return;
    }
    for (const CodecParametersDelegateRegistration& registration :
         listeners.audio_codec_parameter_delegates) {
      if (listeners.audio_codec_parameter_callback_delegate != nullptr &&
          registration.delegate != listeners.audio_codec_parameter_callback_delegate) {
        continue;
      }
      if (registration.delegate != nullptr) {
        registration.delegate->OnAudioCodecParametersChanged(
            snapshot, FilterCodecParametersByKeys(codec_parameters, registration.keys));
      }
    }
  }

  void OnVideoCodecParametersChanged(
      const PlaybackSnapshot& snapshot,
      const CodecParametersDescriptor& codec_parameters) override {
    auto listeners =
        SnapshotListeners(nullptr, ListenerSnapshotKind::kVideoCodecParameters);
    if (listeners.suppress_video_codec_parameter_callback) {
      return;
    }
    for (const CodecParametersDelegateRegistration& registration :
         listeners.video_codec_parameter_delegates) {
      if (listeners.video_codec_parameter_callback_delegate != nullptr &&
          registration.delegate != listeners.video_codec_parameter_callback_delegate) {
        continue;
      }
      if (registration.delegate != nullptr) {
        registration.delegate->OnVideoCodecParametersChanged(
            snapshot, FilterCodecParametersByKeys(codec_parameters, registration.keys));
      }
    }
  }

  void OnVideoFrameAboutToBeRendered(
      const PlaybackSnapshot& snapshot,
      const VideoFrameMetadataSnapshot& video_frame_metadata) override {
    auto listeners = SnapshotListeners();
    if (listeners.video_frame_metadata_delegate != nullptr) {
      listeners.video_frame_metadata_delegate->OnVideoFrameAboutToBeRendered(
          snapshot, video_frame_metadata);
    }
  }

  void OnCameraMotion(
      const PlaybackSnapshot& snapshot,
      const CameraMotionSnapshot& camera_motion) override {
    auto listeners = SnapshotListeners();
    if (listeners.camera_motion_delegate != nullptr) {
      listeners.camera_motion_delegate->OnCameraMotion(snapshot, camera_motion);
    }
  }

  void OnCameraMotionReset(const PlaybackSnapshot& snapshot) override {
    auto listeners = SnapshotListeners();
    if (listeners.camera_motion_delegate != nullptr) {
      listeners.camera_motion_delegate->OnCameraMotionReset(snapshot);
    }
  }

 private:
  enum class ListenerSnapshotKind {
    kNormal,
    kAudioCodecParameters,
    kVideoCodecParameters,
  };

  struct ListenerSnapshot {
    ListenerSnapshot() = default;
    ListenerSnapshot(
        ForwardingPlayerListener* owner_in,
        PlayerListener* delegate_in,
        std::vector<PlayerListener*> analytics_delegates_in,
        std::vector<CodecParametersDelegateRegistration>
            audio_codec_parameter_delegates_in,
        std::vector<CodecParametersDelegateRegistration>
            video_codec_parameter_delegates_in,
        PlayerListener* video_frame_metadata_delegate_in,
        PlayerListener* camera_motion_delegate_in,
        PlayerListener* audio_codec_parameter_callback_delegate_in,
        bool suppress_audio_codec_parameter_callback_in,
        PlayerListener* video_codec_parameter_callback_delegate_in,
        bool suppress_video_codec_parameter_callback_in)
        : owner(owner_in),
          delegate(delegate_in),
          analytics_delegates(std::move(analytics_delegates_in)),
          audio_codec_parameter_delegates(
              std::move(audio_codec_parameter_delegates_in)),
          video_codec_parameter_delegates(
              std::move(video_codec_parameter_delegates_in)),
          video_frame_metadata_delegate(video_frame_metadata_delegate_in),
          camera_motion_delegate(camera_motion_delegate_in),
          audio_codec_parameter_callback_delegate(
              audio_codec_parameter_callback_delegate_in),
          suppress_audio_codec_parameter_callback(
              suppress_audio_codec_parameter_callback_in),
          video_codec_parameter_callback_delegate(
              video_codec_parameter_callback_delegate_in),
          suppress_video_codec_parameter_callback(
              suppress_video_codec_parameter_callback_in) {}

    ListenerSnapshot(const ListenerSnapshot&) = delete;
    ListenerSnapshot& operator=(const ListenerSnapshot&) = delete;

    ListenerSnapshot(ListenerSnapshot&& other) noexcept
        : owner(other.owner),
          delegate(other.delegate),
          analytics_delegates(std::move(other.analytics_delegates)),
          audio_codec_parameter_delegates(
              std::move(other.audio_codec_parameter_delegates)),
          video_codec_parameter_delegates(
              std::move(other.video_codec_parameter_delegates)),
          video_frame_metadata_delegate(other.video_frame_metadata_delegate),
          camera_motion_delegate(other.camera_motion_delegate),
          audio_codec_parameter_callback_delegate(
              other.audio_codec_parameter_callback_delegate),
          suppress_audio_codec_parameter_callback(
              other.suppress_audio_codec_parameter_callback),
          video_codec_parameter_callback_delegate(
              other.video_codec_parameter_callback_delegate),
          suppress_video_codec_parameter_callback(
              other.suppress_video_codec_parameter_callback) {
      other.owner = nullptr;
      other.delegate = nullptr;
      other.video_frame_metadata_delegate = nullptr;
      other.camera_motion_delegate = nullptr;
      other.audio_codec_parameter_callback_delegate = nullptr;
      other.suppress_audio_codec_parameter_callback = false;
      other.video_codec_parameter_callback_delegate = nullptr;
      other.suppress_video_codec_parameter_callback = false;
    }

    ListenerSnapshot& operator=(ListenerSnapshot&& other) noexcept {
      if (this != &other) {
        Release();
        owner = other.owner;
        delegate = other.delegate;
        analytics_delegates = std::move(other.analytics_delegates);
        audio_codec_parameter_delegates =
            std::move(other.audio_codec_parameter_delegates);
        video_codec_parameter_delegates =
            std::move(other.video_codec_parameter_delegates);
        video_frame_metadata_delegate = other.video_frame_metadata_delegate;
        camera_motion_delegate = other.camera_motion_delegate;
        audio_codec_parameter_callback_delegate =
            other.audio_codec_parameter_callback_delegate;
        suppress_audio_codec_parameter_callback =
            other.suppress_audio_codec_parameter_callback;
        video_codec_parameter_callback_delegate =
            other.video_codec_parameter_callback_delegate;
        suppress_video_codec_parameter_callback =
            other.suppress_video_codec_parameter_callback;
        other.owner = nullptr;
        other.delegate = nullptr;
        other.video_frame_metadata_delegate = nullptr;
        other.camera_motion_delegate = nullptr;
        other.audio_codec_parameter_callback_delegate = nullptr;
        other.suppress_audio_codec_parameter_callback = false;
        other.video_codec_parameter_callback_delegate = nullptr;
        other.suppress_video_codec_parameter_callback = false;
      }
      return *this;
    }

    ~ListenerSnapshot() { Release(); }

    explicit operator bool() const {
      return delegate != nullptr || !analytics_delegates.empty() ||
          !audio_codec_parameter_delegates.empty() ||
          !video_codec_parameter_delegates.empty() ||
          video_frame_metadata_delegate != nullptr ||
          camera_motion_delegate != nullptr ||
          audio_codec_parameter_callback_delegate != nullptr ||
          suppress_audio_codec_parameter_callback ||
          video_codec_parameter_callback_delegate != nullptr ||
          suppress_video_codec_parameter_callback;
    }

    void Release() {
      if (owner != nullptr) {
        owner->OnSnapshotReleased();
        owner = nullptr;
      }
    }

    ForwardingPlayerListener* owner = nullptr;
    PlayerListener* delegate = nullptr;
    std::vector<PlayerListener*> analytics_delegates;
    std::vector<CodecParametersDelegateRegistration> audio_codec_parameter_delegates;
    std::vector<CodecParametersDelegateRegistration> video_codec_parameter_delegates;
    PlayerListener* video_frame_metadata_delegate = nullptr;
    PlayerListener* camera_motion_delegate = nullptr;
    PlayerListener* audio_codec_parameter_callback_delegate = nullptr;
    bool suppress_audio_codec_parameter_callback = false;
    PlayerListener* video_codec_parameter_callback_delegate = nullptr;
    bool suppress_video_codec_parameter_callback = false;
  };

  static std::vector<std::string> CollectCodecParameterKeysLocked(
      const std::vector<CodecParametersDelegateRegistration>& registrations) {
    std::vector<std::string> keys;
    for (const CodecParametersDelegateRegistration& registration : registrations) {
      for (const std::string& key : registration.keys) {
        if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
          keys.push_back(key);
        }
      }
    }
    return keys;
  }

  void AddCodecParametersDelegate(
      PlayerListener* delegate,
      const std::vector<std::string>& keys,
      std::vector<CodecParametersDelegateRegistration>* registrations,
      const char* log_name) {
    if (delegate == nullptr || registrations == nullptr) {
      return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(
        registrations->begin(),
        registrations->end(),
        [delegate](const CodecParametersDelegateRegistration& registration) {
          return registration.delegate == delegate;
        });
    if (it != registrations->end()) {
      it->keys = keys;
    } else {
      registrations->push_back({delegate, keys});
    }
    internal::LogInfo(
        std::string("ForwardingPlayerListener ") + log_name +
        " delegatePtr=" + BuildPointerSummary(delegate) + "," + BuildStateLocked());
  }

  void RemoveCodecParametersDelegate(
      PlayerListener* delegate,
      std::vector<CodecParametersDelegateRegistration>* registrations,
      const char* log_name) {
    if (registrations == nullptr) {
      return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    bool removed = false;
    if (delegate == nullptr) {
      removed = !registrations->empty();
      registrations->clear();
    } else {
      const size_t previous_size = registrations->size();
      registrations->erase(
          std::remove_if(
              registrations->begin(),
              registrations->end(),
              [delegate](const CodecParametersDelegateRegistration& registration) {
                return registration.delegate == delegate;
              }),
          registrations->end());
      removed = registrations->size() != previous_size;
    }
    if (removed) {
      if (registrations == &audio_codec_parameter_delegates_) {
        next_audio_codec_parameter_callback_delegate_ = nullptr;
        suppress_next_audio_codec_parameter_callback_ = false;
      } else if (registrations == &video_codec_parameter_delegates_) {
        next_video_codec_parameter_callback_delegate_ = nullptr;
        suppress_next_video_codec_parameter_callback_ = false;
      }
      internal::LogInfo(
          std::string("ForwardingPlayerListener ") + log_name +
          " delegatePtr=" + BuildPointerSummary(delegate) + "," + BuildStateLocked());
      callback_drained_.wait(lock, [this]() { return in_flight_callback_count_ == 0; });
    }
  }

  std::string BuildStateLocked() const {
    return "delegatePtr=" + BuildPointerSummary(delegate_) +
        ",analyticsCount=" + std::to_string(analytics_delegates_.size()) +
        ",audioCodecParamCount=" +
        std::to_string(audio_codec_parameter_delegates_.size()) +
        ",videoCodecParamCount=" +
        std::to_string(video_codec_parameter_delegates_.size()) +
        ",videoFrameMetadataPtr=" + BuildPointerSummary(video_frame_metadata_delegate_) +
        ",cameraMotionPtr=" + BuildPointerSummary(camera_motion_delegate_) +
        ",inFlight=" + std::to_string(in_flight_callback_count_);
  }

  ListenerSnapshot SnapshotListeners(
      const char* callback_name = nullptr,
      ListenerSnapshotKind snapshot_kind = ListenerSnapshotKind::kNormal) {
    std::lock_guard<std::mutex> lock(mutex_);
    PlayerListener* audio_codec_parameter_callback_delegate = nullptr;
    bool suppress_audio_codec_parameter_callback = false;
    PlayerListener* video_codec_parameter_callback_delegate = nullptr;
    bool suppress_video_codec_parameter_callback = false;
    if (snapshot_kind == ListenerSnapshotKind::kAudioCodecParameters) {
      audio_codec_parameter_callback_delegate =
          next_audio_codec_parameter_callback_delegate_;
      suppress_audio_codec_parameter_callback =
          suppress_next_audio_codec_parameter_callback_;
      next_audio_codec_parameter_callback_delegate_ = nullptr;
      suppress_next_audio_codec_parameter_callback_ = false;
    } else if (snapshot_kind == ListenerSnapshotKind::kVideoCodecParameters) {
      video_codec_parameter_callback_delegate =
          next_video_codec_parameter_callback_delegate_;
      suppress_video_codec_parameter_callback =
          suppress_next_video_codec_parameter_callback_;
      next_video_codec_parameter_callback_delegate_ = nullptr;
      suppress_next_video_codec_parameter_callback_ = false;
    }
    if (ShouldTraceSmokeListenerCallback(callback_name)) {
      internal::LogInfo(
          std::string("ForwardingPlayerListener SnapshotListeners callback=") +
          callback_name + "," + BuildStateLocked());
    }
    if (delegate_ == nullptr && analytics_delegates_.empty() &&
        audio_codec_parameter_delegates_.empty() &&
        video_codec_parameter_delegates_.empty() &&
        video_frame_metadata_delegate_ == nullptr && camera_motion_delegate_ == nullptr &&
        audio_codec_parameter_callback_delegate == nullptr &&
        !suppress_audio_codec_parameter_callback &&
        video_codec_parameter_callback_delegate == nullptr &&
        !suppress_video_codec_parameter_callback) {
      return ListenerSnapshot();
    }
    ++in_flight_callback_count_;
    if (ShouldTraceSmokeListenerCallback(callback_name)) {
      internal::LogInfo(
          std::string("ForwardingPlayerListener SnapshotListeners acquired callback=") +
          callback_name + "," + BuildStateLocked());
    }
    return ListenerSnapshot(
        this,
        delegate_,
        analytics_delegates_,
        audio_codec_parameter_delegates_,
        video_codec_parameter_delegates_,
        video_frame_metadata_delegate_,
        camera_motion_delegate_,
        audio_codec_parameter_callback_delegate,
        suppress_audio_codec_parameter_callback,
        video_codec_parameter_callback_delegate,
        suppress_video_codec_parameter_callback);
  }

  template <typename Fn>
  void NotifyDelegate(Fn&& fn) {
    auto listeners = SnapshotListeners();
    if (listeners.delegate != nullptr) {
      fn(listeners.delegate);
    }
  }

  template <typename Fn>
  void NotifyDelegate(const char* callback_name, Fn&& fn) {
    auto listeners = SnapshotListeners(callback_name);
    if (ShouldTraceSmokeListenerCallback(callback_name)) {
      internal::LogInfo(
          std::string("ForwardingPlayerListener NotifyDelegate callback=") +
          callback_name + ",delegatePtr=" + BuildPointerSummary(listeners.delegate) +
          ",analyticsCount=" + std::to_string(listeners.analytics_delegates.size()) +
          ",phase=" + (listeners.delegate != nullptr ? "begin" : "skip"));
    }
    if (listeners.delegate != nullptr) {
      fn(listeners.delegate);
      if (ShouldTraceSmokeListenerCallback(callback_name)) {
        internal::LogInfo(
            std::string("ForwardingPlayerListener NotifyDelegate callback=") +
            callback_name + ",delegatePtr=" + BuildPointerSummary(listeners.delegate) +
            ",analyticsCount=" + std::to_string(listeners.analytics_delegates.size()) +
            ",phase=end");
      }
    }
  }

  void OnSnapshotReleased() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (--in_flight_callback_count_ == 0) {
      callback_drained_.notify_all();
    }
  }

  mutable std::mutex mutex_;
  std::condition_variable callback_drained_;
  int in_flight_callback_count_ = 0;
  PlayerListener* delegate_ = nullptr;
  std::vector<PlayerListener*> analytics_delegates_;
  std::vector<CodecParametersDelegateRegistration> audio_codec_parameter_delegates_;
  std::vector<CodecParametersDelegateRegistration> video_codec_parameter_delegates_;
  PlayerListener* video_frame_metadata_delegate_ = nullptr;
  PlayerListener* camera_motion_delegate_ = nullptr;
  PlayerListener* next_audio_codec_parameter_callback_delegate_ = nullptr;
  bool suppress_next_audio_codec_parameter_callback_ = false;
  PlayerListener* next_video_codec_parameter_callback_delegate_ = nullptr;
  bool suppress_next_video_codec_parameter_callback_ = false;
};

class ForwardingImageOutputListener : public ImageOutputListener {
 public:
  void SetDelegate(ExoPlayerSdkImageOutputListener* delegate) {
    std::unique_lock<std::mutex> lock(mutex_);
    delegate_ = delegate;
    if (delegate == nullptr) {
      callback_drained_.wait(lock, [this]() { return in_flight_callback_count_ == 0; });
    }
  }
  ExoPlayerSdkImageOutputListener* delegate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return delegate_;
  }

  void OnImageAvailable(const ImageFrameSnapshot& image_frame) override {
    NotifyDelegate([&](ExoPlayerSdkImageOutputListener* delegate) {
      delegate->OnImageAvailable(image_frame);
    });
  }

  void OnDisabled() override {
    NotifyDelegate([](ExoPlayerSdkImageOutputListener* delegate) { delegate->OnDisabled(); });
  }

 private:
  class ScopedDelegateCall {
   public:
    ScopedDelegateCall() = default;
    ScopedDelegateCall(
        ForwardingImageOutputListener* owner_in,
        ExoPlayerSdkImageOutputListener* delegate_in)
        : owner(owner_in), delegate(delegate_in) {}

    ScopedDelegateCall(const ScopedDelegateCall&) = delete;
    ScopedDelegateCall& operator=(const ScopedDelegateCall&) = delete;

    ScopedDelegateCall(ScopedDelegateCall&& other) noexcept
        : owner(other.owner), delegate(other.delegate) {
      other.owner = nullptr;
      other.delegate = nullptr;
    }

    ScopedDelegateCall& operator=(ScopedDelegateCall&& other) noexcept {
      if (this != &other) {
        Release();
        owner = other.owner;
        delegate = other.delegate;
        other.owner = nullptr;
        other.delegate = nullptr;
      }
      return *this;
    }

    ~ScopedDelegateCall() { Release(); }

    void Release() {
      if (owner != nullptr) {
        owner->OnDelegateCallReleased();
        owner = nullptr;
      }
    }

    ForwardingImageOutputListener* owner = nullptr;
    ExoPlayerSdkImageOutputListener* delegate = nullptr;
  };

  template <typename Fn>
  void NotifyDelegate(Fn&& fn) {
    ScopedDelegateCall delegate_call;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (delegate_ == nullptr) {
        return;
      }
      ++in_flight_callback_count_;
      delegate_call = ScopedDelegateCall(this, delegate_);
    }
    if (delegate_call.delegate != nullptr) {
      fn(delegate_call.delegate);
    }
  }

  void OnDelegateCallReleased() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (--in_flight_callback_count_ == 0) {
      callback_drained_.notify_all();
    }
  }

  mutable std::mutex mutex_;
  std::condition_variable callback_drained_;
  int in_flight_callback_count_ = 0;
  ExoPlayerSdkImageOutputListener* delegate_ = nullptr;
};

class ExoPlayerSdkPlayerImpl : public ExoPlayerSdkPlayer {
 public:
  ExoPlayerSdkPlayerImpl(JavaVM* java_vm, std::shared_ptr<ExoPlayerBridge> bridge)
      : java_vm_(java_vm), bridge_(std::move(bridge)) {
    forwarding_listener_ = std::make_unique<ForwardingPlayerListener>();
    forwarding_image_output_listener_ = std::make_unique<ForwardingImageOutputListener>();
    bridge_->SetListener(forwarding_listener_.get());
    bridge_->SetImageOutputListener(forwarding_image_output_listener_.get());
  }

  void Release() override {
    internal::LogInfo("ExoPlayerSdkPlayerImpl::Release start");
    bool expected = false;
    if (!released_.compare_exchange_strong(expected, true)) {
      internal::LogInfo("ExoPlayerSdkPlayerImpl::Release skip alreadyReleased");
      return;
    }
    ScopedEnv env(java_vm_);
    if (!env.ok()) {
      internal::LogError("ExoPlayerSdkPlayerImpl::Release missing JNIEnv");
      return;
    }
    internal::LogInfo("ExoPlayerSdkPlayerImpl::Release clear delegates");
    forwarding_listener_->SetDelegate(nullptr);
    forwarding_listener_->RemoveAnalyticsDelegate(nullptr);
    forwarding_listener_->RemoveAudioCodecParametersDelegate(nullptr);
    forwarding_listener_->RemoveVideoCodecParametersDelegate(nullptr);
    forwarding_listener_->RemoveVideoFrameMetadataDelegate(nullptr);
    forwarding_listener_->RemoveCameraMotionDelegate(nullptr);
    forwarding_image_output_listener_->SetDelegate(nullptr);
    internal::LogInfo("ExoPlayerSdkPlayerImpl::Release remove bridge listeners");
    bridge_->ClearAudioCodecParametersChangeListener(env.env());
    bridge_->ClearVideoCodecParametersChangeListener(env.env());
    bridge_->ClearVideoFrameMetadataListener(env.env());
    bridge_->ClearCameraMotionListener(env.env());
    bridge_->RemoveListener(forwarding_listener_.get());
    bridge_->RemoveImageOutputListener(forwarding_image_output_listener_.get());
    internal::LogInfo("ExoPlayerSdkPlayerImpl::Release bridge release begin");
    bridge_->Release(env.env());
    internal::LogInfo("ExoPlayerSdkPlayerImpl::Release done");
  }

  void SetListener(PlayerListener* listener) override {
    forwarding_listener_->SetDelegate(listener);
  }

  void RemoveListener(PlayerListener* listener) override {
    if (listener == nullptr || listener == forwarding_listener_->delegate()) {
      forwarding_listener_->SetDelegate(nullptr);
    }
  }

  void SetImageOutputListener(ExoPlayerSdkImageOutputListener* listener) override {
    forwarding_image_output_listener_->SetDelegate(listener);
    WithEnv([&](JNIEnv* env) { bridge_->SetImageOutputEnabled(env, listener != nullptr); });
  }

  void RemoveImageOutputListener(ExoPlayerSdkImageOutputListener* listener) override {
    if (listener == nullptr || listener == forwarding_image_output_listener_->delegate()) {
      forwarding_image_output_listener_->OnDisabled();
      forwarding_image_output_listener_->SetDelegate(nullptr);
      WithEnv([&](JNIEnv* env) { bridge_->SetImageOutputEnabled(env, false); });
    }
  }

  void AddAnalyticsListener(PlayerListener* listener) override {
    forwarding_listener_->AddAnalyticsDelegate(listener);
  }

  void RemoveAnalyticsListener(PlayerListener* listener) override {
    forwarding_listener_->RemoveAnalyticsDelegate(listener);
  }

  void AddAudioCodecParametersChangeListener(
      PlayerListener* listener,
      const std::vector<std::string>& keys) override {
    if (listener == nullptr) {
      return;
    }
    forwarding_listener_->AddAudioCodecParametersDelegate(listener, keys);
    forwarding_listener_->RouteNextAudioCodecParametersCallbackTo(listener);
    WithEnv([&](JNIEnv* env) {
      bridge_->SetAudioCodecParametersChangeListener(
          env, forwarding_listener_->GetAudioCodecParameterKeys());
    });
    forwarding_listener_->ClearNextAudioCodecParametersCallbackRouting();
  }

  void RemoveAudioCodecParametersChangeListener(PlayerListener* listener) override {
    forwarding_listener_->RemoveAudioCodecParametersDelegate(listener);
    if (forwarding_listener_->HasAudioCodecParametersDelegates()) {
      forwarding_listener_->SuppressNextAudioCodecParametersCallback();
      WithEnv([&](JNIEnv* env) {
        bridge_->SetAudioCodecParametersChangeListener(
            env, forwarding_listener_->GetAudioCodecParameterKeys());
      });
      forwarding_listener_->ClearNextAudioCodecParametersCallbackRouting();
    } else {
      WithEnv([&](JNIEnv* env) {
        bridge_->ClearAudioCodecParametersChangeListener(env);
      });
    }
  }

  void AddVideoCodecParametersChangeListener(
      PlayerListener* listener,
      const std::vector<std::string>& keys) override {
    if (listener == nullptr) {
      return;
    }
    forwarding_listener_->AddVideoCodecParametersDelegate(listener, keys);
    forwarding_listener_->RouteNextVideoCodecParametersCallbackTo(listener);
    WithEnv([&](JNIEnv* env) {
      bridge_->SetVideoCodecParametersChangeListener(
          env, forwarding_listener_->GetVideoCodecParameterKeys());
    });
    forwarding_listener_->ClearNextVideoCodecParametersCallbackRouting();
  }

  void RemoveVideoCodecParametersChangeListener(PlayerListener* listener) override {
    forwarding_listener_->RemoveVideoCodecParametersDelegate(listener);
    if (forwarding_listener_->HasVideoCodecParametersDelegates()) {
      forwarding_listener_->SuppressNextVideoCodecParametersCallback();
      WithEnv([&](JNIEnv* env) {
        bridge_->SetVideoCodecParametersChangeListener(
            env, forwarding_listener_->GetVideoCodecParameterKeys());
      });
      forwarding_listener_->ClearNextVideoCodecParametersCallbackRouting();
    } else {
      WithEnv([&](JNIEnv* env) {
        bridge_->ClearVideoCodecParametersChangeListener(env);
      });
    }
  }

  void SetVideoFrameMetadataListener(PlayerListener* listener) override {
    forwarding_listener_->SetVideoFrameMetadataDelegate(listener);
    WithEnv([&](JNIEnv* env) {
      if (listener != nullptr) {
        bridge_->SetVideoFrameMetadataListener(env);
      } else {
        bridge_->ClearVideoFrameMetadataListener(env);
      }
    });
  }

  void ClearVideoFrameMetadataListener(PlayerListener* listener) override {
    if (forwarding_listener_->RemoveVideoFrameMetadataDelegate(listener)) {
      WithEnv([&](JNIEnv* env) { bridge_->ClearVideoFrameMetadataListener(env); });
    }
  }

  void SetCameraMotionListener(PlayerListener* listener) override {
    forwarding_listener_->SetCameraMotionDelegate(listener);
    WithEnv([&](JNIEnv* env) {
      if (listener != nullptr) {
        bridge_->SetCameraMotionListener(env);
      } else {
        bridge_->ClearCameraMotionListener(env);
      }
    });
  }

  void ClearCameraMotionListener(PlayerListener* listener) override {
    if (forwarding_listener_->RemoveCameraMotionDelegate(listener)) {
      WithEnv([&](JNIEnv* env) { bridge_->ClearCameraMotionListener(env); });
    }
  }

  void BindPlayerView(jobject player_view) override {
    WithEnv([&](JNIEnv* env) { bridge_->BindPlayerView(env, player_view); });
  }

  void UnbindPlayerView(jobject player_view) override {
    WithEnv([&](JNIEnv* env) { bridge_->UnbindPlayerView(env, player_view); });
  }

  void SetVideoSurface(jobject surface) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetVideoSurface(env, surface); });
  }

  void ClearVideoSurface() override {
    WithEnv([&](JNIEnv* env) { bridge_->ClearVideoSurface(env); });
  }

  void ClearVideoSurface(jobject surface) override {
    WithEnv([&](JNIEnv* env) { bridge_->ClearVideoSurface(env, surface); });
  }

  void SetVideoSurfaceHolder(jobject surface_holder) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetVideoSurfaceHolder(env, surface_holder); });
  }

  void ClearVideoSurfaceHolder(jobject surface_holder) override {
    WithEnv([&](JNIEnv* env) { bridge_->ClearVideoSurfaceHolder(env, surface_holder); });
  }

  void SetVideoSurfaceView(jobject surface_view) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetVideoSurfaceView(env, surface_view); });
  }

  void ClearVideoSurfaceView(jobject surface_view) override {
    WithEnv([&](JNIEnv* env) { bridge_->ClearVideoSurfaceView(env, surface_view); });
  }

  void SetVideoTextureView(jobject texture_view) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetVideoTextureView(env, texture_view); });
  }

  void ClearVideoTextureView(jobject texture_view) override {
    WithEnv([&](JNIEnv* env) { bridge_->ClearVideoTextureView(env, texture_view); });
  }

  void SetVideoEffects(const std::vector<VideoEffectDescriptor>& video_effects) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetVideoEffects(env, video_effects); });
  }

  void SetMediaItem(const MediaItemDescriptor& media_item) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetMediaItem(env, media_item); });
  }

  void SetMediaItem(const MediaItemDescriptor& media_item, bool reset_position) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetMediaItem(env, media_item, reset_position); });
  }

  void SetMediaItem(
      const MediaItemDescriptor& media_item,
      int64_t start_position_ms) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetMediaItem(env, media_item, start_position_ms); });
  }

  void SetMediaItems(
      const std::vector<MediaItemDescriptor>& media_items,
      int start_index,
      int64_t start_position_ms) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SetMediaItems(env, media_items, start_index, start_position_ms);
    });
  }

  void SetMediaItems(
      const std::vector<MediaItemDescriptor>& media_items,
      bool reset_position) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetMediaItems(env, media_items, reset_position); });
  }

  void AddMediaItem(const MediaItemDescriptor& media_item) override {
    WithEnv([&](JNIEnv* env) { bridge_->AddMediaItem(env, media_item); });
  }

  void AddMediaItem(int index, const MediaItemDescriptor& media_item) override {
    WithEnv([&](JNIEnv* env) { bridge_->AddMediaItem(env, index, media_item); });
  }

  void AddMediaItems(const std::vector<MediaItemDescriptor>& media_items) override {
    WithEnv([&](JNIEnv* env) { bridge_->AddMediaItems(env, media_items); });
  }

  void AddMediaItems(
      int index,
      const std::vector<MediaItemDescriptor>& media_items) override {
    WithEnv([&](JNIEnv* env) { bridge_->AddMediaItems(env, index, media_items); });
  }

  void RemoveMediaItem(int index) override {
    WithEnv([&](JNIEnv* env) { bridge_->RemoveMediaItem(env, index); });
  }

  void RemoveMediaItems(int from_index, int to_index) override {
    WithEnv([&](JNIEnv* env) { bridge_->RemoveMediaItems(env, from_index, to_index); });
  }

  void MoveMediaItem(int current_index, int new_index) override {
    WithEnv([&](JNIEnv* env) { bridge_->MoveMediaItem(env, current_index, new_index); });
  }

  void MoveMediaItems(int from_index, int to_index, int new_index) override {
    WithEnv(
        [&](JNIEnv* env) { bridge_->MoveMediaItems(env, from_index, to_index, new_index); });
  }

  void ReplaceMediaItems(
      int from_index,
      int to_index,
      const std::vector<MediaItemDescriptor>& media_items) override {
    WithEnv(
        [&](JNIEnv* env) { bridge_->ReplaceMediaItems(env, from_index, to_index, media_items); });
  }

  void ReplaceMediaItem(int index, const MediaItemDescriptor& media_item) override {
    WithEnv([&](JNIEnv* env) { bridge_->ReplaceMediaItem(env, index, media_item); });
  }

  void ClearMediaItems() override {
    WithEnv([&](JNIEnv* env) { bridge_->ClearMediaItems(env); });
  }

  void Prepare() override {
    WithEnv([&](JNIEnv* env) { bridge_->Prepare(env); });
  }

  void Play() override {
    WithEnv([&](JNIEnv* env) { bridge_->Play(env); });
  }

  void Pause() override {
    WithEnv([&](JNIEnv* env) { bridge_->Pause(env); });
  }

  void Stop() override {
    WithEnv([&](JNIEnv* env) { bridge_->Stop(env); });
  }

  void SeekTo(int64_t position_ms) override {
    WithEnv([&](JNIEnv* env) { bridge_->SeekTo(env, position_ms); });
  }

  void SeekToMediaItem(int media_item_index, int64_t position_ms) override {
    WithEnv([&](JNIEnv* env) { bridge_->SeekToMediaItem(env, media_item_index, position_ms); });
  }

  void SeekBack() override {
    WithEnv([&](JNIEnv* env) { bridge_->SeekBack(env); });
  }

  void SeekForward() override {
    WithEnv([&](JNIEnv* env) { bridge_->SeekForward(env); });
  }

  void SeekToDefaultPosition() override {
    WithEnv([&](JNIEnv* env) { bridge_->SeekToDefaultPosition(env); });
  }

  void SeekToDefaultPosition(int media_item_index) override {
    WithEnv([&](JNIEnv* env) { bridge_->SeekToDefaultPosition(env, media_item_index); });
  }

  void SetSeekParameters(const SeekParametersDescriptor& seek_parameters) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetSeekParameters(env, seek_parameters); });
  }

  SeekParametersDescriptor GetSeekParameters() override {
    return WithEnvOrDefault<SeekParametersDescriptor>(
        [&](JNIEnv* env) { return bridge_->GetSeekParameters(env); });
  }

  void SeekToNext() override {
    WithEnv([&](JNIEnv* env) { bridge_->SeekToNext(env); });
  }

  void SeekToPrevious() override {
    WithEnv([&](JNIEnv* env) { bridge_->SeekToPrevious(env); });
  }

  void SeekToNextMediaItem() override {
    WithEnv([&](JNIEnv* env) { bridge_->SeekToNextMediaItem(env); });
  }

  void SeekToPreviousMediaItem() override {
    WithEnv([&](JNIEnv* env) { bridge_->SeekToPreviousMediaItem(env); });
  }

  void SetWakeMode(int wake_mode) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetWakeMode(env, wake_mode); });
  }

  void SetHandleAudioBecomingNoisy(bool handle_audio_becoming_noisy) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SetHandleAudioBecomingNoisy(env, handle_audio_becoming_noisy);
    });
  }

  void SetPriority(int priority) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetPriority(env, priority); });
  }

  void SetPriorityTaskManager(ExoPlayerSdkPriorityTaskManager* priority_task_manager) override {
    WithEnv([&](JNIEnv* env) {
      jobject java_priority_task_manager =
          priority_task_manager != nullptr
              ? priority_task_manager->GetJavaObjectLocalRef(env)
              : nullptr;
      bridge_->SetPriorityTaskManager(env, java_priority_task_manager);
      if (java_priority_task_manager != nullptr) {
        env->DeleteLocalRef(java_priority_task_manager);
      }
    });
  }

  void ClearPriorityTaskManager() override {
    WithEnv([&](JNIEnv* env) { bridge_->SetPriorityTaskManager(env, nullptr); });
  }

  void SetPriorityTaskManagerEnabled(bool enabled) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetPriorityTaskManagerEnabled(env, enabled); });
  }

  void SetPreloadConfiguration(int64_t target_preload_duration_us) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SetPreloadConfiguration(env, target_preload_duration_us);
    });
  }

  void SetForegroundMode(bool foreground_mode) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetForegroundMode(env, foreground_mode); });
  }

  PlayerMessageResult SendPlayerMessage(const PlayerMessageDescriptor& message) override {
    return WithEnvOrDefault<PlayerMessageResult>(
        [&](JNIEnv* env) { return bridge_->SendPlayerMessage(env, message); });
  }

  void SetImageOutputEnabled(bool enabled) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetImageOutputEnabled(env, enabled); });
  }

  void SetAudioAttributes(
      const AudioAttributesDescriptor& attributes,
      bool handle_audio_focus) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetAudioAttributes(env, attributes, handle_audio_focus); });
  }

  void SetAudioSessionId(int audio_session_id) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetAudioSessionId(env, audio_session_id); });
  }

  void SetAuxEffectInfo(const AuxEffectInfoDescriptor& aux_effect_info) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetAuxEffectInfo(env, aux_effect_info); });
  }

  void ClearAuxEffectInfo() override {
    WithEnv([&](JNIEnv* env) { bridge_->ClearAuxEffectInfo(env); });
  }

  void SetPreferredAudioDevice(jobject audio_device_info) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetPreferredAudioDevice(env, audio_device_info); });
  }

  void ClearPreferredAudioDevice() override {
    WithEnv([&](JNIEnv* env) { bridge_->SetPreferredAudioDevice(env, nullptr); });
  }

  void SetVirtualDeviceId(int virtual_device_id) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetVirtualDeviceId(env, virtual_device_id); });
  }

  void SetAudioCodecParameters(
      const CodecParametersDescriptor& codec_parameters) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetAudioCodecParameters(env, codec_parameters); });
  }

  void SetVideoCodecParameters(
      const CodecParametersDescriptor& codec_parameters) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetVideoCodecParameters(env, codec_parameters); });
  }

  void SetDeviceVolume(int volume, int flags) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetDeviceVolume(env, volume, flags); });
  }

  void AdjustDeviceVolume(int direction, int flags) override {
    WithEnv([&](JNIEnv* env) { bridge_->AdjustDeviceVolume(env, direction, flags); });
  }

  void IncreaseDeviceVolume(int flags) override {
    WithEnv([&](JNIEnv* env) { bridge_->IncreaseDeviceVolume(env, flags); });
  }

  void DecreaseDeviceVolume(int flags) override {
    WithEnv([&](JNIEnv* env) { bridge_->DecreaseDeviceVolume(env, flags); });
  }

  void SetDeviceMuted(bool muted, int flags) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetDeviceMuted(env, muted, flags); });
  }

  void SetSkipSilenceEnabled(bool skip_silence_enabled) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetSkipSilenceEnabled(env, skip_silence_enabled); });
  }

  void SetScrubbingModeEnabled(bool scrubbing_mode_enabled) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SetScrubbingModeEnabled(env, scrubbing_mode_enabled);
    });
  }

  bool IsScrubbingModeEnabled() override {
    return WithEnvOrDefault<bool>(
        [&](JNIEnv* env) { return bridge_->IsScrubbingModeEnabled(env); });
  }

  void SetScrubbingModeParameters(
      const ScrubbingModeParametersDescriptor& parameters) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetScrubbingModeParameters(env, parameters); });
  }

  ScrubbingModeParametersDescriptor GetScrubbingModeParameters() override {
    return WithEnvOrDefault<ScrubbingModeParametersDescriptor>(
        [&](JNIEnv* env) { return bridge_->GetScrubbingModeParameters(env); });
  }

  void SetPlayWhenReady(bool play_when_ready) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetPlayWhenReady(env, play_when_ready); });
  }

  void SetRepeatMode(RepeatMode repeat_mode) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetRepeatMode(env, repeat_mode); });
  }

  void SetShuffleModeEnabled(bool shuffle_mode_enabled) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetShuffleModeEnabled(env, shuffle_mode_enabled); });
  }

  void SetVolume(float volume) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetVolume(env, volume); });
  }

  void SetPlaybackSpeed(float speed) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetPlaybackSpeed(env, speed); });
  }

  void SetPlaybackParameters(const PlaybackParametersSnapshot& parameters) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetPlaybackParameters(env, parameters); });
  }

  void SetPauseAtEndOfMediaItems(bool pause_at_end_of_media_items) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetPauseAtEndOfMediaItems(env, pause_at_end_of_media_items); });
  }

  bool GetPauseAtEndOfMediaItems() override {
    return WithEnvOrDefault<bool>(
        [&](JNIEnv* env) { return bridge_->GetPauseAtEndOfMediaItems(env); });
  }

  void SetSeekBackIncrementMs(int64_t seek_back_increment_ms) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetSeekBackIncrementMs(env, seek_back_increment_ms); });
  }

  void SetSeekForwardIncrementMs(int64_t seek_forward_increment_ms) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SetSeekForwardIncrementMs(env, seek_forward_increment_ms);
    });
  }

  void SetMaxSeekToPreviousPositionMs(int64_t max_seek_to_previous_position_ms) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SetMaxSeekToPreviousPositionMs(env, max_seek_to_previous_position_ms);
    });
  }

  void SetVideoScalingMode(int video_scaling_mode) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetVideoScalingMode(env, video_scaling_mode); });
  }

  int GetVideoScalingMode() override {
    return WithEnvOrDefault<int>(
        [&](JNIEnv* env) { return bridge_->GetVideoScalingMode(env); });
  }

  void SetVideoChangeFrameRateStrategy(int video_change_frame_rate_strategy) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SetVideoChangeFrameRateStrategy(env, video_change_frame_rate_strategy);
    });
  }

  int GetVideoChangeFrameRateStrategy() override {
    return WithEnvOrDefault<int>(
        [&](JNIEnv* env) { return bridge_->GetVideoChangeFrameRateStrategy(env); });
  }

  void SetTrackSelectionParameters(
      const TrackSelectionParametersDescriptor& parameters) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetTrackSelectionParameters(env, parameters); });
  }

  TrackSelectionParametersDescriptor GetTrackSelectionParameters() override {
    return WithEnvOrDefault<TrackSelectionParametersDescriptor>(
        [&](JNIEnv* env) { return bridge_->GetTrackSelectionParameters(env); });
  }

  int GetRendererCount() override {
    return WithEnvOrDefault<int>([&](JNIEnv* env) { return bridge_->GetRendererCount(env); });
  }

  int GetRendererType(int index) override {
    return WithEnvOrDefault<int>(
        [&](JNIEnv* env) { return bridge_->GetRendererType(env, index); }, -1);
  }

  TracksSnapshot GetTracks() override {
    return WithEnvOrDefault<TracksSnapshot>(
        [&](JNIEnv* env) { return bridge_->GetTracksSnapshot(env); });
  }

  std::vector<TrackGroupSnapshot> GetTrackGroups() override {
    return WithEnvOrDefault<std::vector<TrackGroupSnapshot>>(
        [&](JNIEnv* env) { return bridge_->GetTrackGroups(env); });
  }

  PlaybackState GetPlaybackState() override {
    return WithEnvOrDefault<PlaybackState>(
        [&](JNIEnv* env) { return bridge_->GetPlaybackState(env); });
  }

  bool GetPlayWhenReady() override {
    return WithEnvOrDefault<bool>([&](JNIEnv* env) { return bridge_->GetPlayWhenReady(env); });
  }

  bool IsPlaying() override {
    return WithEnvOrDefault<bool>([&](JNIEnv* env) { return bridge_->IsPlaying(env); });
  }

  bool IsLoading() override {
    return WithEnvOrDefault<bool>([&](JNIEnv* env) { return bridge_->IsLoading(env); });
  }

  PlayerError GetPlayerError() override {
    return WithEnvOrDefault<PlayerError>([&](JNIEnv* env) { return bridge_->GetPlayerError(env); });
  }

  int64_t GetCurrentPosition() override {
    return WithEnvOrDefault<int64_t>([&](JNIEnv* env) { return bridge_->GetCurrentPosition(env); });
  }

  int64_t GetBufferedPosition() override {
    return WithEnvOrDefault<int64_t>([&](JNIEnv* env) { return bridge_->GetBufferedPosition(env); });
  }

  int64_t GetDuration() override {
    return WithEnvOrDefault<int64_t>([&](JNIEnv* env) { return bridge_->GetDuration(env); });
  }

  int GetCurrentMediaItemIndex() override {
    return WithEnvOrDefault<int>(
        [&](JNIEnv* env) { return bridge_->GetCurrentMediaItemIndex(env); }, -1);
  }

  int GetMediaItemCount() override {
    return WithEnvOrDefault<int>([&](JNIEnv* env) { return bridge_->GetMediaItemCount(env); });
  }

  RepeatMode GetRepeatMode() override {
    return WithEnvOrDefault<RepeatMode>([&](JNIEnv* env) { return bridge_->GetRepeatMode(env); });
  }

  bool GetShuffleModeEnabled() override {
    return WithEnvOrDefault<bool>([&](JNIEnv* env) { return bridge_->GetShuffleModeEnabled(env); });
  }

  float GetVolume() override {
    return WithEnvOrDefault<float>([&](JNIEnv* env) { return bridge_->GetVolume(env); });
  }

  AudioAttributesDescriptor GetAudioAttributes() override {
    return WithEnvOrDefault<AudioAttributesDescriptor>(
        [&](JNIEnv* env) { return bridge_->GetAudioAttributes(env); });
  }

  DeviceInfoDescriptor GetDeviceInfo() override {
    return WithEnvOrDefault<DeviceInfoDescriptor>(
        [&](JNIEnv* env) { return bridge_->GetDeviceInfo(env); });
  }

  int GetDeviceVolume() override {
    return WithEnvOrDefault<int>([&](JNIEnv* env) { return bridge_->GetDeviceVolume(env); });
  }

  bool IsDeviceMuted() override {
    return WithEnvOrDefault<bool>([&](JNIEnv* env) { return bridge_->IsDeviceMuted(env); });
  }

  bool GetSkipSilenceEnabled() override {
    return WithEnvOrDefault<bool>([&](JNIEnv* env) { return bridge_->GetSkipSilenceEnabled(env); });
  }

  VideoSizeSnapshot GetVideoSize() override {
    return WithEnvOrDefault<VideoSizeSnapshot>(
        [&](JNIEnv* env) { return bridge_->GetVideoSize(env); });
  }

  int GetNextMediaItemIndex() override {
    return WithEnvOrDefault<int>(
        [&](JNIEnv* env) { return bridge_->GetNextMediaItemIndex(env); }, -1);
  }

  int GetPreviousMediaItemIndex() override {
    return WithEnvOrDefault<int>(
        [&](JNIEnv* env) { return bridge_->GetPreviousMediaItemIndex(env); }, -1);
  }

  bool HasNextMediaItem() override {
    return WithEnvOrDefault<bool>([&](JNIEnv* env) { return bridge_->HasNextMediaItem(env); });
  }

  bool HasPreviousMediaItem() override {
    return WithEnvOrDefault<bool>([&](JNIEnv* env) { return bridge_->HasPreviousMediaItem(env); });
  }

  int GetBufferedPercentage() override {
    return WithEnvOrDefault<int>([&](JNIEnv* env) { return bridge_->GetBufferedPercentage(env); });
  }

  int64_t GetContentBufferedPosition() override {
    return WithEnvOrDefault<int64_t>(
        [&](JNIEnv* env) { return bridge_->GetContentBufferedPosition(env); });
  }

  int64_t GetContentDuration() override {
    return WithEnvOrDefault<int64_t>([&](JNIEnv* env) { return bridge_->GetContentDuration(env); });
  }

  int64_t GetContentPosition() override {
    return WithEnvOrDefault<int64_t>([&](JNIEnv* env) { return bridge_->GetContentPosition(env); });
  }

  int64_t GetCurrentLiveOffset() override {
    return WithEnvOrDefault<int64_t>([&](JNIEnv* env) { return bridge_->GetCurrentLiveOffset(env); });
  }

  int GetCurrentPeriodIndex() override {
    return WithEnvOrDefault<int>([&](JNIEnv* env) { return bridge_->GetCurrentPeriodIndex(env); }, -1);
  }

  int64_t GetMaxSeekToPreviousPosition() override {
    return WithEnvOrDefault<int64_t>(
        [&](JNIEnv* env) { return bridge_->GetMaxSeekToPreviousPosition(env); });
  }

  PlaybackSuppressionReason GetPlaybackSuppressionReason() override {
    return WithEnvOrDefault<PlaybackSuppressionReason>(
        [&](JNIEnv* env) { return bridge_->GetPlaybackSuppressionReason(env); });
  }

  int64_t GetSeekBackIncrement() override {
    return WithEnvOrDefault<int64_t>([&](JNIEnv* env) { return bridge_->GetSeekBackIncrement(env); });
  }

  int64_t GetSeekForwardIncrement() override {
    return WithEnvOrDefault<int64_t>(
        [&](JNIEnv* env) { return bridge_->GetSeekForwardIncrement(env); });
  }

  int64_t GetTotalBufferedDuration() override {
    return WithEnvOrDefault<int64_t>(
        [&](JNIEnv* env) { return bridge_->GetTotalBufferedDuration(env); });
  }

  int64_t GetTargetPreloadDurationUs() override {
    return WithEnvOrDefault<int64_t>(
        [&](JNIEnv* env) { return bridge_->GetTargetPreloadDurationUs(env); });
  }

  bool IsCommandAvailable(int command_code) override {
    return WithEnvOrDefault<bool>(
        [&](JNIEnv* env) { return bridge_->IsCommandAvailable(env, command_code); });
  }

  bool CanAdvertiseSession() override {
    return WithEnvOrDefault<bool>([&](JNIEnv* env) { return bridge_->CanAdvertiseSession(env); });
  }

  ApplicationLooperDescriptor GetApplicationLooper() override {
    return WithEnvOrDefault<ApplicationLooperDescriptor>(
        [&](JNIEnv* env) { return bridge_->GetApplicationLooper(env); });
  }

  bool IsSleepingForOffload() override {
    return WithEnvOrDefault<bool>(
        [&](JNIEnv* env) { return bridge_->IsSleepingForOffload(env); });
  }

  bool IsTunnelingEnabled() override {
    return WithEnvOrDefault<bool>(
        [&](JNIEnv* env) { return bridge_->IsTunnelingEnabled(env); });
  }

  bool IsReleased() override {
    return WithEnvOrDefault<bool>([&](JNIEnv* env) { return bridge_->IsReleased(env); });
  }

  int GetCurrentAdGroupIndex() override {
    return WithEnvOrDefault<int>([&](JNIEnv* env) { return bridge_->GetCurrentAdGroupIndex(env); }, -1);
  }

  int GetCurrentAdIndexInAdGroup() override {
    return WithEnvOrDefault<int>(
        [&](JNIEnv* env) { return bridge_->GetCurrentAdIndexInAdGroup(env); }, -1);
  }

  bool IsCurrentMediaItemDynamic() override {
    return WithEnvOrDefault<bool>(
        [&](JNIEnv* env) { return bridge_->IsCurrentMediaItemDynamic(env); });
  }

  bool IsCurrentMediaItemLive() override {
    return WithEnvOrDefault<bool>([&](JNIEnv* env) { return bridge_->IsCurrentMediaItemLive(env); });
  }

  bool IsCurrentMediaItemSeekable() override {
    return WithEnvOrDefault<bool>(
        [&](JNIEnv* env) { return bridge_->IsCurrentMediaItemSeekable(env); });
  }

  bool IsPlayingAd() override {
    return WithEnvOrDefault<bool>([&](JNIEnv* env) { return bridge_->IsPlayingAd(env); });
  }

  void SetPlaylistMetadata(const MediaMetadataSnapshot& metadata) override {
    WithEnv([&](JNIEnv* env) { bridge_->SetPlaylistMetadata(env, metadata); });
  }

  MediaMetadataSnapshot GetMediaMetadata() override {
    return WithEnvOrDefault<MediaMetadataSnapshot>(
        [&](JNIEnv* env) { return bridge_->GetMediaMetadata(env); });
  }

  MediaMetadataSnapshot GetPlaylistMetadata() override {
    return WithEnvOrDefault<MediaMetadataSnapshot>(
        [&](JNIEnv* env) { return bridge_->GetPlaylistMetadata(env); });
  }

  PlaybackParametersSnapshot GetPlaybackParameters() override {
    return WithEnvOrDefault<PlaybackParametersSnapshot>(
        [&](JNIEnv* env) { return bridge_->GetPlaybackParameters(env); });
  }

  MediaItemDescriptor GetMediaItemAt(int index) override {
    return WithEnvOrDefault<MediaItemDescriptor>(
        [&](JNIEnv* env) { return bridge_->GetMediaItemAt(env, index); });
  }

  MediaItemDescriptor GetCurrentMediaItem() override {
    return WithEnvOrDefault<MediaItemDescriptor>(
        [&](JNIEnv* env) { return bridge_->GetCurrentMediaItem(env); });
  }

  void ReleaseOpaqueObjectTokens(const std::vector<std::string>& tokens) override {
    WithEnv([&](JNIEnv* env) { bridge_->ReleaseOpaqueObjectTokens(env, tokens); });
  }

  AnalyticsSnapshot GetAnalyticsSnapshot() override {
    return WithEnvOrDefault<AnalyticsSnapshot>(
        [&](JNIEnv* env) { return bridge_->GetAnalyticsSnapshot(env); });
  }

  void SimulateAnalyticsUpdateForTest(const AnalyticsSnapshot& analytics) override {
    WithEnv([&](JNIEnv* env) { bridge_->SimulateAnalyticsUpdateForTest(env, analytics); });
  }

  void SimulateAudioUnderrunForTest(const AudioUnderrunEvent& audio_underrun) override {
    WithEnv([&](JNIEnv* env) { bridge_->SimulateAudioUnderrunForTest(env, audio_underrun); });
  }

  void SimulateDroppedVideoFramesForTest(
      const DroppedVideoFramesEvent& dropped_video_frames) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateDroppedVideoFramesForTest(env, dropped_video_frames);
    });
  }

  void SimulateBandwidthEstimateForTest(
      const BandwidthEstimateEvent& bandwidth_estimate) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateBandwidthEstimateForTest(env, bandwidth_estimate);
    });
  }

  void SimulateLoadStartedForTest(const LoadStartedEvent& load_started) override {
    WithEnv([&](JNIEnv* env) { bridge_->SimulateLoadStartedForTest(env, load_started); });
  }

  void SimulateLoadCompletedForTest(const LoadCompletedEvent& load_completed) override {
    WithEnv([&](JNIEnv* env) { bridge_->SimulateLoadCompletedForTest(env, load_completed); });
  }

  void SimulateAudioInputFormatChangedForTest(
      const AudioInputFormatChangedEvent& audio_input_format_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAudioInputFormatChangedForTest(env, audio_input_format_changed);
    });
  }

  void SimulateAudioDecoderInitializedForTest(
      const AudioDecoderInitializedEvent& audio_decoder_initialized) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAudioDecoderInitializedForTest(env, audio_decoder_initialized);
    });
  }

  void SimulateVideoDecoderInitializedForTest(
      const VideoDecoderInitializedEvent& video_decoder_initialized) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateVideoDecoderInitializedForTest(env, video_decoder_initialized);
    });
  }

  void SimulateAudioDecoderReleasedForTest(
      const AudioDecoderReleasedEvent& audio_decoder_released) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAudioDecoderReleasedForTest(env, audio_decoder_released);
    });
  }

  void SimulateVideoDecoderReleasedForTest(
      const VideoDecoderReleasedEvent& video_decoder_released) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateVideoDecoderReleasedForTest(env, video_decoder_released);
    });
  }

  void SimulateAnalyticsRenderedFirstFrameForTest(
      const AnalyticsRenderedFirstFrameEvent& rendered_first_frame) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsRenderedFirstFrameForTest(env, rendered_first_frame);
    });
  }

  void SimulateAnalyticsVideoSizeChangedForTest(
      const AnalyticsVideoSizeChangedEvent& analytics_video_size) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsVideoSizeChangedForTest(env, analytics_video_size);
    });
  }

  void SimulateAudioPositionAdvancingForTest(
      const AudioPositionAdvancingEvent& audio_position_advancing) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAudioPositionAdvancingForTest(env, audio_position_advancing);
    });
  }

  void SimulateVideoFrameProcessingOffsetForTest(
      const VideoFrameProcessingOffsetEvent& video_frame_processing_offset) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateVideoFrameProcessingOffsetForTest(env, video_frame_processing_offset);
    });
  }

  void SimulateVolumeChangedForTest(
      const VolumeChangedEvent& volume_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateVolumeChangedForTest(env, volume_changed);
    });
  }

  void SimulateAudioSessionIdChangedForTest(
      const AudioSessionIdChangedEvent& audio_session_id_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAudioSessionIdChangedForTest(env, audio_session_id_changed);
    });
  }

  void SimulateAnalyticsSkipSilenceEnabledChangedForTest(
      const AnalyticsSkipSilenceEnabledChangedEvent& skip_silence_enabled_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsSkipSilenceEnabledChangedForTest(
          env, skip_silence_enabled_changed);
    });
  }

  void SimulateAnalyticsDeviceVolumeChangedForTest(
      const AnalyticsDeviceVolumeChangedEvent& device_volume_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsDeviceVolumeChangedForTest(env, device_volume_changed);
    });
  }

  void SimulateAnalyticsPlaybackStateChangedForTest(
      const AnalyticsPlaybackStateChangedEvent& playback_state_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsPlaybackStateChangedForTest(env, playback_state_changed);
    });
  }

  void SimulateAnalyticsIsPlayingChangedForTest(
      const AnalyticsIsPlayingChangedEvent& is_playing_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsIsPlayingChangedForTest(env, is_playing_changed);
    });
  }

  void SimulateAnalyticsPlayWhenReadyChangedForTest(
      const AnalyticsPlayWhenReadyChangedEvent& play_when_ready_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsPlayWhenReadyChangedForTest(env, play_when_ready_changed);
    });
  }

  void SimulateAnalyticsPlaybackSuppressionReasonChangedForTest(
      const AnalyticsPlaybackSuppressionReasonChangedEvent& suppression_reason_changed)
      override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsPlaybackSuppressionReasonChangedForTest(
          env, suppression_reason_changed);
    });
  }

  void SimulateAnalyticsIsLoadingChangedForTest(
      const AnalyticsIsLoadingChangedEvent& is_loading_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsIsLoadingChangedForTest(env, is_loading_changed);
    });
  }

  void SimulateAnalyticsRepeatModeChangedForTest(
      const AnalyticsRepeatModeChangedEvent& repeat_mode_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsRepeatModeChangedForTest(env, repeat_mode_changed);
    });
  }

  void SimulateAnalyticsShuffleModeChangedForTest(
      const AnalyticsShuffleModeChangedEvent& shuffle_mode_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsShuffleModeChangedForTest(env, shuffle_mode_changed);
    });
  }

  void SimulateAnalyticsPlaybackParametersChangedForTest(
      const AnalyticsPlaybackParametersChangedEvent& playback_parameters_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsPlaybackParametersChangedForTest(
          env, playback_parameters_changed);
    });
  }

  void SimulateAnalyticsAvailableCommandsChangedForTest(
      const AnalyticsAvailableCommandsChangedEvent& available_commands_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsAvailableCommandsChangedForTest(
          env, available_commands_changed);
    });
  }

  void SimulateAnalyticsEventsForTest(
      const AnalyticsEventsEvent& analytics_events) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsEventsForTest(env, analytics_events);
    });
  }

  void SimulateIsLoadingChangedForTest(bool is_loading) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateIsLoadingChangedForTest(env, is_loading);
    });
  }

  void SimulateSeekBackIncrementChangedForTest(int64_t seek_back_increment_ms) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateSeekBackIncrementChangedForTest(env, seek_back_increment_ms);
    });
  }

  void SimulateSeekForwardIncrementChangedForTest(int64_t seek_forward_increment_ms) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateSeekForwardIncrementChangedForTest(env, seek_forward_increment_ms);
    });
  }

  void SimulateMaxSeekToPreviousPositionChangedForTest(
      int64_t max_seek_to_previous_position_ms) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateMaxSeekToPreviousPositionChangedForTest(
          env, max_seek_to_previous_position_ms);
    });
  }

  void SimulateAnalyticsSeekBackIncrementChangedForTest(
      const AnalyticsSeekBackIncrementChangedEvent& seek_back_increment_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsSeekBackIncrementChangedForTest(
          env, seek_back_increment_changed);
    });
  }

  void SimulateAnalyticsSeekForwardIncrementChangedForTest(
      const AnalyticsSeekForwardIncrementChangedEvent& seek_forward_increment_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsSeekForwardIncrementChangedForTest(
          env, seek_forward_increment_changed);
    });
  }

  void SimulateAnalyticsMaxSeekToPreviousPositionChangedForTest(
      const AnalyticsMaxSeekToPreviousPositionChangedEvent&
          max_seek_to_previous_position_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsMaxSeekToPreviousPositionChangedForTest(
          env, max_seek_to_previous_position_changed);
    });
  }

  void SimulateAnalyticsTimelineChangedForTest(
      const AnalyticsTimelineChangedEvent& timeline_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsTimelineChangedForTest(env, timeline_changed);
    });
  }

  void SimulateAnalyticsPositionDiscontinuityForTest(
      const AnalyticsPositionDiscontinuityEvent& position_discontinuity) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsPositionDiscontinuityForTest(env, position_discontinuity);
    });
  }

  void SimulateAnalyticsSeekStartedForTest(
      const AnalyticsSeekStartedEvent& seek_started) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsSeekStartedForTest(env, seek_started);
    });
  }

  void SimulateAnalyticsPlayerErrorForTest(
      const PlayerError& error) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsPlayerErrorForTest(env, error);
    });
  }

  void SimulateAnalyticsPlayerErrorChangedForTest(
      const PlayerError& error) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsPlayerErrorChangedForTest(env, error);
    });
  }

  void SimulateAnalyticsTracksChangedForTest(
      const TracksSnapshot& tracks) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsTracksChangedForTest(env, tracks);
    });
  }

  void SimulateAnalyticsMediaItemTransitionForTest(
      const AnalyticsMediaItemTransitionEvent& media_item_transition) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsMediaItemTransitionForTest(env, media_item_transition);
    });
  }

  void SimulateAnalyticsCuesForTest(
      const CueSnapshot& cues) override {
    WithEnv([&](JNIEnv* env) { bridge_->SimulateAnalyticsCuesForTest(env, cues); });
  }

  void SimulateCurrentCuesForTest(
      const CueSnapshot& cues) override {
    WithEnv([&](JNIEnv* env) { bridge_->SimulateCurrentCuesForTest(env, cues); });
  }

  void SimulateAnalyticsMetadataForTest(
      const AnalyticsMetadataEvent& metadata) override {
    WithEnv([&](JNIEnv* env) { bridge_->SimulateAnalyticsMetadataForTest(env, metadata); });
  }

  void SimulateAnalyticsLoadErrorForTest(
      const AnalyticsLoadErrorEvent& load_error) override {
    WithEnv([&](JNIEnv* env) { bridge_->SimulateAnalyticsLoadErrorForTest(env, load_error); });
  }

  void SimulateAnalyticsDeviceInfoChangedForTest(
      const DeviceInfoDescriptor& device_info) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsDeviceInfoChangedForTest(env, device_info);
    });
  }

  void SimulateAnalyticsMediaMetadataChangedForTest(
      const MediaMetadataSnapshot& metadata) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsMediaMetadataChangedForTest(env, metadata);
    });
  }

  void SimulateAnalyticsPlaylistMetadataChangedForTest(
      const MediaMetadataSnapshot& metadata) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAnalyticsPlaylistMetadataChangedForTest(env, metadata);
    });
  }

  void SimulateVideoInputFormatChangedForTest(
      const VideoInputFormatChangedEvent& video_input_format_changed) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateVideoInputFormatChangedForTest(env, video_input_format_changed);
    });
  }

  void SimulateAudioCodecParametersChangedForTest(
      const CodecParametersDescriptor& codec_parameters) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateAudioCodecParametersChangedForTest(env, codec_parameters);
    });
  }

  void SimulateVideoCodecParametersChangedForTest(
      const CodecParametersDescriptor& codec_parameters) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateVideoCodecParametersChangedForTest(env, codec_parameters);
    });
  }

  void SimulateVideoFrameAboutToBeRenderedForTest(
      const VideoFrameMetadataSnapshot& video_frame_metadata) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateVideoFrameAboutToBeRenderedForTest(env, video_frame_metadata);
    });
  }

  void SimulateCameraMotionForTest(
      const CameraMotionSnapshot& camera_motion) override {
    WithEnv([&](JNIEnv* env) {
      bridge_->SimulateCameraMotionForTest(env, camera_motion);
    });
  }

  void SimulateCameraMotionResetForTest() override {
    WithEnv([&](JNIEnv* env) { bridge_->SimulateCameraMotionResetForTest(env); });
  }

  void SimulateImageOutputForTest(const ImageFrameSnapshot& image_frame) override {
    WithEnv([&](JNIEnv* env) { bridge_->SimulateImageOutputForTest(env, image_frame); });
  }

  PlayerConfig::MediaSourceFactoryConfig GetMediaSourceFactoryConfig() override {
    return WithEnvOrDefault<PlayerConfig::MediaSourceFactoryConfig>(
        [&](JNIEnv* env) { return bridge_->GetMediaSourceFactoryConfig(env); });
  }

  int GetAvailableCommandCount() override {
    return WithEnvOrDefault<int>(
        [&](JNIEnv* env) { return bridge_->GetAvailableCommandCount(env); });
  }

  AvailableCommandsSnapshot GetAvailableCommands() override {
    return WithEnvOrDefault<AvailableCommandsSnapshot>(
        [&](JNIEnv* env) { return bridge_->GetAvailableCommands(env); });
  }

  TimelineDetailsSnapshot GetTimeline() override {
    return WithEnvOrDefault<TimelineDetailsSnapshot>([&](JNIEnv* env) {
      TimelineDetailsSnapshot timeline;
      timeline.summary = bridge_->GetTimelineSnapshot(env);
      timeline.windows = bridge_->GetTimelineWindows(env);
      timeline.periods = bridge_->GetTimelinePeriods(env);
      return timeline;
    });
  }

  TimelineSnapshot GetTimelineSnapshot() override {
    return WithEnvOrDefault<TimelineSnapshot>(
        [&](JNIEnv* env) { return bridge_->GetTimelineSnapshot(env); });
  }

  std::vector<TimelineWindowSnapshot> GetTimelineWindows() override {
    return WithEnvOrDefault<std::vector<TimelineWindowSnapshot>>(
        [&](JNIEnv* env) { return bridge_->GetTimelineWindows(env); });
  }

  std::vector<TimelinePeriodSnapshot> GetTimelinePeriods() override {
    return WithEnvOrDefault<std::vector<TimelinePeriodSnapshot>>(
        [&](JNIEnv* env) { return bridge_->GetTimelinePeriods(env); });
  }

  CueSnapshot GetCurrentCues() override {
    return WithEnvOrDefault<CueSnapshot>(
        [&](JNIEnv* env) { return bridge_->GetCurrentCues(env); });
  }

  std::string GetCurrentMediaItemDebugSummary() override {
    return WithEnvOrDefault<std::string>(
        [&](JNIEnv* env) { return bridge_->GetCurrentMediaItemDebugSummary(env); });
  }

  PlaybackSnapshot GetSnapshot() override {
    return WithEnvOrDefault<PlaybackSnapshot>(
        [&](JNIEnv* env) { return bridge_->GetSnapshot(env); });
  }

  ~ExoPlayerSdkPlayerImpl() override { Release(); }

 private:
  template <typename Fn>
  void WithEnv(Fn&& fn) {
    ScopedEnv env(java_vm_);
    if (!env.ok()) {
      return;
    }
    fn(env.env());
  }

  template <typename T, typename Fn>
  T WithEnvOrDefault(Fn&& fn, T fallback = T{}) {
    ScopedEnv env(java_vm_);
    if (!env.ok()) {
      return fallback;
    }
    return fn(env.env());
  }

  JavaVM* java_vm_;
  std::shared_ptr<ExoPlayerBridge> bridge_;
  std::unique_ptr<ForwardingPlayerListener> forwarding_listener_;
  std::unique_ptr<ForwardingImageOutputListener> forwarding_image_output_listener_;
  std::atomic<bool> released_{false};
};

}  // namespace

std::unique_ptr<ExoPlayerSdkPriorityTaskManager> ExoPlayerSdkPriorityTaskManager::Create(
    JNIEnv* env) {
  if (env == nullptr) {
    return nullptr;
  }
  jclass manager_class = env->FindClass("androidx/media3/common/PriorityTaskManager");
  if (manager_class == nullptr) {
    return nullptr;
  }
  jmethodID constructor = env->GetMethodID(manager_class, "<init>", "()V");
  if (constructor == nullptr) {
    env->DeleteLocalRef(manager_class);
    return nullptr;
  }
  jobject local_manager = env->NewObject(manager_class, constructor);
  if (local_manager == nullptr) {
    env->DeleteLocalRef(manager_class);
    return nullptr;
  }
  jobject global_manager = env->NewGlobalRef(local_manager);
  env->DeleteLocalRef(local_manager);
  env->DeleteLocalRef(manager_class);
  if (global_manager == nullptr) {
    return nullptr;
  }
  JavaVM* java_vm = nullptr;
  env->GetJavaVM(&java_vm);
  return std::make_unique<ExoPlayerSdkPriorityTaskManagerImpl>(java_vm, global_manager);
}

std::unique_ptr<ExoPlayerSdkPlayer> ExoPlayerSdkPlayer::Create(
    JNIEnv* env,
    jobject context,
    const PlayerConfig& config) {
  if (env == nullptr || context == nullptr) {
    return nullptr;
  }
  JavaVM* java_vm = nullptr;
  if (env->GetJavaVM(&java_vm) != JNI_OK || java_vm == nullptr) {
    return nullptr;
  }
  std::shared_ptr<ExoPlayerBridge> bridge = ExoPlayerBridge::Create(env, context, config);
  if (bridge == nullptr) {
    return nullptr;
  }
  return std::make_unique<ExoPlayerSdkPlayerImpl>(java_vm, std::move(bridge));
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetHandleAudioFocus(
    bool handle_audio_focus) {
  config_.handle_audio_focus = handle_audio_focus;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetHandleAudioBecomingNoisy(
    bool handle_audio_becoming_noisy) {
  config_.handle_audio_becoming_noisy = handle_audio_becoming_noisy;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetUseLazyPreparation(
    bool use_lazy_preparation) {
  config_.use_lazy_preparation = use_lazy_preparation;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetSeekBackIncrementMs(
    int64_t seek_back_increment_ms) {
  config_.seek_back_increment_ms = seek_back_increment_ms;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetSeekForwardIncrementMs(
    int64_t seek_forward_increment_ms) {
  config_.seek_forward_increment_ms = seek_forward_increment_ms;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetWakeMode(int wake_mode) {
  config_.wake_mode = wake_mode;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetPriority(int priority) {
  config_.priority = priority;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetUsePriorityTaskManager(
    bool use_priority_task_manager) {
  config_.use_priority_task_manager = use_priority_task_manager;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetTargetPreloadDurationUs(
    int64_t target_preload_duration_us) {
  config_.target_preload_duration_us = target_preload_duration_us;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetParseSubtitlesDuringExtraction(
    bool parse_subtitles_during_extraction) {
  config_.media_source_factory_config.parse_subtitles_during_extraction =
      parse_subtitles_during_extraction;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetLoadOnlySelectedTracks(
    bool load_only_selected_tracks) {
  config_.media_source_factory_config.load_only_selected_tracks = load_only_selected_tracks;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetDefaultRequestHeaders(
    const std::vector<std::string>& header_names,
    const std::vector<std::string>& header_values) {
  config_.media_source_factory_config.default_request_header_names = header_names;
  config_.media_source_factory_config.default_request_header_values = header_values;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetUserAgent(
    const std::string& user_agent) {
  config_.media_source_factory_config.user_agent = user_agent;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetConnectTimeoutMs(
    int connect_timeout_ms) {
  config_.media_source_factory_config.connect_timeout_ms = connect_timeout_ms;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetReadTimeoutMs(int read_timeout_ms) {
  config_.media_source_factory_config.read_timeout_ms = read_timeout_ms;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetAllowCrossProtocolRedirects(
    bool allow_cross_protocol_redirects) {
  config_.media_source_factory_config.allow_cross_protocol_redirects =
      allow_cross_protocol_redirects;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetLiveTargetOffsetMs(
    int64_t live_target_offset_ms) {
  config_.media_source_factory_config.live_target_offset_ms = live_target_offset_ms;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetLiveOffsetsMs(
    int64_t live_min_offset_ms,
    int64_t live_max_offset_ms) {
  config_.media_source_factory_config.live_min_offset_ms = live_min_offset_ms;
  config_.media_source_factory_config.live_max_offset_ms = live_max_offset_ms;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetLiveSpeeds(
    float live_min_speed,
    float live_max_speed) {
  config_.media_source_factory_config.live_min_speed = live_min_speed;
  config_.media_source_factory_config.live_max_speed = live_max_speed;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetMediaSourceFactoryToken(
    const std::string& factory_token) {
  config_.media_source_factory_config.factory_token = factory_token;
  return *this;
}

ExoPlayerSdkPlayerBuilder& ExoPlayerSdkPlayerBuilder::SetMediaSourceFactoryConfig(
    const PlayerConfig::MediaSourceFactoryConfig& media_source_factory_config) {
  config_.media_source_factory_config = media_source_factory_config;
  return *this;
}

const PlayerConfig& ExoPlayerSdkPlayerBuilder::GetConfig() const {
  return config_;
}

std::unique_ptr<ExoPlayerSdkPlayer> ExoPlayerSdkPlayerBuilder::Build(
    JNIEnv* env,
    jobject context) const {
  return ExoPlayerSdkPlayer::Create(env, context, config_);
}

}  // namespace androidx::media3::cppbridge
