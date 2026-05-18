#ifndef ANDROIDX_MEDIA3_EXOPLAYER_CPPBRIDGE_JNI_INTERNAL_H_
#define ANDROIDX_MEDIA3_EXOPLAYER_CPPBRIDGE_JNI_INTERNAL_H_

#include "include/exoplayer_bridge.h"
#include "include/exoplayer_sdk.h"

#include <memory>
#include <string>
#include <vector>

namespace androidx::media3::cppbridge::internal {

class JniExoPlayerBridge;

void RegisterBridge(const std::shared_ptr<JniExoPlayerBridge>& bridge);
void UnregisterBridge(const JniExoPlayerBridge* bridge);
void LogInfo(const std::string& message);
void LogError(const std::string& message);
bool ClearJniExceptionIfPresent(JNIEnv* env, const std::string& context);
void DeleteLocalRefIfNotNull(JNIEnv* env, jobject object);
jclass FindClassChecked(JNIEnv* env, const char* class_name);
jclass GetObjectClassChecked(JNIEnv* env, jobject object, const std::string& context);
jmethodID GetMethodChecked(
    JNIEnv* env,
    jclass clazz,
    const char* class_name,
    const char* method_name,
    const char* signature);
std::string JStringToString(JNIEnv* env, jstring value);
jstring NewStringUtfChecked(JNIEnv* env, const std::string& value, const std::string& context);

template <typename... Args>
jobject NewObjectChecked(
    JNIEnv* env,
    jclass clazz,
    jmethodID constructor,
    const std::string& context,
    Args... args) {
  if (clazz == nullptr || constructor == nullptr) {
    return nullptr;
  }
  jobject object = env->NewObject(clazz, constructor, args...);
  if (ClearJniExceptionIfPresent(env, "NewObject(" + context + ")")) {
    return nullptr;
  }
  return object;
}

std::string BuildTrackSummary(const std::vector<TrackGroupSnapshot>& groups);
std::string BuildPlaylistIdsSummary(const std::vector<MediaItemDescriptor>& media_items);
std::string BuildSnapshotSummary(const PlaybackSnapshot& snapshot);
std::string BuildTrackSelectionSummary(const TrackSelectionParametersDescriptor& parameters);

class ScopedEnv {
 public:
  explicit ScopedEnv(
      JavaVM* java_vm,
      const char* get_env_error_context = nullptr,
      const char* attach_error_context = nullptr);
  ~ScopedEnv();

  JNIEnv* env() const { return env_; }
  bool ok() const { return env_ != nullptr; }

 private:
  JavaVM* java_vm_;
  JNIEnv* env_ = nullptr;
  bool attached_by_scope_ = false;
};

jintArray CreateJavaIntArray(JNIEnv* env, const std::vector<int>& values);
jbyteArray CreateJavaByteArray(JNIEnv* env, const std::vector<uint8_t>& values);
jfloatArray CreateJavaFloatArray(JNIEnv* env, const std::vector<float>& values);
jobjectArray CreateJavaStringArray(JNIEnv* env, const std::vector<std::string>& values);
jobjectArray CreateJavaVideoEffectArray(
    JNIEnv* env,
    const std::vector<VideoEffectDescriptor>& video_effects);
jobject CreateJavaMediaItem(JNIEnv* env, const MediaItemDescriptor& media_item);
jobjectArray CreateJavaMediaItemArray(
    JNIEnv* env,
    const std::vector<MediaItemDescriptor>& media_items);
jobject CreateJavaMediaMetadata(JNIEnv* env, const MediaMetadataSnapshot& metadata);
jobjectArray CreateJavaCueArray(JNIEnv* env, const CueSnapshot& cues);
jobject CreateJavaTracks(JNIEnv* env, const TracksSnapshot& tracks);
std::vector<std::string> JStringArrayToVector(JNIEnv* env, jobjectArray values);
std::vector<uint8_t> JByteArrayToVector(JNIEnv* env, jbyteArray values);
std::vector<float> JFloatArrayToVector(JNIEnv* env, jfloatArray values);
std::vector<MediaItemDescriptor> JStringArrayToMediaItems(JNIEnv* env, jobjectArray urls);
AudioAttributesDescriptor FromJavaAudioAttributes(JNIEnv* env, jintArray values);
MediaItemDescriptor FromJavaMediaItem(JNIEnv* env, jobject object);
TrackSelectionParametersDescriptor FromJavaTrackSelectionParameters(JNIEnv* env, jobject object);
TracksSnapshot FromJavaTracks(JNIEnv* env, jobject object);
CueSnapshot FromJavaCues(JNIEnv* env, jobjectArray cues_array, int64_t presentation_time_us);
PositionInfoSnapshot FromJavaPositionInfo(JNIEnv* env, jobject object);
AvailableCommandsSnapshot FromJavaCommands(JNIEnv* env, jobject object);
PlayerEventsSnapshot FromJavaPlayerEvents(JNIEnv* env, jobject object);
DeviceInfoDescriptor FromJavaDeviceInfo(JNIEnv* env, jobject object);
VideoSizeSnapshot FromJavaVideoSize(JNIEnv* env, jobject object);
MediaMetadataSnapshot FromJavaMediaMetadata(JNIEnv* env, jobject object);
SeekParametersDescriptor FromJavaSeekParameters(JNIEnv* env, jobject object);
PlaybackParametersSnapshot FromJavaPlaybackParameters(JNIEnv* env, jobject object);
ApplicationLooperDescriptor FromJavaApplicationLooper(JNIEnv* env, jobject object);
CodecParametersDescriptor FromJavaCodecParameterArray(JNIEnv* env, jobjectArray values);
int ParseIntOrDefault(const std::string& value, int fallback);
int64_t ParseLongOrDefault(const std::string& value, int64_t fallback);
float ParseFloatOrDefault(const std::string& value, float fallback);
std::vector<std::string> SplitString(const std::string& value, char delimiter);
PlaybackState ToPlaybackState(int state);
PlaybackSuppressionReason ToPlaybackSuppressionReason(int reason);

std::shared_ptr<JniExoPlayerBridge> AcquireBridge(jlong native_handle);
PlayerListener& GetDemoLoggingPlayerListener();
void RegisterDemoPlayer(ExoPlayerSdkPlayer* player);
ExoPlayerSdkPlayer* AcquireDemoPlayer(jlong native_handle);
void UnregisterDemoPlayer(ExoPlayerSdkPlayer* player);
void BridgeOnPlaybackStateChanged(jlong native_handle, int playback_state);
void BridgeOnPlayWhenReadyChanged(jlong native_handle, bool play_when_ready, int reason);
void BridgeOnIsPlayingChanged(jlong native_handle, bool is_playing);
void BridgeOnIsLoadingChanged(jlong native_handle, bool is_loading);
void BridgeOnMediaItemTransition(jlong native_handle, int media_item_index, int reason);
void BridgeOnPlayerError(JNIEnv* env, jlong native_handle, int error_code, jstring message);
void BridgeOnPlayerErrorChanged(JNIEnv* env, jlong native_handle, int error_code, jstring message);
void BridgeOnTimelineChanged(jlong native_handle, int window_count, int period_count, int reason);
void BridgeOnTracksChanged(jlong native_handle);
void BridgeOnPositionDiscontinuity(
    JNIEnv* env,
    jlong native_handle,
    jobject old_position,
    jobject new_position,
    int reason);
void BridgeOnAudioAttributesChanged(
    jlong native_handle,
    int content_type,
    int usage,
    int flags,
    int allowed_capture_policy,
    int spatialization_behavior);
void BridgeOnCues(jlong native_handle, int cue_count, int64_t presentation_time_us);
void BridgeOnRepeatModeChanged(jlong native_handle, int repeat_mode);
void BridgeOnShuffleModeEnabledChanged(jlong native_handle, bool shuffle_mode_enabled);
void BridgeOnSeekBackIncrementChanged(jlong native_handle, int64_t seek_back_increment_ms);
void BridgeOnSeekForwardIncrementChanged(jlong native_handle, int64_t seek_forward_increment_ms);
void BridgeOnMaxSeekToPreviousPositionChanged(
    jlong native_handle,
    int64_t max_seek_to_previous_position_ms);
void BridgeOnTrackSelectionParametersChanged(jlong native_handle);
void BridgeOnPlaybackParametersChanged(jlong native_handle, float speed, float pitch);
void BridgeOnPlaybackSuppressionReasonChanged(jlong native_handle, int playback_suppression_reason);
void BridgeOnAvailableCommandsChanged(JNIEnv* env, jlong native_handle, jobject commands_object);
void BridgeOnEvents(JNIEnv* env, jlong native_handle, jobject events_object);
void BridgeOnDeviceInfoChanged(JNIEnv* env, jlong native_handle, jobject device_info_object);
void BridgeOnDeviceVolumeChanged(jlong native_handle, int volume, bool muted);
void BridgeOnSkipSilenceEnabledChanged(jlong native_handle, bool skip_silence_enabled);
void BridgeOnVideoSizeChanged(JNIEnv* env, jlong native_handle, jobject video_size_object);
void BridgeOnSurfaceSizeChanged(jlong native_handle, int width, int height);
void BridgeOnRenderedFirstFrame(jlong native_handle);
void BridgeOnMediaMetadataChanged(JNIEnv* env, jlong native_handle, jobject metadata_object);
void BridgeOnPlaylistMetadataChanged(JNIEnv* env, jlong native_handle, jobject metadata_object);
void BridgeOnAnalyticsUpdated(jlong native_handle);
void BridgeOnAudioUnderrun(
    jlong native_handle,
    int buffer_size,
    int64_t buffer_size_ms,
    int64_t elapsed_since_last_feed_ms);
void BridgeOnDroppedVideoFrames(
    jlong native_handle,
    int dropped_frames,
    int64_t elapsed_ms);
void BridgeOnBandwidthEstimate(
    jlong native_handle,
    int elapsed_ms,
    int64_t bytes_transferred,
    int64_t bitrate_estimate);
void BridgeOnLoadStarted(
    JNIEnv* env,
    jlong native_handle,
    jstring uri,
    int data_type,
    int track_type,
    int retry_count);
void BridgeOnLoadCompleted(
    JNIEnv* env,
    jlong native_handle,
    jstring uri,
    int data_type,
    int track_type);
void BridgeOnAudioInputFormatChanged(
    JNIEnv* env,
    jlong native_handle,
    jstring sample_mime_type,
    jstring codecs,
    int channel_count,
    int sample_rate);
void BridgeOnAudioDecoderInitialized(
    JNIEnv* env,
    jlong native_handle,
    jstring decoder_name,
    int64_t initialized_timestamp_ms,
    int64_t initialization_duration_ms);
void BridgeOnVideoDecoderInitialized(
    JNIEnv* env,
    jlong native_handle,
    jstring decoder_name,
    int64_t initialized_timestamp_ms,
    int64_t initialization_duration_ms);
void BridgeOnAudioDecoderReleased(
    JNIEnv* env,
    jlong native_handle,
    jstring decoder_name);
void BridgeOnVideoDecoderReleased(
    JNIEnv* env,
    jlong native_handle,
    jstring decoder_name);
void BridgeOnAnalyticsRenderedFirstFrame(
    jlong native_handle,
    int64_t render_time_ms);
void BridgeOnAnalyticsVideoSizeChanged(
    jlong native_handle,
    int width,
    int height,
    float pixel_width_height_ratio);
void BridgeOnAudioPositionAdvancing(
    jlong native_handle,
    int64_t playout_start_system_time_ms);
void BridgeOnVideoFrameProcessingOffset(
    jlong native_handle,
    int64_t total_processing_offset_us,
    int frame_count);
void BridgeOnVolumeChanged(
    jlong native_handle,
    float volume);
void BridgeOnAudioSessionIdChanged(
    jlong native_handle,
    int audio_session_id);
void BridgeOnAnalyticsSkipSilenceEnabledChanged(
    jlong native_handle,
    bool skip_silence_enabled);
void BridgeOnAnalyticsDeviceVolumeChanged(
    jlong native_handle,
    int volume,
    bool muted);
void BridgeOnAnalyticsPlaybackStateChanged(
    jlong native_handle,
    int playback_state);
void BridgeOnAnalyticsIsPlayingChanged(
    jlong native_handle,
    bool is_playing);
void BridgeOnAnalyticsPlayWhenReadyChanged(
    jlong native_handle,
    bool play_when_ready,
    int reason);
void BridgeOnAnalyticsPlaybackSuppressionReasonChanged(
    jlong native_handle,
    int playback_suppression_reason);
void BridgeOnAnalyticsIsLoadingChanged(
    jlong native_handle,
    bool is_loading);
void BridgeOnAnalyticsRepeatModeChanged(
    jlong native_handle,
    int repeat_mode);
void BridgeOnAnalyticsShuffleModeChanged(
    jlong native_handle,
    bool shuffle_mode_enabled);
void BridgeOnAnalyticsPlaybackParametersChanged(
    jlong native_handle,
    float speed,
    float pitch);
void BridgeOnAnalyticsAvailableCommandsChanged(
    JNIEnv* env,
    jlong native_handle,
    jobject commands);
void BridgeOnAnalyticsEvents(
    JNIEnv* env,
    jlong native_handle,
    jobject events);
void BridgeOnAnalyticsSeekBackIncrementChanged(
    jlong native_handle,
    int64_t seek_back_increment_ms);
void BridgeOnAnalyticsSeekForwardIncrementChanged(
    jlong native_handle,
    int64_t seek_forward_increment_ms);
void BridgeOnAnalyticsMaxSeekToPreviousPositionChanged(
    jlong native_handle,
    int64_t max_seek_to_previous_position_ms);
void BridgeOnAnalyticsTimelineChanged(
    jlong native_handle,
    int reason);
void BridgeOnAnalyticsPositionDiscontinuity(
    jlong native_handle,
    int reason);
void BridgeOnAnalyticsSeekStarted(
    jlong native_handle);
void BridgeOnAnalyticsPlayerError(
    JNIEnv* env,
    jlong native_handle,
    int error_code,
    jstring message);
void BridgeOnAnalyticsPlayerErrorChanged(
    JNIEnv* env,
    jlong native_handle,
    int error_code,
    jstring message);
void BridgeOnAnalyticsTracksChanged(
    JNIEnv* env,
    jlong native_handle,
    jobject tracks);
void BridgeOnAnalyticsMediaItemTransition(
    JNIEnv* env,
    jlong native_handle,
    jobject media_item,
    int reason);
void BridgeOnAnalyticsCues(
    JNIEnv* env,
    jlong native_handle,
    jobjectArray cues,
    int64_t presentation_time_us);
void BridgeOnAnalyticsMetadata(
    JNIEnv* env,
    jlong native_handle,
    jint entry_count,
    jstring first_entry_type,
    jstring first_entry_text);
void BridgeOnAnalyticsLoadError(
    JNIEnv* env,
    jlong native_handle,
    jstring uri,
    jint data_type,
    jint track_type,
    jstring message,
    jboolean was_canceled);
void BridgeOnAnalyticsDeviceInfoChanged(
    JNIEnv* env,
    jlong native_handle,
    jobject device_info);
void BridgeOnAnalyticsMediaMetadataChanged(
    JNIEnv* env,
    jlong native_handle,
    jobject metadata);
void BridgeOnAnalyticsPlaylistMetadataChanged(
    JNIEnv* env,
    jlong native_handle,
    jobject metadata);
void BridgeOnVideoInputFormatChanged(
    JNIEnv* env,
    jlong native_handle,
    jstring sample_mime_type,
    jstring codecs,
    int width,
    int height,
    float frame_rate);
void BridgeOnAudioCodecParametersChanged(
    JNIEnv* env,
    jlong native_handle,
    jobjectArray codec_parameters);
void BridgeOnVideoCodecParametersChanged(
    JNIEnv* env,
    jlong native_handle,
    jobjectArray codec_parameters);
void BridgeOnVideoFrameAboutToBeRendered(
    JNIEnv* env,
    jlong native_handle,
    int64_t presentation_time_us,
    int64_t release_time_ns,
    jstring format_id,
    jstring sample_mime_type,
    jstring codecs,
    int width,
    int height,
    float frame_rate,
    jstring format_label,
    jstring format_language,
    jstring format_container_mime_type,
    int format_bitrate,
    int format_average_bitrate,
    int format_peak_bitrate,
    int format_rotation_degrees,
    float format_pixel_width_height_ratio,
    int format_color_standard,
    int format_color_range,
    int format_color_transfer,
    int format_channel_count,
    int format_sample_rate,
    int format_role_flags,
    int format_selection_flags,
    bool media_format_present,
    jstring media_format_summary,
    jstring media_format_mime_type,
    int media_format_width,
    int media_format_height,
    float media_format_frame_rate,
    int media_format_rotation_degrees,
    int media_format_color_standard,
    int media_format_color_range,
    int media_format_color_transfer);
void BridgeOnCameraMotion(
    JNIEnv* env,
    jlong native_handle,
    int64_t time_us,
    jfloatArray rotation);
void BridgeOnCameraMotionReset(jlong native_handle);
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
    jstring bitmap_config);
void BridgeOnImageOutputDisabled(jlong native_handle);

std::vector<std::string> BridgeGetPlayerConfigFlagsForTest(
    JNIEnv* env,
    const std::shared_ptr<ExoPlayerBridge>& bridge);
std::vector<std::string> BridgeGetPriorityTaskManagerStateForTest(
    JNIEnv* env,
    const std::shared_ptr<ExoPlayerBridge>& bridge);
std::string BridgeSummarizeVideoEffectsForTest(
    JNIEnv* env,
    const std::shared_ptr<ExoPlayerBridge>& bridge,
    const std::vector<VideoEffectDescriptor>& video_effects);

}  // namespace androidx::media3::cppbridge::internal

#endif  // ANDROIDX_MEDIA3_EXOPLAYER_CPPBRIDGE_JNI_INTERNAL_H_
