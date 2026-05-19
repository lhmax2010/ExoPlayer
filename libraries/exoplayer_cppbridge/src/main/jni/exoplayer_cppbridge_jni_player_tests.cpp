#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string_view>
#include <thread>
#include <utility>

#include "exoplayer_cppbridge_jni_internal.h"

using namespace androidx::media3::cppbridge;
using namespace androidx::media3::cppbridge::internal;

#define CAPTURE_CALLBACK_NAME __func__

#define CAPTURING_LISTENER_LOCK()                                                \
  ScopedCaptureLock scoped_capture_lock(this, CAPTURE_CALLBACK_NAME);             \
  if (!scoped_capture_lock.should_capture()) return

#define CAPTURING_LISTENER_LOCK_NAMED(name_literal)                               \
  ScopedCaptureLock scoped_capture_lock(this, name_literal);                      \
  if (!scoped_capture_lock.should_capture()) return

namespace {

constexpr char kListenerSmokeBuildMarker[] = "listener-smoke-v2026-04-02-13";

void LogListenerSmokeBuildMarker(const char* test_label) {
  LogInfo(std::string(test_label) + " buildMarker=" + kListenerSmokeBuildMarker);
}

std::string BuildPointerSummary(const void* value) {
  return std::to_string(
      static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(value)));
}

void AppendLittleEndian16(std::string* output, uint16_t value) {
  output->push_back(static_cast<char>(value & 0xFF));
  output->push_back(static_cast<char>((value >> 8) & 0xFF));
}

void AppendLittleEndian32(std::string* output, uint32_t value) {
  output->push_back(static_cast<char>(value & 0xFF));
  output->push_back(static_cast<char>((value >> 8) & 0xFF));
  output->push_back(static_cast<char>((value >> 16) & 0xFF));
  output->push_back(static_cast<char>((value >> 24) & 0xFF));
}

std::string Base64Encode(const std::string& input) {
  static constexpr char kAlphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string output;
  output.reserve(((input.size() + 2) / 3) * 4);
  for (size_t i = 0; i < input.size(); i += 3) {
    uint32_t chunk = static_cast<unsigned char>(input[i]) << 16;
    if (i + 1 < input.size()) {
      chunk |= static_cast<unsigned char>(input[i + 1]) << 8;
    }
    if (i + 2 < input.size()) {
      chunk |= static_cast<unsigned char>(input[i + 2]);
    }
    output.push_back(kAlphabet[(chunk >> 18) & 0x3F]);
    output.push_back(kAlphabet[(chunk >> 12) & 0x3F]);
    output.push_back(i + 1 < input.size() ? kAlphabet[(chunk >> 6) & 0x3F] : '=');
    output.push_back(i + 2 < input.size() ? kAlphabet[chunk & 0x3F] : '=');
  }
  return output;
}

std::string BuildSilentWavDataUri() {
  constexpr uint16_t kChannels = 1;
  constexpr uint16_t kBitsPerSample = 16;
  constexpr uint32_t kSampleRate = 8000;
  constexpr uint32_t kDurationMs = 2000;
  constexpr uint16_t kBlockAlign = kChannels * kBitsPerSample / 8;
  constexpr uint32_t kByteRate = kSampleRate * kBlockAlign;
  const uint32_t sample_count = kSampleRate * kDurationMs / 1000;
  const uint32_t data_size = sample_count * kBlockAlign;
  std::string wav;
  wav.reserve(44 + data_size);
  wav.append("RIFF", 4);
  AppendLittleEndian32(&wav, 36 + data_size);
  wav.append("WAVEfmt ", 8);
  AppendLittleEndian32(&wav, 16);
  AppendLittleEndian16(&wav, 1);
  AppendLittleEndian16(&wav, kChannels);
  AppendLittleEndian32(&wav, kSampleRate);
  AppendLittleEndian32(&wav, kByteRate);
  AppendLittleEndian16(&wav, kBlockAlign);
  AppendLittleEndian16(&wav, kBitsPerSample);
  wav.append("data", 4);
  AppendLittleEndian32(&wav, data_size);
  wav.append(data_size, '\0');
  return "data:audio/wav;base64," + Base64Encode(wav);
}

bool WaitForPlaybackState(
    ExoPlayerSdkPlayer* player,
    PlaybackState expected_state,
    int timeout_ms) {
  if (player == nullptr) {
    return false;
  }
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    if (player->GetPlaybackState() == expected_state) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return player->GetPlaybackState() == expected_state;
}

bool WaitForCurrentPositionAtLeast(
    ExoPlayerSdkPlayer* player,
    int64_t minimum_position_ms,
    int timeout_ms) {
  if (player == nullptr) {
    return false;
  }
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    if (player->GetCurrentPosition() >= minimum_position_ms) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return player->GetCurrentPosition() >= minimum_position_ms;
}

bool WaitForPlaybackReadyOrEnded(ExoPlayerSdkPlayer* player, int timeout_ms) {
  if (player == nullptr) {
    return false;
  }
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    PlaybackState state = player->GetPlaybackState();
    if (state == PlaybackState::kReady || state == PlaybackState::kEnded) {
      return true;
    }
    if (player->GetPlayerError().error_code != 0) {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  PlaybackState state = player->GetPlaybackState();
  return state == PlaybackState::kReady || state == PlaybackState::kEnded;
}

bool WaitForPlaybackAdvancedOrEnded(
    ExoPlayerSdkPlayer* player,
    int64_t minimum_position_ms,
    int timeout_ms) {
  if (player == nullptr) {
    return false;
  }
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    if (player->GetCurrentPosition() >= minimum_position_ms ||
        player->GetPlaybackState() == PlaybackState::kEnded) {
      return true;
    }
    if (player->GetPlayerError().error_code != 0) {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return player->GetCurrentPosition() >= minimum_position_ms ||
      player->GetPlaybackState() == PlaybackState::kEnded;
}

struct StreamPlaybackScenario {
  std::string label;
  std::string uri;
  std::string media_id;
  std::string mime_type;
  MediaSourceType source_type = MediaSourceType::kDefault;
};

void AppendStreamPlaybackScenarioSummary(
    ExoPlayerSdkPlayer* player,
    const StreamPlaybackScenario& scenario,
    std::string* summary) {
  if (player == nullptr || summary == nullptr) {
    return;
  }
  MediaItemDescriptor media_item;
  media_item.uri = scenario.uri;
  media_item.media_id = scenario.media_id;
  media_item.mime_type = scenario.mime_type;
  media_item.source_type = scenario.source_type;

  player->SetMediaItem(media_item);
  player->SetVolume(0.0f);
  player->Prepare();
  bool prepared = WaitForPlaybackReadyOrEnded(player, 7000);
  if (prepared) {
    player->Play();
  }
  bool advanced = prepared && WaitForPlaybackAdvancedOrEnded(player, 100, 7000);
  MediaItemDescriptor current_item = player->GetCurrentMediaItem();
  PlayerError error = player->GetPlayerError();

  *summary += scenario.label + "Prepared=" + std::to_string(prepared ? 1 : 0);
  *summary += "," + scenario.label + "Advanced=" + std::to_string(advanced ? 1 : 0);
  *summary += "," + scenario.label + "State=" +
      std::to_string(static_cast<int>(player->GetPlaybackState()));
  *summary += "," + scenario.label + "PositionMs=" +
      std::to_string(player->GetCurrentPosition());
  *summary += "," + scenario.label + "DurationMs=" +
      std::to_string(player->GetDuration());
  *summary += "," + scenario.label + "SourceType=" +
      std::to_string(static_cast<int>(current_item.source_type));
  *summary += "," + scenario.label + "MimeType=" + current_item.mime_type;
  *summary += "," + scenario.label + "ErrorCode=" + std::to_string(error.error_code);
  if (!error.message.empty()) {
    *summary += "," + scenario.label + "ErrorMessage=" + error.message;
  }

  player->Stop();
  player->ClearMediaItems();
}

std::string BuildRuntimeDrivenPlayerSummary(
    ExoPlayerSdkPlayer* player,
    const MediaItemDescriptor& media_item,
    bool start_playback,
    int timeout_ms) {
  if (player == nullptr) {
    return "runtimePlayerReady=0,prepared=0,playWhenReady=0,playbackState=-1";
  }
  player->SetMediaItem(media_item);
  player->Prepare();
  bool prepared = WaitForPlaybackState(player, PlaybackState::kBuffering, timeout_ms) ||
      WaitForPlaybackState(player, PlaybackState::kReady, timeout_ms);
  if (start_playback) {
    player->Play();
    WaitForPlaybackState(player, PlaybackState::kReady, timeout_ms);
  }
  PlaybackState playback_state = player->GetPlaybackState();
  std::string summary = "runtimePlayerReady=1";
  summary += ",prepared=" + std::to_string(prepared ? 1 : 0);
  summary += ",playWhenReady=" + std::to_string(player->GetPlayWhenReady() ? 1 : 0);
  summary += ",playbackState=" + std::to_string(static_cast<int>(playback_state));
  return summary;
}

bool RegisterBundleForOpaqueToken(
    JNIEnv* env,
    const std::string& token,
    const std::string& key,
    const std::string& value) {
  if (token.empty()) {
    return false;
  }
  jclass bundle_class = FindClassChecked(env, "android/os/Bundle");
  jmethodID bundle_ctor = GetMethodChecked(env, bundle_class, "android/os/Bundle", "<init>", "()V");
  jobject bundle = NewObjectChecked(env, bundle_class, bundle_ctor, "Bundle()");
  jmethodID put_string = GetMethodChecked(
      env,
      bundle_class,
      "android/os/Bundle",
      "putString",
      "(Ljava/lang/String;Ljava/lang/String;)V");
  jstring key_string = NewStringUtfChecked(env, key, "Bundle key");
  jstring value_string = NewStringUtfChecked(env, value, "Bundle value");
  if (bundle == nullptr || put_string == nullptr || key_string == nullptr || value_string == nullptr) {
    DeleteLocalRefIfNotNull(env, key_string);
    DeleteLocalRefIfNotNull(env, value_string);
    DeleteLocalRefIfNotNull(env, bundle);
    DeleteLocalRefIfNotNull(env, bundle_class);
    return false;
  }
  env->CallVoidMethod(bundle, put_string, key_string, value_string);
  if (ClearJniExceptionIfPresent(env, "Bundle.putString")) {
    DeleteLocalRefIfNotNull(env, key_string);
    DeleteLocalRefIfNotNull(env, value_string);
    DeleteLocalRefIfNotNull(env, bundle);
    DeleteLocalRefIfNotNull(env, bundle_class);
    return false;
  }

  jclass registry_class =
      FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppOpaqueObjectRegistry");
  if (registry_class == nullptr) {
    DeleteLocalRefIfNotNull(env, key_string);
    DeleteLocalRefIfNotNull(env, value_string);
    DeleteLocalRefIfNotNull(env, bundle);
    DeleteLocalRefIfNotNull(env, bundle_class);
    return false;
  }
  jmethodID register_method = env->GetStaticMethodID(
      registry_class, "register", "(Ljava/lang/String;Ljava/lang/Object;)V");
  if (ClearJniExceptionIfPresent(env, "CppOpaqueObjectRegistry.register") ||
      register_method == nullptr) {
    DeleteLocalRefIfNotNull(env, key_string);
    DeleteLocalRefIfNotNull(env, value_string);
    DeleteLocalRefIfNotNull(env, bundle);
    DeleteLocalRefIfNotNull(env, bundle_class);
    DeleteLocalRefIfNotNull(env, registry_class);
    return false;
  }
  jstring token_string = NewStringUtfChecked(env, token, "Opaque registry token");
  if (token_string == nullptr) {
    DeleteLocalRefIfNotNull(env, key_string);
    DeleteLocalRefIfNotNull(env, value_string);
    DeleteLocalRefIfNotNull(env, bundle);
    DeleteLocalRefIfNotNull(env, bundle_class);
    DeleteLocalRefIfNotNull(env, registry_class);
    return false;
  }
  env->CallStaticVoidMethod(registry_class, register_method, token_string, bundle);
  bool failed = ClearJniExceptionIfPresent(env, "CppOpaqueObjectRegistry.register(token, object)");
  DeleteLocalRefIfNotNull(env, token_string);
  DeleteLocalRefIfNotNull(env, key_string);
  DeleteLocalRefIfNotNull(env, value_string);
  DeleteLocalRefIfNotNull(env, bundle);
  DeleteLocalRefIfNotNull(env, bundle_class);
  DeleteLocalRefIfNotNull(env, registry_class);
  return !failed;
}

int ByteVectorChecksum(const std::vector<uint8_t>& values) {
  int checksum = 0;
  for (uint8_t value : values) {
    checksum += static_cast<int>(value);
  }
  return checksum;
}

void AppendBundleValueSummary(
    std::string* summary,
    const std::string& prefix,
    const std::vector<BundleValueInfo>& values) {
  if (summary == nullptr) {
    return;
  }
  *summary += "," + prefix + "ValueCount=" + std::to_string(values.size());
  for (size_t i = 0; i < values.size(); ++i) {
    const BundleValueInfo& value = values[i];
    *summary += "," + prefix + "Value" + std::to_string(i) + "Key=" + value.key;
    *summary += "," + prefix + "Value" + std::to_string(i) + "Type=" +
        std::to_string(value.value_type);
    switch (value.value_type) {
      case BundleValueInfo::kString:
        *summary += "," + prefix + "Value" + std::to_string(i) + "String=" +
            value.string_value;
        break;
      case BundleValueInfo::kLong:
        *summary += "," + prefix + "Value" + std::to_string(i) + "Long=" +
            std::to_string(value.long_value);
        break;
      case BundleValueInfo::kDouble:
        *summary += "," + prefix + "Value" + std::to_string(i) + "Double=" +
            std::to_string(value.double_value);
        break;
      case BundleValueInfo::kBoolean:
        *summary += "," + prefix + "Value" + std::to_string(i) + "Bool=" +
            std::to_string(value.boolean_value ? 1 : 0);
        break;
      case BundleValueInfo::kByteArray:
        *summary += "," + prefix + "Value" + std::to_string(i) + "Bytes=" +
            std::to_string(value.byte_array_value.size()) + ":" +
            std::to_string(ByteVectorChecksum(value.byte_array_value));
        break;
      default:
        break;
    }
  }
}

void AppendObjectValueSummary(
    std::string* summary,
    const std::string& prefix,
    const ObjectValueInfo& value) {
  if (summary == nullptr) {
    return;
  }
  *summary += "," + prefix + "Present=" + std::to_string(value.present ? 1 : 0);
  *summary += "," + prefix + "Class=" + value.class_name;
  *summary += "," + prefix + "Type=" + std::to_string(value.value_type);
  switch (value.value_type) {
    case ObjectValueInfo::kString:
    case ObjectValueInfo::kOther:
      *summary += "," + prefix + "String=" + value.string_value;
      break;
    case ObjectValueInfo::kLong:
      *summary += "," + prefix + "Long=" + std::to_string(value.long_value);
      break;
    case ObjectValueInfo::kDouble:
      *summary += "," + prefix + "Double=" + std::to_string(value.double_value);
      break;
    case ObjectValueInfo::kBoolean:
      *summary += "," + prefix + "Bool=" + std::to_string(value.boolean_value ? 1 : 0);
      break;
    default:
      break;
  }
}

}  // namespace

class CapturingImageOutputListener : public ExoPlayerSdkImageOutputListener {
 public:
  void OnImageAvailable(const ImageFrameSnapshot& image_frame) override {
    image_count++;
    last_presentation_time_us = image_frame.presentation_time_us;
    last_width = image_frame.width;
    last_height = image_frame.height;
    last_byte_count = image_frame.byte_count;
    last_allocation_byte_count = image_frame.allocation_byte_count;
    last_row_bytes = image_frame.row_bytes;
    last_has_alpha = image_frame.has_alpha;
    last_is_premultiplied = image_frame.is_premultiplied;
    last_is_mutable = image_frame.is_mutable;
    last_bitmap_config = image_frame.bitmap_config;
  }

  void OnDisabled() override { disabled_count++; }

  int image_count = 0;
  int64_t last_presentation_time_us = 0;
  int last_width = 0;
  int last_height = 0;
  int last_byte_count = 0;
  int last_allocation_byte_count = 0;
  int last_row_bytes = 0;
  bool last_has_alpha = false;
  bool last_is_premultiplied = false;
  bool last_is_mutable = false;
  std::string last_bitmap_config;
  int disabled_count = 0;
};

class CapturingPlayerListener : public PlayerListener {
 public:
  static constexpr uint64_t kObjectCanaryHeadValue = 0x4c534d4b48454144ULL;
  static constexpr uint64_t kObjectCanaryTailValue = 0x4c534d4b5441494cULL;

  CapturingPlayerListener() {
    ResetSmokeSignalState();
    LogInfo(
        "CapturingPlayerListener ctor canaryOk=" +
        std::to_string(HasValidObjectCanaries() ? 1 : 0) +
        ",canaryHead=" + std::to_string(object_canary_head) +
        ",canaryTail=" + std::to_string(object_canary_tail));
  }

  class ScopedCaptureLock {
   public:
    ScopedCaptureLock(CapturingPlayerListener* listener, const char* callback_name)
        : listener_(listener),
          callback_name_(callback_name),
          should_capture_(listener != nullptr && listener->ShouldCaptureCallback(callback_name)),
          lock_(listener->mutex, std::defer_lock),
          start_time_(std::chrono::steady_clock::now()) {
      if (should_capture_) {
        LogInfo(
            std::string("listenerCallback dispatch name=") +
            (callback_name_ != nullptr ? callback_name_ : "<none>") +
            ",listenerPtr=" + BuildPointerSummary(listener_) +
            ",canaryOk=" + std::to_string(listener_->HasValidObjectCanaries() ? 1 : 0) +
            ",canaryHead=" + std::to_string(listener_->object_canary_head) +
            ",canaryTail=" + std::to_string(listener_->object_canary_tail));
        lock_.lock();
        listener_->active_callback.store(callback_name_, std::memory_order_release);
      }
      if (should_capture_) {
        LogInfo(
            std::string("listenerCallback enter name=") +
            (callback_name_ != nullptr ? callback_name_ : "<none>"));
      }
    }

    ~ScopedCaptureLock() {
      if (should_capture_) {
        const auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time_);
        listener_->active_callback.store(nullptr, std::memory_order_release);
        lock_.unlock();
        LogInfo(
            std::string("listenerCallback exit name=") +
            (callback_name_ != nullptr ? callback_name_ : "<none>") +
            ",durationMs=" + std::to_string(duration_ms.count()));
      }
    }

    bool should_capture() const { return should_capture_; }

   private:
    CapturingPlayerListener* listener_;
    const char* callback_name_;
    bool should_capture_;
    std::unique_lock<std::recursive_mutex> lock_;
    std::chrono::steady_clock::time_point start_time_;
  };

  void SetCaptureAnalyticsCallbacks(bool capture) {
    capture_analytics_callbacks.store(capture, std::memory_order_release);
    LogInfo(
        "CapturingPlayerListener SetCaptureAnalyticsCallbacks capture=" +
        std::to_string(capture ? 1 : 0) + ",listenerPtr=" + BuildPointerSummary(this));
  }

  void ResetSmokeSignalState() {
    capture_analytics_callbacks.store(true, std::memory_order_release);
    smoke_repeat_callback_count.store(0, std::memory_order_release);
    smoke_shuffle_callback_count.store(0, std::memory_order_release);
    smoke_seek_back_increment_callback_count.store(0, std::memory_order_release);
    smoke_seek_forward_increment_callback_count.store(0, std::memory_order_release);
    smoke_max_seek_to_previous_position_callback_count.store(0, std::memory_order_release);
    smoke_track_selection_callback_count.store(0, std::memory_order_release);
    smoke_playback_parameters_callback_count.store(0, std::memory_order_release);
    smoke_playback_suppression_reason_callback_count.store(0, std::memory_order_release);
    smoke_available_commands_callback_count.store(0, std::memory_order_release);
    smoke_events_callback_count.store(0, std::memory_order_release);
    smoke_timeline_callback_count.store(0, std::memory_order_release);
    smoke_tracks_changed_callback_count.store(0, std::memory_order_release);
    smoke_media_metadata_callback_count.store(0, std::memory_order_release);
    smoke_playlist_metadata_callback_count.store(0, std::memory_order_release);
    smoke_cue_callback_count.store(0, std::memory_order_release);
    smoke_position_discontinuity_callback_count.store(0, std::memory_order_release);
    smoke_is_loading_callback_count.store(0, std::memory_order_release);
    smoke_timeline_window_count.store(0, std::memory_order_release);
    smoke_timeline_period_count.store(0, std::memory_order_release);
    smoke_cue_count.store(0, std::memory_order_release);
    smoke_preferred_text_language_seen.store(false, std::memory_order_release);
    smoke_media_metadata_title_seen.store(false, std::memory_order_release);
    smoke_playlist_metadata_title_seen.store(false, std::memory_order_release);
    smoke_new_position_media_id_seen.store(false, std::memory_order_release);
    active_callback.store(nullptr, std::memory_order_release);
    object_canary_head = kObjectCanaryHeadValue;
    object_canary_tail = kObjectCanaryTailValue;
  }

  bool HasValidObjectCanaries() const {
    return object_canary_head == kObjectCanaryHeadValue &&
        object_canary_tail == kObjectCanaryTailValue;
  }

  bool ShouldCaptureCallback(const char* callback_name) const {
    return capture_analytics_callbacks.load(std::memory_order_acquire) ||
        !IsAnalyticsCallbackName(callback_name);
  }

  static bool IsAnalyticsCallbackName(const char* callback_name) {
    if (callback_name == nullptr) {
      return false;
    }
    static constexpr const char* kAnalyticsCallbackNames[] = {
        "OnAnalyticsUpdated",
        "OnAudioUnderrun",
        "OnDroppedVideoFrames",
        "OnBandwidthEstimate",
        "OnLoadStarted",
        "OnLoadCompleted",
        "OnAnalyticsLoadError",
        "OnAudioInputFormatChanged",
        "OnAudioDecoderInitialized",
        "OnVideoDecoderInitialized",
        "OnAudioDecoderReleased",
        "OnVideoDecoderReleased",
        "OnAnalyticsRenderedFirstFrame",
        "OnAnalyticsVideoSizeChanged",
        "OnAudioPositionAdvancing",
        "OnVideoFrameProcessingOffset",
        "OnVolumeChanged",
        "OnAudioSessionIdChanged",
        "OnAnalyticsAudioAttributesChanged",
        "OnAnalyticsSkipSilenceEnabledChanged",
        "OnAnalyticsDeviceVolumeChanged",
        "OnAnalyticsPlaybackStateChanged",
        "OnAnalyticsIsPlayingChanged",
        "OnAnalyticsPlayWhenReadyChanged",
        "OnAnalyticsPlaybackSuppressionReasonChanged",
        "OnAnalyticsIsLoadingChanged",
        "OnAnalyticsRepeatModeChanged",
        "OnAnalyticsShuffleModeChanged",
        "OnAnalyticsPlaybackParametersChanged",
        "OnAnalyticsAvailableCommandsChanged",
        "OnAnalyticsEvents",
        "OnAnalyticsSeekBackIncrementChanged",
        "OnAnalyticsSeekForwardIncrementChanged",
        "OnAnalyticsMaxSeekToPreviousPositionChanged",
        "OnAnalyticsTimelineChanged",
        "OnAnalyticsPositionDiscontinuity",
        "OnAnalyticsSeekStarted",
        "OnAnalyticsPlayerError",
        "OnAnalyticsPlayerErrorChanged",
        "OnAnalyticsTracksChanged",
        "OnAnalyticsMediaItemTransition",
        "OnAnalyticsCues",
        "OnAnalyticsMetadata",
        "OnAnalyticsDeviceInfoChanged",
        "OnAnalyticsMediaMetadataChanged",
        "OnAnalyticsPlaylistMetadataChanged",
        "OnVideoInputFormatChanged",
        "OnAnalyticsPlayerStateChanged",
        "OnAnalyticsLoadingChanged",
        "OnAnalyticsTrackSelectionParametersChanged",
        "OnAnalyticsLoadCanceled",
        "OnAnalyticsDownstreamFormatChanged",
        "OnAnalyticsUpstreamDiscarded",
        "OnAnalyticsAudioEnabled",
        "OnAnalyticsAudioDisabled",
        "OnAnalyticsAudioSinkError",
        "OnAnalyticsAudioCodecError",
        "OnAnalyticsAudioTrackInitialized",
        "OnAnalyticsAudioTrackReleased",
        "OnAnalyticsVideoEnabled",
        "OnAnalyticsVideoDisabled",
        "OnAnalyticsVideoCodecError",
        "OnAnalyticsSurfaceSizeChanged",
        "OnAnalyticsDrmSessionAcquired",
        "OnAnalyticsDrmKeysLoaded",
        "OnAnalyticsDrmSessionManagerError",
        "OnAnalyticsDrmKeysRestored",
        "OnAnalyticsDrmKeysRemoved",
        "OnAnalyticsDrmSessionReleased",
        "OnAnalyticsRendererReadyChanged",
        "OnAnalyticsDroppedSeeksWhileScrubbing",
        "OnAnalyticsPlayerReleased",
    };
    for (const char* name : kAnalyticsCallbackNames) {
      if (std::string_view(callback_name).find(name) != std::string_view::npos) {
        return true;
      }
    }
    return false;
  }

  void OnPlaybackStateChanged(const PlaybackSnapshot&) override {}
  void OnPlayWhenReadyChanged(const PlaybackSnapshot&, int) override {}
  void OnIsPlayingChanged(const PlaybackSnapshot&) override {}
  void OnIsLoadingChanged(const PlaybackSnapshot& snapshot) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnIsLoadingChanged");
    is_loading = snapshot.is_loading;
    is_loading_callback_count++;
    smoke_is_loading_callback_count.fetch_add(1, std::memory_order_release);
  }
  void OnMediaItemTransition(const PlaybackSnapshot&, int) override {}
  void OnPlayerError(const PlaybackSnapshot&) override {}

  void OnPlayerErrorChanged(const PlaybackSnapshot& snapshot) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnPlayerErrorChanged");
    player_error_changed_code = snapshot.last_error.error_code;
    player_error_changed_callback_count++;
  }

  void OnRepeatModeChanged(const PlaybackSnapshot& snapshot) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnRepeatModeChanged");
    repeat_mode = static_cast<int>(snapshot.repeat_mode);
    repeat_callback_count++;
    smoke_repeat_callback_count.fetch_add(1, std::memory_order_release);
  }

  void OnShuffleModeEnabledChanged(const PlaybackSnapshot& snapshot) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnShuffleModeEnabledChanged");
    shuffle_enabled = snapshot.shuffle_mode_enabled;
    shuffle_callback_count++;
    smoke_shuffle_callback_count.fetch_add(1, std::memory_order_release);
  }

  void OnSeekBackIncrementChanged(
      const PlaybackSnapshot&,
      int64_t seek_back_increment_ms) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnSeekBackIncrementChanged");
    this->seek_back_increment_ms = seek_back_increment_ms;
    seek_back_increment_callback_count++;
    smoke_seek_back_increment_callback_count.fetch_add(1, std::memory_order_release);
  }

  void OnSeekForwardIncrementChanged(
      const PlaybackSnapshot&,
      int64_t seek_forward_increment_ms) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnSeekForwardIncrementChanged");
    this->seek_forward_increment_ms = seek_forward_increment_ms;
    seek_forward_increment_callback_count++;
    smoke_seek_forward_increment_callback_count.fetch_add(1, std::memory_order_release);
  }

  void OnMaxSeekToPreviousPositionChanged(
      const PlaybackSnapshot&,
      int64_t max_seek_to_previous_position_ms) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnMaxSeekToPreviousPositionChanged");
    this->max_seek_to_previous_position_ms = max_seek_to_previous_position_ms;
    max_seek_to_previous_position_callback_count++;
    smoke_max_seek_to_previous_position_callback_count.fetch_add(
        1, std::memory_order_release);
  }

  void OnTrackSelectionParametersChanged(
      const PlaybackSnapshot&,
      const TrackSelectionParametersDescriptor& parameters) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnTrackSelectionParametersChanged");
    preferred_text_language = parameters.preferred_text_language;
    track_selection_callback_count++;
    smoke_track_selection_callback_count.fetch_add(1, std::memory_order_release);
    if (!parameters.preferred_text_language.empty()) {
      smoke_preferred_text_language_seen.store(true, std::memory_order_release);
    }
  }

  void OnPlaybackParametersChanged(
      const PlaybackSnapshot&,
      const PlaybackParametersSnapshot& parameters) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnPlaybackParametersChanged");
    playback_speed = parameters.speed;
    playback_pitch = parameters.pitch;
    playback_parameters_callback_count++;
    smoke_playback_parameters_callback_count.fetch_add(1, std::memory_order_release);
  }

  void OnPlaybackSuppressionReasonChanged(
      const PlaybackSnapshot&,
      PlaybackSuppressionReason suppression_reason) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnPlaybackSuppressionReasonChanged");
    playback_suppression_reason = static_cast<int>(suppression_reason);
    playback_suppression_reason_callback_count++;
    smoke_playback_suppression_reason_callback_count.fetch_add(1, std::memory_order_release);
  }

  void OnAvailableCommandsChanged(
      const PlaybackSnapshot&,
      const AvailableCommandsSnapshot& commands) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAvailableCommandsChanged");
    available_commands_count = static_cast<int>(commands.command_codes.size());
    available_commands_callback_count++;
    smoke_available_commands_callback_count.fetch_add(1, std::memory_order_release);
  }

  void OnEvents(const PlaybackSnapshot&, const PlayerEventsSnapshot& events) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnEvents");
    last_event_count = static_cast<int>(events.event_codes.size());
    events_callback_count++;
    smoke_events_callback_count.fetch_add(1, std::memory_order_release);
  }

  void OnTimelineChanged(
      const PlaybackSnapshot&,
      const TimelineDetailsSnapshot& timeline,
      int reason) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnTimelineChanged");
    timeline_window_count = timeline.summary.window_count;
    timeline_period_count = timeline.summary.period_count;
    timeline_empty = timeline.summary.empty;
    timeline_current_media_item_index = timeline.summary.current_media_item_index;
    timeline_change_reason = reason;
    first_timeline_window_is_live =
        !timeline.windows.empty() && timeline.windows[0].is_live;
    first_timeline_window_is_dynamic =
        !timeline.windows.empty() && timeline.windows[0].is_dynamic;
    first_timeline_window_media_item_id =
        timeline.windows.empty() ? "" : timeline.windows[0].media_item_id;
    first_timeline_window_media_item_uri =
        timeline.windows.empty() ? "" : timeline.windows[0].media_item_uri;
    first_timeline_window_media_item_tag_present =
        !timeline.windows.empty() && timeline.windows[0].media_item_tag_present;
    first_timeline_window_media_item_tag_string =
        timeline.windows.empty() ? "" : timeline.windows[0].media_item_tag_string;
    first_timeline_window_media_item_tag_token_present =
        !timeline.windows.empty() && !timeline.windows[0].media_item_tag_token.empty();
    first_timeline_window_uid =
        timeline.windows.empty() ? "" : timeline.windows[0].uid;
    first_timeline_window_uid_token_present =
        !timeline.windows.empty() && !timeline.windows[0].uid_token.empty();
    first_timeline_window_live_configuration_present =
        !timeline.windows.empty() && timeline.windows[0].live_configuration_present;
    first_timeline_window_live_target_offset_ms =
        timeline.windows.empty() ? 0 : timeline.windows[0].live_target_offset_ms;
    first_timeline_window_live_min_offset_ms =
        timeline.windows.empty() ? 0 : timeline.windows[0].live_min_offset_ms;
    first_timeline_window_live_max_offset_ms =
        timeline.windows.empty() ? 0 : timeline.windows[0].live_max_offset_ms;
    first_timeline_window_live_min_playback_speed =
        timeline.windows.empty() ? 0.0f : timeline.windows[0].live_min_playback_speed;
    first_timeline_window_live_max_playback_speed =
        timeline.windows.empty() ? 0.0f : timeline.windows[0].live_max_playback_speed;
    first_timeline_window_manifest_present =
        !timeline.windows.empty() && timeline.windows[0].manifest_present;
    first_timeline_window_manifest_string =
        timeline.windows.empty() ? "" : timeline.windows[0].manifest_string;
    first_timeline_window_manifest_token_present =
        !timeline.windows.empty() && !timeline.windows[0].manifest_token.empty();
    first_timeline_window_presentation_start_time_ms =
        timeline.windows.empty() ? 0 : timeline.windows[0].presentation_start_time_ms;
    first_timeline_window_default_position_us =
        timeline.windows.empty() ? 0 : timeline.windows[0].default_position_us;
    second_timeline_window_media_item_index =
        timeline.windows.size() > 1 ? timeline.windows[1].media_item_index : 0;
    second_timeline_window_media_item_id =
        timeline.windows.size() > 1 ? timeline.windows[1].media_item_id : "";
    second_timeline_window_media_item_uri =
        timeline.windows.size() > 1 ? timeline.windows[1].media_item_uri : "";
    second_timeline_window_media_item_tag_present =
        timeline.windows.size() > 1 && timeline.windows[1].media_item_tag_present;
    second_timeline_window_media_item_tag_string =
        timeline.windows.size() > 1 ? timeline.windows[1].media_item_tag_string : "";
    second_timeline_window_media_item_tag_token_present =
        timeline.windows.size() > 1 && !timeline.windows[1].media_item_tag_token.empty();
    second_timeline_window_uid =
        timeline.windows.size() > 1 ? timeline.windows[1].uid : "";
    second_timeline_window_uid_token_present =
        timeline.windows.size() > 1 && !timeline.windows[1].uid_token.empty();
    second_timeline_window_live_configuration_present =
        timeline.windows.size() > 1 && timeline.windows[1].live_configuration_present;
    second_timeline_window_live_target_offset_ms =
        timeline.windows.size() > 1 ? timeline.windows[1].live_target_offset_ms : 0;
    second_timeline_window_live_min_offset_ms =
        timeline.windows.size() > 1 ? timeline.windows[1].live_min_offset_ms : 0;
    second_timeline_window_live_max_offset_ms =
        timeline.windows.size() > 1 ? timeline.windows[1].live_max_offset_ms : 0;
    second_timeline_window_live_min_playback_speed =
        timeline.windows.size() > 1 ? timeline.windows[1].live_min_playback_speed : 0.0f;
    second_timeline_window_live_max_playback_speed =
        timeline.windows.size() > 1 ? timeline.windows[1].live_max_playback_speed : 0.0f;
    second_timeline_window_manifest_present =
        timeline.windows.size() > 1 && timeline.windows[1].manifest_present;
    second_timeline_window_manifest_string =
        timeline.windows.size() > 1 ? timeline.windows[1].manifest_string : "";
    second_timeline_window_manifest_token_present =
        timeline.windows.size() > 1 && !timeline.windows[1].manifest_token.empty();
    second_timeline_window_first_period_index =
        timeline.windows.size() > 1 ? timeline.windows[1].first_period_index : 0;
    second_timeline_window_last_period_index =
        timeline.windows.size() > 1 ? timeline.windows[1].last_period_index : 0;
    second_timeline_window_presentation_start_time_ms =
        timeline.windows.size() > 1 ? timeline.windows[1].presentation_start_time_ms : 0;
    second_timeline_window_window_start_time_ms =
        timeline.windows.size() > 1 ? timeline.windows[1].window_start_time_ms : 0;
    second_timeline_window_elapsed_realtime_epoch_offset_ms =
        timeline.windows.size() > 1 ? timeline.windows[1].elapsed_realtime_epoch_offset_ms : 0;
    second_timeline_window_default_position_ms =
        timeline.windows.size() > 1 ? timeline.windows[1].default_position_ms : 0;
    second_timeline_window_default_position_us =
        timeline.windows.size() > 1 ? timeline.windows[1].default_position_us : 0;
    second_timeline_window_duration_ms =
        timeline.windows.size() > 1 ? timeline.windows[1].duration_ms : 0;
    second_timeline_window_duration_us =
        timeline.windows.size() > 1 ? timeline.windows[1].duration_us : 0;
    second_timeline_window_is_seekable =
        timeline.windows.size() > 1 && timeline.windows[1].is_seekable;
    second_timeline_window_is_dynamic =
        timeline.windows.size() > 1 && timeline.windows[1].is_dynamic;
    second_timeline_window_is_live =
        timeline.windows.size() > 1 && timeline.windows[1].is_live;
    second_timeline_window_is_placeholder =
        timeline.windows.size() > 1 && timeline.windows[1].is_placeholder;
    first_timeline_period_id =
        timeline.periods.empty() ? "" : timeline.periods[0].id;
    first_timeline_period_id_token_present =
        !timeline.periods.empty() && !timeline.periods[0].id_token.empty();
    first_timeline_period_uid =
        timeline.periods.empty() ? "" : timeline.periods[0].uid;
    first_timeline_period_uid_token_present =
        !timeline.periods.empty() && !timeline.periods[0].uid_token.empty();
    first_timeline_period_ads_id =
        timeline.periods.empty() ? "" : timeline.periods[0].ads_id;
    first_timeline_period_ads_id_token_present =
        !timeline.periods.empty() && !timeline.periods[0].ads_id_token.empty();
    first_timeline_period_ad_group_count =
        timeline.periods.empty() ? 0 : timeline.periods[0].ad_group_count;
    first_timeline_period_duration_ms =
        timeline.periods.empty() ? 0 : timeline.periods[0].duration_ms;
    first_timeline_period_duration_us =
        timeline.periods.empty() ? 0 : timeline.periods[0].duration_us;
    first_timeline_period_position_in_window_us =
        timeline.periods.empty() ? 0 : timeline.periods[0].position_in_window_us;
    second_timeline_period_id =
        timeline.periods.size() > 1 ? timeline.periods[1].id : "";
    second_timeline_period_id_token_present =
        timeline.periods.size() > 1 && !timeline.periods[1].id_token.empty();
    second_timeline_period_uid =
        timeline.periods.size() > 1 ? timeline.periods[1].uid : "";
    second_timeline_period_uid_token_present =
        timeline.periods.size() > 1 && !timeline.periods[1].uid_token.empty();
    second_timeline_period_ads_id =
        timeline.periods.size() > 1 ? timeline.periods[1].ads_id : "";
    second_timeline_period_ads_id_token_present =
        timeline.periods.size() > 1 && !timeline.periods[1].ads_id_token.empty();
    second_timeline_period_duration_us =
        timeline.periods.size() > 1 ? timeline.periods[1].duration_us : 0;
    timeline_callback_count++;
    smoke_timeline_callback_count.fetch_add(1, std::memory_order_release);
    smoke_timeline_window_count.store(timeline.summary.window_count, std::memory_order_release);
    smoke_timeline_period_count.store(timeline.summary.period_count, std::memory_order_release);
  }

  void OnTracksChanged(
      const PlaybackSnapshot&,
      const TracksSnapshot& tracks) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnTracksChanged");
    track_group_count = static_cast<int>(tracks.groups.size());
    first_track_group_type = tracks.groups.empty() ? -1 : tracks.groups[0].type;
    first_track_group_id = tracks.groups.empty() ? "" : tracks.groups[0].id;
    first_track_group_token_present =
        !tracks.groups.empty() && !tracks.groups[0].group_token.empty();
    first_track_count =
        tracks.groups.empty() ? 0 : static_cast<int>(tracks.groups[0].tracks.size());
    first_track_language =
        tracks.groups.empty() || tracks.groups[0].tracks.empty() ? "" : tracks.groups[0].tracks[0].language;
    first_track_label =
        tracks.groups.empty() || tracks.groups[0].tracks.empty() ? "" : tracks.groups[0].tracks[0].label;
    first_track_label_token_present =
        !tracks.groups.empty() && !tracks.groups[0].tracks.empty() &&
        !tracks.groups[0].tracks[0].label_token.empty();
    first_track_mime_type =
        tracks.groups.empty() || tracks.groups[0].tracks.empty() ? "" : tracks.groups[0].tracks[0].mime_type;
    first_track_accessibility_channel =
        tracks.groups.empty() || tracks.groups[0].tracks.empty()
            ? 0
            : tracks.groups[0].tracks[0].accessibility_channel;
    first_track_role_flags =
        tracks.groups.empty() || tracks.groups[0].tracks.empty() ? 0 : tracks.groups[0].tracks[0].role_flags;
    first_track_selection_flags =
        tracks.groups.empty() || tracks.groups[0].tracks.empty()
            ? 0
            : tracks.groups[0].tracks[0].selection_flags;
    first_track_selected =
        !tracks.groups.empty() && !tracks.groups[0].tracks.empty() && tracks.groups[0].tracks[0].selected;
    first_track_supported =
        !tracks.groups.empty() && !tracks.groups[0].tracks.empty() && tracks.groups[0].tracks[0].supported;
    first_track_supported_within_capabilities =
        !tracks.groups.empty() && !tracks.groups[0].tracks.empty() &&
        tracks.groups[0].tracks[0].supported_within_capabilities;
    second_track_group_id = tracks.groups.size() > 1 ? tracks.groups[1].id : "";
    second_track_group_token_present =
        tracks.groups.size() > 1 && !tracks.groups[1].group_token.empty();
    second_track_count =
        tracks.groups.size() > 1 ? static_cast<int>(tracks.groups[1].tracks.size()) : 0;
    second_track_label =
        tracks.groups.size() > 1 && !tracks.groups[1].tracks.empty()
            ? tracks.groups[1].tracks[0].label
            : "";
    second_track_label_token_present =
        tracks.groups.size() > 1 && !tracks.groups[1].tracks.empty() &&
        !tracks.groups[1].tracks[0].label_token.empty();
    second_track_language =
        tracks.groups.size() > 1 && !tracks.groups[1].tracks.empty()
            ? tracks.groups[1].tracks[0].language
            : "";
    second_track_mime_type =
        tracks.groups.size() > 1 && !tracks.groups[1].tracks.empty()
            ? tracks.groups[1].tracks[0].mime_type
            : "";
    second_track_accessibility_channel =
        tracks.groups.size() > 1 && !tracks.groups[1].tracks.empty()
            ? tracks.groups[1].tracks[0].accessibility_channel
            : 0;
    second_track_role_flags =
        tracks.groups.size() > 1 && !tracks.groups[1].tracks.empty()
            ? tracks.groups[1].tracks[0].role_flags
            : 0;
    second_track_selection_flags =
        tracks.groups.size() > 1 && !tracks.groups[1].tracks.empty()
            ? tracks.groups[1].tracks[0].selection_flags
            : 0;
    second_track_selected =
        tracks.groups.size() > 1 && !tracks.groups[1].tracks.empty() &&
        tracks.groups[1].tracks[0].selected;
    second_track_supported =
        tracks.groups.size() > 1 && !tracks.groups[1].tracks.empty() &&
        tracks.groups[1].tracks[0].supported;
    second_track_supported_within_capabilities =
        tracks.groups.size() > 1 && !tracks.groups[1].tracks.empty() &&
        tracks.groups[1].tracks[0].supported_within_capabilities;
    contains_audio = tracks.contains_audio;
    contains_video = tracks.contains_video;
    contains_text = tracks.contains_text;
    audio_selected = tracks.audio_selected;
    video_selected = tracks.video_selected;
    text_selected = tracks.text_selected;
    audio_supported = tracks.audio_supported;
    video_supported = tracks.video_supported;
    text_supported = tracks.text_supported;
    video_supported_allowing_exceeds = tracks.video_supported_allowing_exceeds_capabilities;
    tracks_changed_callback_count++;
    smoke_tracks_changed_callback_count.fetch_add(1, std::memory_order_release);
  }

  void OnPositionDiscontinuity(
      const PlaybackSnapshot&,
      const PositionInfoSnapshot& old_position,
      const PositionInfoSnapshot& new_position,
      int reason) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnPositionDiscontinuity");
    old_position_media_item_index = old_position.media_item_index;
    old_position_period_index = old_position.period_index;
    old_position_position_ms = old_position.position_ms;
    old_position_content_position_ms = old_position.content_position_ms;
    old_position_ad_group_index = old_position.ad_group_index;
    old_position_ad_index_in_ad_group = old_position.ad_index_in_ad_group;
    old_position_media_id = old_position.media_item.media_id;
    old_position_tag_token_present = !old_position.media_item.tag_token.empty();
    new_position_media_item_index = new_position.media_item_index;
    new_position_period_index = new_position.period_index;
    new_position_position_ms = new_position.position_ms;
    new_position_content_position_ms = new_position.content_position_ms;
    new_position_ad_group_index = new_position.ad_group_index;
    new_position_ad_index_in_ad_group = new_position.ad_index_in_ad_group;
    new_position_media_id = new_position.media_item.media_id;
    new_position_tag_token_present = !new_position.media_item.tag_token.empty();
    position_discontinuity_reason = reason;
    position_discontinuity_callback_count++;
    smoke_position_discontinuity_callback_count.fetch_add(1, std::memory_order_release);
    if (!new_position.media_item.media_id.empty()) {
      smoke_new_position_media_id_seen.store(true, std::memory_order_release);
    }
  }

  void OnMediaMetadataChanged(
      const PlaybackSnapshot&,
      const MediaMetadataSnapshot& metadata) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnMediaMetadataChanged");
    const bool has_meaningful_metadata =
        !metadata.title.empty() || !metadata.artist.empty() ||
        !metadata.album_title.empty() || !metadata.display_title.empty() ||
        !metadata.subtitle.empty() || !metadata.description.empty() ||
        !metadata.writer.empty() || !metadata.author.empty() ||
        !metadata.composer.empty() || !metadata.conductor.empty() ||
        !metadata.compilation.empty() || !metadata.genre.empty() ||
        !metadata.station.empty() || metadata.extras_present ||
        !metadata.artwork_uri.empty() || !metadata.artwork_data.empty() ||
        metadata.media_type != -1;
    if (has_meaningful_metadata || media_metadata_callback_count == 0) {
      media_metadata_title = metadata.title;
      media_metadata_title_token_present = !metadata.title_token.empty();
      media_metadata_artist = metadata.artist;
      media_metadata_artist_token_present = !metadata.artist_token.empty();
      media_metadata_album_title = metadata.album_title;
      media_metadata_album_title_token_present = !metadata.album_title_token.empty();
      media_metadata_album_artist = metadata.album_artist;
      media_metadata_album_artist_token_present = !metadata.album_artist_token.empty();
      media_metadata_display_title = metadata.display_title;
      media_metadata_display_title_token_present = !metadata.display_title_token.empty();
      media_metadata_subtitle = metadata.subtitle;
      media_metadata_subtitle_token_present = !metadata.subtitle_token.empty();
      media_metadata_description = metadata.description;
      media_metadata_description_token_present = !metadata.description_token.empty();
      media_metadata_writer = metadata.writer;
      media_metadata_writer_token_present = !metadata.writer_token.empty();
      media_metadata_author = metadata.author;
      media_metadata_author_token_present = !metadata.author_token.empty();
      media_metadata_composer = metadata.composer;
      media_metadata_composer_token_present = !metadata.composer_token.empty();
      media_metadata_conductor = metadata.conductor;
      media_metadata_conductor_token_present = !metadata.conductor_token.empty();
      media_metadata_compilation = metadata.compilation;
      media_metadata_compilation_token_present = !metadata.compilation_token.empty();
      media_metadata_genre = metadata.genre;
      media_metadata_genre_token_present = !metadata.genre_token.empty();
      media_metadata_station = metadata.station;
      media_metadata_station_token_present = !metadata.station_token.empty();
      media_metadata_media_type = metadata.media_type;
      media_metadata_extras_present = metadata.extras_present;
      media_metadata_extras_key_count = metadata.extras_key_count;
      media_metadata_extras_token_present = !metadata.extras_token.empty();
      media_metadata_artwork_uri = metadata.artwork_uri;
      media_metadata_artwork_data_length = static_cast<int>(metadata.artwork_data.size());
      media_metadata_artwork_data_type = metadata.artwork_data_type;
    }
    media_metadata_callback_count++;
    smoke_media_metadata_callback_count.fetch_add(1, std::memory_order_release);
    if (!metadata.title.empty()) {
      smoke_media_metadata_title_seen.store(true, std::memory_order_release);
    }
  }

  void OnPlaylistMetadataChanged(
      const PlaybackSnapshot&,
      const MediaMetadataSnapshot& metadata) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnPlaylistMetadataChanged");
    playlist_metadata_title = metadata.title;
    playlist_metadata_title_token_present = !metadata.title_token.empty();
    playlist_metadata_artist = metadata.artist;
    playlist_metadata_artist_token_present = !metadata.artist_token.empty();
    playlist_metadata_album_title = metadata.album_title;
    playlist_metadata_album_title_token_present = !metadata.album_title_token.empty();
    playlist_metadata_album_artist = metadata.album_artist;
    playlist_metadata_album_artist_token_present = !metadata.album_artist_token.empty();
    playlist_metadata_display_title = metadata.display_title;
    playlist_metadata_display_title_token_present = !metadata.display_title_token.empty();
    playlist_metadata_subtitle = metadata.subtitle;
    playlist_metadata_subtitle_token_present = !metadata.subtitle_token.empty();
    playlist_metadata_description = metadata.description;
    playlist_metadata_description_token_present = !metadata.description_token.empty();
    playlist_metadata_writer = metadata.writer;
    playlist_metadata_writer_token_present = !metadata.writer_token.empty();
    playlist_metadata_author = metadata.author;
    playlist_metadata_author_token_present = !metadata.author_token.empty();
    playlist_metadata_composer = metadata.composer;
    playlist_metadata_composer_token_present = !metadata.composer_token.empty();
    playlist_metadata_conductor = metadata.conductor;
    playlist_metadata_conductor_token_present = !metadata.conductor_token.empty();
    playlist_metadata_compilation = metadata.compilation;
    playlist_metadata_compilation_token_present = !metadata.compilation_token.empty();
    playlist_metadata_genre = metadata.genre;
    playlist_metadata_genre_token_present = !metadata.genre_token.empty();
    playlist_metadata_station = metadata.station;
    playlist_metadata_station_token_present = !metadata.station_token.empty();
    playlist_metadata_media_type = metadata.media_type;
    playlist_metadata_extras_present = metadata.extras_present;
    playlist_metadata_extras_key_count = metadata.extras_key_count;
    playlist_metadata_extras_token_present = !metadata.extras_token.empty();
    playlist_metadata_artwork_uri = metadata.artwork_uri;
    playlist_metadata_artwork_data_length = static_cast<int>(metadata.artwork_data.size());
    playlist_metadata_artwork_data_type = metadata.artwork_data_type;
    playlist_metadata_callback_count++;
    smoke_playlist_metadata_callback_count.fetch_add(1, std::memory_order_release);
    if (!metadata.title.empty()) {
      smoke_playlist_metadata_title_seen.store(true, std::memory_order_release);
    }
  }

  void OnCues(
      const PlaybackSnapshot&,
      const CueSnapshot& cues) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnCues");
    cue_callback_count++;
    cue_count = cues.cue_count;
    cue_presentation_time_us = cues.presentation_time_us;
    cue0_text = cues.cues.empty() ? "" : cues.cues[0].text;
    cue0_text_token_present = !cues.cues.empty() && !cues.cues[0].text_token.empty();
    cue0_bitmap_token_present = !cues.cues.empty() && !cues.cues[0].bitmap_token.empty();
    cue0_text_alignment = cues.cues.empty() ? 0 : cues.cues[0].text_alignment;
    cue0_multi_row_alignment = cues.cues.empty() ? 0 : cues.cues[0].multi_row_alignment;
    cue0_line = cues.cues.empty() ? 0.0f : cues.cues[0].line;
    cue0_line_type = cues.cues.empty() ? 0 : cues.cues[0].line_type;
    cue0_position_anchor = cues.cues.empty() ? 0 : cues.cues[0].position_anchor;
    cue1_text = cues.cues.size() > 1 ? cues.cues[1].text : "";
    cue1_text_token_present = cues.cues.size() > 1 && !cues.cues[1].text_token.empty();
    cue1_bitmap_token_present = cues.cues.size() > 1 && !cues.cues[1].bitmap_token.empty();
    cue1_line_type = cues.cues.size() > 1 ? cues.cues[1].line_type : 0;
    cue1_position_anchor = cues.cues.size() > 1 ? cues.cues[1].position_anchor : 0;
    cue1_text_size = cues.cues.size() > 1 ? cues.cues[1].text_size : 0.0f;
    cue1_text_size_type = cues.cues.size() > 1 ? cues.cues[1].text_size_type : 0;
    cue1_vertical_type = cues.cues.size() > 1 ? cues.cues[1].vertical_type : 0;
    smoke_cue_callback_count.fetch_add(1, std::memory_order_release);
    smoke_cue_count.store(cues.cue_count, std::memory_order_release);
  }

  void OnAnalyticsUpdated(
      const PlaybackSnapshot&,
      const AnalyticsSnapshot& analytics) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsUpdated");
    analytics_bitrate_estimate = analytics.bitrate_estimate;
    analytics_dropped_video_frames = analytics.dropped_video_frames;
    analytics_load_started_count = analytics.load_started_count;
    analytics_load_completed_count = analytics.load_completed_count;
    analytics_audio_sample_mime_type = analytics.audio_sample_mime_type;
    analytics_video_sample_mime_type = analytics.video_sample_mime_type;
    analytics_callback_count++;
  }

  void OnAudioUnderrun(
      const PlaybackSnapshot&,
      const AudioUnderrunEvent& audio_underrun) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAudioUnderrun");
    audio_underrun_buffer_size = audio_underrun.buffer_size;
    audio_underrun_buffer_size_ms = audio_underrun.buffer_size_ms;
    audio_underrun_elapsed_since_last_feed_ms =
        audio_underrun.elapsed_since_last_feed_ms;
    audio_underrun_callback_count++;
  }

  void OnDroppedVideoFrames(
      const PlaybackSnapshot&,
      const DroppedVideoFramesEvent& dropped_video_frames) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnDroppedVideoFrames");
    dropped_video_frames_event_count = dropped_video_frames.dropped_frames;
    dropped_video_frames_elapsed_ms = dropped_video_frames.elapsed_ms;
    dropped_video_frames_callback_count++;
  }

  void OnBandwidthEstimate(
      const PlaybackSnapshot&,
      const BandwidthEstimateEvent& bandwidth_estimate) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnBandwidthEstimate");
    bandwidth_estimate_elapsed_ms = bandwidth_estimate.elapsed_ms;
    bandwidth_estimate_bytes_transferred = bandwidth_estimate.bytes_transferred;
    bandwidth_estimate_bitrate_estimate = bandwidth_estimate.bitrate_estimate;
    bandwidth_estimate_callback_count++;
  }

  void OnLoadStarted(
      const PlaybackSnapshot&,
      const LoadStartedEvent& load_started) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnLoadStarted");
    load_started_uri = load_started.uri;
    load_started_data_type = load_started.data_type;
    load_started_track_type = load_started.track_type;
    load_started_retry_count = load_started.retry_count;
    load_started_callback_count++;
  }

  void OnLoadCompleted(
      const PlaybackSnapshot&,
      const LoadCompletedEvent& load_completed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnLoadCompleted");
    load_completed_uri = load_completed.uri;
    load_completed_data_type = load_completed.data_type;
    load_completed_track_type = load_completed.track_type;
    load_completed_callback_count++;
  }

  void OnAudioInputFormatChanged(
      const PlaybackSnapshot&,
      const AudioInputFormatChangedEvent& audio_input_format_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAudioInputFormatChanged");
    audio_input_format_sample_mime_type = audio_input_format_changed.sample_mime_type;
    audio_input_format_codecs = audio_input_format_changed.codecs;
    audio_input_format_channel_count = audio_input_format_changed.channel_count;
    audio_input_format_sample_rate = audio_input_format_changed.sample_rate;
    audio_input_format_changed_callback_count++;
  }

  void OnAudioDecoderInitialized(
      const PlaybackSnapshot&,
      const AudioDecoderInitializedEvent& audio_decoder_initialized) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAudioDecoderInitialized");
    audio_decoder_initialized_decoder_name = audio_decoder_initialized.decoder_name;
    audio_decoder_initialized_timestamp_ms =
        audio_decoder_initialized.initialized_timestamp_ms;
    audio_decoder_initialized_duration_ms =
        audio_decoder_initialized.initialization_duration_ms;
    audio_decoder_initialized_callback_count++;
  }

  void OnVideoDecoderInitialized(
      const PlaybackSnapshot&,
      const VideoDecoderInitializedEvent& video_decoder_initialized) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnVideoDecoderInitialized");
    video_decoder_initialized_decoder_name = video_decoder_initialized.decoder_name;
    video_decoder_initialized_timestamp_ms =
        video_decoder_initialized.initialized_timestamp_ms;
    video_decoder_initialized_duration_ms =
        video_decoder_initialized.initialization_duration_ms;
    video_decoder_initialized_callback_count++;
  }

  void OnAudioDecoderReleased(
      const PlaybackSnapshot&,
      const AudioDecoderReleasedEvent& audio_decoder_released) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAudioDecoderReleased");
    audio_decoder_released_decoder_name = audio_decoder_released.decoder_name;
    audio_decoder_released_callback_count++;
  }

  void OnVideoDecoderReleased(
      const PlaybackSnapshot&,
      const VideoDecoderReleasedEvent& video_decoder_released) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnVideoDecoderReleased");
    video_decoder_released_decoder_name = video_decoder_released.decoder_name;
    video_decoder_released_callback_count++;
  }

  void OnAnalyticsRenderedFirstFrame(
      const PlaybackSnapshot&,
      const AnalyticsRenderedFirstFrameEvent& rendered_first_frame) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsRenderedFirstFrame");
    analytics_rendered_first_frame_render_time_ms = rendered_first_frame.render_time_ms;
    analytics_rendered_first_frame_callback_count++;
  }

  void OnAnalyticsVideoSizeChanged(
      const PlaybackSnapshot&,
      const AnalyticsVideoSizeChangedEvent& analytics_video_size) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsVideoSizeChanged");
    analytics_video_size_width = analytics_video_size.width;
    analytics_video_size_height = analytics_video_size.height;
    analytics_video_size_pixel_width_height_ratio =
        analytics_video_size.pixel_width_height_ratio;
    analytics_video_size_changed_callback_count++;
  }

  void OnAudioPositionAdvancing(
      const PlaybackSnapshot&,
      const AudioPositionAdvancingEvent& audio_position_advancing) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAudioPositionAdvancing");
    analytics_audio_position_advancing_playout_start_system_time_ms =
        audio_position_advancing.playout_start_system_time_ms;
    analytics_audio_position_advancing_callback_count++;
  }

  void OnVideoFrameProcessingOffset(
      const PlaybackSnapshot&,
      const VideoFrameProcessingOffsetEvent& video_frame_processing_offset) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnVideoFrameProcessingOffset");
    analytics_video_frame_processing_offset_total_processing_offset_us =
        video_frame_processing_offset.total_processing_offset_us;
    analytics_video_frame_processing_offset_frame_count =
        video_frame_processing_offset.frame_count;
    analytics_video_frame_processing_offset_callback_count++;
  }

  void OnVolumeChanged(
      const PlaybackSnapshot&,
      const VolumeChangedEvent& volume_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnVolumeChanged");
    analytics_volume_changed_volume = volume_changed.volume;
    analytics_volume_changed_callback_count++;
  }

  void OnAudioSessionIdChanged(
      const PlaybackSnapshot&,
      const AudioSessionIdChangedEvent& audio_session_id_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAudioSessionIdChanged");
    analytics_audio_session_id_changed_audio_session_id =
        audio_session_id_changed.audio_session_id;
    analytics_audio_session_id_changed_callback_count++;
  }

  void OnAnalyticsAudioAttributesChanged(
      const PlaybackSnapshot&,
      const AudioAttributesDescriptor& attributes) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsAudioAttributesChanged");
    analytics_audio_attributes_content_type = attributes.content_type;
    analytics_audio_attributes_usage = attributes.usage;
    analytics_audio_attributes_flags = attributes.flags;
    analytics_audio_attributes_allowed_capture_policy =
        attributes.allowed_capture_policy;
    analytics_audio_attributes_spatialization_behavior =
        attributes.spatialization_behavior;
    analytics_audio_attributes_changed_callback_count++;
  }

  void OnAnalyticsSkipSilenceEnabledChanged(
      const PlaybackSnapshot&,
      const AnalyticsSkipSilenceEnabledChangedEvent& skip_silence_enabled_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsSkipSilenceEnabledChanged");
    analytics_skip_silence_enabled_changed_skip_silence_enabled =
        skip_silence_enabled_changed.skip_silence_enabled;
    analytics_skip_silence_enabled_changed_callback_count++;
  }

  void OnAnalyticsDeviceVolumeChanged(
      const PlaybackSnapshot&,
      const AnalyticsDeviceVolumeChangedEvent& device_volume_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsDeviceVolumeChanged");
    analytics_device_volume_changed_volume = device_volume_changed.volume;
    analytics_device_volume_changed_muted = device_volume_changed.muted;
    analytics_device_volume_changed_callback_count++;
  }

  void OnAnalyticsPlaybackStateChanged(
      const PlaybackSnapshot&,
      const AnalyticsPlaybackStateChangedEvent& playback_state_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsPlaybackStateChanged");
    analytics_playback_state_changed_playback_state = playback_state_changed.playback_state;
    analytics_playback_state_changed_callback_count++;
  }

  void OnAnalyticsIsPlayingChanged(
      const PlaybackSnapshot&,
      const AnalyticsIsPlayingChangedEvent& is_playing_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsIsPlayingChanged");
    analytics_is_playing_changed_is_playing = is_playing_changed.is_playing;
    analytics_is_playing_changed_callback_count++;
  }

  void OnAnalyticsPlayWhenReadyChanged(
      const PlaybackSnapshot&,
      const AnalyticsPlayWhenReadyChangedEvent& play_when_ready_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsPlayWhenReadyChanged");
    analytics_play_when_ready_changed_play_when_ready =
        play_when_ready_changed.play_when_ready;
    analytics_play_when_ready_changed_reason = play_when_ready_changed.reason;
    analytics_play_when_ready_changed_callback_count++;
  }

  void OnAnalyticsPlaybackSuppressionReasonChanged(
      const PlaybackSnapshot&,
      const AnalyticsPlaybackSuppressionReasonChangedEvent& suppression_reason_changed)
      override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsPlaybackSuppressionReasonChanged");
    analytics_playback_suppression_reason_changed_reason =
        suppression_reason_changed.playback_suppression_reason;
    analytics_playback_suppression_reason_changed_callback_count++;
  }

  void OnAnalyticsIsLoadingChanged(
      const PlaybackSnapshot&,
      const AnalyticsIsLoadingChangedEvent& is_loading_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsIsLoadingChanged");
    analytics_is_loading_changed_is_loading = is_loading_changed.is_loading;
    analytics_is_loading_changed_callback_count++;
  }

  void OnAnalyticsRepeatModeChanged(
      const PlaybackSnapshot&,
      const AnalyticsRepeatModeChangedEvent& repeat_mode_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsRepeatModeChanged");
    analytics_repeat_mode_changed_repeat_mode = repeat_mode_changed.repeat_mode;
    analytics_repeat_mode_changed_callback_count++;
  }

  void OnAnalyticsShuffleModeChanged(
      const PlaybackSnapshot&,
      const AnalyticsShuffleModeChangedEvent& shuffle_mode_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsShuffleModeChanged");
    analytics_shuffle_mode_changed_shuffle_mode_enabled =
        shuffle_mode_changed.shuffle_mode_enabled;
    analytics_shuffle_mode_changed_callback_count++;
  }

  void OnAnalyticsPlaybackParametersChanged(
      const PlaybackSnapshot&,
      const AnalyticsPlaybackParametersChangedEvent& playback_parameters_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsPlaybackParametersChanged");
    analytics_playback_parameters_changed_speed = playback_parameters_changed.speed;
    analytics_playback_parameters_changed_pitch = playback_parameters_changed.pitch;
    analytics_playback_parameters_changed_callback_count++;
  }

  void OnAnalyticsAvailableCommandsChanged(
      const PlaybackSnapshot&,
      const AnalyticsAvailableCommandsChangedEvent& available_commands_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsAvailableCommandsChanged");
    analytics_available_commands_count =
        static_cast<int>(available_commands_changed.commands.size());
    analytics_available_commands_first_command =
        available_commands_changed.commands.empty() ? 0 : available_commands_changed.commands.front();
    analytics_available_commands_contains_8 =
        std::find(
            available_commands_changed.commands.begin(),
            available_commands_changed.commands.end(),
            8) != available_commands_changed.commands.end();
    analytics_available_commands_callback_count++;
  }

  void OnAnalyticsEvents(
      const PlaybackSnapshot&,
      const AnalyticsEventsEvent& analytics_events) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsEvents");
    bool contains_test_sentinel =
        std::find(
            analytics_events.event_codes.begin(),
            analytics_events.event_codes.end(),
            9009) != analytics_events.event_codes.end();
    if (!contains_test_sentinel) {
      return;
    }
    analytics_events_count = static_cast<int>(analytics_events.event_codes.size());
    analytics_events_first_event =
        analytics_events.event_codes.empty() ? 0 : analytics_events.event_codes.front();
    analytics_events_contains_9009 = contains_test_sentinel;
    analytics_events_callback_count++;
  }

  void OnAnalyticsSeekBackIncrementChanged(
      const PlaybackSnapshot&,
      const AnalyticsSeekBackIncrementChangedEvent& seek_back_increment_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsSeekBackIncrementChanged");
    analytics_seek_back_increment_changed_ms =
        seek_back_increment_changed.seek_back_increment_ms;
    analytics_seek_back_increment_changed_callback_count++;
  }

  void OnAnalyticsSeekForwardIncrementChanged(
      const PlaybackSnapshot&,
      const AnalyticsSeekForwardIncrementChangedEvent& seek_forward_increment_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsSeekForwardIncrementChanged");
    analytics_seek_forward_increment_changed_ms =
        seek_forward_increment_changed.seek_forward_increment_ms;
    analytics_seek_forward_increment_changed_callback_count++;
  }

  void OnAnalyticsMaxSeekToPreviousPositionChanged(
      const PlaybackSnapshot&,
      const AnalyticsMaxSeekToPreviousPositionChangedEvent&
          max_seek_to_previous_position_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsMaxSeekToPreviousPositionChanged");
    analytics_max_seek_to_previous_position_changed_ms =
        max_seek_to_previous_position_changed.max_seek_to_previous_position_ms;
    analytics_max_seek_to_previous_position_changed_callback_count++;
  }

  void OnAnalyticsTimelineChanged(
      const PlaybackSnapshot&,
      const AnalyticsTimelineChangedEvent& timeline_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsTimelineChanged");
    analytics_timeline_changed_reason = timeline_changed.reason;
    analytics_timeline_changed_callback_count++;
  }

  void OnAnalyticsPositionDiscontinuity(
      const PlaybackSnapshot&,
      const AnalyticsPositionDiscontinuityEvent& position_discontinuity) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsPositionDiscontinuity");
    analytics_position_discontinuity_reason = position_discontinuity.reason;
    analytics_position_discontinuity_callback_count++;
  }

  void OnAnalyticsSeekStarted(
      const PlaybackSnapshot&,
      const AnalyticsSeekStartedEvent&) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsSeekStarted");
    analytics_seek_started_started = true;
    analytics_seek_started_callback_count++;
  }

  void OnAnalyticsPlayerError(
      const PlaybackSnapshot&,
      const PlayerError& error) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsPlayerError");
    analytics_player_error_code = error.error_code;
    analytics_player_error_message = error.message;
    analytics_player_error_callback_count++;
  }

  void OnAnalyticsPlayerErrorChanged(
      const PlaybackSnapshot&,
      const PlayerError& error) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsPlayerErrorChanged");
    analytics_player_error_changed_code = error.error_code;
    analytics_player_error_changed_message = error.message;
    analytics_player_error_changed_callback_count++;
  }

  void OnAnalyticsTracksChanged(
      const PlaybackSnapshot&,
      const TracksSnapshot& tracks) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsTracksChanged");
    analytics_tracks_changed_group_count = static_cast<int>(tracks.groups.size());
    analytics_tracks_changed_first_group_type = tracks.groups.empty() ? -1 : tracks.groups[0].type;
    analytics_tracks_changed_first_group_id = tracks.groups.empty() ? "" : tracks.groups[0].id;
    analytics_tracks_changed_first_group_token_present =
        !tracks.groups.empty() && !tracks.groups[0].group_token.empty();
    analytics_tracks_changed_first_track_count =
        tracks.groups.empty() ? 0 : static_cast<int>(tracks.groups[0].tracks.size());
    analytics_tracks_changed_contains_audio = tracks.contains_audio;
    analytics_tracks_changed_contains_video = tracks.contains_video;
    analytics_tracks_changed_video_selected = tracks.video_selected;
    analytics_tracks_changed_callback_count++;
  }

  void OnAnalyticsMediaItemTransition(
      const PlaybackSnapshot&,
      const AnalyticsMediaItemTransitionEvent& media_item_transition) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsMediaItemTransition");
    analytics_media_item_transition_media_id = media_item_transition.media_item.media_id;
    analytics_media_item_transition_source_type =
        static_cast<int>(media_item_transition.media_item.source_type);
    analytics_media_item_transition_reason = media_item_transition.reason;
    analytics_media_item_transition_callback_count++;
  }

  void OnAnalyticsCues(
      const PlaybackSnapshot&,
      const CueSnapshot& cues) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsCues");
    analytics_cues_cue_count = cues.cue_count;
    analytics_cues_presentation_time_us = cues.presentation_time_us;
    analytics_cues_text0 = cues.texts.empty() ? "" : cues.texts[0];
    analytics_cues_text0_token_present =
        !cues.text_tokens.empty() && !cues.text_tokens[0].empty();
    analytics_cues_bitmap0_token_present =
        !cues.bitmap_tokens.empty() && !cues.bitmap_tokens[0].empty();
    analytics_cues_text1 = cues.texts.size() > 1 ? cues.texts[1] : "";
    analytics_cues_text1_token_present =
        cues.text_tokens.size() > 1 && !cues.text_tokens[1].empty();
    analytics_cues_bitmap1_token_present =
        cues.bitmap_tokens.size() > 1 && !cues.bitmap_tokens[1].empty();
    analytics_cues_callback_count++;
  }

  void OnAnalyticsMetadata(
      const PlaybackSnapshot&,
      const AnalyticsMetadataEvent& metadata) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsMetadata");
    analytics_metadata_entry_count = metadata.entry_count;
    analytics_metadata_first_entry_type = metadata.first_entry_type;
    analytics_metadata_first_entry_text = metadata.first_entry_text;
    analytics_metadata_callback_count++;
  }

  void OnAnalyticsLoadError(
      const PlaybackSnapshot&,
      const AnalyticsLoadErrorEvent& load_error) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsLoadError");
    analytics_load_error_uri = load_error.uri;
    analytics_load_error_data_type = load_error.data_type;
    analytics_load_error_track_type = load_error.track_type;
    analytics_load_error_message = load_error.message;
    analytics_load_error_was_canceled = load_error.was_canceled;
    analytics_load_error_callback_count++;
  }

  void OnAnalyticsDeviceInfoChanged(
      const PlaybackSnapshot&,
      const DeviceInfoDescriptor& device_info) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsDeviceInfoChanged");
    analytics_device_info_changed_playback_type = device_info.playback_type;
    analytics_device_info_changed_min_volume = device_info.min_volume;
    analytics_device_info_changed_max_volume = device_info.max_volume;
    analytics_device_info_changed_routing_controller_id = device_info.routing_controller_id;
    analytics_device_info_changed_callback_count++;
  }

  void OnAnalyticsMediaMetadataChanged(
      const PlaybackSnapshot&,
      const MediaMetadataSnapshot& metadata) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsMediaMetadataChanged");
    analytics_media_metadata_changed_title = metadata.title;
    analytics_media_metadata_changed_artist = metadata.artist;
    analytics_media_metadata_changed_display_title = metadata.display_title;
    analytics_media_metadata_changed_callback_count++;
  }

  void OnAnalyticsPlaylistMetadataChanged(
      const PlaybackSnapshot&,
      const MediaMetadataSnapshot& metadata) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsPlaylistMetadataChanged");
    analytics_playlist_metadata_changed_title = metadata.title;
    analytics_playlist_metadata_changed_artist = metadata.artist;
    analytics_playlist_metadata_changed_display_title = metadata.display_title;
    analytics_playlist_metadata_changed_callback_count++;
  }

  void OnVideoInputFormatChanged(
      const PlaybackSnapshot&,
      const VideoInputFormatChangedEvent& video_input_format_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnVideoInputFormatChanged");
    video_input_format_sample_mime_type = video_input_format_changed.sample_mime_type;
    video_input_format_codecs = video_input_format_changed.codecs;
    video_input_format_width = video_input_format_changed.width;
    video_input_format_height = video_input_format_changed.height;
    video_input_format_frame_rate = video_input_format_changed.frame_rate;
    video_input_format_changed_callback_count++;
  }

  void OnAnalyticsPlayerStateChanged(
      const PlaybackSnapshot&,
      const AnalyticsPlayerStateChangedEvent& player_state_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsPlayerStateChanged");
    analytics_player_state_changed_play_when_ready =
        player_state_changed.play_when_ready;
    analytics_player_state_changed_playback_state =
        player_state_changed.playback_state;
    analytics_player_state_changed_callback_count++;
  }

  void OnAnalyticsLoadingChanged(
      const PlaybackSnapshot&,
      const AnalyticsLoadingChangedEvent& loading_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsLoadingChanged");
    analytics_loading_changed_is_loading = loading_changed.is_loading;
    analytics_loading_changed_callback_count++;
  }

  void OnAnalyticsTrackSelectionParametersChanged(
      const PlaybackSnapshot&,
      const TrackSelectionParametersDescriptor& parameters) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsTrackSelectionParametersChanged");
    analytics_track_selection_changed_preferred_text_language =
        parameters.preferred_text_language;
    analytics_track_selection_changed_disable_text = parameters.disable_text;
    analytics_track_selection_changed_callback_count++;
  }

  void OnAnalyticsLoadCanceled(
      const PlaybackSnapshot&,
      const AnalyticsMediaLoadDataEvent& load_canceled) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsLoadCanceled");
    analytics_load_canceled_uri = load_canceled.uri;
    analytics_load_canceled_data_type = load_canceled.data_type;
    analytics_load_canceled_track_type = load_canceled.track_type;
    analytics_load_canceled_sample_mime_type = load_canceled.sample_mime_type;
    analytics_load_canceled_media_start_time_ms = load_canceled.media_start_time_ms;
    analytics_load_canceled_media_end_time_ms = load_canceled.media_end_time_ms;
    analytics_load_canceled_callback_count++;
  }

  void OnAnalyticsDownstreamFormatChanged(
      const PlaybackSnapshot&,
      const AnalyticsMediaLoadDataEvent& downstream_format_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsDownstreamFormatChanged");
    analytics_downstream_format_changed_track_type =
        downstream_format_changed.track_type;
    analytics_downstream_format_changed_sample_mime_type =
        downstream_format_changed.sample_mime_type;
    analytics_downstream_format_changed_callback_count++;
  }

  void OnAnalyticsUpstreamDiscarded(
      const PlaybackSnapshot&,
      const AnalyticsMediaLoadDataEvent& upstream_discarded) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsUpstreamDiscarded");
    analytics_upstream_discarded_track_type = upstream_discarded.track_type;
    analytics_upstream_discarded_sample_mime_type =
        upstream_discarded.sample_mime_type;
    analytics_upstream_discarded_callback_count++;
  }

  void OnAnalyticsAudioEnabled(
      const PlaybackSnapshot&,
      const AnalyticsDecoderCountersSnapshot& counters) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsAudioEnabled");
    analytics_audio_enabled_decoder_init_count = counters.decoder_init_count;
    analytics_audio_enabled_queued_input_buffer_count =
        counters.queued_input_buffer_count;
    analytics_audio_enabled_callback_count++;
  }

  void OnAnalyticsAudioDisabled(
      const PlaybackSnapshot&,
      const AnalyticsDecoderCountersSnapshot& counters) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsAudioDisabled");
    analytics_audio_disabled_decoder_release_count = counters.decoder_release_count;
    analytics_audio_disabled_dropped_buffer_count = counters.dropped_buffer_count;
    analytics_audio_disabled_callback_count++;
  }

  void OnAnalyticsAudioSinkError(
      const PlaybackSnapshot&,
      const AnalyticsExceptionEvent& error) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsAudioSinkError");
    analytics_audio_sink_error_class_name = error.class_name;
    analytics_audio_sink_error_message = error.message;
    analytics_audio_sink_error_callback_count++;
  }

  void OnAnalyticsAudioCodecError(
      const PlaybackSnapshot&,
      const AnalyticsExceptionEvent& error) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsAudioCodecError");
    analytics_audio_codec_error_class_name = error.class_name;
    analytics_audio_codec_error_message = error.message;
    analytics_audio_codec_error_callback_count++;
  }

  void OnAnalyticsAudioTrackInitialized(
      const PlaybackSnapshot&,
      const AnalyticsAudioTrackConfigSnapshot& audio_track_config) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsAudioTrackInitialized");
    analytics_audio_track_initialized_encoding = audio_track_config.encoding;
    analytics_audio_track_initialized_sample_rate = audio_track_config.sample_rate;
    analytics_audio_track_initialized_tunneling = audio_track_config.tunneling;
    analytics_audio_track_initialized_callback_count++;
  }

  void OnAnalyticsAudioTrackReleased(
      const PlaybackSnapshot&,
      const AnalyticsAudioTrackConfigSnapshot& audio_track_config) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsAudioTrackReleased");
    analytics_audio_track_released_encoding = audio_track_config.encoding;
    analytics_audio_track_released_sample_rate = audio_track_config.sample_rate;
    analytics_audio_track_released_offload = audio_track_config.offload;
    analytics_audio_track_released_callback_count++;
  }

  void OnAnalyticsVideoEnabled(
      const PlaybackSnapshot&,
      const AnalyticsDecoderCountersSnapshot& counters) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsVideoEnabled");
    analytics_video_enabled_decoder_init_count = counters.decoder_init_count;
    analytics_video_enabled_total_processing_offset_us =
        counters.total_video_frame_processing_offset_us;
    analytics_video_enabled_callback_count++;
  }

  void OnAnalyticsVideoDisabled(
      const PlaybackSnapshot&,
      const AnalyticsDecoderCountersSnapshot& counters) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsVideoDisabled");
    analytics_video_disabled_decoder_release_count = counters.decoder_release_count;
    analytics_video_disabled_processing_offset_count =
        counters.video_frame_processing_offset_count;
    analytics_video_disabled_callback_count++;
  }

  void OnAnalyticsVideoCodecError(
      const PlaybackSnapshot&,
      const AnalyticsExceptionEvent& error) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsVideoCodecError");
    analytics_video_codec_error_class_name = error.class_name;
    analytics_video_codec_error_message = error.message;
    analytics_video_codec_error_callback_count++;
  }

  void OnAnalyticsSurfaceSizeChanged(
      const PlaybackSnapshot&,
      int width,
      int height) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsSurfaceSizeChanged");
    analytics_surface_size_changed_width = width;
    analytics_surface_size_changed_height = height;
    analytics_surface_size_changed_callback_count++;
  }

  void OnAnalyticsDrmSessionAcquired(
      const PlaybackSnapshot&,
      const AnalyticsDrmSessionAcquiredEvent& drm_session_acquired) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsDrmSessionAcquired");
    analytics_drm_session_acquired_has_state = drm_session_acquired.has_state;
    analytics_drm_session_acquired_state = drm_session_acquired.state;
    analytics_drm_session_acquired_callback_count++;
  }

  void OnAnalyticsDrmKeysLoaded(
      const PlaybackSnapshot&,
      const AnalyticsDrmKeysLoadedEvent& drm_keys_loaded) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsDrmKeysLoaded");
    analytics_drm_keys_loaded_has_key_request_info =
        drm_keys_loaded.has_key_request_info;
    analytics_drm_keys_loaded_load_info_count = drm_keys_loaded.load_info_count;
    analytics_drm_keys_loaded_scheme_data_count = drm_keys_loaded.scheme_data_count;
    analytics_drm_keys_loaded_callback_count++;
  }

  void OnAnalyticsDrmSessionManagerError(
      const PlaybackSnapshot&,
      const AnalyticsExceptionEvent& error) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsDrmSessionManagerError");
    analytics_drm_session_manager_error_class_name = error.class_name;
    analytics_drm_session_manager_error_message = error.message;
    analytics_drm_session_manager_error_callback_count++;
  }

  void OnAnalyticsDrmKeysRestored(const PlaybackSnapshot&) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsDrmKeysRestored");
    analytics_drm_keys_restored_callback_count++;
  }

  void OnAnalyticsDrmKeysRemoved(const PlaybackSnapshot&) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsDrmKeysRemoved");
    analytics_drm_keys_removed_callback_count++;
  }

  void OnAnalyticsDrmSessionReleased(const PlaybackSnapshot&) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsDrmSessionReleased");
    analytics_drm_session_released_callback_count++;
  }

  void OnAnalyticsRendererReadyChanged(
      const PlaybackSnapshot&,
      const AnalyticsRendererReadyChangedEvent& renderer_ready_changed) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsRendererReadyChanged");
    analytics_renderer_ready_changed_renderer_index =
        renderer_ready_changed.renderer_index;
    analytics_renderer_ready_changed_track_type =
        renderer_ready_changed.renderer_track_type;
    analytics_renderer_ready_changed_is_ready =
        renderer_ready_changed.is_renderer_ready;
    analytics_renderer_ready_changed_callback_count++;
  }

  void OnAnalyticsDroppedSeeksWhileScrubbing(
      const PlaybackSnapshot&,
      const AnalyticsDroppedSeeksWhileScrubbingEvent& dropped_seeks) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsDroppedSeeksWhileScrubbing");
    analytics_dropped_seeks_while_scrubbing_dropped_seeks =
        dropped_seeks.dropped_seeks;
    analytics_dropped_seeks_while_scrubbing_callback_count++;
  }

  void OnAnalyticsPlayerReleased(const PlaybackSnapshot&) override {
    CAPTURING_LISTENER_LOCK_NAMED("OnAnalyticsPlayerReleased");
    analytics_player_released_callback_count++;
  }

  uint64_t object_canary_head = kObjectCanaryHeadValue;
  int repeat_mode = 0;
  bool shuffle_enabled = false;
  int64_t seek_back_increment_ms = 0;
  int64_t seek_forward_increment_ms = 0;
  int64_t max_seek_to_previous_position_ms = 0;
  std::string preferred_text_language;
  float playback_speed = 1.0f;
  float playback_pitch = 1.0f;
  int playback_suppression_reason = 0;
  int available_commands_count = 0;
  int player_error_changed_code = 0;
  int last_event_count = 0;
  int old_position_media_item_index = 0;
  int old_position_period_index = 0;
  int64_t old_position_position_ms = 0;
  int64_t old_position_content_position_ms = 0;
  int old_position_ad_group_index = -1;
  int old_position_ad_index_in_ad_group = -1;
  std::string old_position_media_id;
  bool old_position_tag_token_present = false;
  int new_position_media_item_index = 0;
  int new_position_period_index = 0;
  int64_t new_position_position_ms = 0;
  int64_t new_position_content_position_ms = 0;
  int new_position_ad_group_index = -1;
  int new_position_ad_index_in_ad_group = -1;
  std::string new_position_media_id;
  bool new_position_tag_token_present = false;
  int position_discontinuity_reason = 0;
  std::string media_metadata_title;
  bool media_metadata_title_token_present = false;
  std::string media_metadata_artist;
  bool media_metadata_artist_token_present = false;
  std::string media_metadata_album_title;
  bool media_metadata_album_title_token_present = false;
  std::string media_metadata_album_artist;
  bool media_metadata_album_artist_token_present = false;
  std::string media_metadata_display_title;
  bool media_metadata_display_title_token_present = false;
  std::string media_metadata_subtitle;
  bool media_metadata_subtitle_token_present = false;
  std::string media_metadata_description;
  bool media_metadata_description_token_present = false;
  std::string media_metadata_writer;
  bool media_metadata_writer_token_present = false;
  std::string media_metadata_author;
  bool media_metadata_author_token_present = false;
  std::string media_metadata_composer;
  bool media_metadata_composer_token_present = false;
  std::string media_metadata_conductor;
  bool media_metadata_conductor_token_present = false;
  std::string media_metadata_compilation;
  bool media_metadata_compilation_token_present = false;
  std::string media_metadata_genre;
  bool media_metadata_genre_token_present = false;
  std::string media_metadata_station;
  bool media_metadata_station_token_present = false;
  int media_metadata_media_type = 0;
  bool media_metadata_extras_present = false;
  int media_metadata_extras_key_count = 0;
  bool media_metadata_extras_token_present = false;
  std::string media_metadata_artwork_uri;
  int media_metadata_artwork_data_length = 0;
  int media_metadata_artwork_data_type = 0;
  std::string playlist_metadata_title;
  bool playlist_metadata_title_token_present = false;
  std::string playlist_metadata_artist;
  bool playlist_metadata_artist_token_present = false;
  std::string playlist_metadata_album_title;
  bool playlist_metadata_album_title_token_present = false;
  std::string playlist_metadata_album_artist;
  bool playlist_metadata_album_artist_token_present = false;
  std::string playlist_metadata_display_title;
  bool playlist_metadata_display_title_token_present = false;
  std::string playlist_metadata_subtitle;
  bool playlist_metadata_subtitle_token_present = false;
  std::string playlist_metadata_description;
  bool playlist_metadata_description_token_present = false;
  std::string playlist_metadata_writer;
  bool playlist_metadata_writer_token_present = false;
  std::string playlist_metadata_author;
  bool playlist_metadata_author_token_present = false;
  std::string playlist_metadata_composer;
  bool playlist_metadata_composer_token_present = false;
  std::string playlist_metadata_conductor;
  bool playlist_metadata_conductor_token_present = false;
  std::string playlist_metadata_compilation;
  bool playlist_metadata_compilation_token_present = false;
  std::string playlist_metadata_genre;
  bool playlist_metadata_genre_token_present = false;
  std::string playlist_metadata_station;
  bool playlist_metadata_station_token_present = false;
  int playlist_metadata_media_type = 0;
  bool playlist_metadata_extras_present = false;
  int playlist_metadata_extras_key_count = 0;
  bool playlist_metadata_extras_token_present = false;
  std::string playlist_metadata_artwork_uri;
  int playlist_metadata_artwork_data_length = 0;
  int playlist_metadata_artwork_data_type = 0;
  int64_t analytics_bitrate_estimate = 0;
  int analytics_dropped_video_frames = 0;
  int analytics_load_started_count = 0;
  int analytics_load_completed_count = 0;
  std::string analytics_audio_sample_mime_type;
  std::string analytics_video_sample_mime_type;
  int audio_underrun_buffer_size = 0;
  int64_t audio_underrun_buffer_size_ms = 0;
  int64_t audio_underrun_elapsed_since_last_feed_ms = 0;
  int dropped_video_frames_event_count = 0;
  int64_t dropped_video_frames_elapsed_ms = 0;
  int bandwidth_estimate_elapsed_ms = 0;
  int64_t bandwidth_estimate_bytes_transferred = 0;
  int64_t bandwidth_estimate_bitrate_estimate = 0;
  std::string load_started_uri;
  int load_started_data_type = 0;
  int load_started_track_type = 0;
  int load_started_retry_count = 0;
  std::string load_completed_uri;
  int load_completed_data_type = 0;
  int load_completed_track_type = 0;
  std::string audio_input_format_sample_mime_type;
  std::string audio_input_format_codecs;
  int audio_input_format_channel_count = 0;
  int audio_input_format_sample_rate = 0;
  std::string audio_decoder_initialized_decoder_name;
  int64_t audio_decoder_initialized_timestamp_ms = 0;
  int64_t audio_decoder_initialized_duration_ms = 0;
  std::string video_decoder_initialized_decoder_name;
  int64_t video_decoder_initialized_timestamp_ms = 0;
  int64_t video_decoder_initialized_duration_ms = 0;
  std::string audio_decoder_released_decoder_name;
  std::string video_decoder_released_decoder_name;
  int64_t analytics_rendered_first_frame_render_time_ms = 0;
  int analytics_video_size_width = 0;
  int analytics_video_size_height = 0;
  float analytics_video_size_pixel_width_height_ratio = 1.0f;
  int64_t analytics_audio_position_advancing_playout_start_system_time_ms = 0;
  int64_t analytics_video_frame_processing_offset_total_processing_offset_us = 0;
  int analytics_video_frame_processing_offset_frame_count = 0;
  float analytics_volume_changed_volume = 1.0f;
  int analytics_audio_session_id_changed_audio_session_id = 0;
  int analytics_audio_attributes_content_type = 0;
  int analytics_audio_attributes_usage = 0;
  int analytics_audio_attributes_flags = 0;
  int analytics_audio_attributes_allowed_capture_policy = 0;
  int analytics_audio_attributes_spatialization_behavior = 0;
  bool analytics_skip_silence_enabled_changed_skip_silence_enabled = false;
  int analytics_device_volume_changed_volume = 0;
  bool analytics_device_volume_changed_muted = false;
  int analytics_playback_state_changed_playback_state = 0;
  bool analytics_is_playing_changed_is_playing = false;
  bool analytics_play_when_ready_changed_play_when_ready = false;
  int analytics_play_when_ready_changed_reason = 0;
  int analytics_playback_suppression_reason_changed_reason = 0;
  bool analytics_is_loading_changed_is_loading = false;
  int analytics_repeat_mode_changed_repeat_mode = 0;
  bool analytics_shuffle_mode_changed_shuffle_mode_enabled = false;
  float analytics_playback_parameters_changed_speed = 1.0f;
  float analytics_playback_parameters_changed_pitch = 1.0f;
  int analytics_available_commands_count = 0;
  int analytics_available_commands_first_command = 0;
  bool analytics_available_commands_contains_8 = false;
  int analytics_events_count = 0;
  int analytics_events_first_event = 0;
  bool analytics_events_contains_9009 = false;
  int64_t analytics_seek_back_increment_changed_ms = 0;
  int64_t analytics_seek_forward_increment_changed_ms = 0;
  int64_t analytics_max_seek_to_previous_position_changed_ms = 0;
  int analytics_timeline_changed_reason = 0;
  int analytics_position_discontinuity_reason = 0;
  bool analytics_seek_started_started = false;
  int analytics_player_error_code = 0;
  std::string analytics_player_error_message;
  int analytics_player_error_changed_code = 0;
  std::string analytics_player_error_changed_message;
  int analytics_tracks_changed_group_count = 0;
  int analytics_tracks_changed_first_group_type = -1;
  std::string analytics_tracks_changed_first_group_id;
  bool analytics_tracks_changed_first_group_token_present = false;
  int analytics_tracks_changed_first_track_count = 0;
  bool analytics_tracks_changed_contains_audio = false;
  bool analytics_tracks_changed_contains_video = false;
  bool analytics_tracks_changed_video_selected = false;
  std::string analytics_media_item_transition_media_id;
  int analytics_media_item_transition_source_type = 0;
  int analytics_media_item_transition_reason = 0;
  int analytics_cues_cue_count = 0;
  int64_t analytics_cues_presentation_time_us = 0;
  std::string analytics_cues_text0;
  bool analytics_cues_text0_token_present = false;
  bool analytics_cues_bitmap0_token_present = false;
  std::string analytics_cues_text1;
  bool analytics_cues_text1_token_present = false;
  bool analytics_cues_bitmap1_token_present = false;
  int analytics_metadata_entry_count = 0;
  std::string analytics_metadata_first_entry_type;
  std::string analytics_metadata_first_entry_text;
  std::string analytics_load_error_uri;
  int analytics_load_error_data_type = 0;
  int analytics_load_error_track_type = 0;
  std::string analytics_load_error_message;
  bool analytics_load_error_was_canceled = false;
  int analytics_device_info_changed_playback_type = 0;
  int analytics_device_info_changed_min_volume = 0;
  int analytics_device_info_changed_max_volume = 0;
  std::string analytics_device_info_changed_routing_controller_id;
  std::string analytics_media_metadata_changed_title;
  std::string analytics_media_metadata_changed_artist;
  std::string analytics_media_metadata_changed_display_title;
  std::string analytics_playlist_metadata_changed_title;
  std::string analytics_playlist_metadata_changed_artist;
  std::string analytics_playlist_metadata_changed_display_title;
  std::string video_input_format_sample_mime_type;
  std::string video_input_format_codecs;
  int video_input_format_width = 0;
  int video_input_format_height = 0;
  float video_input_format_frame_rate = 0.0f;
  bool analytics_player_state_changed_play_when_ready = false;
  int analytics_player_state_changed_playback_state = 0;
  bool analytics_loading_changed_is_loading = false;
  std::string analytics_track_selection_changed_preferred_text_language;
  bool analytics_track_selection_changed_disable_text = false;
  std::string analytics_load_canceled_uri;
  int analytics_load_canceled_data_type = 0;
  int analytics_load_canceled_track_type = 0;
  std::string analytics_load_canceled_sample_mime_type;
  int64_t analytics_load_canceled_media_start_time_ms = 0;
  int64_t analytics_load_canceled_media_end_time_ms = 0;
  int analytics_downstream_format_changed_track_type = 0;
  std::string analytics_downstream_format_changed_sample_mime_type;
  int analytics_upstream_discarded_track_type = 0;
  std::string analytics_upstream_discarded_sample_mime_type;
  int analytics_audio_enabled_decoder_init_count = 0;
  int analytics_audio_enabled_queued_input_buffer_count = 0;
  int analytics_audio_disabled_decoder_release_count = 0;
  int analytics_audio_disabled_dropped_buffer_count = 0;
  std::string analytics_audio_sink_error_class_name;
  std::string analytics_audio_sink_error_message;
  std::string analytics_audio_codec_error_class_name;
  std::string analytics_audio_codec_error_message;
  int analytics_audio_track_initialized_encoding = 0;
  int analytics_audio_track_initialized_sample_rate = 0;
  bool analytics_audio_track_initialized_tunneling = false;
  int analytics_audio_track_released_encoding = 0;
  int analytics_audio_track_released_sample_rate = 0;
  bool analytics_audio_track_released_offload = false;
  int analytics_video_enabled_decoder_init_count = 0;
  int64_t analytics_video_enabled_total_processing_offset_us = 0;
  int analytics_video_disabled_decoder_release_count = 0;
  int analytics_video_disabled_processing_offset_count = 0;
  std::string analytics_video_codec_error_class_name;
  std::string analytics_video_codec_error_message;
  int analytics_surface_size_changed_width = 0;
  int analytics_surface_size_changed_height = 0;
  bool analytics_drm_session_acquired_has_state = false;
  int analytics_drm_session_acquired_state = 0;
  bool analytics_drm_keys_loaded_has_key_request_info = false;
  int analytics_drm_keys_loaded_load_info_count = 0;
  int analytics_drm_keys_loaded_scheme_data_count = 0;
  std::string analytics_drm_session_manager_error_class_name;
  std::string analytics_drm_session_manager_error_message;
  int analytics_renderer_ready_changed_renderer_index = 0;
  int analytics_renderer_ready_changed_track_type = 0;
  bool analytics_renderer_ready_changed_is_ready = false;
  int analytics_dropped_seeks_while_scrubbing_dropped_seeks = 0;
  int timeline_window_count = 0;
  int timeline_period_count = 0;
  bool timeline_empty = true;
  int timeline_current_media_item_index = 0;
  int timeline_change_reason = 0;
  bool first_timeline_window_is_live = false;
  bool first_timeline_window_is_dynamic = false;
  std::string first_timeline_window_media_item_id;
  std::string first_timeline_window_media_item_uri;
  bool first_timeline_window_media_item_tag_present = false;
  std::string first_timeline_window_media_item_tag_string;
  bool first_timeline_window_media_item_tag_token_present = false;
  std::string first_timeline_window_uid;
  bool first_timeline_window_uid_token_present = false;
  bool first_timeline_window_live_configuration_present = false;
  int64_t first_timeline_window_live_target_offset_ms = 0;
  int64_t first_timeline_window_live_min_offset_ms = 0;
  int64_t first_timeline_window_live_max_offset_ms = 0;
  float first_timeline_window_live_min_playback_speed = 0.0f;
  float first_timeline_window_live_max_playback_speed = 0.0f;
  bool first_timeline_window_manifest_present = false;
  std::string first_timeline_window_manifest_string;
  bool first_timeline_window_manifest_token_present = false;
  int64_t first_timeline_window_presentation_start_time_ms = 0;
  int64_t first_timeline_window_default_position_us = 0;
  std::string second_timeline_window_media_item_id;
  std::string second_timeline_window_media_item_uri;
  int second_timeline_window_media_item_index = 0;
  bool second_timeline_window_media_item_tag_present = false;
  std::string second_timeline_window_media_item_tag_string;
  bool second_timeline_window_media_item_tag_token_present = false;
  std::string second_timeline_window_uid;
  bool second_timeline_window_uid_token_present = false;
  bool second_timeline_window_live_configuration_present = false;
  int64_t second_timeline_window_live_target_offset_ms = 0;
  int64_t second_timeline_window_live_min_offset_ms = 0;
  int64_t second_timeline_window_live_max_offset_ms = 0;
  float second_timeline_window_live_min_playback_speed = 0.0f;
  float second_timeline_window_live_max_playback_speed = 0.0f;
  bool second_timeline_window_manifest_present = false;
  std::string second_timeline_window_manifest_string;
  bool second_timeline_window_manifest_token_present = false;
  int second_timeline_window_first_period_index = 0;
  int second_timeline_window_last_period_index = 0;
  int64_t second_timeline_window_presentation_start_time_ms = 0;
  int64_t second_timeline_window_window_start_time_ms = 0;
  int64_t second_timeline_window_elapsed_realtime_epoch_offset_ms = 0;
  int64_t second_timeline_window_default_position_ms = 0;
  int64_t second_timeline_window_default_position_us = 0;
  int64_t second_timeline_window_duration_ms = 0;
  int64_t second_timeline_window_duration_us = 0;
  bool second_timeline_window_is_seekable = false;
  bool second_timeline_window_is_dynamic = false;
  bool second_timeline_window_is_live = false;
  bool second_timeline_window_is_placeholder = false;
  std::string first_timeline_period_id;
  bool first_timeline_period_id_token_present = false;
  std::string first_timeline_period_uid;
  bool first_timeline_period_uid_token_present = false;
  std::string first_timeline_period_ads_id;
  bool first_timeline_period_ads_id_token_present = false;
  int first_timeline_period_ad_group_count = 0;
  int64_t first_timeline_period_duration_ms = 0;
  int64_t first_timeline_period_duration_us = 0;
  int64_t first_timeline_period_position_in_window_us = 0;
  std::string second_timeline_period_id;
  bool second_timeline_period_id_token_present = false;
  std::string second_timeline_period_uid;
  bool second_timeline_period_uid_token_present = false;
  std::string second_timeline_period_ads_id;
  bool second_timeline_period_ads_id_token_present = false;
  int64_t second_timeline_period_duration_us = 0;
  int track_group_count = 0;
  int first_track_group_type = -1;
  std::string first_track_group_id;
  bool first_track_group_token_present = false;
  int first_track_count = 0;
  std::string first_track_language;
  std::string first_track_label;
  bool first_track_label_token_present = false;
  std::string first_track_mime_type;
  int first_track_accessibility_channel = 0;
  int first_track_role_flags = 0;
  int first_track_selection_flags = 0;
  bool first_track_selected = false;
  bool first_track_supported = false;
  bool first_track_supported_within_capabilities = false;
  std::string second_track_group_id;
  bool second_track_group_token_present = false;
  int second_track_count = 0;
  std::string second_track_label;
  bool second_track_label_token_present = false;
  std::string second_track_language;
  std::string second_track_mime_type;
  int second_track_accessibility_channel = 0;
  int second_track_role_flags = 0;
  int second_track_selection_flags = 0;
  bool second_track_selected = false;
  bool second_track_supported = false;
  bool second_track_supported_within_capabilities = false;
  bool contains_audio = false;
  bool contains_video = false;
  bool contains_text = false;
  bool audio_selected = false;
  bool video_selected = false;
  bool text_selected = false;
  bool audio_supported = false;
  bool video_supported = false;
  bool text_supported = false;
  bool video_supported_allowing_exceeds = false;
  int repeat_callback_count = 0;
  int shuffle_callback_count = 0;
  int seek_back_increment_callback_count = 0;
  int seek_forward_increment_callback_count = 0;
  int max_seek_to_previous_position_callback_count = 0;
  int track_selection_callback_count = 0;
  int playback_parameters_callback_count = 0;
  int playback_suppression_reason_callback_count = 0;
  int available_commands_callback_count = 0;
  int player_error_changed_callback_count = 0;
  int events_callback_count = 0;
  int position_discontinuity_callback_count = 0;
  int timeline_callback_count = 0;
  int tracks_changed_callback_count = 0;
  int media_metadata_callback_count = 0;
  int playlist_metadata_callback_count = 0;
  int cue_callback_count = 0;
  int cue_count = 0;
  int64_t cue_presentation_time_us = 0;
  std::string cue0_text;
  bool cue0_text_token_present = false;
  bool cue0_bitmap_token_present = false;
  int cue0_text_alignment = 0;
  int cue0_multi_row_alignment = 0;
  float cue0_line = 0.0f;
  int cue0_line_type = 0;
  int cue0_position_anchor = 0;
  std::string cue1_text;
  bool cue1_text_token_present = false;
  bool cue1_bitmap_token_present = false;
  int cue1_line_type = 0;
  int cue1_position_anchor = 0;
  float cue1_text_size = 0.0f;
  int cue1_text_size_type = 0;
  int cue1_vertical_type = 0;
  bool is_loading = false;
  int is_loading_callback_count = 0;
  int analytics_callback_count = 0;
  int audio_underrun_callback_count = 0;
  int dropped_video_frames_callback_count = 0;
  int bandwidth_estimate_callback_count = 0;
  int load_started_callback_count = 0;
  int load_completed_callback_count = 0;
  int audio_input_format_changed_callback_count = 0;
  int audio_decoder_initialized_callback_count = 0;
  int video_decoder_initialized_callback_count = 0;
  int audio_decoder_released_callback_count = 0;
  int video_decoder_released_callback_count = 0;
  int analytics_rendered_first_frame_callback_count = 0;
  int analytics_video_size_changed_callback_count = 0;
  int analytics_audio_position_advancing_callback_count = 0;
  int analytics_video_frame_processing_offset_callback_count = 0;
  int analytics_volume_changed_callback_count = 0;
  int analytics_audio_session_id_changed_callback_count = 0;
  int analytics_audio_attributes_changed_callback_count = 0;
  int analytics_skip_silence_enabled_changed_callback_count = 0;
  int analytics_device_volume_changed_callback_count = 0;
  int analytics_playback_state_changed_callback_count = 0;
  int analytics_is_playing_changed_callback_count = 0;
  int analytics_play_when_ready_changed_callback_count = 0;
  int analytics_playback_suppression_reason_changed_callback_count = 0;
  int analytics_is_loading_changed_callback_count = 0;
  int analytics_repeat_mode_changed_callback_count = 0;
  int analytics_shuffle_mode_changed_callback_count = 0;
  int analytics_playback_parameters_changed_callback_count = 0;
  int analytics_available_commands_callback_count = 0;
  int analytics_events_callback_count = 0;
  int analytics_seek_back_increment_changed_callback_count = 0;
  int analytics_seek_forward_increment_changed_callback_count = 0;
  int analytics_max_seek_to_previous_position_changed_callback_count = 0;
  int analytics_timeline_changed_callback_count = 0;
  int analytics_position_discontinuity_callback_count = 0;
  int analytics_seek_started_callback_count = 0;
  int analytics_player_error_callback_count = 0;
  int analytics_player_error_changed_callback_count = 0;
  int analytics_tracks_changed_callback_count = 0;
  int analytics_media_item_transition_callback_count = 0;
  int analytics_cues_callback_count = 0;
  int analytics_metadata_callback_count = 0;
  int analytics_load_error_callback_count = 0;
  int analytics_device_info_changed_callback_count = 0;
  int analytics_media_metadata_changed_callback_count = 0;
  int analytics_playlist_metadata_changed_callback_count = 0;
  int video_input_format_changed_callback_count = 0;
  int analytics_player_state_changed_callback_count = 0;
  int analytics_loading_changed_callback_count = 0;
  int analytics_track_selection_changed_callback_count = 0;
  int analytics_load_canceled_callback_count = 0;
  int analytics_downstream_format_changed_callback_count = 0;
  int analytics_upstream_discarded_callback_count = 0;
  int analytics_audio_enabled_callback_count = 0;
  int analytics_audio_disabled_callback_count = 0;
  int analytics_audio_sink_error_callback_count = 0;
  int analytics_audio_codec_error_callback_count = 0;
  int analytics_audio_track_initialized_callback_count = 0;
  int analytics_audio_track_released_callback_count = 0;
  int analytics_video_enabled_callback_count = 0;
  int analytics_video_disabled_callback_count = 0;
  int analytics_video_codec_error_callback_count = 0;
  int analytics_surface_size_changed_callback_count = 0;
  int analytics_drm_session_acquired_callback_count = 0;
  int analytics_drm_keys_loaded_callback_count = 0;
  int analytics_drm_session_manager_error_callback_count = 0;
  int analytics_drm_keys_restored_callback_count = 0;
  int analytics_drm_keys_removed_callback_count = 0;
  int analytics_drm_session_released_callback_count = 0;
  int analytics_renderer_ready_changed_callback_count = 0;
  int analytics_dropped_seeks_while_scrubbing_callback_count = 0;
  int analytics_player_released_callback_count = 0;
  std::atomic<bool> capture_analytics_callbacks{true};
  std::atomic<int> smoke_repeat_callback_count{0};
  std::atomic<int> smoke_shuffle_callback_count{0};
  std::atomic<int> smoke_seek_back_increment_callback_count{0};
  std::atomic<int> smoke_seek_forward_increment_callback_count{0};
  std::atomic<int> smoke_max_seek_to_previous_position_callback_count{0};
  std::atomic<int> smoke_track_selection_callback_count{0};
  std::atomic<int> smoke_playback_parameters_callback_count{0};
  std::atomic<int> smoke_playback_suppression_reason_callback_count{0};
  std::atomic<int> smoke_available_commands_callback_count{0};
  std::atomic<int> smoke_events_callback_count{0};
  std::atomic<int> smoke_timeline_callback_count{0};
  std::atomic<int> smoke_tracks_changed_callback_count{0};
  std::atomic<int> smoke_media_metadata_callback_count{0};
  std::atomic<int> smoke_playlist_metadata_callback_count{0};
  std::atomic<int> smoke_cue_callback_count{0};
  std::atomic<int> smoke_position_discontinuity_callback_count{0};
  std::atomic<int> smoke_is_loading_callback_count{0};
  std::atomic<int> smoke_timeline_window_count{0};
  std::atomic<int> smoke_timeline_period_count{0};
  std::atomic<int> smoke_cue_count{0};
  std::atomic<bool> smoke_preferred_text_language_seen{false};
  std::atomic<bool> smoke_media_metadata_title_seen{false};
  std::atomic<bool> smoke_playlist_metadata_title_seen{false};
  std::atomic<bool> smoke_new_position_media_id_seen{false};
  // Listener callbacks can reenter on the same thread while a previous callback is
  // still unwinding after seek-driven dispatch. A recursive mutex keeps the test
  // capture synchronized without self-deadlocking that callback chain.
  std::atomic<const char*> active_callback{nullptr};
  mutable std::recursive_mutex mutex;
  uint64_t object_canary_tail = kObjectCanaryTailValue;
};

std::string BuildListenerSmokeProgressSummary(const CapturingPlayerListener& listener) {
  std::string summary = "repeatCb=" + std::to_string(listener.repeat_callback_count);
  summary += ",shuffleCb=" + std::to_string(listener.shuffle_callback_count);
  summary += ",seekBackCb=" + std::to_string(listener.seek_back_increment_callback_count);
  summary += ",seekForwardCb=" + std::to_string(listener.seek_forward_increment_callback_count);
  summary +=
      ",maxSeekPrevCb=" + std::to_string(listener.max_seek_to_previous_position_callback_count);
  summary += ",trackCb=" + std::to_string(listener.track_selection_callback_count);
  summary +=
      ",playbackParamsCb=" + std::to_string(listener.playback_parameters_callback_count);
  summary +=
      ",suppressionCb=" + std::to_string(listener.playback_suppression_reason_callback_count);
  summary += ",commandsCb=" + std::to_string(listener.available_commands_callback_count);
  summary += ",eventsCb=" + std::to_string(listener.events_callback_count);
  summary += ",timelineCb=" + std::to_string(listener.timeline_callback_count);
  summary += ",tracksCb=" + std::to_string(listener.tracks_changed_callback_count);
  summary += ",mediaMetadataCb=" + std::to_string(listener.media_metadata_callback_count);
  summary += ",playlistMetadataCb=" + std::to_string(listener.playlist_metadata_callback_count);
  summary += ",cueCb=" + std::to_string(listener.cue_callback_count);
  summary +=
      ",positionCb=" + std::to_string(listener.position_discontinuity_callback_count);
  summary += ",isLoadingCb=" + std::to_string(listener.is_loading_callback_count);
  summary += ",timelineWindowCount=" + std::to_string(listener.timeline_window_count);
  summary += ",timelinePeriodCount=" + std::to_string(listener.timeline_period_count);
  summary += ",cueCount=" + std::to_string(listener.cue_count);
  summary += ",textLang=" + listener.preferred_text_language;
  summary += ",mediaTitle=" + listener.media_metadata_title;
  summary += ",playlistTitle=" + listener.playlist_metadata_title;
  summary += ",newMediaId=" + listener.new_position_media_id;
  return summary;
}

std::string BuildListenerSmokeSignalSummary(const CapturingPlayerListener& listener) {
  std::string summary =
      "repeatCb=" + std::to_string(listener.smoke_repeat_callback_count.load(std::memory_order_acquire));
  summary +=
      ",shuffleCb=" + std::to_string(listener.smoke_shuffle_callback_count.load(std::memory_order_acquire));
  summary += ",seekBackCb=" +
      std::to_string(listener.smoke_seek_back_increment_callback_count.load(std::memory_order_acquire));
  summary += ",seekForwardCb=" +
      std::to_string(listener.smoke_seek_forward_increment_callback_count.load(std::memory_order_acquire));
  summary += ",maxSeekPrevCb=" +
      std::to_string(
          listener.smoke_max_seek_to_previous_position_callback_count.load(std::memory_order_acquire));
  summary += ",trackCb=" +
      std::to_string(listener.smoke_track_selection_callback_count.load(std::memory_order_acquire));
  summary += ",playbackParamsCb=" +
      std::to_string(listener.smoke_playback_parameters_callback_count.load(std::memory_order_acquire));
  summary += ",suppressionCb=" +
      std::to_string(
          listener.smoke_playback_suppression_reason_callback_count.load(std::memory_order_acquire));
  summary += ",commandsCb=" +
      std::to_string(listener.smoke_available_commands_callback_count.load(std::memory_order_acquire));
  summary += ",eventsCb=" +
      std::to_string(listener.smoke_events_callback_count.load(std::memory_order_acquire));
  summary += ",timelineCb=" +
      std::to_string(listener.smoke_timeline_callback_count.load(std::memory_order_acquire));
  summary += ",tracksCb=" +
      std::to_string(listener.smoke_tracks_changed_callback_count.load(std::memory_order_acquire));
  summary += ",mediaMetadataCb=" +
      std::to_string(listener.smoke_media_metadata_callback_count.load(std::memory_order_acquire));
  summary += ",playlistMetadataCb=" +
      std::to_string(listener.smoke_playlist_metadata_callback_count.load(std::memory_order_acquire));
  summary += ",cueCb=" +
      std::to_string(listener.smoke_cue_callback_count.load(std::memory_order_acquire));
  summary += ",positionCb=" +
      std::to_string(
          listener.smoke_position_discontinuity_callback_count.load(std::memory_order_acquire));
  summary += ",isLoadingCb=" +
      std::to_string(listener.smoke_is_loading_callback_count.load(std::memory_order_acquire));
  summary += ",timelineWindowCount=" +
      std::to_string(listener.smoke_timeline_window_count.load(std::memory_order_acquire));
  summary += ",timelinePeriodCount=" +
      std::to_string(listener.smoke_timeline_period_count.load(std::memory_order_acquire));
  summary += ",cueCount=" +
      std::to_string(listener.smoke_cue_count.load(std::memory_order_acquire));
  summary += ",textLangSeen=" +
      std::to_string(listener.smoke_preferred_text_language_seen.load(std::memory_order_acquire) ? 1 : 0);
  summary += ",mediaTitleSeen=" +
      std::to_string(listener.smoke_media_metadata_title_seen.load(std::memory_order_acquire) ? 1 : 0);
  summary += ",playlistTitleSeen=" +
      std::to_string(listener.smoke_playlist_metadata_title_seen.load(std::memory_order_acquire) ? 1 : 0);
  summary += ",newMediaIdSeen=" +
      std::to_string(listener.smoke_new_position_media_id_seen.load(std::memory_order_acquire) ? 1 : 0);
  return summary;
}

std::string BuildListenerCanarySummary(const CapturingPlayerListener& listener) {
  std::string summary = "canaryOk=" + std::to_string(listener.HasValidObjectCanaries() ? 1 : 0);
  summary += ",canaryHead=" + std::to_string(listener.object_canary_head);
  summary += ",canaryTail=" + std::to_string(listener.object_canary_tail);
  return summary;
}

std::string BuildListenerLayoutSummary(const CapturingPlayerListener& listener) {
  std::string summary = "listenerAddr=" + BuildPointerSummary(&listener);
  summary += ",size=" + std::to_string(sizeof(listener));
  summary += ",headAddr=" + BuildPointerSummary(&listener.object_canary_head);
  summary += ",activeCallbackAddr=" + BuildPointerSummary(&listener.active_callback);
  summary += ",mutexAddr=" + BuildPointerSummary(&listener.mutex);
  summary += ",tailAddr=" + BuildPointerSummary(&listener.object_canary_tail);
  summary += "," + BuildListenerCanarySummary(listener);
  return summary;
}

std::string BuildActiveCallbackSummary(const CapturingPlayerListener& listener) {
  const char* active_callback = listener.active_callback.load(std::memory_order_acquire);
  return "activeCallbackPtr=" + BuildPointerSummary(active_callback);
}

std::string BuildListenerLockSummary(const CapturingPlayerListener& listener) {
  std::unique_lock<std::recursive_mutex> lock(listener.mutex, std::defer_lock);
  const bool acquired = lock.try_lock();
  std::string summary = "mutexBusy=" + std::to_string(acquired ? 0 : 1);
  if (acquired) {
    lock.unlock();
  }
  summary += "," + BuildActiveCallbackSummary(listener);
  return summary;
}

std::string BuildListenerStepStateSummary(const CapturingPlayerListener& listener) {
  std::string summary = BuildListenerSmokeSignalSummary(listener);
  summary += "," + BuildListenerLockSummary(listener);
  summary += "," + BuildListenerCanarySummary(listener);
  return summary;
}

int ObservedCallbackFlag(int callback_count) {
  return callback_count > 0 ? 1 : 0;
}

bool HasExplicitLiveConfiguration(const MediaItemDescriptor& media_item) {
  const auto& live_configuration = media_item.live_configuration;
  return live_configuration.target_offset_ms != -9223372036854775807LL ||
      live_configuration.min_offset_ms != -9223372036854775807LL ||
      live_configuration.max_offset_ms != -9223372036854775807LL ||
      live_configuration.min_playback_speed != -3.4028235e38f ||
      live_configuration.max_playback_speed != -3.4028235e38f;
}

TracksSnapshot BuildListenerSmokeTracksSnapshot() {
  TracksSnapshot tracks;
  tracks.contains_text = true;
  tracks.text_selected = true;
  tracks.text_supported = true;

  TrackGroupSnapshot text_group;
  text_group.id = "listener-text-group";
  text_group.group_token = "generated-opaque-object-token-listener-track-group";
  text_group.type = 3;
  text_group.selected = true;
  text_group.supported = true;

  TrackInfo text_track;
  text_track.id = "listener-text-track";
  text_track.language = "en";
  text_track.label = "Listener Text";
  text_track.label_token = "generated-opaque-object-token-listener-track-label";
  text_track.mime_type = "text/vtt";
  text_track.selected = true;
  text_track.supported = true;
  text_track.supported_within_capabilities = true;
  text_track.format_support = 1;

  text_group.tracks.push_back(text_track);
  tracks.groups.push_back(text_group);
  return tracks;
}

void RunListenerLocalDispatchSanityCheck(CapturingPlayerListener* listener) {
  if (listener == nullptr) {
    return;
  }
  LogInfo(
      "nativeListenerSmokeTest stage=selfDispatchBegin," +
      BuildListenerStepStateSummary(*listener) + "," + BuildListenerLayoutSummary(*listener));

  PlaybackSnapshot snapshot;
  snapshot.repeat_mode = androidx::media3::cppbridge::RepeatMode::kAll;

  PlayerEventsSnapshot events;
  events.event_codes = {1, 9};

  TimelineDetailsSnapshot timeline;
  timeline.summary.window_count = 2;
  timeline.summary.period_count = 2;
  timeline.summary.empty = false;
  timeline.summary.current_media_item_index = 1;
  timeline.windows.resize(2);
  timeline.windows[0].media_item_id = "self-dispatch-item-1";
  timeline.windows[1].media_item_id = "self-dispatch-item-2";

  MediaMetadataSnapshot metadata;
  metadata.title = "self-dispatch-title";

  CueSnapshot cues;
  cues.cue_count = 1;
  cues.cues.resize(1);
  cues.cues[0].text = "self-dispatch-cue";

  PositionInfoSnapshot old_position;
  old_position.media_item.media_id = "self-dispatch-old";
  PositionInfoSnapshot new_position;
  new_position.media_item_index = 1;
  new_position.position_ms = 3456;
  new_position.media_item.media_id = "self-dispatch-new";

  PlayerListener* base_listener = listener;
  base_listener->OnEvents(snapshot, events);
  base_listener->OnRepeatModeChanged(snapshot);
  snapshot.is_loading = true;
  base_listener->OnIsLoadingChanged(snapshot);
  base_listener->OnTimelineChanged(snapshot, timeline, 2);
  base_listener->OnMediaMetadataChanged(snapshot, metadata);
  base_listener->OnCues(snapshot, cues);
  base_listener->OnPositionDiscontinuity(snapshot, old_position, new_position, 1);

  LogInfo(
      "nativeListenerSmokeTest stage=selfDispatchEnd," +
      BuildListenerStepStateSummary(*listener) + "," + BuildListenerLayoutSummary(*listener));
}

bool HasListenerSmokeScenarioState(const CapturingPlayerListener& listener) {
  return listener.smoke_repeat_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_shuffle_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_seek_back_increment_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_seek_forward_increment_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_max_seek_to_previous_position_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_track_selection_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_playback_parameters_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_available_commands_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_events_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_timeline_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_tracks_changed_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_media_metadata_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_playlist_metadata_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_cue_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_position_discontinuity_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_is_loading_callback_count.load(std::memory_order_acquire) > 0 &&
      listener.smoke_timeline_window_count.load(std::memory_order_acquire) >= 2 &&
      listener.smoke_timeline_period_count.load(std::memory_order_acquire) >= 2 &&
      listener.smoke_cue_count.load(std::memory_order_acquire) >= 2 &&
      listener.smoke_preferred_text_language_seen.load(std::memory_order_acquire) &&
      listener.smoke_media_metadata_title_seen.load(std::memory_order_acquire) &&
      listener.smoke_playlist_metadata_title_seen.load(std::memory_order_acquire) &&
      listener.smoke_new_position_media_id_seen.load(std::memory_order_acquire);
}

std::string BuildListenerSummaryLockTimeoutSummary(
    const CapturingPlayerListener& listener,
    bool ready) {
  std::string summary = "waitReady=" + std::to_string(ready ? 1 : 0);
  summary += ",summaryLockTimeout=1";
  summary += "," + BuildActiveCallbackSummary(listener);
  summary += "," + BuildListenerSmokeSignalSummary(listener);
  summary += "," + BuildListenerCanarySummary(listener);
  return summary;
}

bool TryLockListenerForSummary(
    const CapturingPlayerListener* listener,
    int timeout_ms,
    const char* label,
    std::unique_lock<std::recursive_mutex>* lock_out) {
  if (listener == nullptr || lock_out == nullptr) {
    return false;
  }
  const std::string test_label = label != nullptr ? label : "listenerSmoke";
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    if (lock_out->try_lock()) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  LogError(
      test_label + " summary lock timeout timeoutMs=" + std::to_string(timeout_ms) +
      "," + BuildActiveCallbackSummary(*listener) +
      "," + BuildListenerCanarySummary(*listener));
  return false;
}

bool WaitForListenerSmokeScenario(
    const CapturingPlayerListener* listener,
    int timeout_ms,
    const char* label) {
  if (listener == nullptr) {
    return false;
  }
  const std::string test_label = label != nullptr ? label : "listenerSmoke";
  LogInfo(test_label + " wait start timeoutMs=" + std::to_string(timeout_ms));
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  const auto start = std::chrono::steady_clock::now();
  auto next_progress_log = start + std::chrono::milliseconds(250);
  while (std::chrono::steady_clock::now() < deadline) {
    if (HasListenerSmokeScenarioState(*listener)) {
      LogInfo(
          test_label + " wait ready elapsedMs=" +
          std::to_string(
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - start)
                  .count()) +
          "," + BuildListenerSmokeSignalSummary(*listener));
      return true;
    }
    if (std::chrono::steady_clock::now() >= next_progress_log) {
      std::string summary = BuildListenerSmokeSignalSummary(*listener);
      std::unique_lock<std::recursive_mutex> lock(listener->mutex, std::defer_lock);
      if (!lock.try_lock()) {
        summary += ",mutexBusy=1," + BuildActiveCallbackSummary(*listener);
      }
      summary += "," + BuildListenerCanarySummary(*listener);
      LogInfo(
          test_label + " wait pending elapsedMs=" +
          std::to_string(
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - start)
                  .count()) +
          "," + summary);
      next_progress_log = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  std::string summary = BuildListenerSmokeSignalSummary(*listener);
  std::unique_lock<std::recursive_mutex> lock(listener->mutex, std::defer_lock);
  if (!lock.try_lock()) {
    LogError(
        test_label + " wait timeout elapsedMs=" +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start)
                .count()) +
        ",mutexBusy=1," + BuildActiveCallbackSummary(*listener) + "," + summary + "," +
        BuildListenerCanarySummary(*listener));
    return false;
  }
  const bool ready = HasListenerSmokeScenarioState(*listener);
  if (ready) {
    LogInfo(
        test_label + " wait ready elapsedMs=" +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start)
                .count()) +
        "," + summary);
  } else {
    LogError(
        test_label + " wait timeout elapsedMs=" +
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start)
                .count()) +
        "," + summary);
  }
  return ready;
}

extern "C" {

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeCreateConfiguredPlayerSnapshotForTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/configured.mp4";
  media_item.media_id = "configured-item";
  player->SetMediaItem(media_item);
  player->SetPlayWhenReady(true);
  player->SetRepeatMode(androidx::media3::cppbridge::RepeatMode::kAll);
  player->SetShuffleModeEnabled(true);
  player->SetVolume(0.25f);
  player->SetPlaybackSpeed(1.5f);
  std::string summary = BuildSnapshotSummary(player->GetSnapshot());
  return NewStringUtfChecked(env, summary, "nativeCreateConfiguredPlayerSnapshotForTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeCreatePlaylistSnapshotForTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor first_item;
  first_item.uri = "https://example.com/one.mp4";
  first_item.media_id = "item-1";
  MediaItemDescriptor second_item;
  second_item.uri = "https://example.com/two.mp4";
  second_item.media_id = "item-2";
  player->SetMediaItems({first_item, second_item}, 1, 1234);
  std::string summary = BuildSnapshotSummary(player->GetSnapshot());
  return NewStringUtfChecked(env, summary, "nativeCreatePlaylistSnapshotForTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeLifecycleSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/lifecycle.mp4";
  media_item.media_id = "lifecycle-item";
  player->SetMediaItem(media_item);
  player->ClearMediaItems();
  player->Stop();
  player.reset();
  return NewStringUtfChecked(env, "lifecycle-ok", "nativeLifecycleSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePostReleaseCallSafetySmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/post-release.mp4";
  media_item.media_id = "post-release-item";
  player->SetMediaItem(media_item);
  player->Prepare();
  player->Release();
  player->Release();
  player->Play();
  player->Pause();
  PlaybackSnapshot snapshot = player->GetSnapshot();
  std::string debug_summary = player->GetCurrentMediaItemDebugSummary();
  std::string summary = "released=1";
  summary += ",state=" + std::to_string(static_cast<int>(snapshot.playback_state));
  summary += ",count=" + std::to_string(snapshot.media_item_count);
  summary += ",index=" + std::to_string(snapshot.current_media_item_index);
  summary += ",debugSummary=" + debug_summary;
  return NewStringUtfChecked(env, summary, "nativePostReleaseCallSafetySmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeTrackSelectionRoundTripForTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  TrackSelectionParametersDescriptor parameters;
  parameters.preferred_audio_language = "ja";
  parameters.preferred_text_language = "en";
  parameters.max_audio_channel_count = 6;
  parameters.max_audio_bitrate = 384000;
  parameters.max_video_width = 1280;
  parameters.max_video_height = 720;
  parameters.max_video_bitrate = 2000000;
  parameters.select_text_by_default = true;
  parameters.ignored_text_selection_flags = 2;
  parameters.select_undetermined_text_language = true;
  parameters.force_lowest_bitrate = true;
  parameters.disable_audio = true;
  parameters.disabled_track_types = {2};
  player->SetTrackSelectionParameters(parameters);
  std::string summary = BuildTrackSelectionSummary(player->GetTrackSelectionParameters());
  return NewStringUtfChecked(env, summary, "nativeTrackSelectionRoundTripForTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeSubtitleSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/subtitle.mp4";
  media_item.media_id = "subtitle-item";
  MediaItemDescriptor::SubtitleConfigurationDescriptor subtitle;
  subtitle.uri = "https://example.com/subtitle_en.vtt";
  subtitle.mime_type = "text/vtt";
  subtitle.language = "en";
  subtitle.label = "English";
  media_item.subtitle_configurations.push_back(subtitle);
  player->SetMediaItem(media_item);
  std::string summary = player->GetCurrentMediaItemDebugSummary();
  return NewStringUtfChecked(env, summary, "nativeSubtitleSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeDrmSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/drm.mpd";
  media_item.media_id = "drm-item";
  media_item.mime_type = "application/dash+xml";
  media_item.drm_configuration.scheme_uuid = "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed";
  media_item.drm_configuration.license_uri = "https://license.example.com";
  media_item.drm_configuration.license_request_header_names = {"Authorization", "X-Client"};
  media_item.drm_configuration.license_request_header_values = {"Bearer test-token", "cppbridge"};
  media_item.drm_configuration.forced_session_track_types = {1, 2};
  media_item.drm_configuration.key_set_id = {0x01, 0x02, 0x03, 0x04};
  media_item.drm_configuration.multi_session = true;
  media_item.drm_configuration.play_clear_content_without_key = false;
  player->SetMediaItem(media_item);
  std::string summary = player->GetCurrentMediaItemDebugSummary();
  return NewStringUtfChecked(env, summary, "nativeDrmSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeClippingSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/clipped.mp4";
  media_item.media_id = "clip-item";
  media_item.clipping_configuration.start_position_ms = 1000;
  media_item.clipping_configuration.end_position_ms = 5000;
  media_item.clipping_configuration.relative_to_default_position = true;
  media_item.clipping_configuration.starts_at_key_frame = true;
  player->SetMediaItem(media_item);
  std::string summary = player->GetCurrentMediaItemDebugSummary();
  return NewStringUtfChecked(env, summary, "nativeClippingSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeLiveConfigurationSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/live.m3u8";
  media_item.media_id = "live-item";
  media_item.live_configuration.target_offset_ms = 3000;
  media_item.live_configuration.min_offset_ms = 2000;
  media_item.live_configuration.max_offset_ms = 5000;
  media_item.live_configuration.min_playback_speed = 0.97f;
  media_item.live_configuration.max_playback_speed = 1.03f;
  player->SetMediaItem(media_item);
  std::string summary = player->GetCurrentMediaItemDebugSummary();
  return NewStringUtfChecked(env, summary, "nativeLiveConfigurationSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeMultiSubtitleSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/multi-subtitle.mp4";
  media_item.media_id = "multi-sub-item";
  MediaItemDescriptor::SubtitleConfigurationDescriptor subtitle_en;
  subtitle_en.uri = "https://example.com/sub_en.vtt";
  subtitle_en.mime_type = "text/vtt";
  subtitle_en.language = "en";
  subtitle_en.label = "English";
  MediaItemDescriptor::SubtitleConfigurationDescriptor subtitle_zh;
  subtitle_zh.uri = "https://example.com/sub_zh.vtt";
  subtitle_zh.mime_type = "text/vtt";
  subtitle_zh.language = "zh";
  subtitle_zh.label = "Chinese";
  media_item.subtitle_configurations = {subtitle_en, subtitle_zh};
  player->SetMediaItem(media_item);
  TrackSelectionParametersDescriptor parameters;
  parameters.preferred_audio_language = "ja";
  parameters.preferred_text_language = "zh";
  parameters.preferred_text_role_flags = 128;
  parameters.select_text_by_default = true;
  player->SetTrackSelectionParameters(parameters);
  std::string summary = player->GetCurrentMediaItemDebugSummary();
  summary += "," + BuildTrackSelectionSummary(player->GetTrackSelectionParameters());
  return NewStringUtfChecked(env, summary, "nativeMultiSubtitleSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePlaylistMutationSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor first_item;
  first_item.uri = "https://example.com/one.mp4";
  first_item.media_id = "item-1";
  MediaItemDescriptor second_item;
  second_item.uri = "https://example.com/two.mp4";
  second_item.media_id = "item-2";
  MediaItemDescriptor third_item;
  third_item.uri = "https://example.com/three.mp4";
  third_item.media_id = "item-3";
  MediaItemDescriptor fourth_item;
  fourth_item.uri = "https://example.com/four.mp4";
  fourth_item.media_id = "item-4";
  MediaItemDescriptor fifth_item;
  fifth_item.uri = "https://example.com/five.mp4";
  fifth_item.media_id = "item-5";
  player->SetMediaItems({first_item, second_item}, 0, 0);
  player->AddMediaItem(third_item);
  player->AddMediaItem(1, fourth_item);
  player->AddMediaItems({fifth_item});

  player->MoveMediaItem(4, 0);
  MediaItemDescriptor replacement_item;
  replacement_item.uri = "https://example.com/replacement.mp4";
  replacement_item.media_id = "item-r";
  player->ReplaceMediaItem(1, replacement_item);
  player->AddMediaItems(2, {second_item});
  player->ReplaceMediaItems(3, 5, {first_item});
  player->RemoveMediaItems(3, 5);
  player->SeekToMediaItem(0, 0);

  int media_item_count = player->GetMediaItemCount();
  int current_media_item_index = player->GetCurrentMediaItemIndex();
  MediaItemDescriptor first_media_item = player->GetMediaItemAt(0);
  std::string summary = "count=" + std::to_string(media_item_count);
  summary += ",currentIndex=" + std::to_string(current_media_item_index);
  summary += ",firstMediaId=" + first_media_item.media_id;
  summary += "," + player->GetCurrentMediaItemDebugSummary();
  return NewStringUtfChecked(env, summary, "nativePlaylistMutationSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePlaylistMutationSmokeTestV2(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor first_item;
  first_item.uri = "https://example.com/one.mp4";
  first_item.media_id = "item-1";
  MediaItemDescriptor second_item;
  second_item.uri = "https://example.com/two.mp4";
  second_item.media_id = "item-2";
  MediaItemDescriptor third_item;
  third_item.uri = "https://example.com/three.mp4";
  third_item.media_id = "item-3";
  MediaItemDescriptor fourth_item;
  fourth_item.uri = "https://example.com/four.mp4";
  fourth_item.media_id = "item-4";
  MediaItemDescriptor fifth_item;
  fifth_item.uri = "https://example.com/five.mp4";
  fifth_item.media_id = "item-5";
  player->SetMediaItems({first_item, second_item}, 0, 0);
  player->AddMediaItem(third_item);
  player->AddMediaItem(1, fourth_item);
  player->AddMediaItems({fifth_item});

  player->MoveMediaItem(4, 0);
  MediaItemDescriptor replacement_item;
  replacement_item.uri = "https://example.com/replacement.mp4";
  replacement_item.media_id = "item-r";
  player->ReplaceMediaItem(1, replacement_item);
  player->AddMediaItems(2, {second_item});
  player->ReplaceMediaItems(3, 5, {first_item});
  player->RemoveMediaItems(3, 5);
  player->AddMediaItems({fourth_item, first_item});
  player->MoveMediaItems(3, 5, 0);
  MediaItemDescriptor move_range_first_media_item = player->GetMediaItemAt(0);
  player->RemoveMediaItem(0);
  player->RemoveMediaItem(0);
  player->SeekToMediaItem(0, 0);

  int media_item_count = player->GetMediaItemCount();
  int current_media_item_index = player->GetCurrentMediaItemIndex();
  int next_media_item_index = player->GetNextMediaItemIndex();
  int previous_media_item_index = player->GetPreviousMediaItemIndex();
  bool has_next_media_item = player->HasNextMediaItem();
  bool has_previous_media_item = player->HasPreviousMediaItem();
  MediaItemDescriptor first_media_item = player->GetMediaItemAt(0);
  std::string summary = "playlistMutationV2=1";
  summary += ",count=" + std::to_string(media_item_count);
  summary += ",currentIndex=" + std::to_string(current_media_item_index);
  summary += ",firstMediaId=" + first_media_item.media_id;
  summary += ",moveRangeFirstMediaId=" + move_range_first_media_item.media_id;
  summary += ",singleRemoveRestoredCount=" + std::to_string(media_item_count);
  summary += ",nextIndex=" + std::to_string(next_media_item_index);
  summary += ",previousIndex=" + std::to_string(previous_media_item_index);
  summary += ",hasNext=" + std::to_string(has_next_media_item ? 1 : 0);
  summary += ",hasPrevious=" + std::to_string(has_previous_media_item ? 1 : 0);
  if (media_item_count > 0) {
    summary += ",item0=" + player->GetMediaItemAt(0).media_id;
  }
  if (media_item_count > 1) {
    summary += ",item1=" + player->GetMediaItemAt(1).media_id;
  }
  if (media_item_count > 2) {
    summary += ",item2=" + player->GetMediaItemAt(2).media_id;
  }
  summary += "," + player->GetCurrentMediaItemDebugSummary();
  return NewStringUtfChecked(env, summary, "nativePlaylistMutationSmokeTestV2");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeSeekNavigationSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  config.seek_back_increment_ms = 5000;
  config.seek_forward_increment_ms = 15000;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor first_item;
  first_item.uri = "https://example.com/one.mp4";
  first_item.media_id = "nav-1";
  MediaItemDescriptor second_item;
  second_item.uri = "https://example.com/two.mp4";
  second_item.media_id = "nav-2";
  player->SetMediaItems({first_item, second_item}, 0, 0);
  player->SeekTo(6000);
  player->SeekBack();
  player->SeekForward();
  player->SeekToNextMediaItem();
  player->SeekToPreviousMediaItem();
  player->SeekToDefaultPosition(1);
  PlaybackSnapshot snapshot = player->GetSnapshot();
  std::string summary = "count=" + std::to_string(snapshot.media_item_count);
  summary += ",index=" + std::to_string(snapshot.current_media_item_index);
  summary += ",positionMs=" + std::to_string(snapshot.current_position_ms);
  return NewStringUtfChecked(env, summary, "nativeSeekNavigationSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeSeekAliasSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor first_item;
  first_item.uri = "https://example.com/alias-one.mp4";
  first_item.media_id = "alias-1";
  MediaItemDescriptor second_item;
  second_item.uri = "https://example.com/alias-two.mp4";
  second_item.media_id = "alias-2";
  player->SetMediaItems({first_item, second_item}, 0, 0);
  player->SeekToNext();
  int after_next_index = player->GetCurrentMediaItemIndex();
  player->SeekToPrevious();
  int after_previous_index = player->GetCurrentMediaItemIndex();
  std::string summary = "afterNextIndex=" + std::to_string(after_next_index);
  summary += ",afterPreviousIndex=" + std::to_string(after_previous_index);
  return NewStringUtfChecked(env, summary, "nativeSeekAliasSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeSeekParametersSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  SeekParametersDescriptor seek_parameters;
  seek_parameters.tolerance_before_us = 1111;
  seek_parameters.tolerance_after_us = 2222;
  player->SetSeekParameters(seek_parameters);
  SeekParametersDescriptor actual = player->GetSeekParameters();
  std::string summary =
      "beforeUs=" + std::to_string(actual.tolerance_before_us) +
      ",afterUs=" + std::to_string(actual.tolerance_after_us);
  return NewStringUtfChecked(env, summary, "nativeSeekParametersSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAudioAndQuerySmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  config.seek_back_increment_ms = 4321;
  config.seek_forward_increment_ms = 8765;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/audio-query.mp4";
  media_item.media_id = "audio-query-item";
  player->SetMediaItem(media_item);
  AudioAttributesDescriptor attributes;
  attributes.content_type = 2;
  attributes.usage = 1;
  attributes.flags = 0;
  attributes.allowed_capture_policy = 1;
  attributes.spatialization_behavior = 0;
  player->SetAudioAttributes(attributes, true);
  PlaybackParametersSnapshot playback_parameters;
  playback_parameters.speed = 1.25f;
  playback_parameters.pitch = 0.80f;
  player->SetPlaybackParameters(playback_parameters);
  CueSnapshot current_cues;
  current_cues.cue_count = 2;
  current_cues.presentation_time_us = 456789;
  current_cues.texts = {"Query Cue 1", "Query Cue 2"};
  current_cues.text_tokens = {
      "generated-opaque-object-token-query-cue-1",
      "generated-opaque-object-token-query-cue-2"};
  current_cues.bitmap_tokens = {
      "generated-opaque-object-token-query-bitmap-1",
      ""};
  current_cues.cues.resize(2);
  current_cues.cues[0].text = "Query Cue 1";
  current_cues.cues[0].text_token = "generated-opaque-object-token-query-cue-1";
  current_cues.cues[0].bitmap_token = "generated-opaque-object-token-query-bitmap-1";
  current_cues.cues[0].text_alignment = 2;
  current_cues.cues[0].multi_row_alignment = 1;
  current_cues.cues[0].line = 0.25f;
  current_cues.cues[0].line_type = 0;
  current_cues.cues[0].line_anchor = 1;
  current_cues.cues[0].position = 0.5f;
  current_cues.cues[0].position_anchor = 2;
  current_cues.cues[0].size = 0.6f;
  current_cues.cues[0].bitmap_height = 0.75f;
  current_cues.cues[0].text_size = 18.0f;
  current_cues.cues[0].text_size_type = 2;
  current_cues.cues[0].vertical_type = 0;
  current_cues.cues[0].shear_degrees = 12.5f;
  current_cues.cues[0].z_index = 4;
  current_cues.cues[0].window_color_set = true;
  current_cues.cues[0].window_color = 0x00FF00;
  current_cues.cues[0].has_bitmap = true;
  current_cues.cues[1].text = "Query Cue 2";
  current_cues.cues[1].text_token = "generated-opaque-object-token-query-cue-2";
  current_cues.cues[1].line_type = 1;
  current_cues.cues[1].position_anchor = 1;
  current_cues.cues[1].text_size = 22.0f;
  current_cues.cues[1].text_size_type = 3;
  current_cues.cues[1].vertical_type = 1;
  player->SimulateCurrentCuesForTest(current_cues);

  AudioAttributesDescriptor actual_attributes = player->GetAudioAttributes();
  PlaybackState playback_state = player->GetPlaybackState();
  bool play_when_ready = player->GetPlayWhenReady();
  bool is_playing = player->IsPlaying();
  bool is_loading = player->IsLoading();
  PlayerError player_error = player->GetPlayerError();
  int64_t current_position_ms = player->GetCurrentPosition();
  int64_t buffered_position_ms = player->GetBufferedPosition();
  int64_t duration_ms = player->GetDuration();
  int current_media_item_index = player->GetCurrentMediaItemIndex();
  int media_item_count = player->GetMediaItemCount();
  RepeatMode repeat_mode = player->GetRepeatMode();
  bool shuffle_mode_enabled = player->GetShuffleModeEnabled();
  float volume = player->GetVolume();
  TimelineSnapshot timeline = player->GetTimelineSnapshot();
  std::vector<TimelineWindowSnapshot> timeline_windows = player->GetTimelineWindows();
  std::vector<TimelinePeriodSnapshot> timeline_periods = player->GetTimelinePeriods();
  TracksSnapshot tracks = player->GetTracks();
  std::vector<TrackGroupSnapshot> track_groups = player->GetTrackGroups();
  CueSnapshot cues = player->GetCurrentCues();
  int available_command_count = player->GetAvailableCommandCount();
  AvailableCommandsSnapshot commands = player->GetAvailableCommands();
  PlaybackParametersSnapshot actual_playback_parameters = player->GetPlaybackParameters();
  int buffered_percentage = player->GetBufferedPercentage();
  int64_t content_buffered_position_ms = player->GetContentBufferedPosition();
  int64_t content_duration_ms = player->GetContentDuration();
  int64_t content_position_ms = player->GetContentPosition();
  int64_t current_live_offset_ms = player->GetCurrentLiveOffset();
  int current_period_index = player->GetCurrentPeriodIndex();
  int64_t max_seek_to_previous_position_ms = player->GetMaxSeekToPreviousPosition();
  PlaybackSuppressionReason playback_suppression_reason =
      player->GetPlaybackSuppressionReason();
  int64_t seek_back_increment_ms = player->GetSeekBackIncrement();
  int64_t seek_forward_increment_ms = player->GetSeekForwardIncrement();
  int64_t total_buffered_duration_ms = player->GetTotalBufferedDuration();
  bool command_play_pause_available = player->IsCommandAvailable(1);
  bool can_advertise_session = player->CanAdvertiseSession();
  ApplicationLooperDescriptor application_looper = player->GetApplicationLooper();
  int current_ad_group_index = player->GetCurrentAdGroupIndex();
  int current_ad_index_in_group = player->GetCurrentAdIndexInAdGroup();
  bool is_current_media_item_dynamic = player->IsCurrentMediaItemDynamic();
  bool is_current_media_item_live = player->IsCurrentMediaItemLive();
  bool is_current_media_item_seekable = player->IsCurrentMediaItemSeekable();
  bool is_playing_ad = player->IsPlayingAd();
  int bridge_tracks_group_count = -1;
  int bridge_track_group_vector_count = -1;
  {
    std::shared_ptr<ExoPlayerBridge> bridge =
        ExoPlayerBridge::Create(env, context, config);
    if (bridge != nullptr) {
      TracksSnapshot bridge_tracks = bridge->GetTracksSnapshot(env);
      std::vector<TrackGroupSnapshot> bridge_track_groups =
          bridge->GetTrackGroups(env);
      bridge_tracks_group_count =
          static_cast<int>(bridge_tracks.groups.size());
      bridge_track_group_vector_count =
          static_cast<int>(bridge_track_groups.size());
      bridge->Release(env);
    }
  }

  std::string summary = "usage=" + std::to_string(actual_attributes.usage);
  summary += ",contentType=" + std::to_string(actual_attributes.content_type);
  summary += ",playbackState=" + std::to_string(static_cast<int>(playback_state));
  summary += ",playWhenReady=" + std::to_string(play_when_ready ? 1 : 0);
  summary += ",isPlaying=" + std::to_string(is_playing ? 1 : 0);
  summary += ",isLoading=" + std::to_string(is_loading ? 1 : 0);
  summary += ",playerErrorCode=" + std::to_string(player_error.error_code);
  summary += ",currentPositionMs=" + std::to_string(current_position_ms);
  summary += ",bufferedPositionMs=" + std::to_string(buffered_position_ms);
  summary += ",durationMs=" + std::to_string(duration_ms);
  summary += ",currentMediaItemIndex=" + std::to_string(current_media_item_index);
  summary += ",mediaItemCount=" + std::to_string(media_item_count);
  summary += ",repeatMode=" + std::to_string(static_cast<int>(repeat_mode));
  summary += ",shuffleModeEnabled=" + std::to_string(shuffle_mode_enabled ? 1 : 0);
  summary += ",volume=" + std::to_string(volume);
  summary += ",timelineWindows=" + std::to_string(timeline.window_count);
  summary += ",timelinePeriods=" + std::to_string(timeline.period_count);
  summary += ",timelineCurrentIndex=" + std::to_string(timeline.current_media_item_index);
  summary += ",timelineHasNext=" + std::to_string(timeline.has_next_media_item ? 1 : 0);
  summary += ",timelineHasPrevious=" + std::to_string(timeline.has_previous_media_item ? 1 : 0);
  summary += ",timelineSeekable=" + std::to_string(timeline.current_media_item_seekable ? 1 : 0);
  summary += ",timelineWindowSnapshots=" + std::to_string(timeline_windows.size());
  summary += ",timelinePeriodSnapshots=" + std::to_string(timeline_periods.size());
  summary += ",tracksGroupCount=" + std::to_string(tracks.groups.size());
  summary += ",trackGroupVectorCount=" + std::to_string(track_groups.size());
  summary += ",bridgeTracksGroupCount=" +
      std::to_string(bridge_tracks_group_count);
  summary += ",bridgeTrackGroupVectorCount=" +
      std::to_string(bridge_track_group_vector_count);
  if (!timeline_windows.empty()) {
    summary += ",timelineWindow0Dynamic=" +
        std::to_string(timeline_windows[0].is_dynamic ? 1 : 0);
    summary += ",timelineWindow0Seekable=" +
        std::to_string(timeline_windows[0].is_seekable ? 1 : 0);
    summary += ",timelineWindow0MediaId=" + timeline_windows[0].media_item_id;
    summary += ",timelineWindow0MediaUri=" + timeline_windows[0].media_item_uri;
    summary += ",timelineWindow0TagPresent=" +
        std::to_string(timeline_windows[0].media_item_tag_present ? 1 : 0);
    summary += ",timelineWindow0TagString=" + timeline_windows[0].media_item_tag_string;
    summary += ",timelineWindow0TagTokenPresent=" +
        std::to_string(timeline_windows[0].media_item_tag_token.empty() ? 0 : 1);
    AppendObjectValueSummary(
        &summary, "timelineWindow0UidValue", timeline_windows[0].uid_value);
    AppendObjectValueSummary(
        &summary, "timelineWindow0ManifestValue", timeline_windows[0].manifest_value);
    summary += ",timelineWindow0Placeholder=" +
        std::to_string(timeline_windows[0].is_placeholder ? 1 : 0);
    summary += ",timelineWindow0PresentationStartMs=" +
        std::to_string(timeline_windows[0].presentation_start_time_ms);
    summary += ",timelineWindow0WindowStartMs=" +
        std::to_string(timeline_windows[0].window_start_time_ms);
    summary += ",timelineWindow0ElapsedRealtimeEpochOffsetMs=" +
        std::to_string(timeline_windows[0].elapsed_realtime_epoch_offset_ms);
    summary += ",timelineWindow0DefaultPositionUs=" +
        std::to_string(timeline_windows[0].default_position_us);
    summary += ",timelineWindow0PositionInFirstPeriodUs=" +
        std::to_string(timeline_windows[0].position_in_first_period_us);
  }
  if (!timeline_periods.empty()) {
    summary += ",timelinePeriod0WindowIndex=" +
        std::to_string(timeline_periods[0].window_index);
    summary += ",timelinePeriod0Uid=" + timeline_periods[0].uid;
    AppendObjectValueSummary(
        &summary, "timelinePeriod0IdValue", timeline_periods[0].id_value);
    AppendObjectValueSummary(
        &summary, "timelinePeriod0UidValue", timeline_periods[0].uid_value);
    AppendObjectValueSummary(
        &summary, "timelinePeriod0AdsIdValue", timeline_periods[0].ads_id_value);
    summary += ",timelinePeriod0Placeholder=" +
        std::to_string(timeline_periods[0].is_placeholder ? 1 : 0);
    summary += ",timelinePeriod0DurationUs=" +
        std::to_string(timeline_periods[0].duration_us);
    summary += ",timelinePeriod0PositionInWindowUs=" +
        std::to_string(timeline_periods[0].position_in_window_us);
  }
  summary += ",bufferedPercentage=" + std::to_string(buffered_percentage);
  summary += ",contentBufferedPositionMs=" + std::to_string(content_buffered_position_ms);
  summary += ",contentDurationMs=" + std::to_string(content_duration_ms);
  summary += ",contentPositionMs=" + std::to_string(content_position_ms);
  summary += ",currentLiveOffsetMs=" + std::to_string(current_live_offset_ms);
  summary += ",currentPeriodIndex=" + std::to_string(current_period_index);
  summary += ",maxSeekToPreviousPositionMs=" +
      std::to_string(max_seek_to_previous_position_ms);
  summary += ",playbackSuppressionReason=" +
      std::to_string(static_cast<int>(playback_suppression_reason));
  summary += ",seekBackIncrementMs=" + std::to_string(seek_back_increment_ms);
  summary += ",seekForwardIncrementMs=" + std::to_string(seek_forward_increment_ms);
  summary += ",speed=" + std::to_string(actual_playback_parameters.speed);
  summary += ",pitch=" + std::to_string(actual_playback_parameters.pitch);
  summary += ",totalBufferedDurationMs=" + std::to_string(total_buffered_duration_ms);
  summary += ",commandPlayPauseAvailable=" +
      std::to_string(command_play_pause_available ? 1 : 0);
  summary += ",canAdvertiseSession=" + std::to_string(can_advertise_session ? 1 : 0);
  summary += ",applicationLooperThread=" + application_looper.thread_name;
  summary += ",applicationLooperThreadId=" + std::to_string(application_looper.thread_id);
  summary += ",applicationLooperCurrentThread=" +
      std::to_string(application_looper.is_current_thread ? 1 : 0);
  summary += ",currentAdGroupIndex=" + std::to_string(current_ad_group_index);
  summary += ",currentAdIndexInGroup=" + std::to_string(current_ad_index_in_group);
  summary += ",isCurrentMediaItemDynamic=" +
      std::to_string(is_current_media_item_dynamic ? 1 : 0);
  summary += ",isCurrentMediaItemLive=" +
      std::to_string(is_current_media_item_live ? 1 : 0);
  summary += ",isCurrentMediaItemSeekable=" +
      std::to_string(is_current_media_item_seekable ? 1 : 0);
  summary += ",isPlayingAd=" + std::to_string(is_playing_ad ? 1 : 0);
  summary += ",cueCount=" + std::to_string(cues.cue_count);
  summary += ",cuePresentationTimeUs=" + std::to_string(cues.presentation_time_us);
  if (!cues.cues.empty()) {
    summary += ",cue0Text=" + cues.cues[0].text;
    summary += ",cue0TextTokenPresent=" +
        std::to_string(cues.cues[0].text_token.empty() ? 0 : 1);
    summary += ",cue0BitmapTokenPresent=" +
        std::to_string(cues.cues[0].bitmap_token.empty() ? 0 : 1);
    summary += ",cue0TextAlignment=" + std::to_string(cues.cues[0].text_alignment);
    summary += ",cue0MultiRowAlignment=" + std::to_string(cues.cues[0].multi_row_alignment);
    summary += ",cue0Line=" + std::to_string(cues.cues[0].line);
    summary += ",cue0LineType=" + std::to_string(cues.cues[0].line_type);
    summary += ",cue0LineAnchor=" + std::to_string(cues.cues[0].line_anchor);
    summary += ",cue0Position=" + std::to_string(cues.cues[0].position);
    summary += ",cue0PositionAnchor=" + std::to_string(cues.cues[0].position_anchor);
    summary += ",cue0Size=" + std::to_string(cues.cues[0].size);
    summary += ",cue0BitmapHeight=" + std::to_string(cues.cues[0].bitmap_height);
    summary += ",cue0TextSize=" + std::to_string(cues.cues[0].text_size);
    summary += ",cue0ShearDegrees=" + std::to_string(cues.cues[0].shear_degrees);
    summary += ",cue0ZIndex=" + std::to_string(cues.cues[0].z_index);
    summary += ",cue0WindowColorSet=" + std::to_string(cues.cues[0].window_color_set ? 1 : 0);
    summary += ",cue0HasBitmap=" + std::to_string(cues.cues[0].has_bitmap ? 1 : 0);
  }
  if (cues.cues.size() > 1) {
    summary += ",cue1Text=" + cues.cues[1].text;
    summary += ",cue1TextTokenPresent=" +
        std::to_string(cues.cues[1].text_token.empty() ? 0 : 1);
    summary += ",cue1BitmapTokenPresent=" +
        std::to_string(cues.cues[1].bitmap_token.empty() ? 0 : 1);
    summary += ",cue1LineType=" + std::to_string(cues.cues[1].line_type);
    summary += ",cue1PositionAnchor=" + std::to_string(cues.cues[1].position_anchor);
    summary += ",cue1TextSize=" + std::to_string(cues.cues[1].text_size);
    summary += ",cue1TextSizeType=" + std::to_string(cues.cues[1].text_size_type);
    summary += ",cue1VerticalType=" + std::to_string(cues.cues[1].vertical_type);
  }
  summary += ",availableCommandCount=" + std::to_string(available_command_count);
  summary += ",commands=" + std::to_string(commands.command_codes.size());
  return NewStringUtfChecked(env, summary, "nativeAudioAndQuerySmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeCurrentTracksSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  TracksSnapshot tracks;
  tracks.contains_audio = true;
  tracks.contains_video = true;
  tracks.contains_text = false;
  tracks.audio_selected = false;
  tracks.video_selected = true;
  tracks.text_selected = false;
  tracks.audio_supported = true;
  tracks.video_supported = true;
  tracks.text_supported = false;
  tracks.audio_supported_allowing_exceeds_capabilities = false;
  tracks.video_supported_allowing_exceeds_capabilities = true;
  tracks.text_supported_allowing_exceeds_capabilities = false;

  TrackGroupSnapshot video_group;
  video_group.id = "video-group";
  video_group.group_token = "generated-opaque-object-token-track-group-video";
  video_group.type = 2;
  video_group.supported = true;
  video_group.supported_allowing_exceeds_capabilities = true;

  TrackInfo video_hd;
  video_hd.id = "video-hd";
  video_hd.label = "Main Video";
  video_hd.label_token = "generated-opaque-object-token-track-label";
  video_hd.mime_type = "video/avc";
  video_hd.container_mime_type = "video/mp4";
  video_hd.codecs = "avc1.640028";
  video_hd.bitrate = 2500000;
  video_hd.average_bitrate = 2000000;
  video_hd.peak_bitrate = 2500000;
  video_hd.metadata_entry_count = 2;
  video_hd.metadata_token = "generated-opaque-object-token-format-metadata";
  video_hd.labels = {{"en", "Main Video"}, {"es", "Video principal"}};
  video_hd.custom_data_token = "generated-opaque-object-token-format-custom-data";
  video_hd.auxiliary_track_type = 2;
  video_hd.max_input_size = 4096;
  video_hd.max_num_reorder_samples = 3;
  video_hd.initialization_data_count = 2;
  video_hd.initialization_data_total_bytes = 7;
  video_hd.initialization_data = {{0x01, 0x02, 0x03}, {0x04, 0x05, 0x06, 0x07}};
  video_hd.drm_scheme_type = "cenc";
  video_hd.drm_scheme_data_count = 1;
  video_hd.drm_scheme_data = {{
      "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed",
      "https://license.example/video",
      "video/mp4",
      {0x08, 0x09},
  }};
  video_hd.drm_scheme_data[0].has_data = true;
  video_hd.subsample_offset_us = 987654;
  video_hd.has_preroll_samples = true;
  video_hd.width = 1920;
  video_hd.height = 1080;
  video_hd.decoded_width = 1936;
  video_hd.decoded_height = 1096;
  video_hd.frame_rate = 30.0f;
  video_hd.rotation_degrees = 90;
  video_hd.pixel_width_height_ratio = 1.25f;
  video_hd.projection_data_length = 4;
  video_hd.projection_data = {0x0D, 0x0E, 0x0F, 0x10};
  video_hd.stereo_mode = 2;
  video_hd.color_standard = 1;
  video_hd.color_range = 2;
  video_hd.color_transfer = 3;
  video_hd.color_hdr_static_info = {0x0A, 0x0B, 0x0C};
  video_hd.color_luma_bitdepth = 10;
  video_hd.color_chroma_bitdepth = 10;
  video_hd.max_sub_layers = 4;
  video_hd.pcm_encoding = -1;
  video_hd.encoder_delay = 0;
  video_hd.encoder_padding = 0;
  video_hd.accessibility_channel = -1;
  video_hd.cue_replacement_behavior = 1;
  video_hd.tile_count_horizontal = 5;
  video_hd.tile_count_vertical = 6;
  video_hd.crypto_type = 2;
  video_hd.role_flags = 0;
  video_hd.selection_flags = 0;
  video_hd.supported_within_capabilities = true;
  video_hd.selected = true;
  video_hd.format_support = 1;

  TrackInfo video_sd;
  video_sd.id = "video-sd";
  video_sd.mime_type = "video/avc";
  video_sd.container_mime_type = "video/mp4";
  video_sd.codecs = "avc1.4d401f";
  video_sd.bitrate = 1200000;
  video_sd.average_bitrate = 1000000;
  video_sd.peak_bitrate = 1200000;
  video_sd.width = 1280;
  video_sd.height = 720;
  video_sd.frame_rate = 30.0f;
  video_sd.accessibility_channel = -1;
  video_sd.role_flags = 0;
  video_sd.selection_flags = 0;
  video_sd.supported_within_capabilities = false;
  video_sd.selected = false;
  video_sd.format_support = 1;

  video_group.tracks.push_back(video_hd);
  video_group.tracks.push_back(video_sd);
  tracks.groups.push_back(video_group);

  TrackGroupSnapshot audio_group;
  audio_group.id = "audio-group";
  audio_group.group_token = "generated-opaque-object-token-track-group-audio";
  audio_group.type = 1;
  audio_group.supported = true;

  TrackInfo audio_main;
  audio_main.id = "audio-main";
  audio_main.language = "en";
  audio_main.label = "Main Audio";
  audio_main.label_token = "generated-opaque-object-token-audio-track-label";
  audio_main.mime_type = "audio/mp4a-latm";
  audio_main.bitrate = 192000;
  audio_main.average_bitrate = 160000;
  audio_main.peak_bitrate = 192000;
  audio_main.metadata_entry_count = 1;
  audio_main.max_input_size = 1024;
  audio_main.initialization_data_count = 1;
  audio_main.initialization_data_total_bytes = 3;
  audio_main.pcm_encoding = 2;
  audio_main.encoder_delay = 12;
  audio_main.encoder_padding = 34;
  audio_main.channel_count = 2;
  audio_main.sample_rate = 48000;
  audio_main.role_flags = 0;
  audio_main.selection_flags = 0;
  audio_main.supported_within_capabilities = true;
  audio_main.selected = false;
  audio_main.format_support = 1;

  audio_group.tracks.push_back(audio_main);
  tracks.groups.push_back(audio_group);
  std::string summary = "groupCount=" + std::to_string(tracks.groups.size());
  summary += ",containsAudio=" + std::to_string(tracks.contains_audio ? 1 : 0);
  summary += ",containsVideo=" + std::to_string(tracks.contains_video ? 1 : 0);
  summary += ",containsText=" + std::to_string(tracks.contains_text ? 1 : 0);
  summary += ",audioSelected=" + std::to_string(tracks.audio_selected ? 1 : 0);
  summary += ",videoSelected=" + std::to_string(tracks.video_selected ? 1 : 0);
  summary += ",textSelected=" + std::to_string(tracks.text_selected ? 1 : 0);
  summary += ",audioSupported=" + std::to_string(tracks.audio_supported ? 1 : 0);
  summary += ",videoSupported=" + std::to_string(tracks.video_supported ? 1 : 0);
  summary += ",textSupported=" + std::to_string(tracks.text_supported ? 1 : 0);
  summary += ",audioSupportedAllowingExceeds=" +
      std::to_string(tracks.audio_supported_allowing_exceeds_capabilities ? 1 : 0);
  summary += ",videoSupportedAllowingExceeds=" +
      std::to_string(tracks.video_supported_allowing_exceeds_capabilities ? 1 : 0);
  summary += ",textSupportedAllowingExceeds=" +
      std::to_string(tracks.text_supported_allowing_exceeds_capabilities ? 1 : 0);
  if (!tracks.groups.empty()) {
    const auto& group = tracks.groups.front();
    summary += ",group0Id=" + group.id;
    summary += ",group0TokenPresent=" +
        std::to_string(group.group_token.empty() ? 0 : 1);
    summary += ",group0Type=" + std::to_string(group.type);
    summary += ",group0TrackCount=" + std::to_string(group.tracks.size());
    summary += ",group0Supported=" + std::to_string(group.supported ? 1 : 0);
    if (!group.tracks.empty()) {
      const auto& track = group.tracks.front();
      summary += ",track0Id=" + track.id;
      summary += ",track0Language=" + track.language;
      summary += ",track0Label=" + track.label;
      summary += ",track0LabelTokenPresent=" +
          std::to_string(track.label_token.empty() ? 0 : 1);
      summary += ",track0MimeType=" + track.mime_type;
      summary += ",track0ContainerMimeType=" + track.container_mime_type;
      summary += ",track0Codecs=" + track.codecs;
      summary += ",track0Bitrate=" + std::to_string(track.bitrate);
      summary += ",track0AverageBitrate=" + std::to_string(track.average_bitrate);
      summary += ",track0PeakBitrate=" + std::to_string(track.peak_bitrate);
      summary += ",track0MetadataEntryCount=" + std::to_string(track.metadata_entry_count);
      summary += ",track0MetadataTokenPresent=" +
          std::to_string(track.metadata_token.empty() ? 0 : 1);
      summary += ",track0LabelCount=" + std::to_string(track.labels.size());
      if (!track.labels.empty()) {
        summary += ",track0Label0Language=" + track.labels[0].language;
        summary += ",track0Label0Value=" + track.labels[0].value;
      }
      summary += ",track0CustomDataTokenPresent=" +
          std::to_string(track.custom_data_token.empty() ? 0 : 1);
      summary += ",track0AuxiliaryTrackType=" + std::to_string(track.auxiliary_track_type);
      summary += ",track0MaxInputSize=" + std::to_string(track.max_input_size);
      summary += ",track0MaxNumReorderSamples=" +
          std::to_string(track.max_num_reorder_samples);
      summary += ",track0InitializationData=" +
          std::to_string(track.initialization_data_count) + ":" +
          std::to_string(track.initialization_data_total_bytes);
      summary += ",track0InitializationDataVectorCount=" +
          std::to_string(track.initialization_data.size());
      summary += ",track0DrmSchemeDataCount=" +
          std::to_string(track.drm_scheme_data_count);
      summary += ",track0DrmSchemeType=" + track.drm_scheme_type;
      if (!track.drm_scheme_data.empty()) {
        summary += ",track0DrmSchemeUuid=" + track.drm_scheme_data[0].uuid;
        summary += ",track0DrmSchemeLicenseUrl=" +
            track.drm_scheme_data[0].license_server_url;
        summary += ",track0DrmSchemeMimeType=" + track.drm_scheme_data[0].mime_type;
        summary +=
            ",track0DrmSchemeDataLength=" + std::to_string(track.drm_scheme_data[0].data.size());
        summary += ",track0DrmSchemeHasData=" +
            std::to_string(track.drm_scheme_data[0].has_data ? 1 : 0);
      }
      summary += ",track0SubsampleOffsetUs=" + std::to_string(track.subsample_offset_us);
      summary += ",track0HasPrerollSamples=" +
          std::to_string(track.has_preroll_samples ? 1 : 0);
      summary += ",track0Width=" + std::to_string(track.width);
      summary += ",track0Height=" + std::to_string(track.height);
      summary += ",track0DecodedSize=" + std::to_string(track.decoded_width) + "x" +
          std::to_string(track.decoded_height);
      summary += ",track0FrameRate=" + std::to_string(track.frame_rate);
      summary += ",track0RotationDegrees=" + std::to_string(track.rotation_degrees);
      summary += ",track0PixelRatio=" + std::to_string(track.pixel_width_height_ratio);
      summary += ",track0ProjectionDataLength=" +
          std::to_string(track.projection_data_length);
      summary += ",track0ProjectionDataVectorLength=" +
          std::to_string(track.projection_data.size());
      summary += ",track0StereoMode=" + std::to_string(track.stereo_mode);
      summary += ",track0Color=" + std::to_string(track.color_standard) + ":" +
          std::to_string(track.color_range) + ":" + std::to_string(track.color_transfer);
      summary += ",track0ColorHdrStaticInfoLength=" +
          std::to_string(track.color_hdr_static_info.size());
      summary += ",track0ColorBitdepth=" + std::to_string(track.color_luma_bitdepth) + ":" +
          std::to_string(track.color_chroma_bitdepth);
      summary += ",track0MaxSubLayers=" + std::to_string(track.max_sub_layers);
      summary += ",track0PcmEncoding=" + std::to_string(track.pcm_encoding);
      summary += ",track0EncoderTrim=" + std::to_string(track.encoder_delay) + ":" +
          std::to_string(track.encoder_padding);
      summary += ",track0AccessibilityChannel=" + std::to_string(track.accessibility_channel);
      summary += ",track0CueReplacementBehavior=" +
          std::to_string(track.cue_replacement_behavior);
      summary += ",track0Tiles=" + std::to_string(track.tile_count_horizontal) + "x" +
          std::to_string(track.tile_count_vertical);
      summary += ",track0CryptoType=" + std::to_string(track.crypto_type);
      summary += ",track0RoleFlags=" + std::to_string(track.role_flags);
      summary += ",track0SelectionFlags=" + std::to_string(track.selection_flags);
      summary += ",track0SupportedWithinCapabilities=" +
          std::to_string(track.supported_within_capabilities ? 1 : 0);
      summary += ",track0Selected=" + std::to_string(track.selected ? 1 : 0);
      summary += ",track0FormatSupport=" + std::to_string(track.format_support);
    }
  }
  if (tracks.groups.size() > 1) {
    const auto& group = tracks.groups[1];
    summary += ",group1Id=" + group.id;
    summary += ",group1TokenPresent=" +
        std::to_string(group.group_token.empty() ? 0 : 1);
    summary += ",group1Type=" + std::to_string(group.type);
    summary += ",group1TrackCount=" + std::to_string(group.tracks.size());
    summary += ",group1Supported=" + std::to_string(group.supported ? 1 : 0);
    if (!group.tracks.empty()) {
      const auto& track = group.tracks.front();
      summary += ",group1Track0Id=" + track.id;
      summary += ",group1Track0Language=" + track.language;
      summary += ",group1Track0Label=" + track.label;
      summary += ",group1Track0LabelTokenPresent=" +
          std::to_string(track.label_token.empty() ? 0 : 1);
      summary += ",group1Track0MimeType=" + track.mime_type;
      summary += ",group1Track0Bitrate=" + std::to_string(track.bitrate);
      summary += ",group1Track0AverageBitrate=" + std::to_string(track.average_bitrate);
      summary += ",group1Track0PeakBitrate=" + std::to_string(track.peak_bitrate);
      summary += ",group1Track0MetadataEntryCount=" +
          std::to_string(track.metadata_entry_count);
      summary += ",group1Track0InitializationData=" +
          std::to_string(track.initialization_data_count) + ":" +
          std::to_string(track.initialization_data_total_bytes);
      summary += ",group1Track0PcmEncoding=" + std::to_string(track.pcm_encoding);
      summary += ",group1Track0EncoderTrim=" + std::to_string(track.encoder_delay) + ":" +
          std::to_string(track.encoder_padding);
      summary += ",group1Track0ChannelCount=" + std::to_string(track.channel_count);
      summary += ",group1Track0SampleRate=" + std::to_string(track.sample_rate);
      summary += ",group1Track0RoleFlags=" + std::to_string(track.role_flags);
      summary += ",group1Track0SelectionFlags=" + std::to_string(track.selection_flags);
      summary += ",group1Track0SupportedWithinCapabilities=" +
          std::to_string(track.supported_within_capabilities ? 1 : 0);
    }
  }
  return NewStringUtfChecked(env, summary, "nativeCurrentTracksSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeCurrentTimelineSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  TimelineDetailsSnapshot timeline;
  timeline.summary.window_count = 2;
  timeline.summary.period_count = 2;
  timeline.summary.empty = false;
  timeline.summary.current_media_item_index = 0;
  timeline.summary.next_media_item_index = 1;
  timeline.summary.previous_media_item_index = -1;
  timeline.summary.has_next_media_item = true;
  timeline.summary.has_previous_media_item = false;
  timeline.summary.current_media_item_dynamic = true;
  timeline.summary.current_media_item_live = false;
  timeline.summary.current_media_item_seekable = false;

  TimelineWindowSnapshot first_window;
  first_window.media_item_index = 0;
  first_window.media_item_id = "timeline-query-item-1";
  first_window.media_item_uri = "https://example.com/current-timeline-one.m3u8";
  first_window.media_item_tag_present = true;
  first_window.media_item_tag_string = "timeline-query-tag-1";
  first_window.media_item_tag_token = "generated-opaque-object-token-timeline-query-tag-1";
  first_window.uid = "window-uid-0";
  first_window.uid_token = "generated-opaque-object-token-window-uid-0";
  first_window.uid_value.present = true;
  first_window.uid_value.class_name = "java.lang.String";
  first_window.uid_value.value_type = ObjectValueInfo::kString;
  first_window.uid_value.string_value = "window-uid-0";
  first_window.live_configuration_present = false;
  first_window.manifest_present = false;
  first_window.first_period_index = 0;
  first_window.last_period_index = 0;
  first_window.default_position_ms = 0;
  first_window.position_in_first_period_ms = 0;
  first_window.position_in_first_period_us = 0;
  first_window.is_seekable = false;
  first_window.is_dynamic = true;
  first_window.is_live = false;
  first_window.is_placeholder = false;
  first_window.default_position_us = 0;
  timeline.windows.push_back(first_window);

  TimelineWindowSnapshot second_window;
  second_window.media_item_index = 1;
  second_window.media_item_id = "timeline-query-item-2";
  second_window.media_item_uri = "https://example.com/current-timeline-two.mp4";
  second_window.media_item_tag_present = true;
  second_window.media_item_tag_string = "timeline-query-tag-2";
  second_window.media_item_tag_token = "generated-opaque-object-token-timeline-query-tag-2";
  second_window.uid = "window-uid-1";
  second_window.uid_token = "generated-opaque-object-token-window-uid-1";
  second_window.uid_value.present = true;
  second_window.uid_value.class_name = "java.lang.String";
  second_window.uid_value.value_type = ObjectValueInfo::kString;
  second_window.uid_value.string_value = "window-uid-1";
  second_window.live_configuration_present = true;
  second_window.live_target_offset_ms = 7100;
  second_window.live_min_offset_ms = 6400;
  second_window.live_max_offset_ms = 8200;
  second_window.live_min_playback_speed = 0.93f;
  second_window.live_max_playback_speed = 1.07f;
  second_window.manifest_present = true;
  second_window.manifest_string = "timeline-query-manifest";
  second_window.manifest_token = "generated-opaque-object-token-timeline-query-manifest";
  second_window.manifest_value.present = true;
  second_window.manifest_value.class_name = "java.lang.String";
  second_window.manifest_value.value_type = ObjectValueInfo::kString;
  second_window.manifest_value.string_value = "timeline-query-manifest";
  second_window.first_period_index = 1;
  second_window.last_period_index = 1;
  second_window.default_position_ms = 0;
  second_window.is_seekable = false;
  second_window.is_dynamic = true;
  second_window.is_live = false;
  second_window.is_placeholder = false;
  second_window.default_position_us = 0;
  timeline.windows.push_back(second_window);

  TimelinePeriodSnapshot first_period;
  first_period.id = "period-0";
  first_period.id_token = "generated-opaque-object-token-period-id-0";
  first_period.id_value.present = true;
  first_period.id_value.class_name = "java.lang.String";
  first_period.id_value.value_type = ObjectValueInfo::kString;
  first_period.id_value.string_value = "period-0";
  first_period.uid = "period-uid-0";
  first_period.uid_token = "generated-opaque-object-token-period-uid-0";
  first_period.uid_value.present = true;
  first_period.uid_value.class_name = "java.lang.String";
  first_period.uid_value.value_type = ObjectValueInfo::kString;
  first_period.uid_value.string_value = "period-uid-0";
  first_period.ads_id = "";
  first_period.window_index = 0;
  first_period.ad_group_count = 0;
  first_period.position_in_window_ms = 0;
  first_period.position_in_window_us = 0;
  first_period.is_placeholder = false;
  timeline.periods.push_back(first_period);

  TimelinePeriodSnapshot second_period;
  second_period.id = "period-1";
  second_period.id_token = "generated-opaque-object-token-period-id-1";
  second_period.id_value.present = true;
  second_period.id_value.class_name = "java.lang.String";
  second_period.id_value.value_type = ObjectValueInfo::kString;
  second_period.id_value.string_value = "period-1";
  second_period.uid = "period-uid-1";
  second_period.uid_token = "generated-opaque-object-token-period-uid-1";
  second_period.uid_value.present = true;
  second_period.uid_value.class_name = "java.lang.String";
  second_period.uid_value.value_type = ObjectValueInfo::kString;
  second_period.uid_value.string_value = "period-uid-1";
  second_period.ads_id = "period-ads-1";
  second_period.ads_id_token = "generated-opaque-object-token-period-ads-id-1";
  second_period.ads_id_value.present = true;
  second_period.ads_id_value.class_name = "java.lang.String";
  second_period.ads_id_value.value_type = ObjectValueInfo::kString;
  second_period.ads_id_value.string_value = "period-ads-1";
  second_period.window_index = 1;
  second_period.ad_group_count = 0;
  second_period.position_in_window_ms = 0;
  second_period.position_in_window_us = 0;
  second_period.is_placeholder = false;
  timeline.periods.push_back(second_period);
  std::string summary = "windowCount=" + std::to_string(timeline.summary.window_count);
  summary += ",periodCount=" + std::to_string(timeline.summary.period_count);
  summary += ",empty=" + std::to_string(timeline.summary.empty ? 1 : 0);
  summary += ",currentMediaItemIndex=" +
      std::to_string(timeline.summary.current_media_item_index);
  summary += ",nextMediaItemIndex=" +
      std::to_string(timeline.summary.next_media_item_index);
  summary += ",previousMediaItemIndex=" +
      std::to_string(timeline.summary.previous_media_item_index);
  summary += ",hasNext=" + std::to_string(timeline.summary.has_next_media_item ? 1 : 0);
  summary += ",hasPrevious=" + std::to_string(timeline.summary.has_previous_media_item ? 1 : 0);
  summary += ",windowSnapshotCount=" + std::to_string(timeline.windows.size());
  summary += ",periodSnapshotCount=" + std::to_string(timeline.periods.size());
  if (!timeline.windows.empty()) {
    const auto& window = timeline.windows.front();
    summary += ",window0MediaItemIndex=" + std::to_string(window.media_item_index);
    summary += ",window0MediaId=" + window.media_item_id;
    summary += ",window0MediaUri=" + window.media_item_uri;
    summary += ",window0TagPresent=" + std::to_string(window.media_item_tag_present ? 1 : 0);
    summary += ",window0TagString=" + window.media_item_tag_string;
    summary += ",window0TagTokenPresent=" +
        std::to_string(window.media_item_tag_token.empty() ? 0 : 1);
    summary += ",window0Uid=" + window.uid;
    summary += ",window0UidTokenPresent=" +
        std::to_string(window.uid_token.empty() ? 0 : 1);
    AppendObjectValueSummary(&summary, "window0UidValue", window.uid_value);
    summary += ",window0LiveConfigurationPresent=" +
        std::to_string(window.live_configuration_present ? 1 : 0);
    summary += ",window0LiveTargetOffsetMs=" +
        std::to_string(window.live_target_offset_ms);
    summary += ",window0LiveMinOffsetMs=" +
        std::to_string(window.live_min_offset_ms);
    summary += ",window0LiveMaxOffsetMs=" +
        std::to_string(window.live_max_offset_ms);
    summary += ",window0LiveMinSpeed=" +
        std::to_string(window.live_min_playback_speed);
    summary += ",window0LiveMaxSpeed=" +
        std::to_string(window.live_max_playback_speed);
    summary += ",window0ManifestPresent=" + std::to_string(window.manifest_present ? 1 : 0);
    summary += ",window0ManifestString=" + window.manifest_string;
    summary += ",window0ManifestTokenPresent=" +
        std::to_string(window.manifest_token.empty() ? 0 : 1);
    AppendObjectValueSummary(&summary, "window0ManifestValue", window.manifest_value);
    summary += ",window0FirstPeriodIndex=" + std::to_string(window.first_period_index);
    summary += ",window0LastPeriodIndex=" + std::to_string(window.last_period_index);
    summary += ",window0PresentationStartTimeMs=" +
        std::to_string(window.presentation_start_time_ms);
    summary += ",window0WindowStartTimeMs=" + std::to_string(window.window_start_time_ms);
    summary += ",window0ElapsedRealtimeEpochOffsetMs=" +
        std::to_string(window.elapsed_realtime_epoch_offset_ms);
    summary += ",window0DurationMs=" + std::to_string(window.duration_ms);
    summary += ",window0DefaultPositionMs=" + std::to_string(window.default_position_ms);
    summary += ",window0PositionInFirstPeriodMs=" +
        std::to_string(window.position_in_first_period_ms);
    summary += ",window0PositionInFirstPeriodUs=" +
        std::to_string(window.position_in_first_period_us);
    summary += ",window0Seekable=" + std::to_string(window.is_seekable ? 1 : 0);
    summary += ",window0Dynamic=" + std::to_string(window.is_dynamic ? 1 : 0);
    summary += ",window0Live=" + std::to_string(window.is_live ? 1 : 0);
    summary += ",window0Placeholder=" + std::to_string(window.is_placeholder ? 1 : 0);
    summary += ",window0DefaultPositionUs=" + std::to_string(window.default_position_us);
    summary += ",window0DurationUs=" + std::to_string(window.duration_us);
  }
  if (timeline.windows.size() > 1) {
    const auto& window = timeline.windows[1];
    summary += ",window1MediaItemIndex=" + std::to_string(window.media_item_index);
    summary += ",window1MediaId=" + window.media_item_id;
    summary += ",window1MediaUri=" + window.media_item_uri;
    summary += ",window1TagPresent=" + std::to_string(window.media_item_tag_present ? 1 : 0);
    summary += ",window1TagString=" + window.media_item_tag_string;
    summary += ",window1TagTokenPresent=" +
        std::to_string(window.media_item_tag_token.empty() ? 0 : 1);
    summary += ",window1Uid=" + window.uid;
    summary += ",window1UidTokenPresent=" +
        std::to_string(window.uid_token.empty() ? 0 : 1);
    AppendObjectValueSummary(&summary, "window1UidValue", window.uid_value);
    summary += ",window1LiveConfigurationPresent=" +
        std::to_string(window.live_configuration_present ? 1 : 0);
    summary += ",window1LiveTargetOffsetMs=" +
        std::to_string(window.live_target_offset_ms);
    summary += ",window1LiveMinOffsetMs=" +
        std::to_string(window.live_min_offset_ms);
    summary += ",window1LiveMaxOffsetMs=" +
        std::to_string(window.live_max_offset_ms);
    summary += ",window1LiveMinSpeed=" +
        std::to_string(window.live_min_playback_speed);
    summary += ",window1LiveMaxSpeed=" +
        std::to_string(window.live_max_playback_speed);
    summary += ",window1ManifestPresent=" + std::to_string(window.manifest_present ? 1 : 0);
    summary += ",window1ManifestString=" + window.manifest_string;
    summary += ",window1ManifestTokenPresent=" +
        std::to_string(window.manifest_token.empty() ? 0 : 1);
    AppendObjectValueSummary(&summary, "window1ManifestValue", window.manifest_value);
    summary += ",window1FirstPeriodIndex=" + std::to_string(window.first_period_index);
    summary += ",window1LastPeriodIndex=" + std::to_string(window.last_period_index);
    summary += ",window1DurationMs=" + std::to_string(window.duration_ms);
    summary += ",window1DefaultPositionMs=" + std::to_string(window.default_position_ms);
    summary += ",window1Seekable=" + std::to_string(window.is_seekable ? 1 : 0);
    summary += ",window1Dynamic=" + std::to_string(window.is_dynamic ? 1 : 0);
    summary += ",window1Live=" + std::to_string(window.is_live ? 1 : 0);
    summary += ",window1Placeholder=" + std::to_string(window.is_placeholder ? 1 : 0);
    summary += ",window1DefaultPositionUs=" + std::to_string(window.default_position_us);
    summary += ",window1DurationUs=" + std::to_string(window.duration_us);
  }
  if (!timeline.periods.empty()) {
    const auto& period = timeline.periods.front();
    summary += ",period0Id=" + period.id;
    summary += ",period0IdTokenPresent=" +
        std::to_string(period.id_token.empty() ? 0 : 1);
    AppendObjectValueSummary(&summary, "period0IdValue", period.id_value);
    summary += ",period0Uid=" + period.uid;
    summary += ",period0UidTokenPresent=" +
        std::to_string(period.uid_token.empty() ? 0 : 1);
    AppendObjectValueSummary(&summary, "period0UidValue", period.uid_value);
    summary += ",period0AdsId=" + period.ads_id;
    summary += ",period0AdsIdTokenPresent=" +
        std::to_string(period.ads_id_token.empty() ? 0 : 1);
    AppendObjectValueSummary(&summary, "period0AdsIdValue", period.ads_id_value);
    summary += ",period0WindowIndex=" + std::to_string(period.window_index);
    summary += ",period0AdGroupCount=" + std::to_string(period.ad_group_count);
    summary += ",period0DurationMs=" + std::to_string(period.duration_ms);
    summary += ",period0DurationUs=" + std::to_string(period.duration_us);
    summary += ",period0PositionInWindowMs=" + std::to_string(period.position_in_window_ms);
    summary += ",period0PositionInWindowUs=" + std::to_string(period.position_in_window_us);
    summary += ",period0Placeholder=" + std::to_string(period.is_placeholder ? 1 : 0);
  }
  if (timeline.periods.size() > 1) {
    const auto& period = timeline.periods[1];
    summary += ",period1Id=" + period.id;
    summary += ",period1IdTokenPresent=" +
        std::to_string(period.id_token.empty() ? 0 : 1);
    AppendObjectValueSummary(&summary, "period1IdValue", period.id_value);
    summary += ",period1Uid=" + period.uid;
    summary += ",period1UidTokenPresent=" +
        std::to_string(period.uid_token.empty() ? 0 : 1);
    AppendObjectValueSummary(&summary, "period1UidValue", period.uid_value);
    summary += ",period1AdsId=" + period.ads_id;
    summary += ",period1AdsIdTokenPresent=" +
        std::to_string(period.ads_id_token.empty() ? 0 : 1);
    AppendObjectValueSummary(&summary, "period1AdsIdValue", period.ads_id_value);
    summary += ",period1WindowIndex=" + std::to_string(period.window_index);
    summary += ",period1AdGroupCount=" + std::to_string(period.ad_group_count);
    summary += ",period1DurationMs=" + std::to_string(period.duration_ms);
    summary += ",period1DurationUs=" + std::to_string(period.duration_us);
    summary += ",period1PositionInWindowMs=" + std::to_string(period.position_in_window_ms);
    summary += ",period1PositionInWindowUs=" + std::to_string(period.position_in_window_us);
    summary += ",period1Placeholder=" + std::to_string(period.is_placeholder ? 1 : 0);
  }
  return NewStringUtfChecked(env, summary, "nativeCurrentTimelineSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAvailableCommandsSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/available-commands.mp4";
  media_item.media_id = "available-commands-item";
  player->SetMediaItem(media_item);

  AvailableCommandsSnapshot commands = player->GetAvailableCommands();
  std::string summary = "count=" + std::to_string(commands.Count());
  summary += ",containsPlayPause=" + std::to_string(commands.Contains(1) ? 1 : 0);
  summary += ",containsGetTimeline=" + std::to_string(commands.Contains(17) ? 1 : 0);
  summary += ",containsGetTracks=" + std::to_string(commands.Contains(30) ? 1 : 0);
  return NewStringUtfChecked(env, summary, "nativeAvailableCommandsSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeCurrentMediaItemQuerySmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/current-item.m3u8";
  media_item.media_id = "current-item";
  media_item.source_type = MediaSourceType::kHls;
  media_item.tag_present = true;
  media_item.tag_string = "current-tag";
  media_item.subtitle_configurations.push_back(
      {"https://example.com/current.vtt", "text/vtt", "en", "English", "sub-1", 5, 11});
  media_item.subtitle_configurations.push_back(
      {"https://example.com/current-es.vtt", "text/vtt", "es", "Spanish", "sub-2", 3, 13});
  media_item.clipping_configuration.start_position_ms = 1000;
  media_item.clipping_configuration.end_position_ms = 9000;
  media_item.clipping_configuration.relative_to_live_window = true;
  media_item.clipping_configuration.relative_to_default_position = true;
  media_item.clipping_configuration.starts_at_key_frame = true;
  media_item.clipping_configuration.allow_unseekable_media = true;
  media_item.live_configuration.target_offset_ms = 3000;
  media_item.live_configuration.min_offset_ms = 2500;
  media_item.live_configuration.max_offset_ms = 4500;
  media_item.live_configuration.min_playback_speed = 0.96f;
  media_item.live_configuration.max_playback_speed = 1.04f;
  media_item.drm_configuration.scheme_uuid = "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed";
  media_item.drm_configuration.license_uri = "https://license.example.com";
  media_item.drm_configuration.license_request_header_names = {"Authorization", "X-Client"};
  media_item.drm_configuration.license_request_header_values = {"Bearer current-token", "cppbridge"};
  media_item.drm_configuration.forced_session_track_types = {2};
  media_item.drm_configuration.key_set_id = {0x10, 0x20, 0x30};
  media_item.drm_configuration.multi_session = true;
  media_item.drm_configuration.force_default_license_uri = true;
  media_item.drm_configuration.play_clear_content_without_key = false;
  media_item.request_metadata.media_uri = "https://example.com/current-request";
  media_item.request_metadata.search_query = "current search";
  media_item.request_metadata.extras_present = true;
  media_item.request_metadata.extras_key_count = 5;
  media_item.request_metadata.extras_values = {
      {"enabled", BundleValueInfo::kBoolean, "", 0, 0.0, true, {}},
      {"episode", BundleValueInfo::kLong, "", 42, 0.0, false, {}},
      {"gain", BundleValueInfo::kDouble, "", 0, 1.5, false, {}},
      {"payload", BundleValueInfo::kByteArray, "", 0, 0.0, false, {1, 2, 3}},
      {"source", BundleValueInfo::kString, "cppbridge", 0, 0.0, false, {}},
  };
  media_item.ads_configuration.ad_tag_uri = "https://ads.example.com/tag.xml";
  media_item.ads_configuration.ads_id = "ads-current";
  media_item.media_metadata.title = "Current Item Title";
  media_item.media_metadata.artist = "Current Item Artist";
  media_item.media_metadata.album_title = "Current Album";
  media_item.media_metadata.album_artist = "Current Album Artist";
  media_item.media_metadata.display_title = "Current Item Display";
  media_item.media_metadata.subtitle = "Current Item Subtitle";
  media_item.media_metadata.description = "Current Item Description";
  media_item.media_metadata.author = "Current Item Author";
  media_item.media_metadata.composer = "Current Item Composer";
  media_item.media_metadata.conductor = "Current Item Conductor";
  media_item.media_metadata.duration_ms = 321000;
  media_item.media_metadata.track_number = 6;
  media_item.media_metadata.total_track_count = 14;
  media_item.media_metadata.is_browsable = 0;
  media_item.media_metadata.is_playable = 1;
  media_item.media_metadata.folder_type = 3;
  media_item.media_metadata.recording_year = 2023;
  media_item.media_metadata.recording_month = 8;
  media_item.media_metadata.recording_day = 19;
  media_item.media_metadata.release_year = 2025;
  media_item.media_metadata.release_month = 5;
  media_item.media_metadata.release_day = 11;
  media_item.media_metadata.writer = "Current Item Writer";
  media_item.media_metadata.disc_number = 2;
  media_item.media_metadata.total_disc_count = 4;
  media_item.media_metadata.genre = "Current Item Genre";
  media_item.media_metadata.compilation = "Current Item Compilation";
  media_item.media_metadata.media_type = 7;
  media_item.media_metadata.station = "Current Item Station";
  media_item.media_metadata.extras_present = true;
  media_item.media_metadata.extras_key_count = 5;
  media_item.media_metadata.extras_values = {
      {"available", BundleValueInfo::kBoolean, "", 0, 0.0, true, {}},
      {"blob", BundleValueInfo::kByteArray, "", 0, 0.0, false, {9, 8, 7}},
      {"rating", BundleValueInfo::kDouble, "", 0, 4.5, false, {}},
      {"season", BundleValueInfo::kLong, "", 2, 0.0, false, {}},
      {"studio", BundleValueInfo::kString, "Studio", 0, 0.0, false, {}},
  };
  media_item.media_metadata.artwork_uri = "https://example.com/current-artwork.jpg";
  media_item.media_metadata.artwork_data = {1, 2, 3, 4};
  media_item.media_metadata.artwork_data_type = 3;
  player->SetMediaItem(media_item);
  MediaItemDescriptor current_media_item = player->GetCurrentMediaItem();
  std::string summary = "exists=" + std::to_string(current_media_item.uri.empty() ? 0 : 1);
  summary += ",mediaId=" + current_media_item.media_id;
  summary += ",uri=" + current_media_item.uri;
  summary += ",mimeType=" + current_media_item.mime_type;
  summary += ",sourceType=" + std::to_string(static_cast<int>(current_media_item.source_type));
  summary += ",tagPresent=" + std::to_string(current_media_item.tag_present ? 1 : 0);
  summary += ",tagString=" + current_media_item.tag_string;
  summary += ",tagTokenPresent=" +
      std::to_string(current_media_item.tag_token.empty() ? 0 : 1);
  AppendObjectValueSummary(&summary, "tagValue", current_media_item.tag_value);
  summary += ",subtitleCount=" +
      std::to_string(current_media_item.subtitle_configurations.size());
  if (!current_media_item.subtitle_configurations.empty()) {
    const auto& subtitle = current_media_item.subtitle_configurations[0];
    summary += ",subtitle0Uri=" + subtitle.uri;
    summary += ",subtitle0MimeType=" + subtitle.mime_type;
    summary += ",subtitle0Language=" + subtitle.language;
    summary += ",subtitle0Label=" + subtitle.label;
    summary += ",subtitle0Id=" + subtitle.id;
    summary += ",subtitle0SelectionFlags=" + std::to_string(subtitle.selection_flags);
    summary += ",subtitle0RoleFlags=" + std::to_string(subtitle.role_flags);
  }
  if (current_media_item.subtitle_configurations.size() > 1) {
    const auto& subtitle = current_media_item.subtitle_configurations[1];
    summary += ",subtitle1Uri=" + subtitle.uri;
    summary += ",subtitle1MimeType=" + subtitle.mime_type;
    summary += ",subtitle1Language=" + subtitle.language;
    summary += ",subtitle1Label=" + subtitle.label;
    summary += ",subtitle1Id=" + subtitle.id;
    summary += ",subtitle1SelectionFlags=" + std::to_string(subtitle.selection_flags);
    summary += ",subtitle1RoleFlags=" + std::to_string(subtitle.role_flags);
  }
  summary += ",hasClipping=" +
      std::to_string(current_media_item.clipping_configuration.start_position_ms != 0 ? 1 : 0);
  summary += ",clippingStartMs=" +
      std::to_string(current_media_item.clipping_configuration.start_position_ms);
  summary += ",clippingEndMs=" +
      std::to_string(current_media_item.clipping_configuration.end_position_ms);
  summary += ",clippingRelativeToLiveWindow=" +
      std::to_string(current_media_item.clipping_configuration.relative_to_live_window ? 1 : 0);
  summary += ",clippingRelativeToDefaultPosition=" + std::to_string(
      current_media_item.clipping_configuration.relative_to_default_position ? 1 : 0);
  summary += ",clippingStartsAtKeyFrame=" +
      std::to_string(current_media_item.clipping_configuration.starts_at_key_frame ? 1 : 0);
  summary += ",clippingAllowUnseekable=" + std::to_string(
      current_media_item.clipping_configuration.allow_unseekable_media ? 1 : 0);
  summary += ",hasLiveConfiguration=" +
      std::to_string(current_media_item.live_configuration.target_offset_ms != -9223372036854775807LL
                         ? 1
                         : 0);
  summary += ",liveTargetOffsetMs=" +
      std::to_string(current_media_item.live_configuration.target_offset_ms);
  summary += ",liveMinOffsetMs=" +
      std::to_string(current_media_item.live_configuration.min_offset_ms);
  summary += ",liveMaxOffsetMs=" +
      std::to_string(current_media_item.live_configuration.max_offset_ms);
  summary += ",liveMinSpeed=" +
      std::to_string(current_media_item.live_configuration.min_playback_speed);
  summary += ",liveMaxSpeed=" +
      std::to_string(current_media_item.live_configuration.max_playback_speed);
  summary += ",hasDrmConfiguration=" +
      std::to_string(current_media_item.drm_configuration.scheme_uuid.empty() ? 0 : 1);
  summary += ",drmScheme=" + current_media_item.drm_configuration.scheme_uuid;
  summary += ",drmLicenseUri=" + current_media_item.drm_configuration.license_uri;
  summary += ",drmHeaderCount=" +
      std::to_string(current_media_item.drm_configuration.license_request_header_names.size());
  if (!current_media_item.drm_configuration.license_request_header_names.empty() &&
      !current_media_item.drm_configuration.license_request_header_values.empty()) {
    summary += ",drmHeader0=" + current_media_item.drm_configuration.license_request_header_names[0] +
        ":" + current_media_item.drm_configuration.license_request_header_values[0];
  }
  if (current_media_item.drm_configuration.license_request_header_names.size() > 1 &&
      current_media_item.drm_configuration.license_request_header_values.size() > 1) {
    summary += ",drmHeader1=" + current_media_item.drm_configuration.license_request_header_names[1] +
        ":" + current_media_item.drm_configuration.license_request_header_values[1];
  }
  summary += ",drmForcedSessionTrackTypeCount=" +
      std::to_string(current_media_item.drm_configuration.forced_session_track_types.size());
  summary += ",drmKeySetIdLength=" +
      std::to_string(current_media_item.drm_configuration.key_set_id.size());
  summary += ",drmMultiSession=" +
      std::to_string(current_media_item.drm_configuration.multi_session ? 1 : 0);
  summary += ",drmForceDefaultLicenseUri=" + std::to_string(
      current_media_item.drm_configuration.force_default_license_uri ? 1 : 0);
  summary += ",drmPlayClearContentWithoutKey=" + std::to_string(
      current_media_item.drm_configuration.play_clear_content_without_key ? 1 : 0);
  summary += ",requestMetadataMediaUri=" + current_media_item.request_metadata.media_uri;
  summary += ",requestMetadataSearchQuery=" + current_media_item.request_metadata.search_query;
  summary += ",requestMetadataExtrasPresent=" +
      std::to_string(current_media_item.request_metadata.extras_present ? 1 : 0);
  summary += ",requestMetadataExtrasKeyCount=" +
      std::to_string(current_media_item.request_metadata.extras_key_count);
  summary += ",requestMetadataExtrasTokenPresent=" +
      std::to_string(current_media_item.request_metadata.extras_token.empty() ? 0 : 1);
  AppendBundleValueSummary(
      &summary, "requestMetadataExtras", current_media_item.request_metadata.extras_values);
  summary += ",adTagUri=" + current_media_item.ads_configuration.ad_tag_uri;
  summary += ",adsId=" + current_media_item.ads_configuration.ads_id;
  summary += ",adsIdTokenPresent=" +
      std::to_string(current_media_item.ads_configuration.ads_id_token.empty() ? 0 : 1);
  AppendObjectValueSummary(
      &summary, "adsIdValue", current_media_item.ads_configuration.ads_id_value);
  summary += ",mediaMetadataTitle=" + current_media_item.media_metadata.title;
  summary += ",mediaMetadataTitleTokenPresent=" +
      std::to_string(current_media_item.media_metadata.title_token.empty() ? 0 : 1);
  AppendObjectValueSummary(
      &summary, "mediaMetadataTitleValue", current_media_item.media_metadata.title_value);
  summary += ",mediaMetadataArtist=" + current_media_item.media_metadata.artist;
  summary += ",mediaMetadataArtistTokenPresent=" +
      std::to_string(current_media_item.media_metadata.artist_token.empty() ? 0 : 1);
  summary += ",mediaMetadataAlbumTitle=" + current_media_item.media_metadata.album_title;
  summary += ",mediaMetadataAlbumTitleTokenPresent=" +
      std::to_string(current_media_item.media_metadata.album_title_token.empty() ? 0 : 1);
  summary += ",mediaMetadataAlbumArtist=" + current_media_item.media_metadata.album_artist;
  summary += ",mediaMetadataAlbumArtistTokenPresent=" +
      std::to_string(current_media_item.media_metadata.album_artist_token.empty() ? 0 : 1);
  summary += ",mediaMetadataDisplayTitle=" + current_media_item.media_metadata.display_title;
  summary += ",mediaMetadataDisplayTitleTokenPresent=" +
      std::to_string(current_media_item.media_metadata.display_title_token.empty() ? 0 : 1);
  summary += ",mediaMetadataSubtitle=" + current_media_item.media_metadata.subtitle;
  summary += ",mediaMetadataSubtitleTokenPresent=" +
      std::to_string(current_media_item.media_metadata.subtitle_token.empty() ? 0 : 1);
  summary += ",mediaMetadataDescription=" + current_media_item.media_metadata.description;
  summary += ",mediaMetadataDescriptionTokenPresent=" +
      std::to_string(current_media_item.media_metadata.description_token.empty() ? 0 : 1);
  summary += ",mediaMetadataAuthor=" + current_media_item.media_metadata.author;
  summary += ",mediaMetadataAuthorTokenPresent=" +
      std::to_string(current_media_item.media_metadata.author_token.empty() ? 0 : 1);
  summary += ",mediaMetadataComposer=" + current_media_item.media_metadata.composer;
  summary += ",mediaMetadataComposerTokenPresent=" +
      std::to_string(current_media_item.media_metadata.composer_token.empty() ? 0 : 1);
  summary += ",mediaMetadataConductor=" + current_media_item.media_metadata.conductor;
  summary += ",mediaMetadataConductorTokenPresent=" +
      std::to_string(current_media_item.media_metadata.conductor_token.empty() ? 0 : 1);
  summary += ",mediaMetadataDurationMs=" +
      std::to_string(current_media_item.media_metadata.duration_ms);
  summary += ",mediaMetadataTrackNumber=" +
      std::to_string(current_media_item.media_metadata.track_number);
  summary += ",mediaMetadataTotalTrackCount=" +
      std::to_string(current_media_item.media_metadata.total_track_count);
  summary += ",mediaMetadataIsBrowsable=" +
      std::to_string(current_media_item.media_metadata.is_browsable);
  summary += ",mediaMetadataIsPlayable=" +
      std::to_string(current_media_item.media_metadata.is_playable);
  summary += ",mediaMetadataFolderType=" +
      std::to_string(current_media_item.media_metadata.folder_type);
  summary += ",mediaMetadataRecordingYear=" +
      std::to_string(current_media_item.media_metadata.recording_year);
  summary += ",mediaMetadataRecordingMonth=" +
      std::to_string(current_media_item.media_metadata.recording_month);
  summary += ",mediaMetadataRecordingDay=" +
      std::to_string(current_media_item.media_metadata.recording_day);
  summary += ",mediaMetadataReleaseYear=" +
      std::to_string(current_media_item.media_metadata.release_year);
  summary += ",mediaMetadataReleaseMonth=" +
      std::to_string(current_media_item.media_metadata.release_month);
  summary += ",mediaMetadataReleaseDay=" +
      std::to_string(current_media_item.media_metadata.release_day);
  summary += ",mediaMetadataWriter=" + current_media_item.media_metadata.writer;
  summary += ",mediaMetadataWriterTokenPresent=" +
      std::to_string(current_media_item.media_metadata.writer_token.empty() ? 0 : 1);
  summary += ",mediaMetadataDiscNumber=" +
      std::to_string(current_media_item.media_metadata.disc_number);
  summary += ",mediaMetadataTotalDiscCount=" +
      std::to_string(current_media_item.media_metadata.total_disc_count);
  summary += ",mediaMetadataGenre=" + current_media_item.media_metadata.genre;
  summary += ",mediaMetadataGenreTokenPresent=" +
      std::to_string(current_media_item.media_metadata.genre_token.empty() ? 0 : 1);
  AppendObjectValueSummary(
      &summary, "mediaMetadataGenreValue", current_media_item.media_metadata.genre_value);
  summary += ",mediaMetadataCompilation=" + current_media_item.media_metadata.compilation;
  summary += ",mediaMetadataCompilationTokenPresent=" +
      std::to_string(current_media_item.media_metadata.compilation_token.empty() ? 0 : 1);
  summary += ",mediaMetadataMediaType=" +
      std::to_string(current_media_item.media_metadata.media_type);
  summary += ",mediaMetadataStation=" + current_media_item.media_metadata.station;
  summary += ",mediaMetadataStationTokenPresent=" +
      std::to_string(current_media_item.media_metadata.station_token.empty() ? 0 : 1);
  AppendObjectValueSummary(
      &summary, "mediaMetadataStationValue", current_media_item.media_metadata.station_value);
  summary += ",mediaMetadataExtrasPresent=" +
      std::to_string(current_media_item.media_metadata.extras_present ? 1 : 0);
  summary += ",mediaMetadataExtrasKeyCount=" +
      std::to_string(current_media_item.media_metadata.extras_key_count);
  summary += ",mediaMetadataExtrasTokenPresent=" +
      std::to_string(current_media_item.media_metadata.extras_token.empty() ? 0 : 1);
  AppendBundleValueSummary(
      &summary, "mediaMetadataExtras", current_media_item.media_metadata.extras_values);
  summary += ",artworkUri=" + current_media_item.media_metadata.artwork_uri;
  summary += ",artworkDataLength=" +
      std::to_string(current_media_item.media_metadata.artwork_data.size());
  summary += ",artworkDataType=" +
      std::to_string(current_media_item.media_metadata.artwork_data_type);
  return NewStringUtfChecked(env, summary, "nativeCurrentMediaItemQuerySmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePlaylistMetadataSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaMetadataSnapshot playlist_metadata;
  playlist_metadata.title = "Playlist Title";
  playlist_metadata.artist = "Playlist Artist";
  playlist_metadata.album_title = "Playlist Album";
  playlist_metadata.album_artist = "Playlist Album Artist";
  playlist_metadata.display_title = "Playlist Display";
  playlist_metadata.subtitle = "Playlist Subtitle";
  playlist_metadata.description = "Playlist Description";
  playlist_metadata.writer = "Playlist Writer";
  playlist_metadata.author = "Playlist Author";
  playlist_metadata.composer = "Playlist Composer";
  playlist_metadata.conductor = "Playlist Conductor";
  playlist_metadata.disc_number = 2;
  playlist_metadata.total_disc_count = 5;
  playlist_metadata.genre = "Playlist Genre";
  playlist_metadata.compilation = "Playlist Compilation";
  playlist_metadata.artwork_uri = "https://example.com/playlist-artwork.jpg";
  playlist_metadata.artwork_data = {3, 1, 4};
  playlist_metadata.artwork_data_type = 4;
  playlist_metadata.is_browsable = 1;
  playlist_metadata.is_playable = 0;
  playlist_metadata.folder_type = 2;
  playlist_metadata.media_type = 1;
  playlist_metadata.release_month = 4;
  playlist_metadata.release_day = 22;
  playlist_metadata.station = "Playlist Station";
  playlist_metadata.extras_present = true;
  playlist_metadata.extras_key_count = 5;
  playlist_metadata.extras_values = {
      {"enabled", BundleValueInfo::kBoolean, "", 0, 0.0, true, {}},
      {"episode", BundleValueInfo::kLong, "", 12, 0.0, false, {}},
      {"gain", BundleValueInfo::kDouble, "", 0, 0.75, false, {}},
      {"payload", BundleValueInfo::kByteArray, "", 0, 0.0, false, {4, 5, 6}},
      {"source", BundleValueInfo::kString, "playlist-decoded", 0, 0.0, false, {}},
  };
  player->SetPlaylistMetadata(playlist_metadata);
  MediaMetadataSnapshot actual = player->GetPlaylistMetadata();
  std::string summary = "title=" + actual.title;
  summary += ",artist=" + actual.artist;
  summary += ",albumTitle=" + actual.album_title;
  summary += ",albumArtist=" + actual.album_artist;
  summary += ",albumTitleTokenPresent=" +
      std::to_string(actual.album_title_token.empty() ? 0 : 1);
  summary += ",albumArtistTokenPresent=" +
      std::to_string(actual.album_artist_token.empty() ? 0 : 1);
  summary += ",displayTitle=" + actual.display_title;
  summary += ",titleTokenPresent=" + std::to_string(actual.title_token.empty() ? 0 : 1);
  AppendObjectValueSummary(&summary, "titleValue", actual.title_value);
  summary += ",artistTokenPresent=" + std::to_string(actual.artist_token.empty() ? 0 : 1);
  summary +=
      ",displayTitleTokenPresent=" + std::to_string(actual.display_title_token.empty() ? 0 : 1);
  summary += ",subtitle=" + actual.subtitle;
  summary += ",subtitleTokenPresent=" + std::to_string(actual.subtitle_token.empty() ? 0 : 1);
  summary += ",description=" + actual.description;
  summary +=
      ",descriptionTokenPresent=" + std::to_string(actual.description_token.empty() ? 0 : 1);
  summary += ",writer=" + actual.writer;
  summary += ",writerTokenPresent=" + std::to_string(actual.writer_token.empty() ? 0 : 1);
  summary += ",author=" + actual.author;
  summary += ",authorTokenPresent=" + std::to_string(actual.author_token.empty() ? 0 : 1);
  summary += ",composer=" + actual.composer;
  summary += ",composerTokenPresent=" + std::to_string(actual.composer_token.empty() ? 0 : 1);
  summary += ",conductor=" + actual.conductor;
  summary +=
      ",conductorTokenPresent=" + std::to_string(actual.conductor_token.empty() ? 0 : 1);
  summary += ",discNumber=" + std::to_string(actual.disc_number);
  summary += ",totalDiscCount=" + std::to_string(actual.total_disc_count);
  summary += ",genre=" + actual.genre;
  summary += ",genreTokenPresent=" + std::to_string(actual.genre_token.empty() ? 0 : 1);
  AppendObjectValueSummary(&summary, "genreValue", actual.genre_value);
  summary += ",compilation=" + actual.compilation;
  summary += ",compilationTokenPresent=" +
      std::to_string(actual.compilation_token.empty() ? 0 : 1);
  summary += ",artworkUri=" + actual.artwork_uri;
  summary += ",artworkDataLength=" + std::to_string(actual.artwork_data.size());
  summary += ",artworkDataType=" + std::to_string(actual.artwork_data_type);
  summary += ",isBrowsable=" + std::to_string(actual.is_browsable);
  summary += ",isPlayable=" + std::to_string(actual.is_playable);
  summary += ",folderType=" + std::to_string(actual.folder_type);
  summary += ",mediaType=" + std::to_string(actual.media_type);
  summary += ",releaseMonth=" + std::to_string(actual.release_month);
  summary += ",releaseDay=" + std::to_string(actual.release_day);
  summary += ",station=" + actual.station;
  summary += ",stationTokenPresent=" + std::to_string(actual.station_token.empty() ? 0 : 1);
  AppendObjectValueSummary(&summary, "stationValue", actual.station_value);
  summary += ",extrasPresent=" + std::to_string(actual.extras_present ? 1 : 0);
  summary += ",extrasKeyCount=" + std::to_string(actual.extras_key_count);
  summary += ",extrasTokenPresent=" + std::to_string(actual.extras_token.empty() ? 0 : 1);
  AppendBundleValueSummary(&summary, "extras", actual.extras_values);
  return NewStringUtfChecked(env, summary, "nativePlaylistMetadataSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePlaylistMetadataOpaqueTokenSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context,
    jstring title_token,
    jstring artist_token,
    jstring album_title_token,
    jstring album_artist_token,
    jstring display_title_token,
    jstring subtitle_token,
    jstring description_token,
    jstring writer_token,
    jstring author_token,
    jstring composer_token,
    jstring conductor_token,
    jstring genre_token,
    jstring compilation_token,
    jstring station_token,
    jstring extras_token) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaMetadataSnapshot metadata;
  metadata.title = "fallback-playlist-title";
  metadata.title_token = JStringToString(env, title_token);
  metadata.artist = "fallback-playlist-artist";
  metadata.artist_token = JStringToString(env, artist_token);
  metadata.album_title = "fallback-playlist-album-title";
  metadata.album_title_token = JStringToString(env, album_title_token);
  metadata.album_artist = "fallback-playlist-album-artist";
  metadata.album_artist_token = JStringToString(env, album_artist_token);
  metadata.display_title = "fallback-playlist-display";
  metadata.display_title_token = JStringToString(env, display_title_token);
  metadata.subtitle = "fallback-playlist-subtitle";
  metadata.subtitle_token = JStringToString(env, subtitle_token);
  metadata.description = "fallback-playlist-description";
  metadata.description_token = JStringToString(env, description_token);
  metadata.writer = "fallback-playlist-writer";
  metadata.writer_token = JStringToString(env, writer_token);
  metadata.author = "fallback-playlist-author";
  metadata.author_token = JStringToString(env, author_token);
  metadata.composer = "fallback-playlist-composer";
  metadata.composer_token = JStringToString(env, composer_token);
  metadata.conductor = "fallback-playlist-conductor";
  metadata.conductor_token = JStringToString(env, conductor_token);
  metadata.genre = "fallback-playlist-genre";
  metadata.genre_token = JStringToString(env, genre_token);
  metadata.compilation = "fallback-playlist-compilation";
  metadata.compilation_token = JStringToString(env, compilation_token);
  metadata.station = "fallback-playlist-station";
  metadata.station_token = JStringToString(env, station_token);
  metadata.extras_present = true;
  metadata.extras_key_count = 1;
  metadata.extras_token = JStringToString(env, extras_token);
  player->SetPlaylistMetadata(metadata);
  MediaMetadataSnapshot actual = player->GetPlaylistMetadata();
  std::string summary = "title=" + actual.title;
  summary += ",artist=" + actual.artist;
  summary += ",albumTitle=" + actual.album_title;
  summary += ",albumArtist=" + actual.album_artist;
  summary += ",displayTitle=" + actual.display_title;
  summary += ",subtitle=" + actual.subtitle;
  summary += ",description=" + actual.description;
  summary += ",writer=" + actual.writer;
  summary += ",author=" + actual.author;
  summary += ",composer=" + actual.composer;
  summary += ",conductor=" + actual.conductor;
  summary += ",genre=" + actual.genre;
  summary += ",compilation=" + actual.compilation;
  summary += ",station=" + actual.station;
  summary += ",titleTokenPresent=" + std::to_string(actual.title_token.empty() ? 0 : 1);
  summary += ",artistTokenPresent=" + std::to_string(actual.artist_token.empty() ? 0 : 1);
  summary += ",albumTitleTokenPresent=" +
      std::to_string(actual.album_title_token.empty() ? 0 : 1);
  summary += ",albumArtistTokenPresent=" +
      std::to_string(actual.album_artist_token.empty() ? 0 : 1);
  summary +=
      ",displayTitleTokenPresent=" + std::to_string(actual.display_title_token.empty() ? 0 : 1);
  summary += ",subtitleTokenPresent=" + std::to_string(actual.subtitle_token.empty() ? 0 : 1);
  summary +=
      ",descriptionTokenPresent=" + std::to_string(actual.description_token.empty() ? 0 : 1);
  summary += ",writerTokenPresent=" + std::to_string(actual.writer_token.empty() ? 0 : 1);
  summary += ",authorTokenPresent=" + std::to_string(actual.author_token.empty() ? 0 : 1);
  summary += ",composerTokenPresent=" + std::to_string(actual.composer_token.empty() ? 0 : 1);
  summary +=
      ",conductorTokenPresent=" + std::to_string(actual.conductor_token.empty() ? 0 : 1);
  summary += ",genreTokenPresent=" + std::to_string(actual.genre_token.empty() ? 0 : 1);
  summary +=
      ",compilationTokenPresent=" + std::to_string(actual.compilation_token.empty() ? 0 : 1);
  summary += ",stationTokenPresent=" + std::to_string(actual.station_token.empty() ? 0 : 1);
  summary += ",extrasPresent=" + std::to_string(actual.extras_present ? 1 : 0);
  summary += ",extrasKeyCount=" + std::to_string(actual.extras_key_count);
  summary += ",extrasTokenPresent=" + std::to_string(actual.extras_token.empty() ? 0 : 1);
  return NewStringUtfChecked(env, summary, "nativePlaylistMetadataOpaqueTokenSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeMediaItemAtSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor first_item;
  first_item.uri = "https://example.com/at-one.mp4";
  first_item.media_id = "at-1";
  MediaItemDescriptor second_item;
  second_item.uri = "https://example.com/at-two.m3u8";
  second_item.media_id = "at-2";
  second_item.source_type = MediaSourceType::kHls;
  second_item.tag_present = true;
  second_item.tag_string = "at-two-tag";
  second_item.ads_configuration.ad_tag_uri = "https://ads.example.com/at-two.xml";
  second_item.ads_configuration.ads_id = "ads-at-2";
  second_item.media_metadata.title = "At Two Title";
  second_item.media_metadata.artist = "At Two Artist";
  second_item.media_metadata.album_title = "At Two Album";
  second_item.media_metadata.album_artist = "At Two Album Artist";
  second_item.media_metadata.display_title = "At Two Display";
  second_item.media_metadata.subtitle = "At Two Subtitle";
  second_item.media_metadata.description = "At Two Description";
  second_item.media_metadata.author = "At Two Author";
  second_item.media_metadata.composer = "At Two Composer";
  second_item.media_metadata.conductor = "At Two Conductor";
  second_item.media_metadata.writer = "At Two Writer";
  second_item.media_metadata.genre = "At Two Genre";
  second_item.media_metadata.compilation = "At Two Compilation";
  second_item.media_metadata.media_type = 4;
  second_item.media_metadata.station = "At Two Station";
  second_item.media_metadata.extras_present = true;
  second_item.media_metadata.extras_key_count = 1;
  second_item.media_metadata.extras_token =
      "generated-opaque-object-token-at-two-metadata-extras";
  second_item.media_metadata.artwork_uri = "https://example.com/at-two-artwork.jpg";
  second_item.media_metadata.artwork_data = {2, 4, 6, 8};
  second_item.media_metadata.artwork_data_type = 5;
  second_item.clipping_configuration.start_position_ms = 2222;
  second_item.clipping_configuration.end_position_ms = 7777;
  second_item.clipping_configuration.allow_unseekable_media = true;
  second_item.live_configuration.target_offset_ms = 4444;
  second_item.live_configuration.min_offset_ms = 3333;
  second_item.live_configuration.max_offset_ms = 6666;
  second_item.live_configuration.min_playback_speed = 0.95f;
  second_item.live_configuration.max_playback_speed = 1.05f;
  second_item.drm_configuration.scheme_uuid = "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed";
  second_item.drm_configuration.license_uri = "https://license.example.com/at-two";
  second_item.drm_configuration.license_request_header_names = {"Authorization", "X-Env"};
  second_item.drm_configuration.license_request_header_values = {"Bearer at-two", "staging"};
  second_item.drm_configuration.force_default_license_uri = true;
  second_item.drm_configuration.play_clear_content_without_key = false;
  second_item.request_metadata.media_uri = "https://example.com/at-two-request";
  second_item.request_metadata.search_query = "at two search";
  second_item.request_metadata.extras_present = true;
  second_item.request_metadata.extras_key_count = 1;
  second_item.request_metadata.extras_token =
      "generated-opaque-object-token-at-two-request-extras";
  RegisterBundleForOpaqueToken(
      env,
      second_item.media_metadata.extras_token,
      "at-two-metadata-key",
      "at-two-metadata-value");
  RegisterBundleForOpaqueToken(
      env,
      second_item.request_metadata.extras_token,
      "at-two-request-key",
      "at-two-request-value");
  second_item.subtitle_configurations.push_back(
      {"https://example.com/at-two.vtt", "text/vtt", "en", "English", "sub-at-2", 7, 9});
  second_item.subtitle_configurations.push_back(
      {"https://example.com/at-two-es.vtt", "text/vtt", "es", "Spanish", "sub-at-2b", 1, 5});
  player->SetMediaItems({first_item, second_item}, 0, 0);

  MediaItemDescriptor existing = player->GetMediaItemAt(1);
  MediaItemDescriptor missing = player->GetMediaItemAt(8);
  std::string summary = "exists=" + std::to_string(existing.uri.empty() ? 0 : 1);
  summary += ",mediaId=" + existing.media_id;
  summary += ",uri=" + existing.uri;
  summary += ",mimeType=" + existing.mime_type;
  summary += ",sourceType=" + std::to_string(static_cast<int>(existing.source_type));
  summary += ",tagPresent=" + std::to_string(existing.tag_present ? 1 : 0);
  summary += ",tagString=" + existing.tag_string;
  summary += ",tagTokenPresent=" + std::to_string(existing.tag_token.empty() ? 0 : 1);
  AppendObjectValueSummary(&summary, "tagValue", existing.tag_value);
  summary += ",subtitleCount=" + std::to_string(existing.subtitle_configurations.size());
  if (!existing.subtitle_configurations.empty()) {
    const auto& subtitle = existing.subtitle_configurations[0];
    summary += ",subtitle0Uri=" + subtitle.uri;
    summary += ",subtitle0Id=" + subtitle.id;
    summary += ",subtitle0SelectionFlags=" + std::to_string(subtitle.selection_flags);
    summary += ",subtitle0RoleFlags=" + std::to_string(subtitle.role_flags);
  }
  if (existing.subtitle_configurations.size() > 1) {
    const auto& subtitle = existing.subtitle_configurations[1];
    summary += ",subtitle1Uri=" + subtitle.uri;
    summary += ",subtitle1MimeType=" + subtitle.mime_type;
    summary += ",subtitle1Language=" + subtitle.language;
    summary += ",subtitle1Label=" + subtitle.label;
    summary += ",subtitle1Id=" + subtitle.id;
    summary += ",subtitle1SelectionFlags=" + std::to_string(subtitle.selection_flags);
    summary += ",subtitle1RoleFlags=" + std::to_string(subtitle.role_flags);
  }
  summary += ",clippingStartMs=" + std::to_string(existing.clipping_configuration.start_position_ms);
  summary += ",clippingEndMs=" + std::to_string(existing.clipping_configuration.end_position_ms);
  summary += ",clippingAllowUnseekable=" +
      std::to_string(existing.clipping_configuration.allow_unseekable_media ? 1 : 0);
  summary += ",liveTargetOffsetMs=" + std::to_string(existing.live_configuration.target_offset_ms);
  summary += ",liveMinOffsetMs=" + std::to_string(existing.live_configuration.min_offset_ms);
  summary += ",liveMaxOffsetMs=" + std::to_string(existing.live_configuration.max_offset_ms);
  summary += ",liveMinSpeed=" + std::to_string(existing.live_configuration.min_playback_speed);
  summary += ",liveMaxSpeed=" + std::to_string(existing.live_configuration.max_playback_speed);
  summary += ",drmScheme=" + existing.drm_configuration.scheme_uuid;
  summary += ",drmLicenseUri=" + existing.drm_configuration.license_uri;
  summary += ",drmHeaderCount=" +
      std::to_string(existing.drm_configuration.license_request_header_names.size());
  if (!existing.drm_configuration.license_request_header_names.empty() &&
      !existing.drm_configuration.license_request_header_values.empty()) {
    summary += ",drmHeader0=" + existing.drm_configuration.license_request_header_names[0] + ":" +
        existing.drm_configuration.license_request_header_values[0];
  }
  if (existing.drm_configuration.license_request_header_names.size() > 1 &&
      existing.drm_configuration.license_request_header_values.size() > 1) {
    summary += ",drmHeader1=" + existing.drm_configuration.license_request_header_names[1] + ":" +
        existing.drm_configuration.license_request_header_values[1];
  }
  summary += ",drmForceDefaultLicenseUri=" +
      std::to_string(existing.drm_configuration.force_default_license_uri ? 1 : 0);
  summary += ",drmPlayClearContentWithoutKey=" +
      std::to_string(existing.drm_configuration.play_clear_content_without_key ? 1 : 0);
  summary += ",requestMetadataMediaUri=" + existing.request_metadata.media_uri;
  summary += ",requestMetadataSearchQuery=" + existing.request_metadata.search_query;
  summary += ",requestMetadataExtrasPresent=" +
      std::to_string(existing.request_metadata.extras_present ? 1 : 0);
  summary += ",requestMetadataExtrasKeyCount=" +
      std::to_string(existing.request_metadata.extras_key_count);
  summary += ",requestMetadataExtrasTokenPresent=" +
      std::to_string(existing.request_metadata.extras_token.empty() ? 0 : 1);
  summary += ",adTagUri=" + existing.ads_configuration.ad_tag_uri;
  summary += ",adsId=" + existing.ads_configuration.ads_id;
  summary += ",adsIdTokenPresent=" +
      std::to_string(existing.ads_configuration.ads_id_token.empty() ? 0 : 1);
  AppendObjectValueSummary(&summary, "adsIdValue", existing.ads_configuration.ads_id_value);
  summary += ",mediaMetadataTitle=" + existing.media_metadata.title;
  summary += ",mediaMetadataTitleTokenPresent=" +
      std::to_string(existing.media_metadata.title_token.empty() ? 0 : 1);
  summary += ",mediaMetadataArtist=" + existing.media_metadata.artist;
  summary += ",mediaMetadataArtistTokenPresent=" +
      std::to_string(existing.media_metadata.artist_token.empty() ? 0 : 1);
  summary += ",mediaMetadataAlbumTitle=" + existing.media_metadata.album_title;
  summary += ",mediaMetadataAlbumTitleTokenPresent=" +
      std::to_string(existing.media_metadata.album_title_token.empty() ? 0 : 1);
  summary += ",mediaMetadataAlbumArtist=" + existing.media_metadata.album_artist;
  summary += ",mediaMetadataAlbumArtistTokenPresent=" +
      std::to_string(existing.media_metadata.album_artist_token.empty() ? 0 : 1);
  summary += ",mediaMetadataDisplayTitle=" + existing.media_metadata.display_title;
  summary += ",mediaMetadataDisplayTitleTokenPresent=" +
      std::to_string(existing.media_metadata.display_title_token.empty() ? 0 : 1);
  summary += ",mediaMetadataSubtitle=" + existing.media_metadata.subtitle;
  summary += ",mediaMetadataSubtitleTokenPresent=" +
      std::to_string(existing.media_metadata.subtitle_token.empty() ? 0 : 1);
  summary += ",mediaMetadataDescription=" + existing.media_metadata.description;
  summary += ",mediaMetadataDescriptionTokenPresent=" +
      std::to_string(existing.media_metadata.description_token.empty() ? 0 : 1);
  summary += ",mediaMetadataAuthor=" + existing.media_metadata.author;
  summary += ",mediaMetadataAuthorTokenPresent=" +
      std::to_string(existing.media_metadata.author_token.empty() ? 0 : 1);
  summary += ",mediaMetadataComposer=" + existing.media_metadata.composer;
  summary += ",mediaMetadataComposerTokenPresent=" +
      std::to_string(existing.media_metadata.composer_token.empty() ? 0 : 1);
  summary += ",mediaMetadataConductor=" + existing.media_metadata.conductor;
  summary += ",mediaMetadataConductorTokenPresent=" +
      std::to_string(existing.media_metadata.conductor_token.empty() ? 0 : 1);
  summary += ",mediaMetadataWriter=" + existing.media_metadata.writer;
  summary += ",mediaMetadataWriterTokenPresent=" +
      std::to_string(existing.media_metadata.writer_token.empty() ? 0 : 1);
  summary += ",mediaMetadataGenre=" + existing.media_metadata.genre;
  summary += ",mediaMetadataGenreTokenPresent=" +
      std::to_string(existing.media_metadata.genre_token.empty() ? 0 : 1);
  summary += ",mediaMetadataCompilation=" + existing.media_metadata.compilation;
  summary += ",mediaMetadataCompilationTokenPresent=" +
      std::to_string(existing.media_metadata.compilation_token.empty() ? 0 : 1);
  summary += ",mediaMetadataMediaType=" + std::to_string(existing.media_metadata.media_type);
  summary += ",mediaMetadataStation=" + existing.media_metadata.station;
  summary += ",mediaMetadataStationTokenPresent=" +
      std::to_string(existing.media_metadata.station_token.empty() ? 0 : 1);
  summary += ",mediaMetadataExtrasPresent=" +
      std::to_string(existing.media_metadata.extras_present ? 1 : 0);
  summary += ",mediaMetadataExtrasKeyCount=" +
      std::to_string(existing.media_metadata.extras_key_count);
  summary += ",mediaMetadataExtrasTokenPresent=" +
      std::to_string(existing.media_metadata.extras_token.empty() ? 0 : 1);
  summary += ",artworkUri=" + existing.media_metadata.artwork_uri;
  summary += ",artworkDataLength=" +
      std::to_string(existing.media_metadata.artwork_data.size());
  summary += ",artworkDataType=" +
      std::to_string(existing.media_metadata.artwork_data_type);
  summary += ",missingExists=" + std::to_string(missing.uri.empty() ? 0 : 1);
  return NewStringUtfChecked(env, summary, "nativeMediaItemAtSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeMediaItemOpaqueTokenSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context,
    jstring tag_token,
    jstring ads_id_token,
    jstring extras_token) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/token-item.m3u8";
  media_item.media_id = "token-item";
  media_item.source_type = MediaSourceType::kHls;
  media_item.tag_present = true;
  media_item.tag_string = "fallback-tag-string";
  media_item.tag_token = JStringToString(env, tag_token);
  media_item.request_metadata.media_uri = "https://example.com/token-request";
  media_item.request_metadata.search_query = "token search";
  media_item.request_metadata.extras_present = true;
  media_item.request_metadata.extras_key_count = 0;
  media_item.request_metadata.extras_token = JStringToString(env, extras_token);
  media_item.ads_configuration.ad_tag_uri = "https://ads.example.com/token.xml";
  media_item.ads_configuration.ads_id = "fallback-ads-id";
  media_item.ads_configuration.ads_id_token = JStringToString(env, ads_id_token);
  player->SetMediaItem(media_item);

  MediaItemDescriptor current_media_item = player->GetCurrentMediaItem();
  std::string summary = "tagString=" + current_media_item.tag_string;
  summary += ",tagTokenPresent=" + std::to_string(current_media_item.tag_token.empty() ? 0 : 1);
  AppendObjectValueSummary(&summary, "tagValue", current_media_item.tag_value);
  summary += ",adsId=" + current_media_item.ads_configuration.ads_id;
  summary += ",adsIdTokenPresent=" +
      std::to_string(current_media_item.ads_configuration.ads_id_token.empty() ? 0 : 1);
  AppendObjectValueSummary(
      &summary, "adsIdValue", current_media_item.ads_configuration.ads_id_value);
  summary += ",requestMetadataExtrasPresent=" +
      std::to_string(current_media_item.request_metadata.extras_present ? 1 : 0);
  summary += ",requestMetadataExtrasKeyCount=" +
      std::to_string(current_media_item.request_metadata.extras_key_count);
  summary += ",requestMetadataExtrasTokenPresent=" +
      std::to_string(current_media_item.request_metadata.extras_token.empty() ? 0 : 1);
  return NewStringUtfChecked(env, summary, "nativeMediaItemOpaqueTokenSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeMediaSetOverloadsSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor first_item;
  first_item.uri = "https://example.com/overload-one.mp4";
  first_item.media_id = "overload-1";
  MediaItemDescriptor second_item;
  second_item.uri = "https://example.com/overload-two.mp4";
  second_item.media_id = "overload-2";
  MediaItemDescriptor third_item;
  third_item.uri = "https://example.com/overload-three.mp4";
  third_item.media_id = "overload-3";

  player->SetMediaItem(first_item, static_cast<int64_t>(2345));
  player->SetMediaItem(second_item, false);
  player->SetMediaItems({first_item, second_item, third_item}, false);
  player->SeekToMediaItem(1, 3456);

  PlaybackSnapshot snapshot = player->GetSnapshot();
  std::string summary = "count=" + std::to_string(snapshot.media_item_count);
  summary += ",index=" + std::to_string(snapshot.current_media_item_index);
  summary += ",positionMs=" + std::to_string(snapshot.current_position_ms);
  summary += "," + player->GetCurrentMediaItemDebugSummary();
  return NewStringUtfChecked(env, summary, "nativeMediaSetOverloadsSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeSurfaceBridgeSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context,
    jobject surface,
    jobject surface_view,
    jobject texture_view) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  player->SetVideoSurface(surface);
  player->ClearVideoSurface(surface);
  player->SetVideoSurface(surface);
  player->ClearVideoSurface();
  player->SetVideoSurfaceView(surface_view);
  player->ClearVideoSurfaceView(surface_view);
  if (texture_view != nullptr) {
    player->SetVideoTextureView(texture_view);
    player->ClearVideoTextureView(texture_view);
  }
  jclass surface_view_class = GetObjectClassChecked(env, surface_view, "SurfaceView");
  if (surface_view_class == nullptr) {
    return NewStringUtfChecked(
        env, "surface-error:getObjectClass", "nativeSurfaceBridgeSmokeTest.error");
  }
  jmethodID get_holder =
      GetMethodChecked(env, surface_view_class, "SurfaceView", "getHolder",
                       "()Landroid/view/SurfaceHolder;");
  jobject holder = nullptr;
  if (get_holder != nullptr) {
    holder = env->CallObjectMethod(surface_view, get_holder);
    if (ClearJniExceptionIfPresent(env, "CallObjectMethod(SurfaceView.getHolder)")) {
      holder = nullptr;
    }
  }
  env->DeleteLocalRef(surface_view_class);
  player->SetVideoSurfaceHolder(holder);
  player->ClearVideoSurfaceHolder(holder);
  if (holder != nullptr) {
    env->DeleteLocalRef(holder);
  }
  return NewStringUtfChecked(env, "surface-ok", "nativeSurfaceBridgeSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePlayerViewBridgeSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context,
    jobject player_view) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  player->BindPlayerView(player_view);

  jclass player_view_class = GetObjectClassChecked(env, player_view, "PlayerView");
  if (player_view_class == nullptr) {
    return NewStringUtfChecked(
        env, "playerView-error:getObjectClass", "nativePlayerViewBridgeSmokeTest.error");
  }
  jmethodID get_player =
      GetMethodChecked(env, player_view_class, "PlayerView", "getPlayer",
                       "()Landroidx/media3/common/Player;");
  jobject bound_player = nullptr;
  if (get_player != nullptr) {
    bound_player = env->CallObjectMethod(player_view, get_player);
    if (ClearJniExceptionIfPresent(env, "CallObjectMethod(PlayerView.getPlayer)")) {
      bound_player = nullptr;
    }
  }

  player->UnbindPlayerView(player_view);

  jobject unbound_player = nullptr;
  if (get_player != nullptr) {
    unbound_player = env->CallObjectMethod(player_view, get_player);
    if (ClearJniExceptionIfPresent(env, "CallObjectMethod(PlayerView.getPlayer after unbind)")) {
      unbound_player = nullptr;
    }
  }

  bool bound = bound_player != nullptr;
  bool unbound = unbound_player == nullptr;
  DeleteLocalRefIfNotNull(env, bound_player);
  DeleteLocalRefIfNotNull(env, unbound_player);
  env->DeleteLocalRef(player_view_class);
  std::string summary = "bound=" + std::to_string(bound ? 1 : 0);
  summary += ",unbound=" + std::to_string(unbound ? 1 : 0);
  return NewStringUtfChecked(env, summary, "nativePlayerViewBridgeSmokeTest");
}

namespace {

void PopulateListenerSmokeScenario(
    JNIEnv* env,
    ExoPlayerSdkPlayer* player,
    CapturingPlayerListener* listener) {
  if (player == nullptr || listener == nullptr) {
    return;
  }
  auto run_step = [&](const char* step, auto&& fn) {
    LogInfo(std::string("listenerSmoke populate step=") + step + " begin");
    LogInfo(
        std::string("listenerSmoke state step=") + step + ",phase=pre," +
        BuildListenerStepStateSummary(*listener));
    fn();
    LogInfo(std::string("listenerSmoke populate step=") + step + " end");
    LogInfo(
        std::string("listenerSmoke state step=") + step + ",phase=post," +
        BuildListenerStepStateSummary(*listener));
  };
  LogInfo("listenerSmoke populate begin");
  run_step("SetListener", [&]() { player->SetListener(listener); });
  run_step("SimulateIsLoadingChangedForTest", [&]() {
    player->SimulateIsLoadingChangedForTest(true);
  });
  MediaItemDescriptor first_item;
  first_item.uri = "https://example.com/listener.mp4";
  first_item.media_id = "listener-item-1";
  first_item.tag_present = true;
  first_item.tag_string = "listener-tag-1";
  first_item.tag_token = "generated-opaque-object-token-listener-tag-1";
  first_item.media_metadata.title = "Listener Item Title";
  first_item.media_metadata.artist = "Listener Item Artist";
  first_item.media_metadata.album_title = "Listener Item Album";
  first_item.media_metadata.album_artist = "Listener Item Album Artist";
  first_item.media_metadata.display_title = "Listener Item Display";
  first_item.media_metadata.subtitle = "Listener Item Subtitle";
  first_item.media_metadata.description = "Listener Item Description";
  first_item.media_metadata.writer = "Listener Item Writer";
  first_item.media_metadata.author = "Listener Item Author";
  first_item.media_metadata.composer = "Listener Item Composer";
  first_item.media_metadata.conductor = "Listener Item Conductor";
  first_item.media_metadata.compilation = "Listener Item Compilation";
  first_item.media_metadata.genre = "Listener Item Genre";
  first_item.media_metadata.station = "Listener Item Station";
  first_item.media_metadata.media_type = 5;
  first_item.media_metadata.artwork_uri = "https://example.com/listener-item-artwork.jpg";
  first_item.media_metadata.artwork_data = {5, 4, 3, 2};
  first_item.media_metadata.artwork_data_type = 6;
  first_item.media_metadata.extras_present = true;
  first_item.media_metadata.extras_key_count = 1;
  first_item.media_metadata.extras_token =
      "generated-opaque-object-token-listener-item-metadata-extras";
  RegisterBundleForOpaqueToken(
      env,
      first_item.media_metadata.extras_token,
      "listener-item-metadata-key",
      "listener-item-metadata-value");
  MediaItemDescriptor second_item;
  second_item.uri = "https://example.com/listener-two.mp4";
  second_item.media_id = "listener-item-2";
  second_item.tag_present = true;
  second_item.tag_string = "listener-tag-2";
  second_item.tag_token = "generated-opaque-object-token-listener-tag-2";
  second_item.live_configuration.target_offset_ms = 6100;
  second_item.live_configuration.min_offset_ms = 5200;
  second_item.live_configuration.max_offset_ms = 7800;
  second_item.live_configuration.min_playback_speed = 0.94f;
  second_item.live_configuration.max_playback_speed = 1.08f;
  run_step("SetMediaItems", [&]() { player->SetMediaItems({first_item, second_item}, 0, 0); });
  run_step("SetRepeatMode", [&]() {
    player->SetRepeatMode(androidx::media3::cppbridge::RepeatMode::kAll);
  });
  run_step("SetShuffleModeEnabled", [&]() { player->SetShuffleModeEnabled(true); });
  run_step("SimulateSeekBackIncrementChangedForTest", [&]() {
    player->SimulateSeekBackIncrementChangedForTest(15000);
  });
  run_step("SimulateSeekForwardIncrementChangedForTest", [&]() {
    player->SimulateSeekForwardIncrementChangedForTest(25000);
  });
  run_step("SimulateMaxSeekToPreviousPositionChangedForTest", [&]() {
    player->SimulateMaxSeekToPreviousPositionChangedForTest(12000);
  });
  PlaybackParametersSnapshot playback_parameters;
  playback_parameters.speed = 1.10f;
  playback_parameters.pitch = 0.90f;
  run_step("SetPlaybackParameters", [&]() { player->SetPlaybackParameters(playback_parameters); });
  TrackSelectionParametersDescriptor parameters;
  parameters.preferred_text_language = "en";
  parameters.select_text_by_default = true;
  run_step("SetTrackSelectionParameters", [&]() {
    player->SetTrackSelectionParameters(parameters);
  });
  run_step("DispatchTracksChangedForTest", [&]() {
    PlaybackSnapshot snapshot;
    TracksSnapshot tracks = BuildListenerSmokeTracksSnapshot();
    PlayerListener* base_listener = listener;
    base_listener->OnTracksChanged(snapshot, tracks);
  });
  MediaMetadataSnapshot playlist_metadata;
  playlist_metadata.title = "Listener Playlist";
  playlist_metadata.artist = "Listener Artist";
  playlist_metadata.album_title = "Listener Playlist Album";
  playlist_metadata.album_artist = "Listener Playlist Album Artist";
  playlist_metadata.display_title = "Listener Playlist Display";
  playlist_metadata.subtitle = "Listener Playlist Subtitle";
  playlist_metadata.description = "Listener Playlist Description";
  playlist_metadata.writer = "Listener Playlist Writer";
  playlist_metadata.author = "Listener Playlist Author";
  playlist_metadata.composer = "Listener Playlist Composer";
  playlist_metadata.conductor = "Listener Playlist Conductor";
  playlist_metadata.compilation = "Listener Playlist Compilation";
  playlist_metadata.genre = "Listener Playlist Genre";
  playlist_metadata.station = "Listener Playlist Station";
  playlist_metadata.media_type = 6;
  playlist_metadata.artwork_uri = "https://example.com/listener-playlist-artwork.jpg";
  playlist_metadata.artwork_data = {7, 7, 8};
  playlist_metadata.artwork_data_type = 8;
  playlist_metadata.extras_present = true;
  playlist_metadata.extras_key_count = 1;
  playlist_metadata.extras_token =
      "generated-opaque-object-token-listener-playlist-metadata-extras";
  RegisterBundleForOpaqueToken(
      env,
      playlist_metadata.extras_token,
      "listener-playlist-metadata-key",
      "listener-playlist-metadata-value");
  run_step("SetPlaylistMetadata", [&]() { player->SetPlaylistMetadata(playlist_metadata); });
  CueSnapshot listener_cues;
  listener_cues.cue_count = 2;
  listener_cues.presentation_time_us = 567890;
  listener_cues.texts = {"Listener Cue 1", "Listener Cue 2"};
  listener_cues.text_tokens = {
      "generated-opaque-object-token-listener-cue-1",
      "generated-opaque-object-token-listener-cue-2"};
  listener_cues.bitmap_tokens = {
      "generated-opaque-object-token-listener-bitmap-1",
      ""};
  listener_cues.cues.resize(2);
  listener_cues.cues[0].text = "Listener Cue 1";
  listener_cues.cues[0].text_token = "generated-opaque-object-token-listener-cue-1";
  listener_cues.cues[0].bitmap_token = "generated-opaque-object-token-listener-bitmap-1";
  listener_cues.cues[0].text_alignment = 2;
  listener_cues.cues[0].multi_row_alignment = 1;
  listener_cues.cues[0].line = 0.25f;
  listener_cues.cues[0].line_type = 0;
  listener_cues.cues[0].position_anchor = 2;
  listener_cues.cues[1].text = "Listener Cue 2";
  listener_cues.cues[1].text_token = "generated-opaque-object-token-listener-cue-2";
  listener_cues.cues[1].line_type = 1;
  listener_cues.cues[1].position_anchor = 1;
  listener_cues.cues[1].text_size = 22.0f;
  listener_cues.cues[1].text_size_type = 3;
  listener_cues.cues[1].vertical_type = 1;
  run_step("SimulateCurrentCuesForTest", [&]() { player->SimulateCurrentCuesForTest(listener_cues); });
  run_step("SeekToMediaItem", [&]() { player->SeekToMediaItem(1, 3456); });
  LogInfo("listenerSmoke populate end");
}

}  // namespace

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeListenerSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  LogListenerSmokeBuildMarker("nativeListenerSmokeTest");
  LogInfo("nativeListenerSmokeTest stage=start");
  CapturingPlayerListener listener;
  LogInfo(
      "nativeListenerSmokeTest stage=listenerConstructed," +
      BuildListenerLayoutSummary(listener));
  listener.ResetSmokeSignalState();
  LogInfo(
      "nativeListenerSmokeTest stage=listenerReset," +
      BuildListenerLayoutSummary(listener));
  RunListenerLocalDispatchSanityCheck(&listener);
  listener.ResetSmokeSignalState();
  LogInfo(
      "nativeListenerSmokeTest stage=postSelfDispatchReset," +
      BuildListenerLayoutSummary(listener));
  std::unique_ptr<CapturingPlayerListener> heap_listener =
      std::make_unique<CapturingPlayerListener>();
  LogInfo(
      "nativeListenerSmokeTest stage=heapListenerConstructed," +
      BuildListenerLayoutSummary(*heap_listener));
  heap_listener->ResetSmokeSignalState();
  LogInfo(
      "nativeListenerSmokeTest stage=heapListenerReset," +
      BuildListenerLayoutSummary(*heap_listener));
  heap_listener.reset();
  PlayerConfig config;
  LogInfo(
      "nativeListenerSmokeTest stage=configConstructed," +
      BuildListenerLayoutSummary(listener));
  LogInfo(
      "nativeListenerSmokeTest listenerLayout " +
      BuildListenerLayoutSummary(listener));
  listener.SetCaptureAnalyticsCallbacks(false);
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  LogInfo("nativeListenerSmokeTest stage=playerCreated");
  PopulateListenerSmokeScenario(env, player.get(), &listener);
  LogInfo("nativeListenerSmokeTest stage=populateComplete");
  bool ready = WaitForListenerSmokeScenario(&listener, 1500, "nativeListenerSmokeTest");
  LogInfo(
      "nativeListenerSmokeTest stage=waitComplete ready=" + std::to_string(ready ? 1 : 0));
  player->RemoveListener(&listener);
  LogInfo("nativeListenerSmokeTest stage=listenerRemoved");
  RepeatMode repeat_mode = player->GetRepeatMode();
  bool shuffle_enabled = player->GetShuffleModeEnabled();
  TrackSelectionParametersDescriptor track_selection_parameters =
      player->GetTrackSelectionParameters();
  PlaybackParametersSnapshot playback_parameters = player->GetPlaybackParameters();
  TimelineDetailsSnapshot timeline = player->GetTimeline();
  int current_media_item_index = player->GetCurrentMediaItemIndex();
  MediaItemDescriptor first_media_item = player->GetMediaItemAt(0);
  MediaItemDescriptor second_media_item = player->GetMediaItemAt(1);
  MediaMetadataSnapshot first_media_metadata = first_media_item.media_metadata;
  MediaMetadataSnapshot playlist_metadata = player->GetPlaylistMetadata();
  CueSnapshot current_cues = player->GetCurrentCues();
  std::unique_lock<std::recursive_mutex> listener_lock(listener.mutex, std::defer_lock);
  if (!TryLockListenerForSummary(&listener, 1000, "nativeListenerSmokeTest", &listener_lock)) {
    std::string summary = BuildListenerSummaryLockTimeoutSummary(listener, ready);
    player.reset();
    jstring result = NewStringUtfChecked(env, summary, "nativeListenerSmokeTest");
    LogError("nativeListenerSmokeTest stage=summaryLockTimeout," + summary);
    return result;
  }
  LogInfo(
      "nativeListenerSmokeTest stage=summaryBuilding," +
      BuildListenerSmokeProgressSummary(listener));
  std::string summary =
      "repeatMode=" + std::to_string(static_cast<int>(repeat_mode));
  auto append_int = [&summary](const char* key, long long value) {
    summary += ",";
    summary += key;
    summary += "=";
    summary += std::to_string(value);
  };
  auto append_bool = [&summary, &append_int](const char* key, bool value) {
    append_int(key, value ? 1 : 0);
  };
  auto append_float = [&summary](const char* key, float value) {
    summary += ",";
    summary += key;
    summary += "=";
    summary += std::to_string(value);
  };
  auto append_string = [&summary](const char* key, const std::string& value) {
    summary += ",";
    summary += key;
    summary += "=";
    summary += value;
  };
  append_bool("shuffle", shuffle_enabled);
  append_string("text", track_selection_parameters.preferred_text_language);
  append_float("speed", playback_parameters.speed);
  append_float("pitch", playback_parameters.pitch);
  append_int(
      "repeatCb",
      ObservedCallbackFlag(
          listener.smoke_repeat_callback_count.load(std::memory_order_acquire)));
  append_int(
      "shuffleCb",
      ObservedCallbackFlag(
          listener.smoke_shuffle_callback_count.load(std::memory_order_acquire)));
  append_int(
      "trackCb",
      ObservedCallbackFlag(
          listener.smoke_track_selection_callback_count.load(std::memory_order_acquire)));
  append_int(
      "playbackParamsCb",
      ObservedCallbackFlag(
          listener.smoke_playback_parameters_callback_count.load(std::memory_order_acquire)));
  append_int(
      "timelineCb",
      ObservedCallbackFlag(
          listener.smoke_timeline_callback_count.load(std::memory_order_acquire)));
  append_int("timelineWindowCount", timeline.summary.window_count);
  append_int("timelinePeriodCount", timeline.summary.period_count);
  append_int("timelineCurrentMediaItemIndex", current_media_item_index);
  append_int(
      "tracksChangedCb",
      ObservedCallbackFlag(
          listener.smoke_tracks_changed_callback_count.load(std::memory_order_acquire)));
  append_int(
      "mediaMetadataCb",
      ObservedCallbackFlag(
          listener.smoke_media_metadata_callback_count.load(std::memory_order_acquire)));
  append_int(
      "playlistMetadataCb",
      ObservedCallbackFlag(
          listener.smoke_playlist_metadata_callback_count.load(std::memory_order_acquire)));
  append_int(
      "cueCb",
      ObservedCallbackFlag(
          listener.smoke_cue_callback_count.load(std::memory_order_acquire)));
  append_int(
      "positionDiscontinuityCb",
      ObservedCallbackFlag(
          listener.smoke_position_discontinuity_callback_count.load(std::memory_order_acquire)));
  append_int(
      "isLoadingCb",
      ObservedCallbackFlag(
          listener.smoke_is_loading_callback_count.load(std::memory_order_acquire)));
  append_bool("isLoading", listener.is_loading);
  append_string("timelineWindow0MediaId", first_media_item.media_id);
  append_bool("timelineWindow0TagPresent", first_media_item.tag_present);
  append_string("timelineWindow0TagString", first_media_item.tag_string);
  append_bool(
      "timelineWindow0TagTokenPresent",
      !first_media_item.tag_token.empty());
  append_int(
      "timelineWindow1MediaItemIndex",
      timeline.windows.size() > 1
          ? timeline.windows[1].media_item_index
          : (second_media_item.media_id.empty() ? -1 : 1));
  append_string("timelineWindow1MediaId", second_media_item.media_id);
  append_string("timelineWindow1TagString", second_media_item.tag_string);
  append_bool(
      "timelineWindow1TagTokenPresent",
      !second_media_item.tag_token.empty());
  append_bool(
      "timelineWindow1LiveConfigurationPresent",
      HasExplicitLiveConfiguration(second_media_item));
  append_int(
      "timelineWindow1LiveTargetOffsetMs",
      second_media_item.live_configuration.target_offset_ms);
  append_int(
      "timelineWindow1LiveMinOffsetMs",
      second_media_item.live_configuration.min_offset_ms);
  append_int(
      "timelineWindow1LiveMaxOffsetMs",
      second_media_item.live_configuration.max_offset_ms);
  append_float(
      "timelineWindow1LiveMinSpeed",
      second_media_item.live_configuration.min_playback_speed);
  append_float(
      "timelineWindow1LiveMaxSpeed",
      second_media_item.live_configuration.max_playback_speed);
  append_bool("firstTrackGroupTokenPresent", listener.first_track_group_token_present);
  append_string("secondTrackGroupId", listener.second_track_group_id);
  append_string("secondTrackLabel", listener.second_track_label);
  append_string("secondTrackLanguage", listener.second_track_language);
  append_string("secondTrackMimeType", listener.second_track_mime_type);
  append_int(
      "secondTrackAccessibilityChannel", listener.second_track_accessibility_channel);
  append_int("secondTrackRoleFlags", listener.second_track_role_flags);
  append_int("secondTrackSelectionFlags", listener.second_track_selection_flags);
  append_bool("secondTrackSelected", listener.second_track_selected);
  append_bool("secondTrackSupported", listener.second_track_supported);
  append_bool(
      "secondTrackSupportedWithinCapabilities",
      listener.second_track_supported_within_capabilities);
  append_string("mediaMetadataAlbumTitle", first_media_metadata.album_title);
  append_string("mediaMetadataWriter", first_media_metadata.writer);
  append_string("mediaMetadataGenre", first_media_metadata.genre);
  append_bool("mediaMetadataExtrasPresent", first_media_metadata.extras_present);
  append_int("mediaMetadataExtrasKeyCount", first_media_metadata.extras_key_count);
  append_bool(
      "mediaMetadataExtrasTokenPresent", !first_media_metadata.extras_token.empty());
  append_string("mediaMetadataArtworkUri", first_media_metadata.artwork_uri);
  append_int(
      "mediaMetadataArtworkDataLength",
      static_cast<int>(first_media_metadata.artwork_data.size()));
  append_int("mediaMetadataArtworkDataType", first_media_metadata.artwork_data_type);
  append_string("playlistMetadataAlbumTitle", playlist_metadata.album_title);
  append_string("playlistMetadataConductor", playlist_metadata.conductor);
  append_string("playlistMetadataStation", playlist_metadata.station);
  append_bool("playlistMetadataExtrasPresent", playlist_metadata.extras_present);
  append_int(
      "playlistMetadataExtrasKeyCount", playlist_metadata.extras_key_count);
  append_bool(
      "playlistMetadataExtrasTokenPresent",
      !playlist_metadata.extras_token.empty());
  append_string("playlistMetadataArtworkUri", playlist_metadata.artwork_uri);
  append_int(
      "playlistMetadataArtworkDataLength",
      static_cast<int>(playlist_metadata.artwork_data.size()));
  append_int("playlistMetadataArtworkDataType", playlist_metadata.artwork_data_type);
  append_string("cue0Text", current_cues.cues.empty() ? "" : current_cues.cues[0].text);
  append_int(
      "cue0TextAlignment",
      current_cues.cues.empty() ? 0 : current_cues.cues[0].text_alignment);
  append_float("cue0Line", current_cues.cues.empty() ? 0.0f : current_cues.cues[0].line);
  append_string(
      "cue1Text", current_cues.cues.size() > 1 ? current_cues.cues[1].text : "");
  append_int(
      "cue1LineType",
      current_cues.cues.size() > 1 ? current_cues.cues[1].line_type : 0);
  append_float(
      "cue1TextSize",
      current_cues.cues.size() > 1 ? current_cues.cues[1].text_size : 0.0f);
  append_int(
      "cue1VerticalType",
      current_cues.cues.size() > 1 ? current_cues.cues[1].vertical_type : 0);
  append_bool("oldTagTokenPresent", listener.old_position_tag_token_present);
  append_int("newPositionMediaItemIndex", listener.new_position_media_item_index);
  append_int("newPositionMs", listener.new_position_position_ms);
  append_string("newMediaId", listener.new_position_media_id);
  append_bool("newTagTokenPresent", listener.new_position_tag_token_present);
  listener_lock.unlock();
  player.reset();
  jstring result = NewStringUtfChecked(env, summary, "nativeListenerSmokeTest");
  LogInfo(
      "nativeListenerSmokeTest stage=return summaryLength=" +
      std::to_string(summary.size()) + " result=" +
      std::to_string(result != nullptr ? 1 : 0));
  return result;
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeListenerCallbackDetailSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  LogListenerSmokeBuildMarker("nativeListenerCallbackDetailSmokeTest");
  LogInfo("nativeListenerCallbackDetailSmokeTest stage=start");
  PlayerConfig config;
  CapturingPlayerListener listener;
  LogInfo(
      "nativeListenerCallbackDetailSmokeTest listenerLayout " +
      BuildListenerLayoutSummary(listener));
  listener.SetCaptureAnalyticsCallbacks(false);
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  LogInfo("nativeListenerCallbackDetailSmokeTest stage=playerCreated");
  PopulateListenerSmokeScenario(env, player.get(), &listener);
  LogInfo("nativeListenerCallbackDetailSmokeTest stage=populateComplete");
  bool ready =
      WaitForListenerSmokeScenario(&listener, 1500, "nativeListenerCallbackDetailSmokeTest");
  LogInfo(
      "nativeListenerCallbackDetailSmokeTest stage=waitComplete ready=" +
      std::to_string(ready ? 1 : 0));
  player->RemoveListener(&listener);
  LogInfo("nativeListenerCallbackDetailSmokeTest stage=listenerRemoved");
  std::unique_lock<std::recursive_mutex> listener_lock(listener.mutex, std::defer_lock);
  if (!TryLockListenerForSummary(
          &listener, 1000, "nativeListenerCallbackDetailSmokeTest", &listener_lock)) {
    std::string summary = BuildListenerSummaryLockTimeoutSummary(listener, ready);
    player.reset();
    jstring result =
        NewStringUtfChecked(env, summary, "nativeListenerCallbackDetailSmokeTest");
    LogError("nativeListenerCallbackDetailSmokeTest stage=summaryLockTimeout," + summary);
    return result;
  }
  LogInfo(
      "nativeListenerCallbackDetailSmokeTest stage=summaryBuilding," +
      BuildListenerSmokeProgressSummary(listener));
  std::string summary = "seekBackIncrementChangedCb=" +
      std::to_string(
          ObservedCallbackFlag(
              listener.smoke_seek_back_increment_callback_count.load(
                  std::memory_order_acquire)));
  summary += ",seekBackIncrementChangedMs=" + std::to_string(listener.seek_back_increment_ms);
  summary += ",seekForwardIncrementChangedCb=" +
      std::to_string(
          ObservedCallbackFlag(
              listener.smoke_seek_forward_increment_callback_count.load(
                  std::memory_order_acquire)));
  summary += ",seekForwardIncrementChangedMs=" +
      std::to_string(listener.seek_forward_increment_ms);
  summary += ",maxSeekToPreviousPositionChangedCb=" +
      std::to_string(
          ObservedCallbackFlag(
              listener.smoke_max_seek_to_previous_position_callback_count.load(
                  std::memory_order_acquire)));
  summary += ",maxSeekToPreviousPositionChangedMs=" +
      std::to_string(listener.max_seek_to_previous_position_ms);
  summary += ",playbackSuppressionReason=" +
      std::to_string(listener.playback_suppression_reason);
  summary += ",playbackSuppressionCb=" +
      std::to_string(
          ObservedCallbackFlag(
              listener.smoke_playback_suppression_reason_callback_count.load(
                  std::memory_order_acquire)));
  summary += ",availableCommandsCb=" +
      std::to_string(
          ObservedCallbackFlag(
              listener.smoke_available_commands_callback_count.load(
                  std::memory_order_acquire)));
  summary += ",availableCommandsCount=" +
      std::to_string(listener.available_commands_count);
  summary += ",eventsCb=" +
      std::to_string(
          ObservedCallbackFlag(
              listener.smoke_events_callback_count.load(std::memory_order_acquire)));
  summary += ",eventCount=" + std::to_string(listener.last_event_count);
  listener_lock.unlock();
  player.reset();
  jstring result = NewStringUtfChecked(env, summary, "nativeListenerCallbackDetailSmokeTest");
  LogInfo(
      "nativeListenerCallbackDetailSmokeTest stage=return summaryLength=" +
      std::to_string(summary.size()) + " result=" +
      std::to_string(result != nullptr ? 1 : 0));
  return result;
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeListenerMetadataCueDetailSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  LogListenerSmokeBuildMarker("nativeListenerMetadataCueDetailSmokeTest");
  LogInfo("nativeListenerMetadataCueDetailSmokeTest stage=start");
  PlayerConfig config;
  CapturingPlayerListener listener;
  LogInfo(
      "nativeListenerMetadataCueDetailSmokeTest listenerLayout " +
      BuildListenerLayoutSummary(listener));
  listener.SetCaptureAnalyticsCallbacks(false);
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  LogInfo("nativeListenerMetadataCueDetailSmokeTest stage=playerCreated");
  PopulateListenerSmokeScenario(env, player.get(), &listener);
  LogInfo("nativeListenerMetadataCueDetailSmokeTest stage=populateComplete");
  bool ready = WaitForListenerSmokeScenario(
      &listener, 1500, "nativeListenerMetadataCueDetailSmokeTest");
  LogInfo(
      "nativeListenerMetadataCueDetailSmokeTest stage=waitComplete ready=" +
      std::to_string(ready ? 1 : 0));
  player->RemoveListener(&listener);
  LogInfo("nativeListenerMetadataCueDetailSmokeTest stage=listenerRemoved");
  MediaItemDescriptor first_media_item = player->GetMediaItemAt(0);
  MediaMetadataSnapshot media_metadata = first_media_item.media_metadata;
  MediaMetadataSnapshot playlist_metadata = player->GetPlaylistMetadata();
  CueSnapshot current_cues = player->GetCurrentCues();
  std::unique_lock<std::recursive_mutex> listener_lock(listener.mutex, std::defer_lock);
  if (!TryLockListenerForSummary(
          &listener, 1000, "nativeListenerMetadataCueDetailSmokeTest", &listener_lock)) {
    std::string summary = BuildListenerSummaryLockTimeoutSummary(listener, ready);
    player.reset();
    jstring result =
        NewStringUtfChecked(env, summary, "nativeListenerMetadataCueDetailSmokeTest");
    LogError("nativeListenerMetadataCueDetailSmokeTest stage=summaryLockTimeout," + summary);
    return result;
  }
  LogInfo(
      "nativeListenerMetadataCueDetailSmokeTest stage=summaryBuilding," +
      BuildListenerSmokeProgressSummary(listener));
  std::string summary = "mediaMetadataTitle=" + media_metadata.title;
  summary += ",mediaMetadataTitleTokenPresent=" +
      std::to_string(media_metadata.title_token.empty() ? 0 : 1);
  AppendObjectValueSummary(&summary, "mediaMetadataTitleValue", media_metadata.title_value);
  summary += ",mediaMetadataArtist=" + media_metadata.artist;
  summary += ",mediaMetadataArtistTokenPresent=" +
      std::to_string(media_metadata.artist_token.empty() ? 0 : 1);
  summary += ",mediaMetadataAlbumArtist=" + media_metadata.album_artist;
  summary += ",mediaMetadataAlbumArtistTokenPresent=" +
      std::to_string(media_metadata.album_artist_token.empty() ? 0 : 1);
  summary += ",mediaMetadataDisplayTitle=" + media_metadata.display_title;
  summary += ",mediaMetadataDisplayTitleTokenPresent=" +
      std::to_string(media_metadata.display_title_token.empty() ? 0 : 1);
  summary += ",mediaMetadataSubtitle=" + media_metadata.subtitle;
  summary += ",mediaMetadataSubtitleTokenPresent=" +
      std::to_string(media_metadata.subtitle_token.empty() ? 0 : 1);
  summary += ",mediaMetadataDescription=" + media_metadata.description;
  summary += ",mediaMetadataDescriptionTokenPresent=" +
      std::to_string(media_metadata.description_token.empty() ? 0 : 1);
  summary += ",mediaMetadataAuthor=" + media_metadata.author;
  summary += ",mediaMetadataAuthorTokenPresent=" +
      std::to_string(media_metadata.author_token.empty() ? 0 : 1);
  summary += ",mediaMetadataComposer=" + media_metadata.composer;
  summary += ",mediaMetadataComposerTokenPresent=" +
      std::to_string(media_metadata.composer_token.empty() ? 0 : 1);
  summary += ",mediaMetadataConductor=" + media_metadata.conductor;
  summary += ",mediaMetadataConductorTokenPresent=" +
      std::to_string(media_metadata.conductor_token.empty() ? 0 : 1);
  summary += ",mediaMetadataCompilation=" + media_metadata.compilation;
  summary += ",mediaMetadataCompilationTokenPresent=" +
      std::to_string(media_metadata.compilation_token.empty() ? 0 : 1);
  summary += ",mediaMetadataStation=" + media_metadata.station;
  summary += ",mediaMetadataStationTokenPresent=" +
      std::to_string(media_metadata.station_token.empty() ? 0 : 1);
  AppendObjectValueSummary(
      &summary, "mediaMetadataStationValue", media_metadata.station_value);
  summary += ",mediaMetadataMediaType=" +
      std::to_string(media_metadata.media_type);
  summary += ",playlistMetadataTitle=" + playlist_metadata.title;
  AppendObjectValueSummary(&summary, "playlistMetadataTitleValue", playlist_metadata.title_value);
  summary += ",playlistMetadataArtist=" + playlist_metadata.artist;
  summary += ",playlistMetadataAlbumArtist=" + playlist_metadata.album_artist;
  summary += ",playlistMetadataDisplayTitle=" + playlist_metadata.display_title;
  summary += ",playlistMetadataSubtitle=" + playlist_metadata.subtitle;
  summary += ",playlistMetadataDescription=" + playlist_metadata.description;
  summary += ",playlistMetadataWriter=" + playlist_metadata.writer;
  summary += ",playlistMetadataAuthor=" + playlist_metadata.author;
  summary += ",playlistMetadataComposer=" + playlist_metadata.composer;
  summary += ",playlistMetadataCompilation=" + playlist_metadata.compilation;
  summary += ",playlistMetadataGenre=" + playlist_metadata.genre;
  summary += ",playlistMetadataMediaType=" +
      std::to_string(playlist_metadata.media_type);
  summary += ",cueCount=" + std::to_string(current_cues.cue_count);
  summary += ",cuePresentationTimeUs=" + std::to_string(current_cues.presentation_time_us);
  summary += ",cue0TextTokenPresent=" +
      std::to_string(
          !current_cues.cues.empty() && !current_cues.cues[0].text_token.empty() ? 1 : 0);
  summary += ",cue0BitmapTokenPresent=" +
      std::to_string(
          !current_cues.cues.empty() && !current_cues.cues[0].bitmap_token.empty() ? 1 : 0);
  summary += ",cue0MultiRowAlignment=" +
      std::to_string(current_cues.cues.empty() ? 0 : current_cues.cues[0].multi_row_alignment);
  summary += ",cue0LineType=" +
      std::to_string(current_cues.cues.empty() ? 0 : current_cues.cues[0].line_type);
  summary += ",cue0PositionAnchor=" +
      std::to_string(current_cues.cues.empty() ? 0 : current_cues.cues[0].position_anchor);
  summary += ",cue1TextTokenPresent=" +
      std::to_string(
          current_cues.cues.size() > 1 && !current_cues.cues[1].text_token.empty() ? 1 : 0);
  summary += ",cue1BitmapTokenPresent=" +
      std::to_string(
          current_cues.cues.size() > 1 && !current_cues.cues[1].bitmap_token.empty() ? 1 : 0);
  summary += ",cue1PositionAnchor=" +
      std::to_string(current_cues.cues.size() > 1 ? current_cues.cues[1].position_anchor : 0);
  summary += ",cue1TextSizeType=" +
      std::to_string(current_cues.cues.size() > 1 ? current_cues.cues[1].text_size_type : 0);
  summary += ",oldPositionMediaItemIndex=" +
      std::to_string(listener.old_position_media_item_index);
  summary += ",oldPositionPeriodIndex=" +
      std::to_string(listener.old_position_period_index);
  summary += ",oldPositionMs=" + std::to_string(listener.old_position_position_ms);
  summary += ",oldContentPositionMs=" +
      std::to_string(listener.old_position_content_position_ms);
  summary += ",oldAdGroupIndex=" + std::to_string(listener.old_position_ad_group_index);
  summary += ",oldAdIndexInAdGroup=" +
      std::to_string(listener.old_position_ad_index_in_ad_group);
  summary += ",oldMediaId=" + listener.old_position_media_id;
  summary += ",newPositionPeriodIndex=" +
      std::to_string(listener.new_position_period_index);
  summary += ",newContentPositionMs=" +
      std::to_string(listener.new_position_content_position_ms);
  summary += ",newAdGroupIndex=" + std::to_string(listener.new_position_ad_group_index);
  summary += ",newAdIndexInAdGroup=" +
      std::to_string(listener.new_position_ad_index_in_ad_group);
  listener_lock.unlock();
  player.reset();
  jstring result =
      NewStringUtfChecked(env, summary, "nativeListenerMetadataCueDetailSmokeTest");
  LogInfo(
      "nativeListenerMetadataCueDetailSmokeTest stage=return summaryLength=" +
      std::to_string(summary.size()) + " result=" +
      std::to_string(result != nullptr ? 1 : 0));
  return result;
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeListenerDetachSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener listener;
  player->SetListener(&listener);
  player->SetRepeatMode(androidx::media3::cppbridge::RepeatMode::kAll);
  int repeat_before_detach = listener.repeat_callback_count;
  player->RemoveListener(&listener);
  player->SetRepeatMode(androidx::media3::cppbridge::RepeatMode::kOff);
  player->SetShuffleModeEnabled(true);
  PlaybackParametersSnapshot playback_parameters;
  playback_parameters.speed = 1.25f;
  playback_parameters.pitch = 0.95f;
  player->SetPlaybackParameters(playback_parameters);
  std::string summary = "repeatBeforeDetach=" + std::to_string(repeat_before_detach);
  summary += ",repeatAfterDetach=" + std::to_string(listener.repeat_callback_count);
  summary += ",shuffleAfterDetach=" + std::to_string(listener.shuffle_callback_count);
  summary += ",playbackParamsAfterDetach=" +
      std::to_string(listener.playback_parameters_callback_count);
  return NewStringUtfChecked(env, summary, "nativeListenerDetachSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeDeviceAndSkipSilenceSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  player->SetWakeMode(0);
  player->SetSkipSilenceEnabled(true);
  const int initial_device_volume = player->GetDeviceVolume();
  const bool initial_device_muted = player->IsDeviceMuted();
  player->SetDeviceVolume(initial_device_volume, 0);
  player->IncreaseDeviceVolume(0);
  player->DecreaseDeviceVolume(0);
  player->SetDeviceMuted(initial_device_muted, 0);
  DeviceInfoDescriptor device_info = player->GetDeviceInfo();
  std::string summary = "deviceType=" + std::to_string(device_info.playback_type);
  summary += ",minVol=" + std::to_string(device_info.min_volume);
  summary += ",maxVol=" + std::to_string(device_info.max_volume);
  summary += ",routingControllerId=" + device_info.routing_controller_id;
  summary += ",deviceVol=" + std::to_string(player->GetDeviceVolume());
  summary += ",muted=" + std::to_string(player->IsDeviceMuted() ? 1 : 0);
  summary += ",skipSilence=" + std::to_string(player->GetSkipSilenceEnabled() ? 1 : 0);
  summary += ",deviceControlCalls=1";
  return NewStringUtfChecked(env, summary, "nativeDeviceAndSkipSilenceSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeVideoAndMetadataSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/video-metadata.mp4";
  media_item.media_id = "video-metadata";
  media_item.source_type = MediaSourceType::kDefault;
  media_item.media_metadata.title = "Video Metadata Title";
  media_item.media_metadata.artist = "Video Metadata Artist";
  media_item.media_metadata.display_title = "Video Metadata Display";
  media_item.media_metadata.description = "Video Metadata Description";
  media_item.media_metadata.author = "Video Metadata Author";
  media_item.media_metadata.genre = "Video Metadata Genre";
  media_item.media_metadata.writer = "Video Metadata Writer";
  media_item.media_metadata.conductor = "Video Metadata Conductor";
  media_item.media_metadata.duration_ms = 654321;
  media_item.media_metadata.track_number = 7;
  media_item.media_metadata.total_track_count = 12;
  media_item.media_metadata.disc_number = 1;
  media_item.media_metadata.total_disc_count = 3;
  media_item.media_metadata.is_playable = 1;
  media_item.media_metadata.folder_type = 4;
  media_item.media_metadata.release_year = 2024;
  media_item.media_metadata.release_month = 6;
  media_item.media_metadata.release_day = 14;
  media_item.media_metadata.media_type = 1;
  media_item.media_metadata.station = "Video Metadata Station";
  media_item.media_metadata.compilation = "Video Metadata Compilation";
  media_item.media_metadata.extras_present = true;
  media_item.media_metadata.extras_key_count = 1;
  media_item.media_metadata.extras_token =
      "generated-opaque-object-token-video-metadata-extras";
  RegisterBundleForOpaqueToken(
      env,
      media_item.media_metadata.extras_token,
      "video-metadata-key",
      "video-metadata-value");
  media_item.media_metadata.artwork_uri = "https://example.com/video-metadata-artwork.jpg";
  media_item.media_metadata.artwork_data = {9, 8, 7, 6};
  media_item.media_metadata.artwork_data_type = 3;
  player->SetMediaItem(media_item);
  MediaMetadataSnapshot playlist_input;
  playlist_input.title = "Metadata Playlist Title";
  playlist_input.artist = "Metadata Playlist Artist";
  playlist_input.display_title = "Metadata Playlist Display";
  playlist_input.genre = "Metadata Playlist Genre";
  playlist_input.recording_year = 2023;
  playlist_input.recording_month = 9;
  playlist_input.recording_day = 18;
  playlist_input.release_month = 10;
  playlist_input.release_day = 4;
  playlist_input.author = "Metadata Playlist Author";
  playlist_input.composer = "Metadata Playlist Composer";
  playlist_input.conductor = "Metadata Playlist Conductor";
  playlist_input.disc_number = 4;
  playlist_input.total_disc_count = 8;
  playlist_input.is_browsable = 1;
  playlist_input.folder_type = 2;
  playlist_input.media_type = 1;
  playlist_input.compilation = "Metadata Playlist Compilation";
  playlist_input.extras_present = true;
  playlist_input.extras_key_count = 1;
  playlist_input.extras_token =
      "generated-opaque-object-token-playlist-query-metadata-extras";
  RegisterBundleForOpaqueToken(
      env,
      playlist_input.extras_token,
      "playlist-query-metadata-key",
      "playlist-query-metadata-value");
  playlist_input.artwork_uri = "https://example.com/metadata-playlist-artwork.jpg";
  playlist_input.artwork_data = {4, 5, 6};
  playlist_input.artwork_data_type = 4;
  player->SetPlaylistMetadata(playlist_input);
  player->AdjustDeviceVolume(0, 0);
  VideoSizeSnapshot video_size = player->GetVideoSize();
  MediaMetadataSnapshot media_metadata = player->GetMediaMetadata();
  MediaMetadataSnapshot playlist_metadata = player->GetPlaylistMetadata();
  std::string summary = "videoWidth=" + std::to_string(video_size.width);
  summary += ",videoHeight=" + std::to_string(video_size.height);
  summary += ",videoUnappliedRotationDegrees=" +
      std::to_string(video_size.unapplied_rotation_degrees);
  summary += ",videoPixelWidthHeightRatio=" +
      std::to_string(video_size.pixel_width_height_ratio);
  summary += ",mediaTitle=" + media_metadata.title;
  summary += ",mediaTitleTokenPresent=" +
      std::to_string(media_metadata.title_token.empty() ? 0 : 1);
  summary += ",mediaArtist=" + media_metadata.artist;
  summary += ",mediaArtistTokenPresent=" +
      std::to_string(media_metadata.artist_token.empty() ? 0 : 1);
  summary += ",mediaDisplayTitle=" + media_metadata.display_title;
  summary += ",mediaDisplayTitleTokenPresent=" +
      std::to_string(media_metadata.display_title_token.empty() ? 0 : 1);
  summary += ",mediaDescription=" + media_metadata.description;
  summary += ",mediaAuthor=" + media_metadata.author;
  summary += ",mediaGenre=" + media_metadata.genre;
  summary += ",mediaWriter=" + media_metadata.writer;
  summary += ",mediaConductor=" + media_metadata.conductor;
  summary += ",mediaDurationMs=" + std::to_string(media_metadata.duration_ms);
  summary += ",mediaTrackNumber=" + std::to_string(media_metadata.track_number);
  summary += ",mediaTotalTrackCount=" + std::to_string(media_metadata.total_track_count);
  summary += ",mediaDiscNumber=" + std::to_string(media_metadata.disc_number);
  summary += ",mediaTotalDiscCount=" + std::to_string(media_metadata.total_disc_count);
  summary += ",mediaIsPlayable=" + std::to_string(media_metadata.is_playable);
  summary += ",mediaFolderType=" + std::to_string(media_metadata.folder_type);
  summary += ",mediaReleaseYear=" + std::to_string(media_metadata.release_year);
  summary += ",mediaReleaseMonth=" + std::to_string(media_metadata.release_month);
  summary += ",mediaReleaseDay=" + std::to_string(media_metadata.release_day);
  summary += ",mediaType=" + std::to_string(media_metadata.media_type);
  summary += ",mediaStation=" + media_metadata.station;
  summary += ",mediaCompilation=" + media_metadata.compilation;
  summary += ",mediaExtrasPresent=" + std::to_string(media_metadata.extras_present ? 1 : 0);
  summary += ",mediaExtrasKeyCount=" + std::to_string(media_metadata.extras_key_count);
  summary += ",mediaExtrasTokenPresent=" +
      std::to_string(media_metadata.extras_token.empty() ? 0 : 1);
  summary += ",mediaArtworkUri=" + media_metadata.artwork_uri;
  summary += ",mediaArtworkDataLength=" + std::to_string(media_metadata.artwork_data.size());
  summary += ",mediaArtworkDataType=" + std::to_string(media_metadata.artwork_data_type);
  summary += ",playlistTitle=" + playlist_metadata.title;
  summary += ",playlistTitleTokenPresent=" +
      std::to_string(playlist_metadata.title_token.empty() ? 0 : 1);
  summary += ",playlistArtist=" + playlist_metadata.artist;
  summary += ",playlistArtistTokenPresent=" +
      std::to_string(playlist_metadata.artist_token.empty() ? 0 : 1);
  summary += ",playlistDisplayTitle=" + playlist_metadata.display_title;
  summary += ",playlistDisplayTitleTokenPresent=" +
      std::to_string(playlist_metadata.display_title_token.empty() ? 0 : 1);
  summary += ",playlistGenre=" + playlist_metadata.genre;
  summary += ",playlistRecordingYear=" + std::to_string(playlist_metadata.recording_year);
  summary += ",playlistRecordingMonth=" + std::to_string(playlist_metadata.recording_month);
  summary += ",playlistRecordingDay=" + std::to_string(playlist_metadata.recording_day);
  summary += ",playlistReleaseMonth=" + std::to_string(playlist_metadata.release_month);
  summary += ",playlistReleaseDay=" + std::to_string(playlist_metadata.release_day);
  summary += ",playlistAuthor=" + playlist_metadata.author;
  summary += ",playlistComposer=" + playlist_metadata.composer;
  summary += ",playlistConductor=" + playlist_metadata.conductor;
  summary += ",playlistDiscNumber=" + std::to_string(playlist_metadata.disc_number);
  summary += ",playlistTotalDiscCount=" + std::to_string(playlist_metadata.total_disc_count);
  summary += ",playlistIsBrowsable=" + std::to_string(playlist_metadata.is_browsable);
  summary += ",playlistFolderType=" + std::to_string(playlist_metadata.folder_type);
  summary += ",playlistMediaType=" + std::to_string(playlist_metadata.media_type);
  summary += ",playlistCompilation=" + playlist_metadata.compilation;
  summary += ",playlistExtrasPresent=" +
      std::to_string(playlist_metadata.extras_present ? 1 : 0);
  summary += ",playlistExtrasKeyCount=" + std::to_string(playlist_metadata.extras_key_count);
  summary += ",playlistExtrasTokenPresent=" +
      std::to_string(playlist_metadata.extras_token.empty() ? 0 : 1);
  summary += ",playlistArtworkUri=" + playlist_metadata.artwork_uri;
  summary += ",playlistArtworkDataLength=" +
      std::to_string(playlist_metadata.artwork_data.size());
  summary += ",playlistArtworkDataType=" +
      std::to_string(playlist_metadata.artwork_data_type);
  return NewStringUtfChecked(env, summary, "nativeVideoAndMetadataSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  AnalyticsSnapshot analytics = player->GetAnalyticsSnapshot();
  std::string summary = "bitrate=" + std::to_string(analytics.bitrate_estimate);
  summary += ",dropped=" + std::to_string(analytics.dropped_video_frames);
  summary += ",loadStarted=" + std::to_string(analytics.load_started_count);
  summary += ",loadCompleted=" + std::to_string(analytics.load_completed_count);
  summary += ",loadDelta=" +
      std::to_string(analytics.load_started_count - analytics.load_completed_count);
  summary += ",audioMime=" + analytics.audio_sample_mime_type;
  summary += ",videoMime=" + analytics.video_sample_mime_type;
  return NewStringUtfChecked(env, summary, "nativeAnalyticsSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsCallbackSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::shared_ptr<ExoPlayerBridge> bridge = ExoPlayerBridge::Create(env, context, config);
  if (bridge == nullptr) {
    return NewStringUtfChecked(
        env, "analytics-error:createBridge", "nativeAnalyticsCallbackSmokeTest.error");
  }
  CapturingPlayerListener listener;
  bridge->SetListener(&listener);
  AnalyticsSnapshot first_analytics;
  first_analytics.bitrate_estimate = 1234567;
  first_analytics.dropped_video_frames = 4;
  first_analytics.load_started_count = 2;
  first_analytics.load_completed_count = 1;
  first_analytics.audio_sample_mime_type = "audio/test";
  first_analytics.video_sample_mime_type = "video/test";
  bridge->SimulateAnalyticsUpdateForTest(env, first_analytics);
  AnalyticsSnapshot second_analytics;
  second_analytics.bitrate_estimate = 2222222;
  second_analytics.dropped_video_frames = 7;
  second_analytics.load_started_count = 5;
  second_analytics.load_completed_count = 3;
  second_analytics.audio_sample_mime_type = "audio/final";
  second_analytics.video_sample_mime_type = "video/final";
  bridge->SimulateAnalyticsUpdateForTest(env, second_analytics);
  std::string summary = "analyticsCb=" + std::to_string(listener.analytics_callback_count);
  summary += ",bitrate=" + std::to_string(listener.analytics_bitrate_estimate);
  summary += ",dropped=" + std::to_string(listener.analytics_dropped_video_frames);
  summary += ",loadStarted=" + std::to_string(listener.analytics_load_started_count);
  summary += ",loadCompleted=" + std::to_string(listener.analytics_load_completed_count);
  summary += ",loadDelta=" + std::to_string(
      listener.analytics_load_started_count - listener.analytics_load_completed_count);
  summary += ",audioMime=" + listener.analytics_audio_sample_mime_type;
  summary += ",videoMime=" + listener.analytics_video_sample_mime_type;
  bridge->RemoveListener(&listener);
  bridge->Release(env);
  return NewStringUtfChecked(env, summary, "nativeAnalyticsCallbackSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsListenerRegistrationSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsSnapshot analytics;
  analytics.bitrate_estimate = 7654321;
  analytics.dropped_video_frames = 3;
  analytics.load_started_count = 5;
  analytics.load_completed_count = 2;
  analytics.audio_sample_mime_type = "audio/only";
  analytics.video_sample_mime_type = "video/only";
  player->SimulateAnalyticsUpdateForTest(analytics);
  AnalyticsSnapshot second_analytics;
  second_analytics.bitrate_estimate = 8765432;
  second_analytics.dropped_video_frames = 4;
  second_analytics.load_started_count = 6;
  second_analytics.load_completed_count = 4;
  second_analytics.audio_sample_mime_type = "audio/final";
  second_analytics.video_sample_mime_type = "video/final";
  player->SimulateAnalyticsUpdateForTest(second_analytics);
  int callback_count_before_remove = analytics_listener.analytics_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsSnapshot ignored_analytics;
  ignored_analytics.bitrate_estimate = 1;
  ignored_analytics.dropped_video_frames = 1;
  ignored_analytics.load_started_count = 1;
  ignored_analytics.load_completed_count = 1;
  ignored_analytics.audio_sample_mime_type = "audio/ignored";
  ignored_analytics.video_sample_mime_type = "video/ignored";
  player->SimulateAnalyticsUpdateForTest(ignored_analytics);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" + std::to_string(analytics_listener.analytics_callback_count);
  summary += ",callbackStopped=" +
      std::to_string(analytics_listener.analytics_callback_count == callback_count_before_remove ? 1 : 0);
  summary += ",bitrate=" + std::to_string(analytics_listener.analytics_bitrate_estimate);
  summary += ",dropped=" + std::to_string(analytics_listener.analytics_dropped_video_frames);
  summary += ",loadStarted=" + std::to_string(analytics_listener.analytics_load_started_count);
  summary += ",loadCompleted=" + std::to_string(analytics_listener.analytics_load_completed_count);
  summary += ",loadDelta=" + std::to_string(
      analytics_listener.analytics_load_started_count -
      analytics_listener.analytics_load_completed_count);
  summary += ",audioMime=" + analytics_listener.analytics_audio_sample_mime_type;
  summary += ",videoMime=" + analytics_listener.analytics_video_sample_mime_type;
  return NewStringUtfChecked(env, summary, "nativeAnalyticsListenerRegistrationSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsAudioUnderrunSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AudioUnderrunEvent first_audio_underrun;
  first_audio_underrun.buffer_size = 2048;
  first_audio_underrun.buffer_size_ms = 42;
  first_audio_underrun.elapsed_since_last_feed_ms = 11;
  player->SimulateAudioUnderrunForTest(first_audio_underrun);
  AudioUnderrunEvent second_audio_underrun;
  second_audio_underrun.buffer_size = 4096;
  second_audio_underrun.buffer_size_ms = 87;
  second_audio_underrun.elapsed_since_last_feed_ms = 23;
  player->SimulateAudioUnderrunForTest(second_audio_underrun);
  int callback_count_before_remove = analytics_listener.audio_underrun_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AudioUnderrunEvent ignored_audio_underrun;
  ignored_audio_underrun.buffer_size = 1;
  ignored_audio_underrun.buffer_size_ms = 2;
  ignored_audio_underrun.elapsed_since_last_feed_ms = 3;
  player->SimulateAudioUnderrunForTest(ignored_audio_underrun);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" + std::to_string(analytics_listener.audio_underrun_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.audio_underrun_callback_count == callback_count_before_remove ? 1 : 0);
  summary += ",bufferSize=" + std::to_string(analytics_listener.audio_underrun_buffer_size);
  summary += ",bufferSizeMs=" + std::to_string(analytics_listener.audio_underrun_buffer_size_ms);
  summary += ",elapsedSinceLastFeedMs=" +
      std::to_string(analytics_listener.audio_underrun_elapsed_since_last_feed_ms);
  return NewStringUtfChecked(env, summary, "nativeAnalyticsAudioUnderrunSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsDroppedVideoFramesSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  DroppedVideoFramesEvent first_dropped_video_frames;
  first_dropped_video_frames.dropped_frames = 3;
  first_dropped_video_frames.elapsed_ms = 17;
  player->SimulateDroppedVideoFramesForTest(first_dropped_video_frames);
  DroppedVideoFramesEvent second_dropped_video_frames;
  second_dropped_video_frames.dropped_frames = 8;
  second_dropped_video_frames.elapsed_ms = 41;
  player->SimulateDroppedVideoFramesForTest(second_dropped_video_frames);
  int callback_count_before_remove = analytics_listener.dropped_video_frames_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  DroppedVideoFramesEvent ignored_dropped_video_frames;
  ignored_dropped_video_frames.dropped_frames = 1;
  ignored_dropped_video_frames.elapsed_ms = 2;
  player->SimulateDroppedVideoFramesForTest(ignored_dropped_video_frames);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.dropped_video_frames_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.dropped_video_frames_callback_count == callback_count_before_remove
          ? 1
          : 0);
  summary += ",droppedFrames=" +
      std::to_string(analytics_listener.dropped_video_frames_event_count);
  summary += ",elapsedMs=" +
      std::to_string(analytics_listener.dropped_video_frames_elapsed_ms);
  return NewStringUtfChecked(env, summary, "nativeAnalyticsDroppedVideoFramesSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsBandwidthEstimateSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  BandwidthEstimateEvent first_bandwidth_estimate;
  first_bandwidth_estimate.elapsed_ms = 21;
  first_bandwidth_estimate.bytes_transferred = 12345;
  first_bandwidth_estimate.bitrate_estimate = 555555;
  player->SimulateBandwidthEstimateForTest(first_bandwidth_estimate);
  BandwidthEstimateEvent second_bandwidth_estimate;
  second_bandwidth_estimate.elapsed_ms = 34;
  second_bandwidth_estimate.bytes_transferred = 67890;
  second_bandwidth_estimate.bitrate_estimate = 999999;
  player->SimulateBandwidthEstimateForTest(second_bandwidth_estimate);
  int callback_count_before_remove = analytics_listener.bandwidth_estimate_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  BandwidthEstimateEvent ignored_bandwidth_estimate;
  ignored_bandwidth_estimate.elapsed_ms = 1;
  ignored_bandwidth_estimate.bytes_transferred = 2;
  ignored_bandwidth_estimate.bitrate_estimate = 3;
  player->SimulateBandwidthEstimateForTest(ignored_bandwidth_estimate);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.bandwidth_estimate_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.bandwidth_estimate_callback_count == callback_count_before_remove
          ? 1
          : 0);
  summary += ",elapsedMs=" + std::to_string(analytics_listener.bandwidth_estimate_elapsed_ms);
  summary += ",bytesTransferred=" +
      std::to_string(analytics_listener.bandwidth_estimate_bytes_transferred);
  summary += ",bitrateEstimate=" +
      std::to_string(analytics_listener.bandwidth_estimate_bitrate_estimate);
  return NewStringUtfChecked(env, summary, "nativeAnalyticsBandwidthEstimateSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsLoadStartedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  LoadStartedEvent first_load_started;
  first_load_started.uri = "https://example.com/analytics-first.m3u8";
  first_load_started.data_type = 1;
  first_load_started.track_type = 2;
  first_load_started.retry_count = 0;
  player->SimulateLoadStartedForTest(first_load_started);
  LoadStartedEvent second_load_started;
  second_load_started.uri = "https://example.com/analytics-final.m3u8";
  second_load_started.data_type = 3;
  second_load_started.track_type = 1;
  second_load_started.retry_count = 2;
  player->SimulateLoadStartedForTest(second_load_started);
  int callback_count_before_remove = analytics_listener.load_started_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  LoadStartedEvent ignored_load_started;
  ignored_load_started.uri = "https://example.com/analytics-ignored.m3u8";
  ignored_load_started.data_type = 9;
  ignored_load_started.track_type = 9;
  ignored_load_started.retry_count = 9;
  player->SimulateLoadStartedForTest(ignored_load_started);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" + std::to_string(analytics_listener.load_started_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.load_started_callback_count == callback_count_before_remove ? 1 : 0);
  summary += ",uri=" + analytics_listener.load_started_uri;
  summary += ",dataType=" + std::to_string(analytics_listener.load_started_data_type);
  summary += ",trackType=" + std::to_string(analytics_listener.load_started_track_type);
  summary += ",retryCount=" + std::to_string(analytics_listener.load_started_retry_count);
  return NewStringUtfChecked(env, summary, "nativeAnalyticsLoadStartedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsLoadCompletedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  LoadCompletedEvent first_load_completed;
  first_load_completed.uri = "https://example.com/analytics-first-complete.m3u8";
  first_load_completed.data_type = 2;
  first_load_completed.track_type = 3;
  player->SimulateLoadCompletedForTest(first_load_completed);
  LoadCompletedEvent second_load_completed;
  second_load_completed.uri = "https://example.com/analytics-final-complete.m3u8";
  second_load_completed.data_type = 4;
  second_load_completed.track_type = 1;
  player->SimulateLoadCompletedForTest(second_load_completed);
  int callback_count_before_remove = analytics_listener.load_completed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  LoadCompletedEvent ignored_load_completed;
  ignored_load_completed.uri = "https://example.com/analytics-ignored-complete.m3u8";
  ignored_load_completed.data_type = 9;
  ignored_load_completed.track_type = 9;
  player->SimulateLoadCompletedForTest(ignored_load_completed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" + std::to_string(analytics_listener.load_completed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.load_completed_callback_count == callback_count_before_remove ? 1 : 0);
  summary += ",uri=" + analytics_listener.load_completed_uri;
  summary += ",dataType=" + std::to_string(analytics_listener.load_completed_data_type);
  summary += ",trackType=" + std::to_string(analytics_listener.load_completed_track_type);
  return NewStringUtfChecked(env, summary, "nativeAnalyticsLoadCompletedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsAudioInputFormatChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AudioInputFormatChangedEvent first_audio_input_format_changed;
  first_audio_input_format_changed.sample_mime_type = "audio/first";
  first_audio_input_format_changed.codecs = "mp4a.40.2";
  first_audio_input_format_changed.channel_count = 2;
  first_audio_input_format_changed.sample_rate = 44100;
  player->SimulateAudioInputFormatChangedForTest(first_audio_input_format_changed);
  AudioInputFormatChangedEvent second_audio_input_format_changed;
  second_audio_input_format_changed.sample_mime_type = "audio/final";
  second_audio_input_format_changed.codecs = "ec-3";
  second_audio_input_format_changed.channel_count = 6;
  second_audio_input_format_changed.sample_rate = 48000;
  player->SimulateAudioInputFormatChangedForTest(second_audio_input_format_changed);
  int callback_count_before_remove =
      analytics_listener.audio_input_format_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AudioInputFormatChangedEvent ignored_audio_input_format_changed;
  ignored_audio_input_format_changed.sample_mime_type = "audio/ignored";
  ignored_audio_input_format_changed.codecs = "ignored";
  ignored_audio_input_format_changed.channel_count = 1;
  ignored_audio_input_format_changed.sample_rate = 1;
  player->SimulateAudioInputFormatChangedForTest(ignored_audio_input_format_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.audio_input_format_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.audio_input_format_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",sampleMimeType=" + analytics_listener.audio_input_format_sample_mime_type;
  summary += ",codecs=" + analytics_listener.audio_input_format_codecs;
  summary += ",channelCount=" +
      std::to_string(analytics_listener.audio_input_format_channel_count);
  summary += ",sampleRate=" +
      std::to_string(analytics_listener.audio_input_format_sample_rate);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsAudioInputFormatChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsAudioDecoderInitializedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AudioDecoderInitializedEvent first_audio_decoder_initialized;
  first_audio_decoder_initialized.decoder_name = "c2.android.aac.decoder";
  first_audio_decoder_initialized.initialized_timestamp_ms = 111;
  first_audio_decoder_initialized.initialization_duration_ms = 7;
  player->SimulateAudioDecoderInitializedForTest(first_audio_decoder_initialized);
  AudioDecoderInitializedEvent second_audio_decoder_initialized;
  second_audio_decoder_initialized.decoder_name = "c2.android.eac3.decoder";
  second_audio_decoder_initialized.initialized_timestamp_ms = 222;
  second_audio_decoder_initialized.initialization_duration_ms = 19;
  player->SimulateAudioDecoderInitializedForTest(second_audio_decoder_initialized);
  int callback_count_before_remove =
      analytics_listener.audio_decoder_initialized_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AudioDecoderInitializedEvent ignored_audio_decoder_initialized;
  ignored_audio_decoder_initialized.decoder_name = "ignored";
  ignored_audio_decoder_initialized.initialized_timestamp_ms = 1;
  ignored_audio_decoder_initialized.initialization_duration_ms = 1;
  player->SimulateAudioDecoderInitializedForTest(ignored_audio_decoder_initialized);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.audio_decoder_initialized_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.audio_decoder_initialized_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",decoderName=" + analytics_listener.audio_decoder_initialized_decoder_name;
  summary += ",initializedTimestampMs=" +
      std::to_string(analytics_listener.audio_decoder_initialized_timestamp_ms);
  summary += ",initializationDurationMs=" +
      std::to_string(analytics_listener.audio_decoder_initialized_duration_ms);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsAudioDecoderInitializedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsVideoDecoderInitializedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  VideoDecoderInitializedEvent first_video_decoder_initialized;
  first_video_decoder_initialized.decoder_name = "c2.android.avc.decoder";
  first_video_decoder_initialized.initialized_timestamp_ms = 333;
  first_video_decoder_initialized.initialization_duration_ms = 13;
  player->SimulateVideoDecoderInitializedForTest(first_video_decoder_initialized);
  VideoDecoderInitializedEvent second_video_decoder_initialized;
  second_video_decoder_initialized.decoder_name = "c2.android.hevc.decoder";
  second_video_decoder_initialized.initialized_timestamp_ms = 444;
  second_video_decoder_initialized.initialization_duration_ms = 29;
  player->SimulateVideoDecoderInitializedForTest(second_video_decoder_initialized);
  int callback_count_before_remove =
      analytics_listener.video_decoder_initialized_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  VideoDecoderInitializedEvent ignored_video_decoder_initialized;
  ignored_video_decoder_initialized.decoder_name = "ignored";
  ignored_video_decoder_initialized.initialized_timestamp_ms = 1;
  ignored_video_decoder_initialized.initialization_duration_ms = 1;
  player->SimulateVideoDecoderInitializedForTest(ignored_video_decoder_initialized);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.video_decoder_initialized_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.video_decoder_initialized_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",decoderName=" + analytics_listener.video_decoder_initialized_decoder_name;
  summary += ",initializedTimestampMs=" +
      std::to_string(analytics_listener.video_decoder_initialized_timestamp_ms);
  summary += ",initializationDurationMs=" +
      std::to_string(analytics_listener.video_decoder_initialized_duration_ms);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsVideoDecoderInitializedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsAudioDecoderReleasedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AudioDecoderReleasedEvent first_audio_decoder_released;
  first_audio_decoder_released.decoder_name = "c2.android.aac.decoder";
  player->SimulateAudioDecoderReleasedForTest(first_audio_decoder_released);
  AudioDecoderReleasedEvent second_audio_decoder_released;
  second_audio_decoder_released.decoder_name = "c2.android.eac3.decoder";
  player->SimulateAudioDecoderReleasedForTest(second_audio_decoder_released);
  int callback_count_before_remove = analytics_listener.audio_decoder_released_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AudioDecoderReleasedEvent ignored_audio_decoder_released;
  ignored_audio_decoder_released.decoder_name = "ignored";
  player->SimulateAudioDecoderReleasedForTest(ignored_audio_decoder_released);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.audio_decoder_released_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.audio_decoder_released_callback_count == callback_count_before_remove
          ? 1
          : 0);
  summary += ",decoderName=" + analytics_listener.audio_decoder_released_decoder_name;
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsAudioDecoderReleasedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsVideoDecoderReleasedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  VideoDecoderReleasedEvent first_video_decoder_released;
  first_video_decoder_released.decoder_name = "c2.android.avc.decoder";
  player->SimulateVideoDecoderReleasedForTest(first_video_decoder_released);
  VideoDecoderReleasedEvent second_video_decoder_released;
  second_video_decoder_released.decoder_name = "c2.android.hevc.decoder";
  player->SimulateVideoDecoderReleasedForTest(second_video_decoder_released);
  int callback_count_before_remove = analytics_listener.video_decoder_released_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  VideoDecoderReleasedEvent ignored_video_decoder_released;
  ignored_video_decoder_released.decoder_name = "ignored";
  player->SimulateVideoDecoderReleasedForTest(ignored_video_decoder_released);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.video_decoder_released_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.video_decoder_released_callback_count == callback_count_before_remove
          ? 1
          : 0);
  summary += ",decoderName=" + analytics_listener.video_decoder_released_decoder_name;
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsVideoDecoderReleasedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsRenderedFirstFrameSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsRenderedFirstFrameEvent first_rendered_first_frame;
  first_rendered_first_frame.render_time_ms = 123;
  player->SimulateAnalyticsRenderedFirstFrameForTest(first_rendered_first_frame);
  AnalyticsRenderedFirstFrameEvent second_rendered_first_frame;
  second_rendered_first_frame.render_time_ms = 456;
  player->SimulateAnalyticsRenderedFirstFrameForTest(second_rendered_first_frame);
  int callback_count_before_remove =
      analytics_listener.analytics_rendered_first_frame_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsRenderedFirstFrameEvent ignored_rendered_first_frame;
  ignored_rendered_first_frame.render_time_ms = 1;
  player->SimulateAnalyticsRenderedFirstFrameForTest(ignored_rendered_first_frame);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_rendered_first_frame_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_rendered_first_frame_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",renderTimeMs=" +
      std::to_string(analytics_listener.analytics_rendered_first_frame_render_time_ms);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsRenderedFirstFrameSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsVideoSizeChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsVideoSizeChangedEvent first_video_size;
  first_video_size.width = 1280;
  first_video_size.height = 720;
  first_video_size.pixel_width_height_ratio = 1.0f;
  player->SimulateAnalyticsVideoSizeChangedForTest(first_video_size);
  AnalyticsVideoSizeChangedEvent second_video_size;
  second_video_size.width = 1920;
  second_video_size.height = 1080;
  second_video_size.pixel_width_height_ratio = 1.25f;
  player->SimulateAnalyticsVideoSizeChangedForTest(second_video_size);
  int callback_count_before_remove = analytics_listener.analytics_video_size_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsVideoSizeChangedEvent ignored_video_size;
  ignored_video_size.width = 1;
  ignored_video_size.height = 1;
  ignored_video_size.pixel_width_height_ratio = 1.0f;
  player->SimulateAnalyticsVideoSizeChangedForTest(ignored_video_size);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_video_size_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_video_size_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",width=" + std::to_string(analytics_listener.analytics_video_size_width);
  summary += ",height=" + std::to_string(analytics_listener.analytics_video_size_height);
  summary += ",pixelWidthHeightRatio=" +
      std::to_string(analytics_listener.analytics_video_size_pixel_width_height_ratio);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsVideoSizeChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsAudioPositionAdvancingSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AudioPositionAdvancingEvent first_audio_position_advancing;
  first_audio_position_advancing.playout_start_system_time_ms = 1111;
  player->SimulateAudioPositionAdvancingForTest(first_audio_position_advancing);
  AudioPositionAdvancingEvent second_audio_position_advancing;
  second_audio_position_advancing.playout_start_system_time_ms = 2222;
  player->SimulateAudioPositionAdvancingForTest(second_audio_position_advancing);
  int callback_count_before_remove =
      analytics_listener.analytics_audio_position_advancing_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AudioPositionAdvancingEvent ignored_audio_position_advancing;
  ignored_audio_position_advancing.playout_start_system_time_ms = 1;
  player->SimulateAudioPositionAdvancingForTest(ignored_audio_position_advancing);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_audio_position_advancing_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_audio_position_advancing_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",playoutStartSystemTimeMs=" +
      std::to_string(
          analytics_listener.analytics_audio_position_advancing_playout_start_system_time_ms);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsAudioPositionAdvancingSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsVideoFrameProcessingOffsetSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  VideoFrameProcessingOffsetEvent first_video_frame_processing_offset;
  first_video_frame_processing_offset.total_processing_offset_us = 12345;
  first_video_frame_processing_offset.frame_count = 4;
  player->SimulateVideoFrameProcessingOffsetForTest(first_video_frame_processing_offset);
  VideoFrameProcessingOffsetEvent second_video_frame_processing_offset;
  second_video_frame_processing_offset.total_processing_offset_us = 67890;
  second_video_frame_processing_offset.frame_count = 8;
  player->SimulateVideoFrameProcessingOffsetForTest(second_video_frame_processing_offset);
  int callback_count_before_remove =
      analytics_listener.analytics_video_frame_processing_offset_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  VideoFrameProcessingOffsetEvent ignored_video_frame_processing_offset;
  ignored_video_frame_processing_offset.total_processing_offset_us = 1;
  ignored_video_frame_processing_offset.frame_count = 1;
  player->SimulateVideoFrameProcessingOffsetForTest(ignored_video_frame_processing_offset);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(
          analytics_listener.analytics_video_frame_processing_offset_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_video_frame_processing_offset_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",totalProcessingOffsetUs=" +
      std::to_string(
          analytics_listener.analytics_video_frame_processing_offset_total_processing_offset_us);
  summary += ",frameCount=" +
      std::to_string(analytics_listener.analytics_video_frame_processing_offset_frame_count);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsVideoFrameProcessingOffsetSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsVolumeChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  VolumeChangedEvent first_volume_changed;
  first_volume_changed.volume = 0.25f;
  player->SimulateVolumeChangedForTest(first_volume_changed);
  VolumeChangedEvent second_volume_changed;
  second_volume_changed.volume = 0.75f;
  player->SimulateVolumeChangedForTest(second_volume_changed);
  int callback_count_before_remove = analytics_listener.analytics_volume_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  VolumeChangedEvent ignored_volume_changed;
  ignored_volume_changed.volume = 0.1f;
  player->SimulateVolumeChangedForTest(ignored_volume_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_volume_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_volume_changed_callback_count == callback_count_before_remove
          ? 1
          : 0);
  summary += ",volume=" + std::to_string(analytics_listener.analytics_volume_changed_volume);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsVolumeChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsAudioSessionIdChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  class AudioSessionIdCapturingListener : public PlayerListener {
   public:
    void OnPlaybackStateChanged(const PlaybackSnapshot&) override {}
    void OnPlayWhenReadyChanged(const PlaybackSnapshot&, int) override {}
    void OnIsPlayingChanged(const PlaybackSnapshot&) override {}
    void OnMediaItemTransition(const PlaybackSnapshot&, int) override {}
    void OnPlayerError(const PlaybackSnapshot&) override {}

    void OnAudioSessionIdChanged(
        const PlaybackSnapshot&,
        const AudioSessionIdChangedEvent& audio_session_id_changed) override {
      int audio_session_id = audio_session_id_changed.audio_session_id;
      if (audio_session_id != 700001 &&
          audio_session_id != 700042 &&
          audio_session_id != 700009) {
        return;
      }
      ++callback_count;
      last_audio_session_id = audio_session_id;
    }

    int callback_count = 0;
    int last_audio_session_id = 0;
  };

  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  AudioSessionIdCapturingListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AudioSessionIdChangedEvent first_audio_session_id_changed;
  first_audio_session_id_changed.audio_session_id = 700001;
  player->SimulateAudioSessionIdChangedForTest(first_audio_session_id_changed);
  AudioSessionIdChangedEvent second_audio_session_id_changed;
  second_audio_session_id_changed.audio_session_id = 700042;
  player->SimulateAudioSessionIdChangedForTest(second_audio_session_id_changed);
  int callback_count_before_remove = analytics_listener.callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AudioSessionIdChangedEvent ignored_audio_session_id_changed;
  ignored_audio_session_id_changed.audio_session_id = 700009;
  player->SimulateAudioSessionIdChangedForTest(ignored_audio_session_id_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" + std::to_string(analytics_listener.callback_count);
  summary += ",callbackStopped=" +
      std::to_string(
          analytics_listener.callback_count == callback_count_before_remove ? 1 : 0);
  summary += ",audioSessionId=" +
      std::to_string(analytics_listener.last_audio_session_id);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsAudioSessionIdChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsAudioAttributesChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AudioAttributesDescriptor first_attributes;
  first_attributes.content_type = 1;
  first_attributes.usage = 2;
  first_attributes.flags = 3;
  first_attributes.allowed_capture_policy = 1;
  first_attributes.spatialization_behavior = 0;
  player->SimulateAnalyticsAudioAttributesChangedForTest(first_attributes);
  AudioAttributesDescriptor second_attributes;
  second_attributes.content_type = 4;
  second_attributes.usage = 5;
  second_attributes.flags = 6;
  second_attributes.allowed_capture_policy = 2;
  second_attributes.spatialization_behavior = 1;
  player->SimulateAnalyticsAudioAttributesChangedForTest(second_attributes);
  int callback_count_before_remove =
      analytics_listener.analytics_audio_attributes_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AudioAttributesDescriptor ignored_attributes;
  ignored_attributes.content_type = 7;
  ignored_attributes.usage = 8;
  ignored_attributes.flags = 9;
  ignored_attributes.allowed_capture_policy = 3;
  ignored_attributes.spatialization_behavior = 2;
  player->SimulateAnalyticsAudioAttributesChangedForTest(ignored_attributes);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(
          analytics_listener.analytics_audio_attributes_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_audio_attributes_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",contentType=" +
      std::to_string(analytics_listener.analytics_audio_attributes_content_type);
  summary += ",usage=" + std::to_string(analytics_listener.analytics_audio_attributes_usage);
  summary += ",flags=" + std::to_string(analytics_listener.analytics_audio_attributes_flags);
  summary += ",allowedCapturePolicy=" +
      std::to_string(analytics_listener.analytics_audio_attributes_allowed_capture_policy);
  summary += ",spatializationBehavior=" +
      std::to_string(analytics_listener.analytics_audio_attributes_spatialization_behavior);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsAudioAttributesChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsStage4RemainingCallbacksSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  auto stage4_callback_count = [&]() {
    return analytics_listener.analytics_player_state_changed_callback_count +
        analytics_listener.analytics_loading_changed_callback_count +
        analytics_listener.analytics_track_selection_changed_callback_count +
        analytics_listener.analytics_load_canceled_callback_count +
        analytics_listener.analytics_downstream_format_changed_callback_count +
        analytics_listener.analytics_upstream_discarded_callback_count +
        analytics_listener.analytics_audio_enabled_callback_count +
        analytics_listener.analytics_audio_disabled_callback_count +
        analytics_listener.analytics_audio_sink_error_callback_count +
        analytics_listener.analytics_audio_codec_error_callback_count +
        analytics_listener.analytics_audio_track_initialized_callback_count +
        analytics_listener.analytics_audio_track_released_callback_count +
        analytics_listener.analytics_video_enabled_callback_count +
        analytics_listener.analytics_video_disabled_callback_count +
        analytics_listener.analytics_video_codec_error_callback_count +
        analytics_listener.analytics_surface_size_changed_callback_count +
        analytics_listener.analytics_drm_session_acquired_callback_count +
        analytics_listener.analytics_drm_keys_loaded_callback_count +
        analytics_listener.analytics_drm_session_manager_error_callback_count +
        analytics_listener.analytics_drm_keys_restored_callback_count +
        analytics_listener.analytics_drm_keys_removed_callback_count +
        analytics_listener.analytics_drm_session_released_callback_count +
        analytics_listener.analytics_renderer_ready_changed_callback_count +
        analytics_listener.analytics_dropped_seeks_while_scrubbing_callback_count +
        analytics_listener.analytics_player_released_callback_count;
  };
  player->SimulateAnalyticsStage4RemainingEventsForTest();
  int callback_count_before_remove = stage4_callback_count();
  player->RemoveAnalyticsListener(&analytics_listener);
  player->SimulateAnalyticsStage4RemainingEventsForTest();
  int callback_count_after_remove = stage4_callback_count();
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" + std::to_string(callback_count_after_remove);
  summary += ",callbackStopped=" +
      std::to_string(callback_count_after_remove == callback_count_before_remove ? 1 : 0);
  summary += ",playerStatePlayWhenReady=" +
      std::to_string(analytics_listener.analytics_player_state_changed_play_when_ready ? 1 : 0);
  summary += ",playerState=" +
      std::to_string(analytics_listener.analytics_player_state_changed_playback_state);
  summary += ",loading=" +
      std::to_string(analytics_listener.analytics_loading_changed_is_loading ? 1 : 0);
  summary += ",trackTextLanguage=" +
      analytics_listener.analytics_track_selection_changed_preferred_text_language;
  summary += ",trackDisableText=" +
      std::to_string(analytics_listener.analytics_track_selection_changed_disable_text ? 1 : 0);
  summary += ",loadCanceledUri=" + analytics_listener.analytics_load_canceled_uri;
  summary += ",loadCanceledSampleMimeType=" +
      analytics_listener.analytics_load_canceled_sample_mime_type;
  summary += ",downstreamSampleMimeType=" +
      analytics_listener.analytics_downstream_format_changed_sample_mime_type;
  summary += ",upstreamSampleMimeType=" +
      analytics_listener.analytics_upstream_discarded_sample_mime_type;
  summary += ",audioEnabledInitCount=" +
      std::to_string(analytics_listener.analytics_audio_enabled_decoder_init_count);
  summary += ",audioDisabledReleaseCount=" +
      std::to_string(analytics_listener.analytics_audio_disabled_decoder_release_count);
  summary += ",audioSinkError=" + analytics_listener.analytics_audio_sink_error_message;
  summary += ",audioCodecError=" + analytics_listener.analytics_audio_codec_error_message;
  summary += ",audioTrackInitSampleRate=" +
      std::to_string(analytics_listener.analytics_audio_track_initialized_sample_rate);
  summary += ",audioTrackReleasedOffload=" +
      std::to_string(analytics_listener.analytics_audio_track_released_offload ? 1 : 0);
  summary += ",videoEnabledProcessingOffsetUs=" +
      std::to_string(analytics_listener.analytics_video_enabled_total_processing_offset_us);
  summary += ",videoDisabledProcessingOffsetCount=" +
      std::to_string(analytics_listener.analytics_video_disabled_processing_offset_count);
  summary += ",videoCodecError=" + analytics_listener.analytics_video_codec_error_message;
  summary += ",surfaceWidth=" +
      std::to_string(analytics_listener.analytics_surface_size_changed_width);
  summary += ",surfaceHeight=" +
      std::to_string(analytics_listener.analytics_surface_size_changed_height);
  summary += ",drmAcquiredHasState=" +
      std::to_string(analytics_listener.analytics_drm_session_acquired_has_state ? 1 : 0);
  summary += ",drmAcquiredState=" +
      std::to_string(analytics_listener.analytics_drm_session_acquired_state);
  summary += ",drmKeysLoadedHasInfo=" +
      std::to_string(
          analytics_listener.analytics_drm_keys_loaded_has_key_request_info ? 1 : 0);
  summary += ",drmKeysLoadedLoadInfoCount=" +
      std::to_string(analytics_listener.analytics_drm_keys_loaded_load_info_count);
  summary += ",drmKeysLoadedSchemeDataCount=" +
      std::to_string(analytics_listener.analytics_drm_keys_loaded_scheme_data_count);
  summary += ",drmError=" +
      analytics_listener.analytics_drm_session_manager_error_message;
  summary += ",drmRestoredCb=" +
      std::to_string(analytics_listener.analytics_drm_keys_restored_callback_count);
  summary += ",drmRemovedCb=" +
      std::to_string(analytics_listener.analytics_drm_keys_removed_callback_count);
  summary += ",drmReleasedCb=" +
      std::to_string(analytics_listener.analytics_drm_session_released_callback_count);
  summary += ",rendererIndex=" +
      std::to_string(analytics_listener.analytics_renderer_ready_changed_renderer_index);
  summary += ",rendererTrackType=" +
      std::to_string(analytics_listener.analytics_renderer_ready_changed_track_type);
  summary += ",rendererReady=" +
      std::to_string(analytics_listener.analytics_renderer_ready_changed_is_ready ? 1 : 0);
  summary += ",droppedSeeks=" +
      std::to_string(
          analytics_listener.analytics_dropped_seeks_while_scrubbing_dropped_seeks);
  summary += ",playerReleasedCb=" +
      std::to_string(analytics_listener.analytics_player_released_callback_count);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsStage4RemainingCallbacksSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsSkipSilenceEnabledChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsSkipSilenceEnabledChangedEvent first_skip_silence_enabled_changed;
  first_skip_silence_enabled_changed.skip_silence_enabled = false;
  player->SimulateAnalyticsSkipSilenceEnabledChangedForTest(
      first_skip_silence_enabled_changed);
  AnalyticsSkipSilenceEnabledChangedEvent second_skip_silence_enabled_changed;
  second_skip_silence_enabled_changed.skip_silence_enabled = true;
  player->SimulateAnalyticsSkipSilenceEnabledChangedForTest(
      second_skip_silence_enabled_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_skip_silence_enabled_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsSkipSilenceEnabledChangedEvent ignored_skip_silence_enabled_changed;
  ignored_skip_silence_enabled_changed.skip_silence_enabled = false;
  player->SimulateAnalyticsSkipSilenceEnabledChangedForTest(
      ignored_skip_silence_enabled_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(
          analytics_listener.analytics_skip_silence_enabled_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_skip_silence_enabled_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",skipSilenceEnabled=" + std::to_string(
      analytics_listener.analytics_skip_silence_enabled_changed_skip_silence_enabled ? 1 : 0);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsSkipSilenceEnabledChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsDeviceVolumeChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsDeviceVolumeChangedEvent first_device_volume_changed;
  first_device_volume_changed.volume = 3;
  first_device_volume_changed.muted = true;
  player->SimulateAnalyticsDeviceVolumeChangedForTest(first_device_volume_changed);
  AnalyticsDeviceVolumeChangedEvent second_device_volume_changed;
  second_device_volume_changed.volume = 7;
  second_device_volume_changed.muted = false;
  player->SimulateAnalyticsDeviceVolumeChangedForTest(second_device_volume_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_device_volume_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsDeviceVolumeChangedEvent ignored_device_volume_changed;
  ignored_device_volume_changed.volume = 1;
  ignored_device_volume_changed.muted = true;
  player->SimulateAnalyticsDeviceVolumeChangedForTest(ignored_device_volume_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_device_volume_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_device_volume_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",volume=" +
      std::to_string(analytics_listener.analytics_device_volume_changed_volume);
  summary += ",muted=" +
      std::to_string(analytics_listener.analytics_device_volume_changed_muted ? 1 : 0);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsDeviceVolumeChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsPlaybackStateChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsPlaybackStateChangedEvent first_playback_state_changed;
  first_playback_state_changed.playback_state =
      static_cast<int>(PlaybackState::kBuffering);
  player->SimulateAnalyticsPlaybackStateChangedForTest(first_playback_state_changed);
  AnalyticsPlaybackStateChangedEvent second_playback_state_changed;
  second_playback_state_changed.playback_state =
      static_cast<int>(PlaybackState::kReady);
  player->SimulateAnalyticsPlaybackStateChangedForTest(second_playback_state_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_playback_state_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsPlaybackStateChangedEvent ignored_playback_state_changed;
  ignored_playback_state_changed.playback_state =
      static_cast<int>(PlaybackState::kEnded);
  player->SimulateAnalyticsPlaybackStateChangedForTest(ignored_playback_state_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_playback_state_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_playback_state_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",playbackState=" +
      std::to_string(analytics_listener.analytics_playback_state_changed_playback_state);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsPlaybackStateChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsIsPlayingChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsIsPlayingChangedEvent first_is_playing_changed;
  first_is_playing_changed.is_playing = false;
  player->SimulateAnalyticsIsPlayingChangedForTest(first_is_playing_changed);
  AnalyticsIsPlayingChangedEvent second_is_playing_changed;
  second_is_playing_changed.is_playing = true;
  player->SimulateAnalyticsIsPlayingChangedForTest(second_is_playing_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_is_playing_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsIsPlayingChangedEvent ignored_is_playing_changed;
  ignored_is_playing_changed.is_playing = false;
  player->SimulateAnalyticsIsPlayingChangedForTest(ignored_is_playing_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_is_playing_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_is_playing_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",isPlaying=" +
      std::to_string(analytics_listener.analytics_is_playing_changed_is_playing ? 1 : 0);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsIsPlayingChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsPlayWhenReadyChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsPlayWhenReadyChangedEvent first_play_when_ready_changed;
  first_play_when_ready_changed.play_when_ready = false;
  first_play_when_ready_changed.reason = 1;
  player->SimulateAnalyticsPlayWhenReadyChangedForTest(first_play_when_ready_changed);
  AnalyticsPlayWhenReadyChangedEvent second_play_when_ready_changed;
  second_play_when_ready_changed.play_when_ready = true;
  second_play_when_ready_changed.reason = 2;
  player->SimulateAnalyticsPlayWhenReadyChangedForTest(second_play_when_ready_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_play_when_ready_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsPlayWhenReadyChangedEvent ignored_play_when_ready_changed;
  ignored_play_when_ready_changed.play_when_ready = false;
  ignored_play_when_ready_changed.reason = 3;
  player->SimulateAnalyticsPlayWhenReadyChangedForTest(ignored_play_when_ready_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_play_when_ready_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_play_when_ready_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",playWhenReady=" +
      std::to_string(analytics_listener.analytics_play_when_ready_changed_play_when_ready ? 1 : 0);
  summary += ",reason=" +
      std::to_string(analytics_listener.analytics_play_when_ready_changed_reason);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsPlayWhenReadyChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsPlaybackSuppressionReasonChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsPlaybackSuppressionReasonChangedEvent first_suppression_reason_changed;
  first_suppression_reason_changed.playback_suppression_reason = 0;
  player->SimulateAnalyticsPlaybackSuppressionReasonChangedForTest(
      first_suppression_reason_changed);
  AnalyticsPlaybackSuppressionReasonChangedEvent second_suppression_reason_changed;
  second_suppression_reason_changed.playback_suppression_reason = 1;
  player->SimulateAnalyticsPlaybackSuppressionReasonChangedForTest(
      second_suppression_reason_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_playback_suppression_reason_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsPlaybackSuppressionReasonChangedEvent ignored_suppression_reason_changed;
  ignored_suppression_reason_changed.playback_suppression_reason = 2;
  player->SimulateAnalyticsPlaybackSuppressionReasonChangedForTest(
      ignored_suppression_reason_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" + std::to_string(
      analytics_listener.analytics_playback_suppression_reason_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_playback_suppression_reason_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",playbackSuppressionReason=" + std::to_string(
      analytics_listener.analytics_playback_suppression_reason_changed_reason);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsPlaybackSuppressionReasonChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsIsLoadingChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsIsLoadingChangedEvent first_is_loading_changed;
  first_is_loading_changed.is_loading = false;
  player->SimulateAnalyticsIsLoadingChangedForTest(first_is_loading_changed);
  AnalyticsIsLoadingChangedEvent second_is_loading_changed;
  second_is_loading_changed.is_loading = true;
  player->SimulateAnalyticsIsLoadingChangedForTest(second_is_loading_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_is_loading_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsIsLoadingChangedEvent ignored_is_loading_changed;
  ignored_is_loading_changed.is_loading = false;
  player->SimulateAnalyticsIsLoadingChangedForTest(ignored_is_loading_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_is_loading_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_is_loading_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",isLoading=" +
      std::to_string(analytics_listener.analytics_is_loading_changed_is_loading ? 1 : 0);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsIsLoadingChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsRepeatModeChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsRepeatModeChangedEvent first_repeat_mode_changed;
  first_repeat_mode_changed.repeat_mode = 0;
  player->SimulateAnalyticsRepeatModeChangedForTest(first_repeat_mode_changed);
  AnalyticsRepeatModeChangedEvent second_repeat_mode_changed;
  second_repeat_mode_changed.repeat_mode = 2;
  player->SimulateAnalyticsRepeatModeChangedForTest(second_repeat_mode_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_repeat_mode_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsRepeatModeChangedEvent ignored_repeat_mode_changed;
  ignored_repeat_mode_changed.repeat_mode = 1;
  player->SimulateAnalyticsRepeatModeChangedForTest(ignored_repeat_mode_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_repeat_mode_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_repeat_mode_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",repeatMode=" +
      std::to_string(analytics_listener.analytics_repeat_mode_changed_repeat_mode);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsRepeatModeChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsShuffleModeChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsShuffleModeChangedEvent first_shuffle_mode_changed;
  first_shuffle_mode_changed.shuffle_mode_enabled = false;
  player->SimulateAnalyticsShuffleModeChangedForTest(first_shuffle_mode_changed);
  AnalyticsShuffleModeChangedEvent second_shuffle_mode_changed;
  second_shuffle_mode_changed.shuffle_mode_enabled = true;
  player->SimulateAnalyticsShuffleModeChangedForTest(second_shuffle_mode_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_shuffle_mode_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsShuffleModeChangedEvent ignored_shuffle_mode_changed;
  ignored_shuffle_mode_changed.shuffle_mode_enabled = false;
  player->SimulateAnalyticsShuffleModeChangedForTest(ignored_shuffle_mode_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_shuffle_mode_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_shuffle_mode_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",shuffleModeEnabled=" + std::to_string(
      analytics_listener.analytics_shuffle_mode_changed_shuffle_mode_enabled ? 1 : 0);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsShuffleModeChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsPlaybackParametersChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsPlaybackParametersChangedEvent first_playback_parameters_changed;
  first_playback_parameters_changed.speed = 1.0f;
  first_playback_parameters_changed.pitch = 1.0f;
  player->SimulateAnalyticsPlaybackParametersChangedForTest(first_playback_parameters_changed);
  AnalyticsPlaybackParametersChangedEvent second_playback_parameters_changed;
  second_playback_parameters_changed.speed = 1.5f;
  second_playback_parameters_changed.pitch = 0.75f;
  player->SimulateAnalyticsPlaybackParametersChangedForTest(second_playback_parameters_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_playback_parameters_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsPlaybackParametersChangedEvent ignored_playback_parameters_changed;
  ignored_playback_parameters_changed.speed = 2.0f;
  ignored_playback_parameters_changed.pitch = 0.5f;
  player->SimulateAnalyticsPlaybackParametersChangedForTest(ignored_playback_parameters_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" + std::to_string(
      analytics_listener.analytics_playback_parameters_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_playback_parameters_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",speed=" +
      std::to_string(analytics_listener.analytics_playback_parameters_changed_speed);
  summary += ",pitch=" +
      std::to_string(analytics_listener.analytics_playback_parameters_changed_pitch);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsPlaybackParametersChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsAvailableCommandsChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsAvailableCommandsChangedEvent first_available_commands_changed;
  first_available_commands_changed.commands = {1, 2};
  player->SimulateAnalyticsAvailableCommandsChangedForTest(first_available_commands_changed);
  AnalyticsAvailableCommandsChangedEvent second_available_commands_changed;
  second_available_commands_changed.commands = {3, 5, 8};
  player->SimulateAnalyticsAvailableCommandsChangedForTest(second_available_commands_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_available_commands_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsAvailableCommandsChangedEvent ignored_available_commands_changed;
  ignored_available_commands_changed.commands = {9};
  player->SimulateAnalyticsAvailableCommandsChangedForTest(ignored_available_commands_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_available_commands_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_available_commands_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",commandCount=" +
      std::to_string(analytics_listener.analytics_available_commands_count);
  summary += ",firstCommand=" +
      std::to_string(analytics_listener.analytics_available_commands_first_command);
  summary += ",contains8=" +
      std::to_string(analytics_listener.analytics_available_commands_contains_8 ? 1 : 0);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsAvailableCommandsChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsEventsSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsEventsEvent first_analytics_events;
  first_analytics_events.event_codes = {100, 101, 9009};
  player->SimulateAnalyticsEventsForTest(first_analytics_events);
  AnalyticsEventsEvent second_analytics_events;
  second_analytics_events.event_codes = {7, 8, 9009};
  player->SimulateAnalyticsEventsForTest(second_analytics_events);
  int callback_count_before_remove = analytics_listener.analytics_events_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsEventsEvent ignored_analytics_events;
  ignored_analytics_events.event_codes = {1};
  player->SimulateAnalyticsEventsForTest(ignored_analytics_events);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_events_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_events_callback_count == callback_count_before_remove ? 1 : 0);
  summary += ",eventCount=" + std::to_string(analytics_listener.analytics_events_count);
  summary += ",firstEvent=" + std::to_string(analytics_listener.analytics_events_first_event);
  summary += ",contains9009=" +
      std::to_string(analytics_listener.analytics_events_contains_9009 ? 1 : 0);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsEventsSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsSeekBackIncrementChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsSeekBackIncrementChangedEvent first_seek_back_increment_changed;
  first_seek_back_increment_changed.seek_back_increment_ms = 5000;
  player->SimulateAnalyticsSeekBackIncrementChangedForTest(first_seek_back_increment_changed);
  AnalyticsSeekBackIncrementChangedEvent second_seek_back_increment_changed;
  second_seek_back_increment_changed.seek_back_increment_ms = 15000;
  player->SimulateAnalyticsSeekBackIncrementChangedForTest(second_seek_back_increment_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_seek_back_increment_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsSeekBackIncrementChangedEvent ignored_seek_back_increment_changed;
  ignored_seek_back_increment_changed.seek_back_increment_ms = 1;
  player->SimulateAnalyticsSeekBackIncrementChangedForTest(ignored_seek_back_increment_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" + std::to_string(
      analytics_listener.analytics_seek_back_increment_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_seek_back_increment_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",seekBackIncrementMs=" +
      std::to_string(analytics_listener.analytics_seek_back_increment_changed_ms);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsSeekBackIncrementChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsSeekForwardIncrementChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsSeekForwardIncrementChangedEvent first_seek_forward_increment_changed;
  first_seek_forward_increment_changed.seek_forward_increment_ms = 7000;
  player->SimulateAnalyticsSeekForwardIncrementChangedForTest(
      first_seek_forward_increment_changed);
  AnalyticsSeekForwardIncrementChangedEvent second_seek_forward_increment_changed;
  second_seek_forward_increment_changed.seek_forward_increment_ms = 25000;
  player->SimulateAnalyticsSeekForwardIncrementChangedForTest(
      second_seek_forward_increment_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_seek_forward_increment_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsSeekForwardIncrementChangedEvent ignored_seek_forward_increment_changed;
  ignored_seek_forward_increment_changed.seek_forward_increment_ms = 1;
  player->SimulateAnalyticsSeekForwardIncrementChangedForTest(
      ignored_seek_forward_increment_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" + std::to_string(
      analytics_listener.analytics_seek_forward_increment_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_seek_forward_increment_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",seekForwardIncrementMs=" +
      std::to_string(analytics_listener.analytics_seek_forward_increment_changed_ms);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsSeekForwardIncrementChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsMaxSeekToPreviousPositionChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsMaxSeekToPreviousPositionChangedEvent first_max_seek_to_previous_position_changed;
  first_max_seek_to_previous_position_changed.max_seek_to_previous_position_ms = 3000;
  player->SimulateAnalyticsMaxSeekToPreviousPositionChangedForTest(
      first_max_seek_to_previous_position_changed);
  AnalyticsMaxSeekToPreviousPositionChangedEvent second_max_seek_to_previous_position_changed;
  second_max_seek_to_previous_position_changed.max_seek_to_previous_position_ms = 12000;
  player->SimulateAnalyticsMaxSeekToPreviousPositionChangedForTest(
      second_max_seek_to_previous_position_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_max_seek_to_previous_position_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsMaxSeekToPreviousPositionChangedEvent ignored_max_seek_to_previous_position_changed;
  ignored_max_seek_to_previous_position_changed.max_seek_to_previous_position_ms = 1;
  player->SimulateAnalyticsMaxSeekToPreviousPositionChangedForTest(
      ignored_max_seek_to_previous_position_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" + std::to_string(
      analytics_listener.analytics_max_seek_to_previous_position_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_max_seek_to_previous_position_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",maxSeekToPreviousPositionMs=" +
      std::to_string(analytics_listener.analytics_max_seek_to_previous_position_changed_ms);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsMaxSeekToPreviousPositionChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsTimelineChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsTimelineChangedEvent first_timeline_changed;
  first_timeline_changed.reason = 1;
  player->SimulateAnalyticsTimelineChangedForTest(first_timeline_changed);
  AnalyticsTimelineChangedEvent second_timeline_changed;
  second_timeline_changed.reason = 2;
  player->SimulateAnalyticsTimelineChangedForTest(second_timeline_changed);
  int callback_count_before_remove =
      analytics_listener.analytics_timeline_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsTimelineChangedEvent ignored_timeline_changed;
  ignored_timeline_changed.reason = 9;
  player->SimulateAnalyticsTimelineChangedForTest(ignored_timeline_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_timeline_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_timeline_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",reason=" + std::to_string(analytics_listener.analytics_timeline_changed_reason);
  return NewStringUtfChecked(env, summary, "nativeAnalyticsTimelineChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsPositionDiscontinuitySmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsPositionDiscontinuityEvent first_position_discontinuity;
  first_position_discontinuity.reason = 3;
  player->SimulateAnalyticsPositionDiscontinuityForTest(first_position_discontinuity);
  AnalyticsPositionDiscontinuityEvent second_position_discontinuity;
  second_position_discontinuity.reason = 5;
  player->SimulateAnalyticsPositionDiscontinuityForTest(second_position_discontinuity);
  int callback_count_before_remove =
      analytics_listener.analytics_position_discontinuity_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsPositionDiscontinuityEvent ignored_position_discontinuity;
  ignored_position_discontinuity.reason = 8;
  player->SimulateAnalyticsPositionDiscontinuityForTest(ignored_position_discontinuity);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_position_discontinuity_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_position_discontinuity_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",reason=" +
      std::to_string(analytics_listener.analytics_position_discontinuity_reason);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsPositionDiscontinuitySmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsSeekStartedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsSeekStartedEvent first_seek_started;
  player->SimulateAnalyticsSeekStartedForTest(first_seek_started);
  AnalyticsSeekStartedEvent second_seek_started;
  player->SimulateAnalyticsSeekStartedForTest(second_seek_started);
  int callback_count_before_remove = analytics_listener.analytics_seek_started_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsSeekStartedEvent ignored_seek_started;
  player->SimulateAnalyticsSeekStartedForTest(ignored_seek_started);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_seek_started_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_seek_started_callback_count == callback_count_before_remove
          ? 1
          : 0);
  summary += ",started=" +
      std::to_string(analytics_listener.analytics_seek_started_started ? 1 : 0);
  return NewStringUtfChecked(env, summary, "nativeAnalyticsSeekStartedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsPlayerErrorSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  PlayerError first_error;
  first_error.error_code = 1001;
  first_error.message = "analytics-first-error";
  player->SimulateAnalyticsPlayerErrorForTest(first_error);
  PlayerError second_error;
  second_error.error_code = 2002;
  second_error.message = "analytics-final-error";
  player->SimulateAnalyticsPlayerErrorForTest(second_error);
  int callback_count_before_remove = analytics_listener.analytics_player_error_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  PlayerError ignored_error;
  ignored_error.error_code = 9;
  ignored_error.message = "ignored";
  player->SimulateAnalyticsPlayerErrorForTest(ignored_error);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_player_error_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_player_error_callback_count == callback_count_before_remove
          ? 1
          : 0);
  summary += ",errorCode=" + std::to_string(analytics_listener.analytics_player_error_code);
  summary += ",message=" + analytics_listener.analytics_player_error_message;
  return NewStringUtfChecked(env, summary, "nativeAnalyticsPlayerErrorSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsPlayerErrorChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  PlayerError first_error;
  first_error.error_code = 3003;
  first_error.message = "analytics-first-changed";
  player->SimulateAnalyticsPlayerErrorChangedForTest(first_error);
  PlayerError second_error;
  second_error.error_code = 4004;
  second_error.message = "analytics-final-changed";
  player->SimulateAnalyticsPlayerErrorChangedForTest(second_error);
  int callback_count_before_remove =
      analytics_listener.analytics_player_error_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  PlayerError ignored_error;
  ignored_error.error_code = 7;
  ignored_error.message = "ignored";
  player->SimulateAnalyticsPlayerErrorChangedForTest(ignored_error);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_player_error_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_player_error_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",errorCode=" +
      std::to_string(analytics_listener.analytics_player_error_changed_code);
  summary += ",message=" + analytics_listener.analytics_player_error_changed_message;
  return NewStringUtfChecked(env, summary, "nativeAnalyticsPlayerErrorChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsTracksChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  TracksSnapshot first_tracks;
  TrackGroupSnapshot first_group;
  first_group.id = "audio-only";
  first_group.group_token = "generated-opaque-object-token-analytics-audio-group-first";
  first_group.type = 1;
  TrackInfo first_track;
  first_track.id = "audio-track";
  first_track.mime_type = "audio/mp4a-latm";
  first_group.tracks = {first_track};
  first_tracks.groups = {first_group};
  first_tracks.contains_audio = true;
  player->SimulateAnalyticsTracksChangedForTest(first_tracks);
  TracksSnapshot second_tracks;
  TrackGroupSnapshot video_group;
  video_group.id = "video-main";
  video_group.group_token = "generated-opaque-object-token-analytics-video-group-final";
  video_group.type = 2;
  video_group.selected = true;
  TrackInfo video_track;
  video_track.id = "video-hd";
  video_track.mime_type = "video/avc";
  video_track.selected = true;
  video_group.tracks = {video_track};
  TrackGroupSnapshot audio_group;
  audio_group.id = "audio-main";
  audio_group.group_token = "generated-opaque-object-token-analytics-audio-group-final";
  audio_group.type = 1;
  TrackInfo audio_track;
  audio_track.id = "audio-en";
  audio_track.mime_type = "audio/mp4a-latm";
  audio_group.tracks = {audio_track};
  second_tracks.groups = {video_group, audio_group};
  second_tracks.contains_audio = true;
  second_tracks.contains_video = true;
  second_tracks.video_selected = true;
  player->SimulateAnalyticsTracksChangedForTest(second_tracks);
  int callback_count_before_remove = analytics_listener.analytics_tracks_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  TracksSnapshot ignored_tracks;
  player->SimulateAnalyticsTracksChangedForTest(ignored_tracks);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_tracks_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_tracks_changed_callback_count == callback_count_before_remove
          ? 1
          : 0);
  summary += ",groupCount=" +
      std::to_string(analytics_listener.analytics_tracks_changed_group_count);
  summary += ",firstGroupType=" +
      std::to_string(analytics_listener.analytics_tracks_changed_first_group_type);
  summary += ",firstGroupId=" + analytics_listener.analytics_tracks_changed_first_group_id;
  summary += ",firstGroupTokenPresent=" +
      std::to_string(analytics_listener.analytics_tracks_changed_first_group_token_present ? 1 : 0);
  summary += ",firstTrackCount=" +
      std::to_string(analytics_listener.analytics_tracks_changed_first_track_count);
  summary += ",containsAudio=" +
      std::to_string(analytics_listener.analytics_tracks_changed_contains_audio ? 1 : 0);
  summary += ",containsVideo=" +
      std::to_string(analytics_listener.analytics_tracks_changed_contains_video ? 1 : 0);
  summary += ",videoSelected=" +
      std::to_string(analytics_listener.analytics_tracks_changed_video_selected ? 1 : 0);
  return NewStringUtfChecked(env, summary, "nativeAnalyticsTracksChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsMediaItemTransitionSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsMediaItemTransitionEvent first_media_item_transition;
  first_media_item_transition.media_item.media_id = "analytics-transition-first";
  first_media_item_transition.media_item.source_type = MediaSourceType::kDash;
  first_media_item_transition.reason = 1;
  player->SimulateAnalyticsMediaItemTransitionForTest(first_media_item_transition);
  AnalyticsMediaItemTransitionEvent second_media_item_transition;
  second_media_item_transition.media_item.media_id = "analytics-transition-final";
  second_media_item_transition.media_item.source_type = MediaSourceType::kHls;
  second_media_item_transition.reason = 2;
  player->SimulateAnalyticsMediaItemTransitionForTest(second_media_item_transition);
  int callback_count_before_remove =
      analytics_listener.analytics_media_item_transition_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsMediaItemTransitionEvent ignored_media_item_transition;
  ignored_media_item_transition.media_item.media_id = "analytics-transition-ignored";
  ignored_media_item_transition.media_item.source_type = MediaSourceType::kSmoothStreaming;
  ignored_media_item_transition.reason = 9;
  player->SimulateAnalyticsMediaItemTransitionForTest(ignored_media_item_transition);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_media_item_transition_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_media_item_transition_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",mediaId=" + analytics_listener.analytics_media_item_transition_media_id;
  summary += ",sourceType=" +
      std::to_string(analytics_listener.analytics_media_item_transition_source_type);
  summary += ",reason=" +
      std::to_string(analytics_listener.analytics_media_item_transition_reason);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsMediaItemTransitionSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsCuesSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  CueSnapshot first_cues;
  first_cues.presentation_time_us = 123456;
  first_cues.cue_count = 1;
  first_cues.texts = {"Analytics Cue First"};
  first_cues.text_tokens = {"generated-opaque-object-token-analytics-cue-first-text"};
  first_cues.bitmap_tokens = {"generated-opaque-object-token-analytics-cue-first-bitmap"};
  first_cues.cues.resize(1);
  first_cues.cues[0].text = "Analytics Cue First";
  first_cues.cues[0].text_token = "generated-opaque-object-token-analytics-cue-first-text";
  first_cues.cues[0].bitmap_token = "generated-opaque-object-token-analytics-cue-first-bitmap";
  player->SimulateAnalyticsCuesForTest(first_cues);
  CueSnapshot second_cues;
  second_cues.presentation_time_us = 654321;
  second_cues.cue_count = 2;
  second_cues.texts = {"Analytics Cue Final", "Analytics Cue Final 2"};
  second_cues.text_tokens = {
      "generated-opaque-object-token-analytics-cue-final-text",
      "generated-opaque-object-token-analytics-cue-final-text-2"};
  second_cues.bitmap_tokens = {"generated-opaque-object-token-analytics-cue-final-bitmap", ""};
  second_cues.cues.resize(2);
  second_cues.cues[0].text = "Analytics Cue Final";
  second_cues.cues[0].text_token = "generated-opaque-object-token-analytics-cue-final-text";
  second_cues.cues[0].bitmap_token = "generated-opaque-object-token-analytics-cue-final-bitmap";
  second_cues.cues[1].text = "Analytics Cue Final 2";
  second_cues.cues[1].text_token = "generated-opaque-object-token-analytics-cue-final-text-2";
  player->SimulateAnalyticsCuesForTest(second_cues);
  int callback_count_before_remove = analytics_listener.analytics_cues_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  CueSnapshot ignored_cues;
  ignored_cues.presentation_time_us = 1;
  ignored_cues.cue_count = 1;
  ignored_cues.texts = {"Ignored Cue"};
  ignored_cues.cues.resize(1);
  ignored_cues.cues[0].text = "Ignored Cue";
  player->SimulateAnalyticsCuesForTest(ignored_cues);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_cues_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_cues_callback_count == callback_count_before_remove ? 1 : 0);
  summary += ",cueCount=" + std::to_string(analytics_listener.analytics_cues_cue_count);
  summary += ",presentationTimeUs=" +
      std::to_string(analytics_listener.analytics_cues_presentation_time_us);
  summary += ",text0=" + analytics_listener.analytics_cues_text0;
  summary += ",text0TokenPresent=" +
      std::to_string(analytics_listener.analytics_cues_text0_token_present ? 1 : 0);
  summary += ",bitmap0TokenPresent=" +
      std::to_string(analytics_listener.analytics_cues_bitmap0_token_present ? 1 : 0);
  summary += ",text1=" + analytics_listener.analytics_cues_text1;
  summary += ",text1TokenPresent=" +
      std::to_string(analytics_listener.analytics_cues_text1_token_present ? 1 : 0);
  summary += ",bitmap1TokenPresent=" +
      std::to_string(analytics_listener.analytics_cues_bitmap1_token_present ? 1 : 0);
  return NewStringUtfChecked(env, summary, "nativeAnalyticsCuesSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsMetadataSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsMetadataEvent first_metadata;
  first_metadata.entry_count = 1;
  first_metadata.first_entry_type = "TextInformationFrame";
  first_metadata.first_entry_text = "Analytics Metadata First";
  player->SimulateAnalyticsMetadataForTest(first_metadata);
  AnalyticsMetadataEvent second_metadata;
  second_metadata.entry_count = 2;
  second_metadata.first_entry_type = "MdtaMetadataEntry";
  second_metadata.first_entry_text = "analytics-metadata-final";
  player->SimulateAnalyticsMetadataForTest(second_metadata);
  int callback_count_before_remove = analytics_listener.analytics_metadata_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsMetadataEvent ignored_metadata;
  ignored_metadata.entry_count = 9;
  ignored_metadata.first_entry_type = "Ignored";
  ignored_metadata.first_entry_text = "ignored";
  player->SimulateAnalyticsMetadataForTest(ignored_metadata);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_metadata_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_metadata_callback_count == callback_count_before_remove ? 1 : 0);
  summary += ",entryCount=" + std::to_string(analytics_listener.analytics_metadata_entry_count);
  summary += ",firstEntryType=" + analytics_listener.analytics_metadata_first_entry_type;
  summary += ",firstEntryText=" + analytics_listener.analytics_metadata_first_entry_text;
  return NewStringUtfChecked(env, summary, "nativeAnalyticsMetadataSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsLoadErrorSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  AnalyticsLoadErrorEvent first_load_error;
  first_load_error.uri = "https://example.com/analytics-error-first.m3u8";
  first_load_error.data_type = 1;
  first_load_error.track_type = 1;
  first_load_error.message = "analytics-load-first";
  first_load_error.was_canceled = true;
  player->SimulateAnalyticsLoadErrorForTest(first_load_error);
  AnalyticsLoadErrorEvent second_load_error;
  second_load_error.uri = "https://example.com/analytics-error-final.m3u8";
  second_load_error.data_type = 4;
  second_load_error.track_type = 2;
  second_load_error.message = "analytics-load-final";
  second_load_error.was_canceled = false;
  player->SimulateAnalyticsLoadErrorForTest(second_load_error);
  int callback_count_before_remove = analytics_listener.analytics_load_error_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  AnalyticsLoadErrorEvent ignored_load_error;
  ignored_load_error.uri = "https://example.com/ignored.m3u8";
  ignored_load_error.data_type = 9;
  ignored_load_error.track_type = 9;
  ignored_load_error.message = "ignored";
  ignored_load_error.was_canceled = true;
  player->SimulateAnalyticsLoadErrorForTest(ignored_load_error);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_load_error_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_load_error_callback_count == callback_count_before_remove
          ? 1
          : 0);
  summary += ",uri=" + analytics_listener.analytics_load_error_uri;
  summary += ",dataType=" + std::to_string(analytics_listener.analytics_load_error_data_type);
  summary += ",trackType=" + std::to_string(analytics_listener.analytics_load_error_track_type);
  summary += ",message=" + analytics_listener.analytics_load_error_message;
  summary += ",wasCanceled=" +
      std::to_string(analytics_listener.analytics_load_error_was_canceled ? 1 : 0);
  return NewStringUtfChecked(env, summary, "nativeAnalyticsLoadErrorSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsDeviceInfoChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  DeviceInfoDescriptor first_device_info;
  first_device_info.playback_type = 0;
  first_device_info.min_volume = 1;
  first_device_info.max_volume = 10;
  first_device_info.routing_controller_id = "route-first";
  player->SimulateAnalyticsDeviceInfoChangedForTest(first_device_info);
  DeviceInfoDescriptor second_device_info;
  second_device_info.playback_type = 1;
  second_device_info.min_volume = 2;
  second_device_info.max_volume = 15;
  second_device_info.routing_controller_id = "route-final";
  player->SimulateAnalyticsDeviceInfoChangedForTest(second_device_info);
  int callback_count_before_remove =
      analytics_listener.analytics_device_info_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  DeviceInfoDescriptor ignored_device_info;
  ignored_device_info.playback_type = 9;
  ignored_device_info.min_volume = 9;
  ignored_device_info.max_volume = 9;
  ignored_device_info.routing_controller_id = "route-ignored";
  player->SimulateAnalyticsDeviceInfoChangedForTest(ignored_device_info);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_device_info_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_device_info_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",playbackType=" +
      std::to_string(analytics_listener.analytics_device_info_changed_playback_type);
  summary += ",minVolume=" +
      std::to_string(analytics_listener.analytics_device_info_changed_min_volume);
  summary += ",maxVolume=" +
      std::to_string(analytics_listener.analytics_device_info_changed_max_volume);
  summary += ",routingControllerId=" +
      analytics_listener.analytics_device_info_changed_routing_controller_id;
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsDeviceInfoChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsMediaMetadataChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  MediaMetadataSnapshot first_metadata;
  first_metadata.title = "Analytics Media First";
  first_metadata.artist = "Analytics Artist First";
  first_metadata.display_title = "Analytics Display First";
  player->SimulateAnalyticsMediaMetadataChangedForTest(first_metadata);
  MediaMetadataSnapshot second_metadata;
  second_metadata.title = "Analytics Media Final";
  second_metadata.artist = "Analytics Artist Final";
  second_metadata.display_title = "Analytics Display Final";
  player->SimulateAnalyticsMediaMetadataChangedForTest(second_metadata);
  int callback_count_before_remove =
      analytics_listener.analytics_media_metadata_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  MediaMetadataSnapshot ignored_metadata;
  ignored_metadata.title = "Ignored";
  ignored_metadata.artist = "Ignored";
  ignored_metadata.display_title = "Ignored";
  player->SimulateAnalyticsMediaMetadataChangedForTest(ignored_metadata);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_media_metadata_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_media_metadata_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",title=" + analytics_listener.analytics_media_metadata_changed_title;
  summary += ",artist=" + analytics_listener.analytics_media_metadata_changed_artist;
  summary += ",displayTitle=" +
      analytics_listener.analytics_media_metadata_changed_display_title;
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsMediaMetadataChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsPlaylistMetadataChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  MediaMetadataSnapshot first_metadata;
  first_metadata.title = "Analytics Playlist First";
  first_metadata.artist = "Analytics Playlist Artist First";
  first_metadata.display_title = "Analytics Playlist Display First";
  player->SimulateAnalyticsPlaylistMetadataChangedForTest(first_metadata);
  MediaMetadataSnapshot second_metadata;
  second_metadata.title = "Analytics Playlist Final";
  second_metadata.artist = "Analytics Playlist Artist Final";
  second_metadata.display_title = "Analytics Playlist Display Final";
  player->SimulateAnalyticsPlaylistMetadataChangedForTest(second_metadata);
  int callback_count_before_remove =
      analytics_listener.analytics_playlist_metadata_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  MediaMetadataSnapshot ignored_metadata;
  ignored_metadata.title = "Ignored Playlist";
  ignored_metadata.artist = "Ignored Playlist";
  ignored_metadata.display_title = "Ignored Playlist";
  player->SimulateAnalyticsPlaylistMetadataChangedForTest(ignored_metadata);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.analytics_playlist_metadata_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.analytics_playlist_metadata_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",title=" + analytics_listener.analytics_playlist_metadata_changed_title;
  summary += ",artist=" + analytics_listener.analytics_playlist_metadata_changed_artist;
  summary += ",displayTitle=" +
      analytics_listener.analytics_playlist_metadata_changed_display_title;
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsPlaylistMetadataChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAnalyticsVideoInputFormatChangedSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingPlayerListener analytics_listener;
  player->AddAnalyticsListener(&analytics_listener);
  VideoInputFormatChangedEvent first_video_input_format_changed;
  first_video_input_format_changed.sample_mime_type = "video/first";
  first_video_input_format_changed.codecs = "avc1.64001f";
  first_video_input_format_changed.width = 1280;
  first_video_input_format_changed.height = 720;
  first_video_input_format_changed.frame_rate = 24.0f;
  player->SimulateVideoInputFormatChangedForTest(first_video_input_format_changed);
  VideoInputFormatChangedEvent second_video_input_format_changed;
  second_video_input_format_changed.sample_mime_type = "video/final";
  second_video_input_format_changed.codecs = "hvc1.1.6.L93.B0";
  second_video_input_format_changed.width = 1920;
  second_video_input_format_changed.height = 1080;
  second_video_input_format_changed.frame_rate = 59.94f;
  player->SimulateVideoInputFormatChangedForTest(second_video_input_format_changed);
  int callback_count_before_remove =
      analytics_listener.video_input_format_changed_callback_count;
  player->RemoveAnalyticsListener(&analytics_listener);
  VideoInputFormatChangedEvent ignored_video_input_format_changed;
  ignored_video_input_format_changed.sample_mime_type = "video/ignored";
  ignored_video_input_format_changed.codecs = "ignored";
  ignored_video_input_format_changed.width = 1;
  ignored_video_input_format_changed.height = 1;
  ignored_video_input_format_changed.frame_rate = 1.0f;
  player->SimulateVideoInputFormatChangedForTest(ignored_video_input_format_changed);
  std::string summary = "beforeRemoveCb=" + std::to_string(callback_count_before_remove);
  summary += ",afterRemoveCb=" +
      std::to_string(analytics_listener.video_input_format_changed_callback_count);
  summary += ",callbackStopped=" + std::to_string(
      analytics_listener.video_input_format_changed_callback_count ==
              callback_count_before_remove
          ? 1
          : 0);
  summary += ",sampleMimeType=" + analytics_listener.video_input_format_sample_mime_type;
  summary += ",codecs=" + analytics_listener.video_input_format_codecs;
  summary += ",width=" + std::to_string(analytics_listener.video_input_format_width);
  summary += ",height=" + std::to_string(analytics_listener.video_input_format_height);
  summary += ",frameRate=" + std::to_string(analytics_listener.video_input_format_frame_rate);
  return NewStringUtfChecked(
      env,
      summary,
      "nativeAnalyticsVideoInputFormatChangedSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeImageOutputSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  CapturingImageOutputListener image_output_listener;
  player->SetImageOutputListener(&image_output_listener);
  ImageFrameSnapshot first_image_frame;
  first_image_frame.presentation_time_us = 123456;
  first_image_frame.width = 4;
  first_image_frame.height = 3;
  first_image_frame.byte_count = 48;
  first_image_frame.allocation_byte_count = 64;
  first_image_frame.row_bytes = 16;
  first_image_frame.has_alpha = true;
  first_image_frame.is_premultiplied = true;
  first_image_frame.is_mutable = false;
  first_image_frame.bitmap_config = "ARGB_8888";
  player->SimulateImageOutputForTest(first_image_frame);
  ImageFrameSnapshot second_image_frame;
  second_image_frame.presentation_time_us = 234567;
  second_image_frame.width = 8;
  second_image_frame.height = 6;
  second_image_frame.byte_count = 192;
  second_image_frame.allocation_byte_count = 224;
  second_image_frame.row_bytes = 32;
  second_image_frame.has_alpha = true;
  second_image_frame.is_premultiplied = true;
  second_image_frame.is_mutable = false;
  second_image_frame.bitmap_config = "ARGB_8888";
  player->SimulateImageOutputForTest(second_image_frame);
  player->SetImageOutputEnabled(false);
  int disabled_count_after_runtime_disable = image_output_listener.disabled_count;
  ImageFrameSnapshot ignored_while_disabled_image_frame;
  ignored_while_disabled_image_frame.presentation_time_us = 300000;
  ignored_while_disabled_image_frame.width = 10;
  ignored_while_disabled_image_frame.height = 5;
  ignored_while_disabled_image_frame.byte_count = 200;
  ignored_while_disabled_image_frame.allocation_byte_count = 240;
  ignored_while_disabled_image_frame.row_bytes = 40;
  ignored_while_disabled_image_frame.has_alpha = true;
  ignored_while_disabled_image_frame.is_premultiplied = true;
  ignored_while_disabled_image_frame.is_mutable = false;
  ignored_while_disabled_image_frame.bitmap_config = "ARGB_8888";
  player->SimulateImageOutputForTest(ignored_while_disabled_image_frame);
  int image_count_after_runtime_disable = image_output_listener.image_count;
  player->SetImageOutputEnabled(true);
  ImageFrameSnapshot resumed_image_frame;
  resumed_image_frame.presentation_time_us = 345678;
  resumed_image_frame.width = 12;
  resumed_image_frame.height = 7;
  resumed_image_frame.byte_count = 336;
  resumed_image_frame.allocation_byte_count = 384;
  resumed_image_frame.row_bytes = 48;
  resumed_image_frame.has_alpha = true;
  resumed_image_frame.is_premultiplied = true;
  resumed_image_frame.is_mutable = false;
  resumed_image_frame.bitmap_config = "ARGB_8888";
  player->SimulateImageOutputForTest(resumed_image_frame);
  int image_count_after_runtime_reenable = image_output_listener.image_count;
  int image_count_before_remove = image_output_listener.image_count;
  player->RemoveImageOutputListener(&image_output_listener);
  ImageFrameSnapshot ignored_image_frame;
  ignored_image_frame.presentation_time_us = 999999;
  ignored_image_frame.width = 16;
  ignored_image_frame.height = 9;
  ignored_image_frame.byte_count = 576;
  ignored_image_frame.allocation_byte_count = 640;
  ignored_image_frame.row_bytes = 64;
  ignored_image_frame.has_alpha = true;
  ignored_image_frame.is_premultiplied = true;
  ignored_image_frame.is_mutable = false;
  ignored_image_frame.bitmap_config = "ARGB_8888";
  player->SimulateImageOutputForTest(ignored_image_frame);
  CapturingImageOutputListener second_image_output_listener;
  player->SetImageOutputListener(&second_image_output_listener);
  ImageFrameSnapshot reattached_image_frame;
  reattached_image_frame.presentation_time_us = 456789;
  reattached_image_frame.width = 14;
  reattached_image_frame.height = 8;
  reattached_image_frame.byte_count = 224;
  reattached_image_frame.allocation_byte_count = 256;
  reattached_image_frame.row_bytes = 28;
  reattached_image_frame.has_alpha = false;
  reattached_image_frame.is_premultiplied = false;
  reattached_image_frame.is_mutable = false;
  reattached_image_frame.bitmap_config = "RGB_565";
  player->SimulateImageOutputForTest(reattached_image_frame);
  player->RemoveImageOutputListener(&second_image_output_listener);
  std::string summary = "imageCount=" + std::to_string(image_output_listener.image_count);
  summary += ",runtimeDisableDisabledCount=" +
      std::to_string(disabled_count_after_runtime_disable);
  summary += ",afterRuntimeDisableImageCount=" +
      std::to_string(image_count_after_runtime_disable);
  summary += ",afterRuntimeReenableImageCount=" +
      std::to_string(image_count_after_runtime_reenable);
  summary += ",beforeRemoveImageCount=" + std::to_string(image_count_before_remove);
  summary += ",afterRemoveImageCount=" + std::to_string(image_output_listener.image_count);
  summary += ",callbackStopped=" +
      std::to_string(image_output_listener.image_count == image_count_before_remove ? 1 : 0);
  summary += ",lastPresentationTimeUs=" +
      std::to_string(image_output_listener.last_presentation_time_us);
  summary += ",lastWidth=" + std::to_string(image_output_listener.last_width);
  summary += ",lastHeight=" + std::to_string(image_output_listener.last_height);
  summary += ",lastByteCount=" + std::to_string(image_output_listener.last_byte_count);
  summary += ",lastAllocationByteCount=" +
      std::to_string(image_output_listener.last_allocation_byte_count);
  summary += ",lastRowBytes=" + std::to_string(image_output_listener.last_row_bytes);
  summary += ",lastHasAlpha=" + std::to_string(image_output_listener.last_has_alpha ? 1 : 0);
  summary += ",lastIsPremultiplied=" +
      std::to_string(image_output_listener.last_is_premultiplied ? 1 : 0);
  summary += ",lastIsMutable=" +
      std::to_string(image_output_listener.last_is_mutable ? 1 : 0);
  summary += ",lastBitmapConfig=" + image_output_listener.last_bitmap_config;
  summary += ",disabledCount=" + std::to_string(image_output_listener.disabled_count);
  summary += ",reattachImageCount=" + std::to_string(second_image_output_listener.image_count);
  summary += ",reattachLastPresentationTimeUs=" +
      std::to_string(second_image_output_listener.last_presentation_time_us);
  summary += ",reattachLastWidth=" + std::to_string(second_image_output_listener.last_width);
  summary += ",reattachLastHeight=" + std::to_string(second_image_output_listener.last_height);
  summary += ",reattachLastByteCount=" + std::to_string(second_image_output_listener.last_byte_count);
  summary += ",reattachLastAllocationByteCount=" +
      std::to_string(second_image_output_listener.last_allocation_byte_count);
  summary += ",reattachLastRowBytes=" +
      std::to_string(second_image_output_listener.last_row_bytes);
  summary += ",reattachLastHasAlpha=" +
      std::to_string(second_image_output_listener.last_has_alpha ? 1 : 0);
  summary += ",reattachLastIsPremultiplied=" +
      std::to_string(second_image_output_listener.last_is_premultiplied ? 1 : 0);
  summary += ",reattachLastIsMutable=" +
      std::to_string(second_image_output_listener.last_is_mutable ? 1 : 0);
  summary += ",reattachLastBitmapConfig=" + second_image_output_listener.last_bitmap_config;
  summary += ",reattachDisabledCount=" + std::to_string(second_image_output_listener.disabled_count);
  return NewStringUtfChecked(env, summary, "nativeImageOutputSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeSourceTypeSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  config.media_source_factory_config.parse_subtitles_during_extraction = false;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/master.m3u8";
  media_item.media_id = "source-type-item";
  player->SetMediaItem(media_item);
  MediaItemDescriptor current_media_item = player->GetCurrentMediaItem();
  std::string summary = "mediaId=" + current_media_item.media_id;
  summary += ",uri=" + current_media_item.uri;
  summary += ",mimeType=" + current_media_item.mime_type;
  summary += ",sourceType=" +
      std::to_string(static_cast<int>(current_media_item.source_type));
  return NewStringUtfChecked(env, summary, "nativeSourceTypeSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeHttpHlsDashPlaybackSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context,
    jstring http_url,
    jstring hls_url,
    jstring dash_url) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player =
      ExoPlayerSdkPlayer::Create(env, context, config);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "stream-playback-error:createPlayer",
        "nativeHttpHlsDashPlaybackSmokeTest.error");
  }

  std::string summary;
  AppendStreamPlaybackScenarioSummary(
      player.get(),
      {
          "http",
          JStringToString(env, http_url),
          "http-progressive-item",
          "audio/mp4",
          MediaSourceType::kProgressive,
      },
      &summary);
  summary += ";";
  AppendStreamPlaybackScenarioSummary(
      player.get(),
      {
          "hls",
          JStringToString(env, hls_url),
          "hls-item",
          "application/x-mpegURL",
          MediaSourceType::kHls,
      },
      &summary);
  summary += ";";
  AppendStreamPlaybackScenarioSummary(
      player.get(),
      {
          "dash",
          JStringToString(env, dash_url),
          "dash-item",
          "application/dash+xml",
          MediaSourceType::kDash,
      },
      &summary);
  player->Release();
  return NewStringUtfChecked(env, summary, "nativeHttpHlsDashPlaybackSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeHttpDataSourceConfigPlaybackSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context,
    jstring http_url) {
  PlayerConfig config;
  config.media_source_factory_config.default_request_header_names = {
      "X-CppBridge-Stage", "X-CppBridge-Source"};
  config.media_source_factory_config.default_request_header_values = {
      "5", "http-config"};
  config.media_source_factory_config.user_agent = "cppbridge-stage5-agent";
  config.media_source_factory_config.connect_timeout_ms = 12345;
  config.media_source_factory_config.read_timeout_ms = 23456;
  config.media_source_factory_config.allow_cross_protocol_redirects = true;

  std::unique_ptr<ExoPlayerSdkPlayer> player =
      ExoPlayerSdkPlayer::Create(env, context, config);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "http-data-source-config-playback-error:createPlayer",
        "nativeHttpDataSourceConfigPlaybackSmokeTest.error");
  }

  PlayerConfig::MediaSourceFactoryConfig resolved = player->GetMediaSourceFactoryConfig();
  std::string summary = "headerCount=" +
      std::to_string(resolved.default_request_header_names.size());
  summary += ",userAgent=" + resolved.user_agent;
  summary += ",connectTimeoutMs=" + std::to_string(resolved.connect_timeout_ms);
  summary += ",readTimeoutMs=" + std::to_string(resolved.read_timeout_ms);
  summary += ",allowCrossProtocolRedirects=" +
      std::to_string(resolved.allow_cross_protocol_redirects ? 1 : 0);
  summary += ";";
  AppendStreamPlaybackScenarioSummary(
      player.get(),
      {
          "httpConfig",
          JStringToString(env, http_url),
          "http-config-stage5-item",
          "audio/mp4",
          MediaSourceType::kProgressive,
      },
      &summary);
  player->Release();
  return NewStringUtfChecked(
      env, summary, "nativeHttpDataSourceConfigPlaybackSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeCustomMediaSourceFactoryPlaybackSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context,
    jstring factory_token) {
  PlayerConfig config;
  config.media_source_factory_config.factory_token = JStringToString(env, factory_token);
  std::unique_ptr<ExoPlayerSdkPlayer> player =
      ExoPlayerSdkPlayer::Create(env, context, config);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "custom-source-factory-playback-error:createPlayer",
        "nativeCustomMediaSourceFactoryPlaybackSmokeTest.error");
  }

  PlayerConfig::MediaSourceFactoryConfig resolved = player->GetMediaSourceFactoryConfig();
  std::string summary = "factoryToken=" + resolved.factory_token;
  summary += ",injectedFactoryUsed=" +
      std::to_string(resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",factoryIdentity=" +
      std::to_string(resolved.injected_factory_identity_for_test);
  summary += ";";
  AppendStreamPlaybackScenarioSummary(
      player.get(),
      {
          "smooth",
          "https://example.com/stage5/smooth.ism/manifest",
          "smooth-stage5-item",
          "application/vnd.ms-sstr+xml",
          MediaSourceType::kSmoothStreaming,
      },
      &summary);
  summary += ";";
  AppendStreamPlaybackScenarioSummary(
      player.get(),
      {
          "rtsp",
          "rtsp://localhost/stage5",
          "rtsp-stage5-item",
          "application/x-rtsp",
          MediaSourceType::kRtsp,
      },
      &summary);
  player->Release();
  return NewStringUtfChecked(
      env, summary, "nativeCustomMediaSourceFactoryPlaybackSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeMediaSourceFactoryConfigSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  config.media_source_factory_config.parse_subtitles_during_extraction = false;
  config.media_source_factory_config.load_only_selected_tracks = true;
  config.media_source_factory_config.default_request_header_names = {
      "X-Test-Header", "X-Trace-Id"};
  config.media_source_factory_config.default_request_header_values = {
      "bridge", "trace-123"};
  config.media_source_factory_config.user_agent = "cppbridge-agent";
  config.media_source_factory_config.connect_timeout_ms = 2345;
  config.media_source_factory_config.read_timeout_ms = 6789;
  config.media_source_factory_config.allow_cross_protocol_redirects = true;
  config.media_source_factory_config.live_target_offset_ms = 3000;
  config.media_source_factory_config.live_min_offset_ms = 2000;
  config.media_source_factory_config.live_max_offset_ms = 5000;
  config.media_source_factory_config.live_min_speed = 0.97f;
  config.media_source_factory_config.live_max_speed = 1.03f;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  PlayerConfig::MediaSourceFactoryConfig resolved = player->GetMediaSourceFactoryConfig();
  std::string summary = "parseSubtitlesDuringExtraction=";
  summary += resolved.parse_subtitles_during_extraction ? "1" : "0";
  summary += ",loadOnlySelectedTracks=";
  summary += resolved.load_only_selected_tracks ? "1" : "0";
  summary += ",headerCount=" +
      std::to_string(std::min(
          resolved.default_request_header_names.size(),
          resolved.default_request_header_values.size()));
  if (!resolved.default_request_header_names.empty() &&
      !resolved.default_request_header_values.empty()) {
    summary += ",header0=" + resolved.default_request_header_names[0] + ":" +
        resolved.default_request_header_values[0];
  }
  if (resolved.default_request_header_names.size() > 1 &&
      resolved.default_request_header_values.size() > 1) {
    summary += ",header1=" + resolved.default_request_header_names[1] + ":" +
        resolved.default_request_header_values[1];
  }
  summary += ",userAgent=" + resolved.user_agent;
  summary += ",connectTimeoutMs=" + std::to_string(resolved.connect_timeout_ms);
  summary += ",readTimeoutMs=" + std::to_string(resolved.read_timeout_ms);
  summary += ",allowCrossProtocolRedirects=";
  summary += resolved.allow_cross_protocol_redirects ? "1" : "0";
  summary += ",liveTargetOffsetMs=" + std::to_string(resolved.live_target_offset_ms);
  summary += ",liveMinOffsetMs=" + std::to_string(resolved.live_min_offset_ms);
  summary += ",liveMaxOffsetMs=" + std::to_string(resolved.live_max_offset_ms);
  summary += ",liveMinSpeed=" + std::to_string(resolved.live_min_speed);
  summary += ",liveMaxSpeed=" + std::to_string(resolved.live_max_speed);
  return NewStringUtfChecked(env, summary, "nativeMediaSourceFactoryConfigSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeMediaSourceFactoryInjectionSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  config.media_source_factory_config.factory_token = "test-injected-media-source-factory";
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  PlayerConfig::MediaSourceFactoryConfig resolved = player->GetMediaSourceFactoryConfig();
  std::string summary = "factoryToken=" + resolved.factory_token;
  summary += ",injectedFactoryUsed=" +
      std::to_string(resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",factoryIdentity=" +
      std::to_string(resolved.injected_factory_identity_for_test);
  return NewStringUtfChecked(env, summary, "nativeMediaSourceFactoryInjectionSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeMediaSourceFactoryInjectionFallbackSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  config.media_source_factory_config.factory_token = "missing-media-source-factory-token";
  config.media_source_factory_config.user_agent = "fallback-agent";
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  PlayerConfig::MediaSourceFactoryConfig resolved = player->GetMediaSourceFactoryConfig();
  std::string summary = "factoryToken=" + resolved.factory_token;
  summary += ",injectedFactoryUsed=" +
      std::to_string(resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",factoryIdentity=" +
      std::to_string(resolved.injected_factory_identity_for_test);
  summary += ",fallbackApplied=" +
      std::to_string(!resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",userAgent=" + resolved.user_agent;
  return NewStringUtfChecked(
      env, summary, "nativeMediaSourceFactoryInjectionFallbackSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeMediaSourceFactoryInjectionReplacementSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  config.media_source_factory_config.factory_token = "replaceable-media-source-factory-token";
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  PlayerConfig::MediaSourceFactoryConfig resolved = player->GetMediaSourceFactoryConfig();
  std::string summary = "factoryToken=" + resolved.factory_token;
  summary += ",injectedFactoryUsed=" +
      std::to_string(resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",factoryIdentity=" +
      std::to_string(resolved.injected_factory_identity_for_test);
  return NewStringUtfChecked(
      env, summary, "nativeMediaSourceFactoryInjectionReplacementSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeMediaSourceFactoryInjectionMultiTokenSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig first_config;
  first_config.media_source_factory_config.factory_token = "multi-token-media-source-factory-a";
  std::unique_ptr<ExoPlayerSdkPlayer> first_player =
      ExoPlayerSdkPlayer::Create(env, context, first_config);
  PlayerConfig::MediaSourceFactoryConfig first_resolved =
      first_player->GetMediaSourceFactoryConfig();

  PlayerConfig second_config;
  second_config.media_source_factory_config.factory_token = "multi-token-media-source-factory-b";
  std::unique_ptr<ExoPlayerSdkPlayer> second_player =
      ExoPlayerSdkPlayer::Create(env, context, second_config);
  PlayerConfig::MediaSourceFactoryConfig second_resolved =
      second_player->GetMediaSourceFactoryConfig();

  std::string summary = "firstFactoryToken=" + first_resolved.factory_token;
  summary += ",firstInjectedFactoryUsed=" +
      std::to_string(first_resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",firstFactoryIdentity=" +
      std::to_string(first_resolved.injected_factory_identity_for_test);
  summary += ",secondFactoryToken=" + second_resolved.factory_token;
  summary += ",secondInjectedFactoryUsed=" +
      std::to_string(second_resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",secondFactoryIdentity=" +
      std::to_string(second_resolved.injected_factory_identity_for_test);
  summary += ",tokensIsolated=" + std::to_string(
      first_resolved.injected_factory_identity_for_test > 0 &&
      second_resolved.injected_factory_identity_for_test > 0 &&
      first_resolved.injected_factory_identity_for_test !=
          second_resolved.injected_factory_identity_for_test
          ? 1
          : 0);
  return NewStringUtfChecked(
      env, summary, "nativeMediaSourceFactoryInjectionMultiTokenSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeMediaSourceFactoryGeneratedTokenSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context,
    jstring token) {
  PlayerConfig config;
  config.media_source_factory_config.factory_token = JStringToString(env, token);
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  PlayerConfig::MediaSourceFactoryConfig resolved = player->GetMediaSourceFactoryConfig();
  std::string summary = "factoryToken=" + resolved.factory_token;
  summary += ",injectedFactoryUsed=" +
      std::to_string(resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",factoryIdentity=" +
      std::to_string(resolved.injected_factory_identity_for_test);
  summary += ",generatedTokenPath=1";
  return NewStringUtfChecked(
      env, summary, "nativeMediaSourceFactoryGeneratedTokenSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePlayerConfigFlagsSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  config.handle_audio_focus = false;
  config.handle_audio_becoming_noisy = false;
  config.use_lazy_preparation = false;
  config.seek_back_increment_ms = 1357;
  config.seek_forward_increment_ms = 2468;
  config.wake_mode = 1;
  config.priority = 77;
  config.use_priority_task_manager = true;
  config.target_preload_duration_us = 456789;
  std::shared_ptr<ExoPlayerBridge> bridge = ExoPlayerBridge::Create(env, context, config);
  if (bridge == nullptr) {
    return NewStringUtfChecked(
        env, "config-error:createBridge", "nativePlayerConfigFlagsSmokeTest.error");
  }
  std::vector<std::string> flags = BridgeGetPlayerConfigFlagsForTest(env, bridge);
  std::string summary = "handleAudioFocus=";
  summary += flags.size() > 0 ? flags[0] : "";
  summary += ",handleAudioBecomingNoisy=";
  summary += flags.size() > 1 ? flags[1] : "";
  summary += ",useLazyPreparation=";
  summary += flags.size() > 2 ? flags[2] : "";
  summary += ",seekBackIncrementMs=";
  summary += flags.size() > 3 ? flags[3] : "";
  summary += ",seekForwardIncrementMs=";
  summary += flags.size() > 4 ? flags[4] : "";
  summary += ",wakeMode=";
  summary += flags.size() > 5 ? flags[5] : "";
  summary += ",priority=";
  summary += flags.size() > 6 ? flags[6] : "";
  summary += ",usePriorityTaskManager=";
  summary += flags.size() > 7 ? flags[7] : "";
  summary += ",targetPreloadDurationUs=";
  summary += flags.size() > 8 ? flags[8] : "";
  bridge->Release(env);
  return NewStringUtfChecked(env, summary, "nativePlayerConfigFlagsSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeWakeModeRuntimeSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  config.wake_mode = 1;
  std::shared_ptr<ExoPlayerBridge> bridge = ExoPlayerBridge::Create(env, context, config);
  if (bridge == nullptr) {
    return NewStringUtfChecked(
        env, "wake-mode-error:createBridge", "nativeWakeModeRuntimeSmokeTest.error");
  }
  std::vector<std::string> before = BridgeGetPlayerConfigFlagsForTest(env, bridge);
  bridge->SetWakeMode(env, 0);
  std::vector<std::string> after = BridgeGetPlayerConfigFlagsForTest(env, bridge);
  std::string summary = "beforeWakeMode=";
  summary += before.size() > 5 ? before[5] : "";
  summary += ",afterWakeMode=";
  summary += after.size() > 5 ? after[5] : "";
  summary += ",runtimeApplied=";
  summary += after.size() > 5 && after[5] == "0" ? "1" : "0";
  bridge->Release(env);
  return NewStringUtfChecked(env, summary, "nativeWakeModeRuntimeSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeRuntimeControlParitySmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  constexpr int kVideoScalingModeScaleToFitWithCropping = 2;
  constexpr int kVideoChangeFrameRateStrategyOff = -2147483647 - 1;

  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player =
      ExoPlayerSdkPlayer::Create(env, context, config);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env, "runtime-control-error:createPlayer", "nativeRuntimeControlParitySmokeTest.error");
  }

  player->SetHandleAudioBecomingNoisy(false);
  player->SetHandleAudioBecomingNoisy(true);
  player->SetSeekBackIncrementMs(4321);
  player->SetSeekForwardIncrementMs(8765);
  player->SetMaxSeekToPreviousPositionMs(9999);
  const int64_t seek_back_increment_ms = player->GetSeekBackIncrement();
  const int64_t seek_forward_increment_ms = player->GetSeekForwardIncrement();
  const int64_t max_seek_to_previous_position_ms =
      player->GetMaxSeekToPreviousPosition();
  const bool initial_pause_at_end = player->GetPauseAtEndOfMediaItems();
  player->SetPauseAtEndOfMediaItems(true);
  const bool after_enable_pause_at_end = player->GetPauseAtEndOfMediaItems();
  player->SetPauseAtEndOfMediaItems(false);
  const bool after_disable_pause_at_end = player->GetPauseAtEndOfMediaItems();
  player->SetForegroundMode(true);
  player->SetForegroundMode(false);
  player->SetVideoScalingMode(kVideoScalingModeScaleToFitWithCropping);
  const int video_scaling_mode = player->GetVideoScalingMode();
  player->SetVideoChangeFrameRateStrategy(kVideoChangeFrameRateStrategyOff);
  const int video_change_frame_rate_strategy =
      player->GetVideoChangeFrameRateStrategy();
  player->Release();

  std::shared_ptr<ExoPlayerBridge> bridge = ExoPlayerBridge::Create(env, context, config);
  if (bridge == nullptr) {
    return NewStringUtfChecked(
        env, "runtime-control-error:createBridge", "nativeRuntimeControlParitySmokeTest.error");
  }
  std::vector<std::string> initial_flags = BridgeGetPlayerConfigFlagsForTest(env, bridge);
  bridge->SetHandleAudioBecomingNoisy(env, false);
  std::vector<std::string> noisy_disabled_flags =
      BridgeGetPlayerConfigFlagsForTest(env, bridge);
  bridge->SetHandleAudioBecomingNoisy(env, true);
  std::vector<std::string> noisy_enabled_flags =
      BridgeGetPlayerConfigFlagsForTest(env, bridge);
  bridge->SetForegroundMode(env, true);
  std::vector<std::string> foreground_enabled_flags =
      BridgeGetPlayerConfigFlagsForTest(env, bridge);
  bridge->SetForegroundMode(env, false);
  std::vector<std::string> foreground_disabled_flags =
      BridgeGetPlayerConfigFlagsForTest(env, bridge);

  std::string summary = "seekBackIncrementMs=" + std::to_string(seek_back_increment_ms);
  summary += ",seekForwardIncrementMs=" + std::to_string(seek_forward_increment_ms);
  summary += ",maxSeekToPreviousPositionMs=" +
      std::to_string(max_seek_to_previous_position_ms);
  summary += ",initialPauseAtEnd=" + std::to_string(initial_pause_at_end ? 1 : 0);
  summary += ",afterEnablePauseAtEnd=" +
      std::to_string(after_enable_pause_at_end ? 1 : 0);
  summary += ",afterDisablePauseAtEnd=" +
      std::to_string(after_disable_pause_at_end ? 1 : 0);
  summary += ",videoScalingMode=" + std::to_string(video_scaling_mode);
  summary += ",videoChangeFrameRateStrategy=" +
      std::to_string(video_change_frame_rate_strategy);
  summary += ",initialNoisyFlag=";
  summary += initial_flags.size() > 1 ? initial_flags[1] : "";
  summary += ",afterDisableNoisyFlag=";
  summary += noisy_disabled_flags.size() > 1 ? noisy_disabled_flags[1] : "";
  summary += ",afterEnableNoisyFlag=";
  summary += noisy_enabled_flags.size() > 1 ? noisy_enabled_flags[1] : "";
  summary += ",afterEnableForegroundFlag=";
  summary += foreground_enabled_flags.size() > 13 ? foreground_enabled_flags[13] : "";
  summary += ",afterDisableForegroundFlag=";
  summary += foreground_disabled_flags.size() > 13 ? foreground_disabled_flags[13] : "";
  summary += ",runtimeApplied=" +
      std::to_string(
          seek_back_increment_ms == 4321 &&
                  seek_forward_increment_ms == 8765 &&
                  max_seek_to_previous_position_ms == 9999 &&
                  !initial_pause_at_end &&
                  after_enable_pause_at_end &&
                  !after_disable_pause_at_end &&
                  video_scaling_mode == kVideoScalingModeScaleToFitWithCropping &&
                  video_change_frame_rate_strategy == kVideoChangeFrameRateStrategyOff &&
                  noisy_disabled_flags.size() > 1 && noisy_disabled_flags[1] == "0" &&
                  noisy_enabled_flags.size() > 1 && noisy_enabled_flags[1] == "1" &&
                  foreground_enabled_flags.size() > 13 &&
                  foreground_enabled_flags[13] == "1" &&
                  foreground_disabled_flags.size() > 13 &&
                  foreground_disabled_flags[13] == "0"
              ? 1
              : 0);
  bridge->Release(env);
  return NewStringUtfChecked(env, summary, "nativeRuntimeControlParitySmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAudioAndScrubbingParitySmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player =
      ExoPlayerSdkPlayer::Create(env, context, config);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "audio-scrubbing-error:createPlayer",
        "nativeAudioAndScrubbingParitySmokeTest.error");
  }

  const bool scrubbing_initially = player->IsScrubbingModeEnabled();
  player->SetAudioSessionId(1234);
  player->SetAuxEffectInfo(AuxEffectInfoDescriptor{/*effect_id=*/0, /*send_level=*/0.37f});
  player->ClearAuxEffectInfo();
  player->ClearPreferredAudioDevice();
  player->SetVirtualDeviceId(42);
  player->SetScrubbingModeEnabled(true);
  const bool scrubbing_after_enable = player->IsScrubbingModeEnabled();
  ScrubbingModeParametersDescriptor requested_scrubbing_parameters;
  requested_scrubbing_parameters.disabled_track_types = {2, 3};
  requested_scrubbing_parameters.has_fractional_seek_tolerance = true;
  requested_scrubbing_parameters.fractional_seek_tolerance_before = 0.125;
  requested_scrubbing_parameters.fractional_seek_tolerance_after = 0.5;
  requested_scrubbing_parameters.should_increase_codec_operating_rate = false;
  requested_scrubbing_parameters.allow_skipping_media_codec_flush = true;
  requested_scrubbing_parameters.allow_skipping_key_frame_reset = false;
  requested_scrubbing_parameters.should_enable_dynamic_scheduling = true;
  requested_scrubbing_parameters.use_decode_only_flag = false;
  player->SetScrubbingModeParameters(requested_scrubbing_parameters);
  ScrubbingModeParametersDescriptor actual_scrubbing_parameters =
      player->GetScrubbingModeParameters();
  player->SetScrubbingModeEnabled(false);
  const bool scrubbing_after_disable = player->IsScrubbingModeEnabled();
  player->Release();

  std::shared_ptr<ExoPlayerBridge> bridge = ExoPlayerBridge::Create(env, context, config);
  if (bridge == nullptr) {
    return NewStringUtfChecked(
        env,
        "audio-scrubbing-error:createBridge",
        "nativeAudioAndScrubbingParitySmokeTest.error");
  }
  bridge->SetAudioSessionId(env, 1234);
  bridge->SetAuxEffectInfo(env, AuxEffectInfoDescriptor{/*effect_id=*/0, /*send_level=*/0.37f});
  std::vector<std::string> aux_set_flags = BridgeGetPlayerConfigFlagsForTest(env, bridge);
  bridge->ClearAuxEffectInfo(env);
  std::vector<std::string> aux_clear_flags = BridgeGetPlayerConfigFlagsForTest(env, bridge);
  bridge->SetPreferredAudioDevice(env, nullptr);
  std::vector<std::string> preferred_audio_clear_flags =
      BridgeGetPlayerConfigFlagsForTest(env, bridge);
  bridge->SetVirtualDeviceId(env, 42);
  std::vector<std::string> virtual_device_flags =
      BridgeGetPlayerConfigFlagsForTest(env, bridge);
  bridge->Release(env);

  const bool has_track_type_2 =
      std::find(
          actual_scrubbing_parameters.disabled_track_types.begin(),
          actual_scrubbing_parameters.disabled_track_types.end(),
          2) != actual_scrubbing_parameters.disabled_track_types.end();
  const bool has_track_type_3 =
      std::find(
          actual_scrubbing_parameters.disabled_track_types.begin(),
          actual_scrubbing_parameters.disabled_track_types.end(),
          3) != actual_scrubbing_parameters.disabled_track_types.end();
  const bool scrubbing_parameters_match =
      actual_scrubbing_parameters.disabled_track_types.size() == 2 &&
      has_track_type_2 &&
      has_track_type_3 &&
      actual_scrubbing_parameters.has_fractional_seek_tolerance &&
      actual_scrubbing_parameters.fractional_seek_tolerance_before == 0.125 &&
      actual_scrubbing_parameters.fractional_seek_tolerance_after == 0.5 &&
      !actual_scrubbing_parameters.should_increase_codec_operating_rate &&
      actual_scrubbing_parameters.allow_skipping_media_codec_flush &&
      !actual_scrubbing_parameters.allow_skipping_key_frame_reset &&
      actual_scrubbing_parameters.should_enable_dynamic_scheduling &&
      !actual_scrubbing_parameters.use_decode_only_flag;

  std::string summary = "audioSessionId=";
  summary += aux_set_flags.size() > 14 ? aux_set_flags[14] : "";
  summary += ",auxEffectAfterSet=";
  summary += aux_set_flags.size() > 15 ? aux_set_flags[15] : "";
  summary += ":";
  summary += aux_set_flags.size() > 16 ? aux_set_flags[16] : "";
  summary += ",auxEffectAfterClear=";
  summary += aux_clear_flags.size() > 15 ? aux_clear_flags[15] : "";
  summary += ":";
  summary += aux_clear_flags.size() > 16 ? aux_clear_flags[16] : "";
  summary += ",preferredAudioDeviceAfterClear=";
  summary += preferred_audio_clear_flags.size() > 17 ? preferred_audio_clear_flags[17] : "";
  summary += ",virtualDeviceId=";
  summary += virtual_device_flags.size() > 18 ? virtual_device_flags[18] : "";
  summary += ",scrubbingInitially=" + std::to_string(scrubbing_initially ? 1 : 0);
  summary += ",scrubbingAfterEnable=" + std::to_string(scrubbing_after_enable ? 1 : 0);
  summary += ",scrubbingAfterDisable=" + std::to_string(scrubbing_after_disable ? 1 : 0);
  summary += ",scrubTracks=";
  summary += has_track_type_2 ? "2" : "";
  summary += ",";
  summary += has_track_type_3 ? "3" : "";
  summary += ",scrubTolerance=" +
      std::to_string(actual_scrubbing_parameters.fractional_seek_tolerance_before);
  summary += ":" +
      std::to_string(actual_scrubbing_parameters.fractional_seek_tolerance_after);
  summary += ",scrubFlags=";
  summary += actual_scrubbing_parameters.should_increase_codec_operating_rate ? "1" : "0";
  summary += actual_scrubbing_parameters.allow_skipping_media_codec_flush ? "1" : "0";
  summary += actual_scrubbing_parameters.allow_skipping_key_frame_reset ? "1" : "0";
  summary += actual_scrubbing_parameters.should_enable_dynamic_scheduling ? "1" : "0";
  summary += actual_scrubbing_parameters.use_decode_only_flag ? "1" : "0";
  summary += ",runtimeApplied=" +
      std::to_string(
          !scrubbing_initially &&
                  scrubbing_after_enable &&
                  !scrubbing_after_disable &&
                  aux_set_flags.size() > 16 &&
                  aux_set_flags[14] == "1234" &&
                  aux_set_flags[15] == "0" &&
                  aux_set_flags[16] == "0.37" &&
                  aux_clear_flags.size() > 16 &&
                  aux_clear_flags[15] == "0" &&
                  aux_clear_flags[16] == "0.0" &&
                  preferred_audio_clear_flags.size() > 17 &&
                  preferred_audio_clear_flags[17] == "0" &&
                  virtual_device_flags.size() > 18 &&
                  virtual_device_flags[18] == "42" &&
                  scrubbing_parameters_match
              ? 1
              : 0);
  return NewStringUtfChecked(env, summary, "nativeAudioAndScrubbingParitySmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeCodecParametersParitySmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  auto integer_parameter = [](const std::string& key, int value) {
    CodecParameterDescriptor parameter;
    parameter.key = key;
    parameter.value_type = CodecParameterDescriptor::ValueType::kInteger;
    parameter.int_value = value;
    return parameter;
  };
  auto long_parameter = [](const std::string& key, int64_t value) {
    CodecParameterDescriptor parameter;
    parameter.key = key;
    parameter.value_type = CodecParameterDescriptor::ValueType::kLong;
    parameter.long_value = value;
    return parameter;
  };
  auto float_parameter = [](const std::string& key, float value) {
    CodecParameterDescriptor parameter;
    parameter.key = key;
    parameter.value_type = CodecParameterDescriptor::ValueType::kFloat;
    parameter.float_value = value;
    return parameter;
  };
  auto string_parameter = [](const std::string& key, const std::string& value) {
    CodecParameterDescriptor parameter;
    parameter.key = key;
    parameter.value_type = CodecParameterDescriptor::ValueType::kString;
    parameter.string_value = value;
    return parameter;
  };
  auto byte_buffer_parameter = [](const std::string& key, std::vector<uint8_t> value) {
    CodecParameterDescriptor parameter;
    parameter.key = key;
    parameter.value_type = CodecParameterDescriptor::ValueType::kByteBuffer;
    parameter.byte_buffer_value = std::move(value);
    return parameter;
  };
  auto null_parameter = [](const std::string& key) {
    CodecParameterDescriptor parameter;
    parameter.key = key;
    parameter.value_type = CodecParameterDescriptor::ValueType::kNull;
    return parameter;
  };

  CodecParametersDescriptor audio_parameters;
  audio_parameters.parameters = {
      integer_parameter("audio-int", 7),
      long_parameter("audio-long", 9876543210LL),
      float_parameter("audio-float", 1.25f),
      string_parameter("audio-string", "music"),
      byte_buffer_parameter("audio-bytes", {0x01, 0x2A, 0xFF}),
      null_parameter("audio-null"),
  };
  CodecParametersDescriptor video_parameters;
  video_parameters.parameters = {
      string_parameter("video-string", "video"),
      byte_buffer_parameter("video-bytes", {0x10, 0x20}),
  };

  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player =
      ExoPlayerSdkPlayer::Create(env, context, config);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "codec-parameters-error:createPlayer",
        "nativeCodecParametersParitySmokeTest.error");
  }
  player->SetAudioCodecParameters(audio_parameters);
  player->SetVideoCodecParameters(video_parameters);
  player->Release();

  std::shared_ptr<ExoPlayerBridge> bridge = ExoPlayerBridge::Create(env, context, config);
  if (bridge == nullptr) {
    return NewStringUtfChecked(
        env,
        "codec-parameters-error:createBridge",
        "nativeCodecParametersParitySmokeTest.error");
  }
  bridge->SetAudioCodecParameters(env, audio_parameters);
  bridge->SetVideoCodecParameters(env, video_parameters);
  std::vector<std::string> flags = BridgeGetPlayerConfigFlagsForTest(env, bridge);
  bridge->Release(env);

  const std::string audio_summary = flags.size() > 20 ? flags[20] : "";
  const std::string video_summary = flags.size() > 21 ? flags[21] : "";
  const std::string expected_audio_summary =
      "audio-int=int:7;audio-long=long:9876543210;audio-float=float:1.25;"
      "audio-string=string:music;audio-bytes=bytes:3:012aff;audio-null=null";
  const std::string expected_video_summary =
      "video-string=string:video;video-bytes=bytes:2:1020";
  std::string summary = "audioCodec=" + audio_summary;
  summary += ",videoCodec=" + video_summary;
  summary += ",runtimeApplied=" +
      std::to_string(
          audio_summary == expected_audio_summary &&
                  video_summary == expected_video_summary
              ? 1
              : 0);
  return NewStringUtfChecked(env, summary, "nativeCodecParametersParitySmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeAuxiliaryCallbackParitySmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  auto integer_parameter = [](const std::string& key, int value) {
    CodecParameterDescriptor parameter;
    parameter.key = key;
    parameter.value_type = CodecParameterDescriptor::ValueType::kInteger;
    parameter.int_value = value;
    return parameter;
  };
  auto string_parameter = [](const std::string& key, const std::string& value) {
    CodecParameterDescriptor parameter;
    parameter.key = key;
    parameter.value_type = CodecParameterDescriptor::ValueType::kString;
    parameter.string_value = value;
    return parameter;
  };
  auto summarize_codec_parameters = [](const CodecParametersDescriptor& parameters) {
    std::string summary;
    static constexpr char kHex[] = "0123456789abcdef";
    for (size_t i = 0; i < parameters.parameters.size(); ++i) {
      const CodecParameterDescriptor& parameter = parameters.parameters[i];
      if (i > 0) {
        summary += ";";
      }
      summary += parameter.key + "=";
      switch (parameter.value_type) {
        case CodecParameterDescriptor::ValueType::kInteger:
          summary += "int:" + std::to_string(parameter.int_value);
          break;
        case CodecParameterDescriptor::ValueType::kLong:
          summary += "long:" + std::to_string(parameter.long_value);
          break;
        case CodecParameterDescriptor::ValueType::kFloat:
          summary += "float:" + std::to_string(parameter.float_value);
          break;
        case CodecParameterDescriptor::ValueType::kString:
          summary += "string:" + parameter.string_value;
          break;
        case CodecParameterDescriptor::ValueType::kByteBuffer:
          summary += "bytes:" +
              std::to_string(parameter.byte_buffer_value.size()) + ":";
          for (uint8_t value : parameter.byte_buffer_value) {
            summary.push_back(kHex[(value >> 4) & 0x0F]);
            summary.push_back(kHex[value & 0x0F]);
          }
          break;
        case CodecParameterDescriptor::ValueType::kNull:
          summary += "null";
          break;
      }
    }
    return summary;
  };
  class AuxiliaryCallbackCapturingListener : public PlayerListener {
   public:
    explicit AuxiliaryCallbackCapturingListener(
        std::string (*summarize)(const CodecParametersDescriptor&))
        : summarize_(summarize) {}

    void OnPlaybackStateChanged(const PlaybackSnapshot&) override {}
    void OnPlayWhenReadyChanged(const PlaybackSnapshot&, int) override {}
    void OnIsPlayingChanged(const PlaybackSnapshot&) override {}
    void OnMediaItemTransition(const PlaybackSnapshot&, int) override {}
    void OnPlayerError(const PlaybackSnapshot&) override {}

    void Reset() {
      audio_codec_callback_count = 0;
      video_codec_callback_count = 0;
      video_frame_callback_count = 0;
      camera_motion_callback_count = 0;
      camera_reset_callback_count = 0;
      audio_codec_summary.clear();
      video_codec_summary.clear();
      frame_mime.clear();
      frame_format_id.clear();
      frame_label.clear();
      frame_language.clear();
      frame_container_mime.clear();
      frame_bitrate = 0;
      frame_average_bitrate = 0;
      frame_peak_bitrate = 0;
      frame_rotation_degrees = 0;
      frame_pixel_width_height_ratio = 0.0f;
      frame_color_standard = 0;
      frame_color_range = 0;
      frame_color_transfer = 0;
      frame_channel_count = 0;
      frame_sample_rate = 0;
      frame_role_flags = 0;
      frame_selection_flags = 0;
      frame_media_format_present = false;
      frame_media_format_summary.clear();
      frame_media_format_mime.clear();
      frame_media_format_width = 0;
      frame_media_format_height = 0;
      frame_media_format_frame_rate = 0.0f;
      frame_media_format_rotation_degrees = 0;
      frame_media_format_color_standard = 0;
      frame_media_format_color_range = 0;
      frame_media_format_color_transfer = 0;
      camera_rotation_summary.clear();
    }

    void OnAudioCodecParametersChanged(
        const PlaybackSnapshot&,
        const CodecParametersDescriptor& codec_parameters) override {
      ++audio_codec_callback_count;
      audio_codec_summary = summarize_(codec_parameters);
    }

    void OnVideoCodecParametersChanged(
        const PlaybackSnapshot&,
        const CodecParametersDescriptor& codec_parameters) override {
      ++video_codec_callback_count;
      video_codec_summary = summarize_(codec_parameters);
    }

    void OnVideoFrameAboutToBeRendered(
        const PlaybackSnapshot&,
        const VideoFrameMetadataSnapshot& video_frame_metadata) override {
      ++video_frame_callback_count;
      frame_presentation_time_us = video_frame_metadata.presentation_time_us;
      frame_release_time_ns = video_frame_metadata.release_time_ns;
      frame_format_id = video_frame_metadata.format_id;
      frame_mime = video_frame_metadata.sample_mime_type;
      frame_width = video_frame_metadata.width;
      frame_height = video_frame_metadata.height;
      frame_rate = video_frame_metadata.frame_rate;
      frame_label = video_frame_metadata.format_label;
      frame_language = video_frame_metadata.format_language;
      frame_container_mime = video_frame_metadata.format_container_mime_type;
      frame_bitrate = video_frame_metadata.format_bitrate;
      frame_average_bitrate = video_frame_metadata.format_average_bitrate;
      frame_peak_bitrate = video_frame_metadata.format_peak_bitrate;
      frame_rotation_degrees = video_frame_metadata.format_rotation_degrees;
      frame_pixel_width_height_ratio =
          video_frame_metadata.format_pixel_width_height_ratio;
      frame_color_standard = video_frame_metadata.format_color_standard;
      frame_color_range = video_frame_metadata.format_color_range;
      frame_color_transfer = video_frame_metadata.format_color_transfer;
      frame_channel_count = video_frame_metadata.format_channel_count;
      frame_sample_rate = video_frame_metadata.format_sample_rate;
      frame_role_flags = video_frame_metadata.format_role_flags;
      frame_selection_flags = video_frame_metadata.format_selection_flags;
      frame_media_format_present = video_frame_metadata.media_format_present;
      frame_media_format_summary = video_frame_metadata.media_format_summary;
      frame_media_format_mime = video_frame_metadata.media_format_mime_type;
      frame_media_format_width = video_frame_metadata.media_format_width;
      frame_media_format_height = video_frame_metadata.media_format_height;
      frame_media_format_frame_rate =
          video_frame_metadata.media_format_frame_rate;
      frame_media_format_rotation_degrees =
          video_frame_metadata.media_format_rotation_degrees;
      frame_media_format_color_standard =
          video_frame_metadata.media_format_color_standard;
      frame_media_format_color_range =
          video_frame_metadata.media_format_color_range;
      frame_media_format_color_transfer =
          video_frame_metadata.media_format_color_transfer;
    }

    void OnCameraMotion(
        const PlaybackSnapshot&,
        const CameraMotionSnapshot& camera_motion) override {
      ++camera_motion_callback_count;
      camera_time_us = camera_motion.time_us;
      camera_rotation_summary.clear();
      for (size_t i = 0; i < camera_motion.rotation.size(); ++i) {
        if (i > 0) {
          camera_rotation_summary += ":";
        }
        camera_rotation_summary += std::to_string(camera_motion.rotation[i]);
      }
    }

    void OnCameraMotionReset(const PlaybackSnapshot&) override {
      ++camera_reset_callback_count;
    }

    std::string (*summarize_)(const CodecParametersDescriptor&);
    int audio_codec_callback_count = 0;
    int video_codec_callback_count = 0;
    int video_frame_callback_count = 0;
    int camera_motion_callback_count = 0;
    int camera_reset_callback_count = 0;
    std::string audio_codec_summary;
    std::string video_codec_summary;
    int64_t frame_presentation_time_us = 0;
    int64_t frame_release_time_ns = 0;
    std::string frame_format_id;
    std::string frame_mime;
    int frame_width = 0;
    int frame_height = 0;
    float frame_rate = 0.0f;
    std::string frame_label;
    std::string frame_language;
    std::string frame_container_mime;
    int frame_bitrate = 0;
    int frame_average_bitrate = 0;
    int frame_peak_bitrate = 0;
    int frame_rotation_degrees = 0;
    float frame_pixel_width_height_ratio = 0.0f;
    int frame_color_standard = 0;
    int frame_color_range = 0;
    int frame_color_transfer = 0;
    int frame_channel_count = 0;
    int frame_sample_rate = 0;
    int frame_role_flags = 0;
    int frame_selection_flags = 0;
    bool frame_media_format_present = false;
    std::string frame_media_format_summary;
    std::string frame_media_format_mime;
    int frame_media_format_width = 0;
    int frame_media_format_height = 0;
    float frame_media_format_frame_rate = 0.0f;
    int frame_media_format_rotation_degrees = 0;
    int frame_media_format_color_standard = 0;
    int frame_media_format_color_range = 0;
    int frame_media_format_color_transfer = 0;
    int64_t camera_time_us = 0;
    std::string camera_rotation_summary;
  };

  PlayerConfig config;
  bool bridge_codec_listener_registration_safe = false;
  {
    std::shared_ptr<ExoPlayerBridge> bridge =
        ExoPlayerBridge::Create(env, context, config);
    if (bridge != nullptr) {
      bridge->SetAudioCodecParametersChangeListener(
          env, {"codec-rate", "codec-mode"});
      bridge->ClearAudioCodecParametersChangeListener(env);
      bridge->SetVideoCodecParametersChangeListener(env, {"video-profile"});
      bridge->ClearVideoCodecParametersChangeListener(env);
      bridge->Release(env);
      bridge_codec_listener_registration_safe = true;
    }
  }

  std::unique_ptr<ExoPlayerSdkPlayer> player =
      ExoPlayerSdkPlayer::Create(env, context, config);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "aux-callback-error:createPlayer",
        "nativeAuxiliaryCallbackParitySmokeTest.error");
  }
  AuxiliaryCallbackCapturingListener listener(+summarize_codec_parameters);
  player->AddAudioCodecParametersChangeListener(
      &listener, {"codec-rate", "codec-mode"});
  player->AddVideoCodecParametersChangeListener(&listener, {"video-profile"});
  player->SetVideoFrameMetadataListener(&listener);
  player->SetCameraMotionListener(&listener);
  listener.Reset();

  CodecParametersDescriptor audio_parameters;
  audio_parameters.parameters = {
      integer_parameter("codec-rate", 60),
      string_parameter("codec-mode", "low-latency"),
      integer_parameter("ignored-audio", 99),
  };
  CodecParametersDescriptor video_parameters;
  video_parameters.parameters = {
      string_parameter("video-profile", "main"),
      integer_parameter("ignored-video", 7),
  };
  VideoFrameMetadataSnapshot frame;
  frame.presentation_time_us = 123456;
  frame.release_time_ns = 987654321;
  frame.format_id = "frame-format";
  frame.sample_mime_type = "video/avc";
  frame.codecs = "avc1.64001f";
  frame.width = 1920;
  frame.height = 1080;
  frame.frame_rate = 23.976f;
  frame.format_label = "Main Camera";
  frame.format_language = "en";
  frame.format_container_mime_type = "video/mp4";
  frame.format_average_bitrate = 222000;
  frame.format_peak_bitrate = 333000;
  frame.format_rotation_degrees = 180;
  frame.format_pixel_width_height_ratio = 1.5f;
  frame.format_color_standard = 1;
  frame.format_color_range = 2;
  frame.format_color_transfer = 3;
  frame.format_channel_count = 2;
  frame.format_sample_rate = 48000;
  frame.format_role_flags = 5;
  frame.format_selection_flags = 7;
  frame.media_format_present = true;
  frame.media_format_summary = "media-format-ok";
  frame.media_format_mime_type = "video/avc";
  frame.media_format_width = 1920;
  frame.media_format_height = 1080;
  frame.media_format_frame_rate = 23.976f;
  frame.media_format_rotation_degrees = 90;
  frame.media_format_color_standard = 1;
  frame.media_format_color_range = 2;
  frame.media_format_color_transfer = 3;
  CameraMotionSnapshot camera_motion;
  camera_motion.time_us = 654321;
  camera_motion.rotation = {1.0f, 2.0f, 3.0f};

  player->SimulateAudioCodecParametersChangedForTest(audio_parameters);
  player->SimulateVideoCodecParametersChangedForTest(video_parameters);
  player->SimulateVideoFrameAboutToBeRenderedForTest(frame);
  player->SimulateCameraMotionForTest(camera_motion);
  player->SimulateCameraMotionResetForTest();

  const int callbacks_before_remove =
      listener.audio_codec_callback_count +
      listener.video_codec_callback_count +
      listener.video_frame_callback_count +
      listener.camera_motion_callback_count +
      listener.camera_reset_callback_count;
  player->RemoveAudioCodecParametersChangeListener(&listener);
  player->RemoveVideoCodecParametersChangeListener(&listener);
  player->ClearVideoFrameMetadataListener(&listener);
  player->ClearCameraMotionListener(&listener);
  player->SimulateAudioCodecParametersChangedForTest(audio_parameters);
  player->SimulateVideoCodecParametersChangedForTest(video_parameters);
  player->SimulateVideoFrameAboutToBeRenderedForTest(frame);
  player->SimulateCameraMotionForTest(camera_motion);
  player->SimulateCameraMotionResetForTest();
  const int callbacks_after_remove =
      listener.audio_codec_callback_count +
      listener.video_codec_callback_count +
      listener.video_frame_callback_count +
      listener.camera_motion_callback_count +
      listener.camera_reset_callback_count;
  player->Release();

  std::string summary =
      "audioCodecCb=" + std::to_string(listener.audio_codec_callback_count);
  summary += ",audioCodec=" + listener.audio_codec_summary;
  summary += ",videoCodecCb=" + std::to_string(listener.video_codec_callback_count);
  summary += ",videoCodec=" + listener.video_codec_summary;
  summary += ",videoFrameCb=" + std::to_string(listener.video_frame_callback_count);
  summary += ",framePresentationUs=" +
      std::to_string(listener.frame_presentation_time_us);
  summary += ",frameReleaseNs=" + std::to_string(listener.frame_release_time_ns);
  summary += ",frameFormatId=" + listener.frame_format_id;
  summary += ",frameMime=" + listener.frame_mime;
  summary += ",frameSize=" + std::to_string(listener.frame_width) + "x" +
      std::to_string(listener.frame_height);
  summary += ",frameRate=" + std::to_string(listener.frame_rate);
  summary += ",frameLabel=" + listener.frame_label;
  summary += ",frameLanguage=" + listener.frame_language;
  summary += ",frameContainerMime=" + listener.frame_container_mime;
  summary += ",frameBitrates=" + std::to_string(listener.frame_bitrate) + ":" +
      std::to_string(listener.frame_average_bitrate) + ":" +
      std::to_string(listener.frame_peak_bitrate);
  summary += ",frameRotation=" +
      std::to_string(listener.frame_rotation_degrees);
  summary += ",framePixelRatio=" +
      std::to_string(listener.frame_pixel_width_height_ratio);
  summary += ",frameColor=" +
      std::to_string(listener.frame_color_standard) + ":" +
      std::to_string(listener.frame_color_range) + ":" +
      std::to_string(listener.frame_color_transfer);
  summary += ",frameAudioShape=" +
      std::to_string(listener.frame_channel_count) + ":" +
      std::to_string(listener.frame_sample_rate);
  summary += ",frameFlags=" + std::to_string(listener.frame_role_flags) + ":" +
      std::to_string(listener.frame_selection_flags);
  summary += ",frameMediaFormatPresent=" +
      std::to_string(listener.frame_media_format_present ? 1 : 0);
  summary += ",frameMediaFormatSummary=" + listener.frame_media_format_summary;
  summary += ",frameMediaFormatMime=" + listener.frame_media_format_mime;
  summary += ",frameMediaFormatSize=" +
      std::to_string(listener.frame_media_format_width) + "x" +
      std::to_string(listener.frame_media_format_height);
  summary += ",frameMediaFormatFrameRate=" +
      std::to_string(listener.frame_media_format_frame_rate);
  summary += ",frameMediaFormatRotation=" +
      std::to_string(listener.frame_media_format_rotation_degrees);
  summary += ",frameMediaFormatColor=" +
      std::to_string(listener.frame_media_format_color_standard) + ":" +
      std::to_string(listener.frame_media_format_color_range) + ":" +
      std::to_string(listener.frame_media_format_color_transfer);
  summary += ",cameraMotionCb=" +
      std::to_string(listener.camera_motion_callback_count);
  summary += ",cameraTimeUs=" + std::to_string(listener.camera_time_us);
  summary += ",cameraRotation=" + listener.camera_rotation_summary;
  summary += ",cameraResetCb=" +
      std::to_string(listener.camera_reset_callback_count);
  summary += ",afterRemoveStopped=" +
      std::to_string(callbacks_before_remove == callbacks_after_remove ? 1 : 0);
  summary += ",bridgeCodecRegistrationSafe=" +
      std::to_string(bridge_codec_listener_registration_safe ? 1 : 0);
  summary += ",callbackApplied=" +
      std::to_string(
          listener.audio_codec_callback_count == 1 &&
                  listener.audio_codec_summary ==
                      "codec-mode=string:low-latency;codec-rate=int:60" &&
                  listener.video_codec_callback_count == 1 &&
                  listener.video_codec_summary == "video-profile=string:main" &&
                  listener.video_frame_callback_count == 1 &&
                  listener.frame_presentation_time_us == 123456 &&
                  listener.frame_release_time_ns == 987654321 &&
                  listener.frame_format_id == "frame-format" &&
                  listener.frame_mime == "video/avc" &&
                  listener.frame_width == 1920 &&
                  listener.frame_height == 1080 &&
                  listener.frame_label == "Main Camera" &&
                  listener.frame_language == "en" &&
                  listener.frame_container_mime == "video/mp4" &&
                  listener.frame_bitrate == 333000 &&
                  listener.frame_average_bitrate == 222000 &&
                  listener.frame_peak_bitrate == 333000 &&
                  listener.frame_rotation_degrees == 180 &&
                  listener.frame_pixel_width_height_ratio == 1.5f &&
                  listener.frame_color_standard == 1 &&
                  listener.frame_color_range == 2 &&
                  listener.frame_color_transfer == 3 &&
                  listener.frame_channel_count == 2 &&
                  listener.frame_sample_rate == 48000 &&
                  listener.frame_role_flags == 5 &&
                  listener.frame_selection_flags == 7 &&
                  listener.frame_media_format_present &&
                  listener.frame_media_format_mime == "video/avc" &&
                  listener.frame_media_format_width == 1920 &&
                  listener.frame_media_format_height == 1080 &&
                  listener.frame_media_format_rotation_degrees == 90 &&
                  listener.frame_media_format_color_standard == 1 &&
                  listener.frame_media_format_color_range == 2 &&
                  listener.frame_media_format_color_transfer == 3 &&
                  listener.camera_motion_callback_count == 1 &&
                  listener.camera_time_us == 654321 &&
                  listener.camera_rotation_summary ==
                      "1.000000:2.000000:3.000000" &&
                  listener.camera_reset_callback_count == 1 &&
                  callbacks_before_remove == callbacks_after_remove &&
                  bridge_codec_listener_registration_safe
              ? 1
              : 0);
  return NewStringUtfChecked(env, summary, "nativeAuxiliaryCallbackParitySmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeVideoFrameMetadataSimulationFallbackSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  class VideoFrameFallbackCapturingListener : public PlayerListener {
   public:
    void OnPlaybackStateChanged(const PlaybackSnapshot&) override {}
    void OnPlayWhenReadyChanged(const PlaybackSnapshot&, int) override {}
    void OnIsPlayingChanged(const PlaybackSnapshot&) override {}
    void OnMediaItemTransition(const PlaybackSnapshot&, int) override {}
    void OnPlayerError(const PlaybackSnapshot&) override {}

    void OnVideoFrameAboutToBeRendered(
        const PlaybackSnapshot&,
        const VideoFrameMetadataSnapshot& video_frame_metadata) override {
      ++frame_callback_count;
      frame_bitrate = video_frame_metadata.format_bitrate;
      frame_average_bitrate = video_frame_metadata.format_average_bitrate;
      frame_peak_bitrate = video_frame_metadata.format_peak_bitrate;
      frame_color_standard = video_frame_metadata.format_color_standard;
      frame_color_range = video_frame_metadata.format_color_range;
      frame_color_transfer = video_frame_metadata.format_color_transfer;
      frame_channel_count = video_frame_metadata.format_channel_count;
      frame_sample_rate = video_frame_metadata.format_sample_rate;
    }

    int frame_callback_count = 0;
    int frame_bitrate = 0;
    int frame_average_bitrate = 0;
    int frame_peak_bitrate = 0;
    int frame_color_standard = 0;
    int frame_color_range = 0;
    int frame_color_transfer = 0;
    int frame_channel_count = 0;
    int frame_sample_rate = 0;
  };

  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player =
      ExoPlayerSdkPlayer::Create(env, context, config);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "video-frame-fallback-error:createPlayer",
        "nativeVideoFrameMetadataSimulationFallbackSmokeTest.error");
  }

  VideoFrameFallbackCapturingListener listener;
  player->SetVideoFrameMetadataListener(&listener);

  VideoFrameMetadataSnapshot frame;
  frame.presentation_time_us = 222333;
  frame.release_time_ns = 444555;
  frame.format_id = "fallback-format";
  frame.sample_mime_type = "video/avc";
  frame.codecs = "avc1.fallback";
  frame.width = 640;
  frame.height = 360;
  frame.frame_rate = 30.0f;
  frame.format_bitrate = 123000;
  frame.format_color_standard = 1;

  player->SimulateVideoFrameAboutToBeRenderedForTest(frame);
  player->ClearVideoFrameMetadataListener(&listener);
  player->Release();

  std::string summary =
      "frameCb=" + std::to_string(listener.frame_callback_count);
  summary += ",fallbackBitrates=" + std::to_string(listener.frame_bitrate) + ":" +
      std::to_string(listener.frame_average_bitrate) + ":" +
      std::to_string(listener.frame_peak_bitrate);
  summary += ",fallbackColor=" +
      std::to_string(listener.frame_color_standard) + ":" +
      std::to_string(listener.frame_color_range) + ":" +
      std::to_string(listener.frame_color_transfer);
  summary += ",fallbackAudioShape=" +
      std::to_string(listener.frame_channel_count) + ":" +
      std::to_string(listener.frame_sample_rate);
  summary += ",fallbackApplied=" +
      std::to_string(
          listener.frame_callback_count == 1 &&
                  listener.frame_bitrate == 123000 &&
                  listener.frame_average_bitrate == 123000 &&
                  listener.frame_peak_bitrate == -1 &&
                  listener.frame_color_standard == 1 &&
                  listener.frame_color_range == -1 &&
                  listener.frame_color_transfer == -1 &&
                  listener.frame_channel_count == -1 &&
                  listener.frame_sample_rate == -1
              ? 1
              : 0);
  return NewStringUtfChecked(
      env, summary, "nativeVideoFrameMetadataSimulationFallbackSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeCodecParametersMultiListenerParitySmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  auto integer_parameter = [](const std::string& key, int value) {
    CodecParameterDescriptor parameter;
    parameter.key = key;
    parameter.value_type = CodecParameterDescriptor::ValueType::kInteger;
    parameter.int_value = value;
    return parameter;
  };
  auto summarize_codec_parameters = [](const CodecParametersDescriptor& parameters) {
    std::string summary;
    for (size_t i = 0; i < parameters.parameters.size(); ++i) {
      const CodecParameterDescriptor& parameter = parameters.parameters[i];
      if (i > 0) {
        summary += ";";
      }
      summary += parameter.key + "=";
      if (parameter.value_type == CodecParameterDescriptor::ValueType::kInteger) {
        summary += "int:" + std::to_string(parameter.int_value);
      } else {
        summary += "other";
      }
    }
    return summary;
  };
  class CodecParameterCapturingListener : public PlayerListener {
   public:
    explicit CodecParameterCapturingListener(
        std::string (*summarize)(const CodecParametersDescriptor&))
        : summarize_(summarize) {}

    void OnPlaybackStateChanged(const PlaybackSnapshot&) override {}
    void OnPlayWhenReadyChanged(const PlaybackSnapshot&, int) override {}
    void OnIsPlayingChanged(const PlaybackSnapshot&) override {}
    void OnMediaItemTransition(const PlaybackSnapshot&, int) override {}
    void OnPlayerError(const PlaybackSnapshot&) override {}

    void Reset() {
      audio_count = 0;
      video_count = 0;
      audio_summary.clear();
      video_summary.clear();
    }

    void OnAudioCodecParametersChanged(
        const PlaybackSnapshot&,
        const CodecParametersDescriptor& codec_parameters) override {
      ++audio_count;
      audio_summary = summarize_(codec_parameters);
    }

    void OnVideoCodecParametersChanged(
        const PlaybackSnapshot&,
        const CodecParametersDescriptor& codec_parameters) override {
      ++video_count;
      video_summary = summarize_(codec_parameters);
    }

    std::string (*summarize_)(const CodecParametersDescriptor&);
    int audio_count = 0;
    int video_count = 0;
    std::string audio_summary;
    std::string video_summary;
  };

  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player =
      ExoPlayerSdkPlayer::Create(env, context, config);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "codec-multi-listener-error:createPlayer",
        "nativeCodecParametersMultiListenerParitySmokeTest.error");
  }

  CodecParameterCapturingListener audio_first(+summarize_codec_parameters);
  CodecParameterCapturingListener audio_second(+summarize_codec_parameters);
  CodecParameterCapturingListener video_first(+summarize_codec_parameters);
  CodecParameterCapturingListener video_second(+summarize_codec_parameters);
  player->AddAudioCodecParametersChangeListener(&audio_first, {"keyA", "keyB"});
  player->AddVideoCodecParametersChangeListener(&video_first, {"vKeyA", "vKeyB"});
  audio_first.Reset();
  video_first.Reset();

  player->AddAudioCodecParametersChangeListener(&audio_second, {"keyB", "keyC"});
  player->AddVideoCodecParametersChangeListener(&video_second, {"vKeyB", "vKeyC"});
  const int audio_first_after_second_add = audio_first.audio_count;
  const int audio_second_initial = audio_second.audio_count;
  const int video_first_after_second_add = video_first.video_count;
  const int video_second_initial = video_second.video_count;

  CodecParametersDescriptor audio_parameters;
  audio_parameters.parameters = {
      integer_parameter("keyA", 10),
      integer_parameter("keyB", 20),
      integer_parameter("keyC", 30),
  };
  CodecParametersDescriptor video_parameters;
  video_parameters.parameters = {
      integer_parameter("vKeyA", 100),
      integer_parameter("vKeyB", 200),
      integer_parameter("vKeyC", 300),
  };
  player->SimulateAudioCodecParametersChangedForTest(audio_parameters);
  player->SimulateVideoCodecParametersChangedForTest(video_parameters);
  const std::string audio_first_summary = audio_first.audio_summary;
  const std::string audio_second_summary = audio_second.audio_summary;
  const std::string video_first_summary = video_first.video_summary;
  const std::string video_second_summary = video_second.video_summary;
  const int audio_first_before_remove = audio_first.audio_count;
  const int audio_second_before_remove = audio_second.audio_count;
  const int video_first_before_remove = video_first.video_count;
  const int video_second_before_remove = video_second.video_count;

  player->RemoveAudioCodecParametersChangeListener(&audio_second);
  player->RemoveVideoCodecParametersChangeListener(&video_second);
  const int audio_first_after_remove_delta =
      audio_first.audio_count - audio_first_before_remove;
  const int video_first_after_remove_delta =
      video_first.video_count - video_first_before_remove;

  CodecParametersDescriptor next_audio_parameters;
  next_audio_parameters.parameters = {
      integer_parameter("keyA", 11),
      integer_parameter("keyB", 22),
      integer_parameter("keyC", 33),
  };
  CodecParametersDescriptor next_video_parameters;
  next_video_parameters.parameters = {
      integer_parameter("vKeyA", 101),
      integer_parameter("vKeyB", 202),
      integer_parameter("vKeyC", 303),
  };
  player->SimulateAudioCodecParametersChangedForTest(next_audio_parameters);
  player->SimulateVideoCodecParametersChangedForTest(next_video_parameters);
  const bool audio_second_after_remove_stopped =
      audio_second.audio_count == audio_second_before_remove;
  const bool video_second_after_remove_stopped =
      video_second.video_count == video_second_before_remove;
  player->Release();

  const bool applied =
      audio_first_after_second_add == 0 &&
      audio_second_initial == 1 &&
      audio_first_summary == "keyA=int:10;keyB=int:20" &&
      audio_second_summary == "keyB=int:20;keyC=int:30" &&
      audio_first_after_remove_delta == 0 &&
      audio_second_after_remove_stopped &&
      video_first_after_second_add == 0 &&
      video_second_initial == 1 &&
      video_first_summary == "vKeyA=int:100;vKeyB=int:200" &&
      video_second_summary == "vKeyB=int:200;vKeyC=int:300" &&
      video_first_after_remove_delta == 0 &&
      video_second_after_remove_stopped;

  std::string summary = "audioFirstAfterSecondAdd=" +
      std::to_string(audio_first_after_second_add);
  summary += ",audioSecondInitial=" + std::to_string(audio_second_initial);
  summary += ",audioFirst=" + audio_first_summary;
  summary += ",audioSecond=" + audio_second_summary;
  summary += ",audioFirstAfterRemoveDelta=" +
      std::to_string(audio_first_after_remove_delta);
  summary += ",audioSecondAfterRemoveStopped=" +
      std::to_string(audio_second_after_remove_stopped ? 1 : 0);
  summary += ",videoFirstAfterSecondAdd=" +
      std::to_string(video_first_after_second_add);
  summary += ",videoSecondInitial=" + std::to_string(video_second_initial);
  summary += ",videoFirst=" + video_first_summary;
  summary += ",videoSecond=" + video_second_summary;
  summary += ",videoFirstAfterRemoveDelta=" +
      std::to_string(video_first_after_remove_delta);
  summary += ",videoSecondAfterRemoveStopped=" +
      std::to_string(video_second_after_remove_stopped ? 1 : 0);
  summary += ",multiListenerApplied=" + std::to_string(applied ? 1 : 0);
  return NewStringUtfChecked(
      env, summary, "nativeCodecParametersMultiListenerParitySmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeRendererAndDeviceStateGetterSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player =
      ExoPlayerSdkPlayer::Create(env, context, config);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "renderer-state-error:createPlayer",
        "nativeRendererAndDeviceStateGetterSmokeTest.error");
  }
  const int renderer_count = player->GetRendererCount();
  const int first_renderer_type =
      renderer_count > 0 ? player->GetRendererType(0) : -1;
  const int invalid_renderer_type = player->GetRendererType(renderer_count + 1);
  const bool sleeping_for_offload = player->IsSleepingForOffload();
  const bool tunneling_enabled = player->IsTunnelingEnabled();
  const bool released_before = player->IsReleased();
  player->Release();

  std::string summary = "rendererCount=" + std::to_string(renderer_count);
  summary += ",firstRendererType=" + std::to_string(first_renderer_type);
  summary += ",invalidRendererType=" + std::to_string(invalid_renderer_type);
  summary += ",sleepingForOffload=" + std::to_string(sleeping_for_offload ? 1 : 0);
  summary += ",tunnelingEnabled=" + std::to_string(tunneling_enabled ? 1 : 0);
  summary += ",releasedBefore=" + std::to_string(released_before ? 1 : 0);
  summary += ",getterApplied=" +
      std::to_string(
          renderer_count >= 0 &&
                  (renderer_count == 0 || first_renderer_type >= 0) &&
                  invalid_renderer_type == -1 &&
                  !sleeping_for_offload &&
                  !tunneling_enabled &&
                  !released_before
              ? 1
              : 0);
  return NewStringUtfChecked(env, summary, "nativeRendererAndDeviceStateGetterSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePreloadConfigurationSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  player->SetPreloadConfiguration(654321);
  std::string summary = "targetPreloadDurationUs=" +
      std::to_string(player->GetTargetPreloadDurationUs());
  player->SetPreloadConfiguration(-9223372036854775807LL);
  summary += ",afterUnsetTargetPreloadDurationUs=" +
      std::to_string(player->GetTargetPreloadDurationUs());
  summary += ",unsetApplied=" +
      std::to_string(
          player->GetTargetPreloadDurationUs() == -9223372036854775807LL ? 1 : 0);
  return NewStringUtfChecked(env, summary, "nativePreloadConfigurationSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePreloadRoundTripSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  config.target_preload_duration_us = 111111;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  int64_t initial = player->GetTargetPreloadDurationUs();
  player->SetPreloadConfiguration(222222);
  int64_t after_update = player->GetTargetPreloadDurationUs();
  player->SetPreloadConfiguration(-9223372036854775807LL);
  int64_t after_unset = player->GetTargetPreloadDurationUs();
  player->SetPreloadConfiguration(333333);
  int64_t after_reset = player->GetTargetPreloadDurationUs();
  std::string summary = "initialTargetPreloadDurationUs=" + std::to_string(initial);
  summary += ",afterUpdateTargetPreloadDurationUs=" + std::to_string(after_update);
  summary += ",afterUnsetTargetPreloadDurationUs=" + std::to_string(after_unset);
  summary += ",afterResetTargetPreloadDurationUs=" + std::to_string(after_reset);
  summary += ",updateApplied=" + std::to_string(after_update == 222222 ? 1 : 0);
  summary += ",unsetApplied=" +
      std::to_string(after_unset == -9223372036854775807LL ? 1 : 0);
  summary += ",resetApplied=" + std::to_string(after_reset == 333333 ? 1 : 0);
  return NewStringUtfChecked(env, summary, "nativePreloadRoundTripSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePreloadBridgeRuntimeSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  config.target_preload_duration_us = 111111;
  std::shared_ptr<ExoPlayerBridge> bridge = ExoPlayerBridge::Create(env, context, config);
  if (bridge == nullptr) {
    return NewStringUtfChecked(
        env, "preload-bridge-error:createBridge", "nativePreloadBridgeRuntimeSmokeTest.error");
  }
  std::vector<std::string> initial_flags = BridgeGetPlayerConfigFlagsForTest(env, bridge);
  int64_t initial_direct = bridge->GetTargetPreloadDurationUs(env);
  bridge->SetPreloadConfiguration(env, 222222);
  std::vector<std::string> after_update_flags = BridgeGetPlayerConfigFlagsForTest(env, bridge);
  int64_t after_update_direct = bridge->GetTargetPreloadDurationUs(env);
  bridge->SetPreloadConfiguration(env, -9223372036854775807LL);
  std::vector<std::string> after_unset_flags = BridgeGetPlayerConfigFlagsForTest(env, bridge);
  int64_t after_unset_direct = bridge->GetTargetPreloadDurationUs(env);
  bridge->SetPreloadConfiguration(env, 333333);
  std::vector<std::string> after_reset_flags = BridgeGetPlayerConfigFlagsForTest(env, bridge);
  int64_t after_reset_direct = bridge->GetTargetPreloadDurationUs(env);
  std::string summary = "initialFlagTargetPreloadDurationUs=";
  summary += initial_flags.size() > 8 ? initial_flags[8] : "";
  summary += ",initialDirectTargetPreloadDurationUs=" + std::to_string(initial_direct);
  summary += ",afterUpdateFlagTargetPreloadDurationUs=";
  summary += after_update_flags.size() > 8 ? after_update_flags[8] : "";
  summary += ",afterUpdateDirectTargetPreloadDurationUs=" + std::to_string(after_update_direct);
  summary += ",afterUnsetFlagTargetPreloadDurationUs=";
  summary += after_unset_flags.size() > 8 ? after_unset_flags[8] : "";
  summary += ",afterUnsetDirectTargetPreloadDurationUs=" + std::to_string(after_unset_direct);
  summary += ",afterResetFlagTargetPreloadDurationUs=";
  summary += after_reset_flags.size() > 8 ? after_reset_flags[8] : "";
  summary += ",afterResetDirectTargetPreloadDurationUs=" + std::to_string(after_reset_direct);
  summary += ",flagAndDirectMatch=" + std::to_string(
      (initial_flags.size() > 8 && initial_flags[8] == std::to_string(initial_direct) &&
       after_update_flags.size() > 8 && after_update_flags[8] == std::to_string(after_update_direct) &&
       after_unset_flags.size() > 8 && after_unset_flags[8] == std::to_string(after_unset_direct) &&
       after_reset_flags.size() > 8 && after_reset_flags[8] == std::to_string(after_reset_direct))
          ? 1
          : 0);
  summary += ",updateApplied=" + std::to_string(after_update_direct == 222222 ? 1 : 0);
  summary += ",unsetApplied=" +
      std::to_string(after_unset_direct == -9223372036854775807LL ? 1 : 0);
  summary += ",resetApplied=" + std::to_string(after_reset_direct == 333333 ? 1 : 0);
  bridge->Release(env);
  return NewStringUtfChecked(env, summary, "nativePreloadBridgeRuntimeSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePriorityTaskManagerSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  config.priority = 77;
  config.use_priority_task_manager = true;
  std::shared_ptr<ExoPlayerBridge> bridge = ExoPlayerBridge::Create(env, context, config);
  if (bridge == nullptr) {
    return NewStringUtfChecked(
        env, "priority-task-manager-error:createBridge", "nativePriorityTaskManagerSmokeTest.error");
  }
  std::vector<std::string> initial = BridgeGetPriorityTaskManagerStateForTest(env, bridge);
  bridge->SetPriority(env, 88);
  std::vector<std::string> after_priority = BridgeGetPriorityTaskManagerStateForTest(env, bridge);
  bridge->SetPriorityTaskManagerEnabled(env, false);
  std::vector<std::string> after_disable = BridgeGetPriorityTaskManagerStateForTest(env, bridge);
  bridge->SetPriorityTaskManagerEnabled(env, true);
  std::vector<std::string> after_enable = BridgeGetPriorityTaskManagerStateForTest(env, bridge);
  std::string summary = "initialEnabled=";
  summary += initial.size() > 0 ? initial[0] : "";
  summary += ",initialAttached=";
  summary += initial.size() > 1 ? initial[1] : "";
  summary += ",initialRegistered=";
  summary += initial.size() > 2 ? initial[2] : "";
  summary += ",initialPriority=";
  summary += initial.size() > 3 ? initial[3] : "";
  summary += ",afterSetPriority=";
  summary += after_priority.size() > 3 ? after_priority[3] : "";
  summary += ",afterSetPriorityAttached=";
  summary += after_priority.size() > 1 ? after_priority[1] : "";
  summary += ",afterSetPriorityRegistered=";
  summary += after_priority.size() > 2 ? after_priority[2] : "";
  summary += ",afterDisableEnabled=";
  summary += after_disable.size() > 0 ? after_disable[0] : "";
  summary += ",afterDisableAttached=";
  summary += after_disable.size() > 1 ? after_disable[1] : "";
  summary += ",afterDisableRegistered=";
  summary += after_disable.size() > 2 ? after_disable[2] : "";
  summary += ",afterEnableEnabled=";
  summary += after_enable.size() > 0 ? after_enable[0] : "";
  summary += ",afterEnableAttached=";
  summary += after_enable.size() > 1 ? after_enable[1] : "";
  summary += ",afterEnableRegistered=";
  summary += after_enable.size() > 2 ? after_enable[2] : "";
  summary += ",afterEnablePriority=";
  summary += after_enable.size() > 3 ? after_enable[3] : "";
  bridge->Release(env);
  return NewStringUtfChecked(env, summary, "nativePriorityTaskManagerSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePlayerMessageSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/player-message.mp4";
  media_item.media_id = "player-message-item";
  player->SetMediaItem(media_item);
  PlayerMessageDescriptor message;
  message.target_type = PlayerMessageDescriptor::TargetType::kInternal;
  message.type = 42;
  message.payload = "payload-test";
  message.block_timeout_ms = 2000;
  PlayerMessageResult result = player->SendPlayerMessage(message);
  std::string summary = "delivered=" + std::to_string(result.delivered ? 1 : 0);
  summary += ",timedOut=" + std::to_string(result.timed_out ? 1 : 0);
  summary += ",canceled=" + std::to_string(result.canceled ? 1 : 0);
  summary += ",deliveryCount=" + std::to_string(result.delivery_count);
  summary += ",type=" + std::to_string(result.type);
  summary += ",payload=" + result.payload;
  summary += ",mediaItemIndex=" + std::to_string(result.media_item_index);
  summary += ",positionMs=" + std::to_string(result.position_ms);
  summary += ",deleteAfterDelivery=" +
      std::to_string(result.delete_after_delivery ? 1 : 0);
  summary += ",thread=" + result.thread_name;
  return NewStringUtfChecked(env, summary, "nativePlayerMessageSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeTimedPlayerMessageSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = BuildSilentWavDataUri();
  media_item.media_id = "player-message-timed-item";
  media_item.mime_type = "audio/wav";
  media_item.source_type = MediaSourceType::kProgressive;
  std::string runtime_summary =
      BuildRuntimeDrivenPlayerSummary(player.get(), media_item, true, 5000);
  bool playback_advanced = WaitForCurrentPositionAtLeast(player.get(), 100, 5000);
  int64_t scheduled_position_ms = std::max<int64_t>(player->GetCurrentPosition() + 100, 100);
  PlayerMessageDescriptor message;
  message.target_type = PlayerMessageDescriptor::TargetType::kInternal;
  message.type = 42;
  message.payload = "payload-test";
  message.media_item_index = 0;
  message.position_ms = scheduled_position_ms;
  message.block_timeout_ms = 5000;
  PlayerMessageResult result = player->SendPlayerMessage(message);
  std::string summary = "delivered=" + std::to_string(result.delivered ? 1 : 0);
  summary += ",timedOut=" + std::to_string(result.timed_out ? 1 : 0);
  summary += ",canceled=" + std::to_string(result.canceled ? 1 : 0);
  summary += ",deliveryCount=" + std::to_string(result.delivery_count);
  summary += ",type=" + std::to_string(result.type);
  summary += ",payload=" + result.payload;
  summary += ",mediaItemIndex=" + std::to_string(result.media_item_index);
  summary += ",positionMs=" + std::to_string(result.position_ms);
  summary += ",deleteAfterDelivery=" +
      std::to_string(result.delete_after_delivery ? 1 : 0);
  summary += ",thread=" + result.thread_name;
  summary += ",playbackAdvanced=" + std::to_string(playback_advanced ? 1 : 0);
  summary += ",scheduledPositionMs=" + std::to_string(scheduled_position_ms);
  summary += "," + runtime_summary;
  return NewStringUtfChecked(env, summary, "nativeTimedPlayerMessageSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativeRendererPlayerMessageSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri =
      "https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4";
  media_item.media_id = "player-message-renderer-item";
  std::string runtime_summary =
      BuildRuntimeDrivenPlayerSummary(player.get(), media_item, true, 2000);
  bool playback_advanced = WaitForCurrentPositionAtLeast(player.get(), 1500, 5000);
  PlayerMessageDescriptor message;
  message.target_type = PlayerMessageDescriptor::TargetType::kVideoRenderer;
  message.type = 42;
  message.payload = "payload-test";
  message.media_item_index = 0;
  message.position_ms = 1234;
  message.block_timeout_ms = 2000;
  PlayerMessageResult result = player->SendPlayerMessage(message);
  std::string summary = "delivered=" + std::to_string(result.delivered ? 1 : 0);
  summary += ",timedOut=" + std::to_string(result.timed_out ? 1 : 0);
  summary += ",canceled=" + std::to_string(result.canceled ? 1 : 0);
  summary += ",deliveryCount=" + std::to_string(result.delivery_count);
  summary += ",type=" + std::to_string(result.type);
  summary += ",payload=" + result.payload;
  summary += ",mediaItemIndex=" + std::to_string(result.media_item_index);
  summary += ",positionMs=" + std::to_string(result.position_ms);
  summary += ",deleteAfterDelivery=" +
      std::to_string(result.delete_after_delivery ? 1 : 0);
  summary += ",thread=" + result.thread_name;
  summary += ",playbackAdvanced=" + std::to_string(playback_advanced ? 1 : 0);
  summary += "," + runtime_summary;
  return NewStringUtfChecked(env, summary, "nativeRendererPlayerMessageSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativePlayerTestHelper_nativePlayerMessageCancelSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> player = ExoPlayerSdkPlayer::Create(env, context, config);
  MediaItemDescriptor media_item;
  media_item.uri = "https://example.com/player-message-cancel.mp4";
  media_item.media_id = "player-message-cancel-item";
  player->SetMediaItem(media_item);
  PlayerMessageDescriptor message;
  message.target_type = PlayerMessageDescriptor::TargetType::kInternal;
  message.type = 43;
  message.payload = "payload-cancel";
  message.position_ms = 10000;
  message.media_item_index = 0;
  message.delete_after_delivery = false;
  message.cancel_after_send = true;
  message.block_timeout_ms = 2000;
  PlayerMessageResult result = player->SendPlayerMessage(message);
  std::string summary = "delivered=" + std::to_string(result.delivered ? 1 : 0);
  summary += ",timedOut=" + std::to_string(result.timed_out ? 1 : 0);
  summary += ",canceled=" + std::to_string(result.canceled ? 1 : 0);
  summary += ",deliveryCount=" + std::to_string(result.delivery_count);
  summary += ",type=" + std::to_string(result.type);
  summary += ",payload=" + result.payload;
  return NewStringUtfChecked(env, summary, "nativePlayerMessageCancelSmokeTest");
}
}  // extern "C"
