#ifndef ANDROIDX_MEDIA3_EXOPLAYER_CPPBRIDGE_EXOPLAYER_SDK_H_
#define ANDROIDX_MEDIA3_EXOPLAYER_CPPBRIDGE_EXOPLAYER_SDK_H_

#include "exoplayer_bridge.h"

#include <memory>
#include <vector>

namespace androidx::media3::cppbridge {

class ExoPlayerSdkPriorityTaskManager {
 public:
  static std::unique_ptr<ExoPlayerSdkPriorityTaskManager> Create(JNIEnv* env);

  virtual ~ExoPlayerSdkPriorityTaskManager() = default;

  virtual void Release() = 0;
  virtual void Add(int priority) = 0;
  virtual void Remove(int priority) = 0;
  virtual bool ProceedNonBlocking(int priority) = 0;
  virtual jobject GetJavaObjectLocalRef(JNIEnv* env) = 0;
};

class ExoPlayerSdkImageOutputListener {
 public:
  virtual ~ExoPlayerSdkImageOutputListener() = default;

  virtual void OnImageAvailable(const ImageFrameSnapshot& image_frame) = 0;
  virtual void OnDisabled() {}
};

class ExoPlayerSdkPlayer {
 public:
  static std::unique_ptr<ExoPlayerSdkPlayer> Create(
      JNIEnv* env,
      jobject context,
      const PlayerConfig& config);

  virtual ~ExoPlayerSdkPlayer() = default;

  virtual void Release() = 0;
  virtual void SetListener(PlayerListener* listener) = 0;
  virtual void RemoveListener(PlayerListener* listener) = 0;
  virtual void SetImageOutputListener(ExoPlayerSdkImageOutputListener* listener) = 0;
  virtual void RemoveImageOutputListener(ExoPlayerSdkImageOutputListener* listener) = 0;
  // Analytics listeners receive the analytics-prefixed callbacks exposed by the
  // bridge. Standard PlayerListener callbacks continue to flow through SetListener.
  // Some state changes are intentionally exposed on both channels, so consumers
  // listening to both should deduplicate at a higher level if they merge them.
  virtual void AddAnalyticsListener(PlayerListener* listener) = 0;
  virtual void RemoveAnalyticsListener(PlayerListener* listener) = 0;
  virtual void AddAudioCodecParametersChangeListener(
      PlayerListener* listener,
      const std::vector<std::string>& keys) = 0;
  virtual void RemoveAudioCodecParametersChangeListener(PlayerListener* listener) = 0;
  virtual void AddVideoCodecParametersChangeListener(
      PlayerListener* listener,
      const std::vector<std::string>& keys) = 0;
  virtual void RemoveVideoCodecParametersChangeListener(PlayerListener* listener) = 0;
  virtual void SetVideoFrameMetadataListener(PlayerListener* listener) = 0;
  virtual void ClearVideoFrameMetadataListener(PlayerListener* listener) = 0;
  virtual void SetCameraMotionListener(PlayerListener* listener) = 0;
  virtual void ClearCameraMotionListener(PlayerListener* listener) = 0;
  virtual void BindPlayerView(jobject player_view) = 0;
  virtual void UnbindPlayerView(jobject player_view) = 0;
  virtual void SetVideoSurface(jobject surface) = 0;
  virtual void ClearVideoSurface() = 0;
  virtual void ClearVideoSurface(jobject surface) = 0;
  virtual void SetVideoSurfaceHolder(jobject surface_holder) = 0;
  virtual void ClearVideoSurfaceHolder(jobject surface_holder) = 0;
  virtual void SetVideoSurfaceView(jobject surface_view) = 0;
  virtual void ClearVideoSurfaceView(jobject surface_view) = 0;
  virtual void SetVideoTextureView(jobject texture_view) = 0;
  virtual void ClearVideoTextureView(jobject texture_view) = 0;
  virtual void SetVideoEffects(const std::vector<VideoEffectDescriptor>& video_effects) = 0;
  virtual void SetMediaItem(const MediaItemDescriptor& media_item) = 0;
  virtual void SetMediaItem(const MediaItemDescriptor& media_item, bool reset_position) = 0;
  virtual void SetMediaItem(
      const MediaItemDescriptor& media_item,
      int64_t start_position_ms) = 0;
  virtual void SetMediaItems(
      const std::vector<MediaItemDescriptor>& media_items,
      int start_index,
      int64_t start_position_ms) = 0;
  virtual void SetMediaItems(
      const std::vector<MediaItemDescriptor>& media_items,
      bool reset_position) = 0;
  virtual void AddMediaItem(const MediaItemDescriptor& media_item) = 0;
  virtual void AddMediaItem(int index, const MediaItemDescriptor& media_item) = 0;
  virtual void AddMediaItems(const std::vector<MediaItemDescriptor>& media_items) = 0;
  virtual void AddMediaItems(
      int index,
      const std::vector<MediaItemDescriptor>& media_items) = 0;
  virtual void RemoveMediaItem(int index) = 0;
  virtual void RemoveMediaItems(int from_index, int to_index) = 0;
  virtual void MoveMediaItem(int current_index, int new_index) = 0;
  virtual void MoveMediaItems(int from_index, int to_index, int new_index) = 0;
  virtual void ReplaceMediaItems(
      int from_index,
      int to_index,
      const std::vector<MediaItemDescriptor>& media_items) = 0;
  virtual void ReplaceMediaItem(int index, const MediaItemDescriptor& media_item) = 0;
  virtual void ClearMediaItems() = 0;
  virtual void Prepare() = 0;
  virtual void Play() = 0;
  virtual void Pause() = 0;
  virtual void Stop() = 0;
  virtual void SeekTo(int64_t position_ms) = 0;
  virtual void SeekToMediaItem(int media_item_index, int64_t position_ms) = 0;
  virtual void SeekBack() = 0;
  virtual void SeekForward() = 0;
  virtual void SeekToDefaultPosition() = 0;
  virtual void SeekToDefaultPosition(int media_item_index) = 0;
  virtual void SetSeekParameters(const SeekParametersDescriptor& seek_parameters) = 0;
  virtual SeekParametersDescriptor GetSeekParameters() = 0;
  virtual void SeekToNext() = 0;
  virtual void SeekToPrevious() = 0;
  virtual void SeekToNextMediaItem() = 0;
  virtual void SeekToPreviousMediaItem() = 0;
  virtual void SetWakeMode(int wake_mode) = 0;
  virtual void SetHandleAudioBecomingNoisy(bool handle_audio_becoming_noisy) = 0;
  virtual void SetPriority(int priority) = 0;
  virtual void SetPriorityTaskManager(ExoPlayerSdkPriorityTaskManager* priority_task_manager) = 0;
  virtual void ClearPriorityTaskManager() = 0;
  virtual void SetPriorityTaskManagerEnabled(bool enabled) = 0;
  virtual void SetPreloadConfiguration(int64_t target_preload_duration_us) = 0;
  virtual void SetForegroundMode(bool foreground_mode) = 0;
  virtual PlayerMessageResult SendPlayerMessage(const PlayerMessageDescriptor& message) = 0;
  virtual void SetImageOutputEnabled(bool enabled) = 0;
  virtual void SetAudioAttributes(
      const AudioAttributesDescriptor& attributes,
      bool handle_audio_focus) = 0;
  virtual void SetAudioSessionId(int audio_session_id) = 0;
  virtual void SetAuxEffectInfo(const AuxEffectInfoDescriptor& aux_effect_info) = 0;
  virtual void ClearAuxEffectInfo() = 0;
  virtual void SetPreferredAudioDevice(jobject audio_device_info) = 0;
  virtual void ClearPreferredAudioDevice() = 0;
  virtual void SetVirtualDeviceId(int virtual_device_id) = 0;
  virtual void SetAudioCodecParameters(
      const CodecParametersDescriptor& codec_parameters) = 0;
  virtual void SetVideoCodecParameters(
      const CodecParametersDescriptor& codec_parameters) = 0;
  virtual void SetDeviceVolume(int volume, int flags) = 0;
  virtual void AdjustDeviceVolume(int direction, int flags) = 0;
  virtual void IncreaseDeviceVolume(int flags) = 0;
  virtual void DecreaseDeviceVolume(int flags) = 0;
  virtual void SetDeviceMuted(bool muted, int flags) = 0;
  virtual void SetSkipSilenceEnabled(bool skip_silence_enabled) = 0;
  virtual void SetScrubbingModeEnabled(bool scrubbing_mode_enabled) = 0;
  virtual bool IsScrubbingModeEnabled() = 0;
  virtual void SetScrubbingModeParameters(
      const ScrubbingModeParametersDescriptor& parameters) = 0;
  virtual ScrubbingModeParametersDescriptor GetScrubbingModeParameters() = 0;
  virtual void SetPlayWhenReady(bool play_when_ready) = 0;
  virtual void SetRepeatMode(RepeatMode repeat_mode) = 0;
  virtual void SetShuffleModeEnabled(bool shuffle_mode_enabled) = 0;
  virtual void SetVolume(float volume) = 0;
  virtual void SetPlaybackSpeed(float speed) = 0;
  virtual void SetPlaybackParameters(const PlaybackParametersSnapshot& parameters) = 0;
  virtual void SetPauseAtEndOfMediaItems(bool pause_at_end_of_media_items) = 0;
  virtual bool GetPauseAtEndOfMediaItems() = 0;
  virtual void SetSeekBackIncrementMs(int64_t seek_back_increment_ms) = 0;
  virtual void SetSeekForwardIncrementMs(int64_t seek_forward_increment_ms) = 0;
  virtual void SetMaxSeekToPreviousPositionMs(int64_t max_seek_to_previous_position_ms) = 0;
  virtual void SetVideoScalingMode(int video_scaling_mode) = 0;
  virtual int GetVideoScalingMode() = 0;
  virtual void SetVideoChangeFrameRateStrategy(int video_change_frame_rate_strategy) = 0;
  virtual int GetVideoChangeFrameRateStrategy() = 0;
  virtual void SetTrackSelectionParameters(
      const TrackSelectionParametersDescriptor& parameters) = 0;
  virtual TrackSelectionParametersDescriptor GetTrackSelectionParameters() = 0;
  virtual int GetRendererCount() = 0;
  virtual int GetRendererType(int index) = 0;
  virtual TracksSnapshot GetTracks() = 0;
  virtual std::vector<TrackGroupSnapshot> GetTrackGroups() = 0;
  virtual PlaybackState GetPlaybackState() = 0;
  virtual bool GetPlayWhenReady() = 0;
  virtual bool IsPlaying() = 0;
  virtual bool IsLoading() = 0;
  virtual PlayerError GetPlayerError() = 0;
  virtual int64_t GetCurrentPosition() = 0;
  virtual int64_t GetBufferedPosition() = 0;
  virtual int64_t GetDuration() = 0;
  virtual int GetCurrentMediaItemIndex() = 0;
  virtual int GetMediaItemCount() = 0;
  virtual RepeatMode GetRepeatMode() = 0;
  virtual bool GetShuffleModeEnabled() = 0;
  virtual float GetVolume() = 0;
  virtual AudioAttributesDescriptor GetAudioAttributes() = 0;
  virtual DeviceInfoDescriptor GetDeviceInfo() = 0;
  virtual int GetDeviceVolume() = 0;
  virtual bool IsDeviceMuted() = 0;
  virtual bool GetSkipSilenceEnabled() = 0;
  virtual VideoSizeSnapshot GetVideoSize() = 0;
  virtual int GetNextMediaItemIndex() = 0;
  virtual int GetPreviousMediaItemIndex() = 0;
  virtual bool HasNextMediaItem() = 0;
  virtual bool HasPreviousMediaItem() = 0;
  virtual int GetBufferedPercentage() = 0;
  virtual int64_t GetContentBufferedPosition() = 0;
  virtual int64_t GetContentDuration() = 0;
  virtual int64_t GetContentPosition() = 0;
  virtual int64_t GetCurrentLiveOffset() = 0;
  virtual int GetCurrentPeriodIndex() = 0;
  virtual int64_t GetMaxSeekToPreviousPosition() = 0;
  virtual PlaybackSuppressionReason GetPlaybackSuppressionReason() = 0;
  virtual int64_t GetSeekBackIncrement() = 0;
  virtual int64_t GetSeekForwardIncrement() = 0;
  virtual int64_t GetTotalBufferedDuration() = 0;
  virtual int64_t GetTargetPreloadDurationUs() = 0;
  virtual bool IsCommandAvailable(int command_code) = 0;
  virtual bool CanAdvertiseSession() = 0;
  virtual ApplicationLooperDescriptor GetApplicationLooper() = 0;
  virtual bool IsSleepingForOffload() = 0;
  virtual bool IsTunnelingEnabled() = 0;
  virtual bool IsReleased() = 0;
  virtual int GetCurrentAdGroupIndex() = 0;
  virtual int GetCurrentAdIndexInAdGroup() = 0;
  virtual bool IsCurrentMediaItemDynamic() = 0;
  virtual bool IsCurrentMediaItemLive() = 0;
  virtual bool IsCurrentMediaItemSeekable() = 0;
  virtual bool IsPlayingAd() = 0;
  virtual void SetPlaylistMetadata(const MediaMetadataSnapshot& metadata) = 0;
  virtual MediaMetadataSnapshot GetMediaMetadata() = 0;
  virtual MediaMetadataSnapshot GetPlaylistMetadata() = 0;
  virtual PlaybackParametersSnapshot GetPlaybackParameters() = 0;
  virtual MediaItemDescriptor GetMediaItemAt(int index) = 0;
  virtual MediaItemDescriptor GetCurrentMediaItem() = 0;
  // Releases opaque bridge tokens after the caller has finished consuming the
  // associated descriptors/snapshots. Tokens should not be released while the
  // caller still needs to round-trip those opaque objects back into Java.
  virtual void ReleaseOpaqueObjectTokens(const std::vector<std::string>& tokens) = 0;
  virtual AnalyticsSnapshot GetAnalyticsSnapshot() = 0;
  virtual void SimulateAnalyticsUpdateForTest(const AnalyticsSnapshot& analytics) = 0;
  virtual void SimulateAudioUnderrunForTest(
      const AudioUnderrunEvent& audio_underrun) = 0;
  virtual void SimulateDroppedVideoFramesForTest(
      const DroppedVideoFramesEvent& dropped_video_frames) = 0;
  virtual void SimulateBandwidthEstimateForTest(
      const BandwidthEstimateEvent& bandwidth_estimate) = 0;
  virtual void SimulateLoadStartedForTest(
      const LoadStartedEvent& load_started) = 0;
  virtual void SimulateLoadCompletedForTest(
      const LoadCompletedEvent& load_completed) = 0;
  virtual void SimulateAnalyticsLoadErrorForTest(
      const AnalyticsLoadErrorEvent& load_error) = 0;
  virtual void SimulateAudioInputFormatChangedForTest(
      const AudioInputFormatChangedEvent& audio_input_format_changed) = 0;
  virtual void SimulateAudioDecoderInitializedForTest(
      const AudioDecoderInitializedEvent& audio_decoder_initialized) = 0;
  virtual void SimulateVideoDecoderInitializedForTest(
      const VideoDecoderInitializedEvent& video_decoder_initialized) = 0;
  virtual void SimulateAudioDecoderReleasedForTest(
      const AudioDecoderReleasedEvent& audio_decoder_released) = 0;
  virtual void SimulateVideoDecoderReleasedForTest(
      const VideoDecoderReleasedEvent& video_decoder_released) = 0;
  virtual void SimulateAnalyticsRenderedFirstFrameForTest(
      const AnalyticsRenderedFirstFrameEvent& rendered_first_frame) = 0;
  virtual void SimulateAnalyticsVideoSizeChangedForTest(
      const AnalyticsVideoSizeChangedEvent& analytics_video_size) = 0;
  virtual void SimulateAudioPositionAdvancingForTest(
      const AudioPositionAdvancingEvent& audio_position_advancing) = 0;
  virtual void SimulateVideoFrameProcessingOffsetForTest(
      const VideoFrameProcessingOffsetEvent& video_frame_processing_offset) = 0;
  virtual void SimulateVolumeChangedForTest(
      const VolumeChangedEvent& volume_changed) = 0;
  virtual void SimulateAudioSessionIdChangedForTest(
      const AudioSessionIdChangedEvent& audio_session_id_changed) = 0;
  virtual void SimulateAnalyticsAudioAttributesChangedForTest(
      const AudioAttributesDescriptor& attributes) = 0;
  virtual void SimulateAnalyticsSkipSilenceEnabledChangedForTest(
      const AnalyticsSkipSilenceEnabledChangedEvent& skip_silence_enabled_changed) = 0;
  virtual void SimulateAnalyticsDeviceVolumeChangedForTest(
      const AnalyticsDeviceVolumeChangedEvent& device_volume_changed) = 0;
  virtual void SimulateAnalyticsPlaybackStateChangedForTest(
      const AnalyticsPlaybackStateChangedEvent& playback_state_changed) = 0;
  virtual void SimulateAnalyticsIsPlayingChangedForTest(
      const AnalyticsIsPlayingChangedEvent& is_playing_changed) = 0;
  virtual void SimulateAnalyticsPlayWhenReadyChangedForTest(
      const AnalyticsPlayWhenReadyChangedEvent& play_when_ready_changed) = 0;
  virtual void SimulateAnalyticsPlaybackSuppressionReasonChangedForTest(
      const AnalyticsPlaybackSuppressionReasonChangedEvent& suppression_reason_changed) = 0;
  virtual void SimulateAnalyticsIsLoadingChangedForTest(
      const AnalyticsIsLoadingChangedEvent& is_loading_changed) = 0;
  virtual void SimulateAnalyticsRepeatModeChangedForTest(
      const AnalyticsRepeatModeChangedEvent& repeat_mode_changed) = 0;
  virtual void SimulateAnalyticsShuffleModeChangedForTest(
      const AnalyticsShuffleModeChangedEvent& shuffle_mode_changed) = 0;
  virtual void SimulateAnalyticsPlaybackParametersChangedForTest(
      const AnalyticsPlaybackParametersChangedEvent& playback_parameters_changed) = 0;
  virtual void SimulateAnalyticsAvailableCommandsChangedForTest(
      const AnalyticsAvailableCommandsChangedEvent& available_commands_changed) = 0;
  virtual void SimulateAnalyticsEventsForTest(
      const AnalyticsEventsEvent& analytics_events) = 0;
  virtual void SimulateIsLoadingChangedForTest(bool is_loading) = 0;
  virtual void SimulateSeekBackIncrementChangedForTest(
      int64_t seek_back_increment_ms) = 0;
  virtual void SimulateSeekForwardIncrementChangedForTest(
      int64_t seek_forward_increment_ms) = 0;
  virtual void SimulateMaxSeekToPreviousPositionChangedForTest(
      int64_t max_seek_to_previous_position_ms) = 0;
  virtual void SimulateAnalyticsSeekBackIncrementChangedForTest(
      const AnalyticsSeekBackIncrementChangedEvent& seek_back_increment_changed) = 0;
  virtual void SimulateAnalyticsSeekForwardIncrementChangedForTest(
      const AnalyticsSeekForwardIncrementChangedEvent& seek_forward_increment_changed) = 0;
  virtual void SimulateAnalyticsMaxSeekToPreviousPositionChangedForTest(
      const AnalyticsMaxSeekToPreviousPositionChangedEvent&
          max_seek_to_previous_position_changed) = 0;
  virtual void SimulateAnalyticsTimelineChangedForTest(
      const AnalyticsTimelineChangedEvent& timeline_changed) = 0;
  virtual void SimulateAnalyticsPositionDiscontinuityForTest(
      const AnalyticsPositionDiscontinuityEvent& position_discontinuity) = 0;
  virtual void SimulateAnalyticsSeekStartedForTest(
      const AnalyticsSeekStartedEvent& seek_started) = 0;
  virtual void SimulateAnalyticsPlayerErrorForTest(
      const PlayerError& error) = 0;
  virtual void SimulateAnalyticsPlayerErrorChangedForTest(
      const PlayerError& error) = 0;
  virtual void SimulateAnalyticsTracksChangedForTest(
      const TracksSnapshot& tracks) = 0;
  virtual void SimulateAnalyticsMediaItemTransitionForTest(
      const AnalyticsMediaItemTransitionEvent& media_item_transition) = 0;
  virtual void SimulateAnalyticsCuesForTest(
      const CueSnapshot& cues) = 0;
  virtual void SimulateCurrentCuesForTest(
      const CueSnapshot& cues) = 0;
  virtual void SimulateAnalyticsMetadataForTest(
      const AnalyticsMetadataEvent& metadata) = 0;
  virtual void SimulateAnalyticsDeviceInfoChangedForTest(
      const DeviceInfoDescriptor& device_info) = 0;
  virtual void SimulateAnalyticsMediaMetadataChangedForTest(
      const MediaMetadataSnapshot& metadata) = 0;
  virtual void SimulateAnalyticsPlaylistMetadataChangedForTest(
      const MediaMetadataSnapshot& metadata) = 0;
  virtual void SimulateVideoInputFormatChangedForTest(
      const VideoInputFormatChangedEvent& video_input_format_changed) = 0;
  virtual void SimulateAudioCodecParametersChangedForTest(
      const CodecParametersDescriptor& codec_parameters) = 0;
  virtual void SimulateVideoCodecParametersChangedForTest(
      const CodecParametersDescriptor& codec_parameters) = 0;
  virtual void SimulateVideoFrameAboutToBeRenderedForTest(
      const VideoFrameMetadataSnapshot& video_frame_metadata) = 0;
  virtual void SimulateCameraMotionForTest(
      const CameraMotionSnapshot& camera_motion) = 0;
  virtual void SimulateCameraMotionResetForTest() = 0;
  virtual void SimulateImageOutputForTest(const ImageFrameSnapshot& image_frame) = 0;
  virtual PlayerConfig::MediaSourceFactoryConfig GetMediaSourceFactoryConfig() = 0;
  virtual int GetAvailableCommandCount() = 0;
  virtual AvailableCommandsSnapshot GetAvailableCommands() = 0;
  virtual TimelineDetailsSnapshot GetTimeline() = 0;
  virtual TimelineSnapshot GetTimelineSnapshot() = 0;
  virtual std::vector<TimelineWindowSnapshot> GetTimelineWindows() = 0;
  virtual std::vector<TimelinePeriodSnapshot> GetTimelinePeriods() = 0;
  virtual CueSnapshot GetCurrentCues() = 0;
  virtual std::string GetCurrentMediaItemDebugSummary() = 0;
  virtual PlaybackSnapshot GetSnapshot() = 0;
};

template <typename SnapshotType>
struct OpaqueTokenBatch {
  SnapshotType value;
  std::vector<std::string> tokens;

  void Release(ExoPlayerSdkPlayer* player) {
    if (player == nullptr || tokens.empty()) {
      return;
    }
    player->ReleaseOpaqueObjectTokens(tokens);
    tokens.clear();
  }
};

// Convenience wrappers for the most common public query APIs that return
// token-bearing reduced descriptors. These helpers keep token collection close
// to the originating query so callers are less likely to forget the matching
// explicit release call.
//
// Example usage:
//   auto current_item = GetCurrentMediaItemWithOpaqueTokens(player.get());
//   UseMediaItem(current_item.value);
//   current_item.Release(player.get());
//
// Automatic scope/lease cleanup is intentionally not enabled yet. The current
// bridge still expects callers to release tokens only after they are finished
// with any round-trip back into Java.
inline OpaqueTokenBatch<MediaItemDescriptor> GetCurrentMediaItemWithOpaqueTokens(
    ExoPlayerSdkPlayer* player) {
  MediaItemDescriptor media_item = player->GetCurrentMediaItem();
  return {media_item, CollectOpaqueObjectTokens(media_item)};
}

inline OpaqueTokenBatch<MediaItemDescriptor> GetMediaItemAtWithOpaqueTokens(
    ExoPlayerSdkPlayer* player,
    int index) {
  MediaItemDescriptor media_item = player->GetMediaItemAt(index);
  return {media_item, CollectOpaqueObjectTokens(media_item)};
}

inline OpaqueTokenBatch<MediaMetadataSnapshot> GetMediaMetadataWithOpaqueTokens(
    ExoPlayerSdkPlayer* player) {
  MediaMetadataSnapshot metadata = player->GetMediaMetadata();
  return {metadata, CollectOpaqueObjectTokens(metadata)};
}

inline OpaqueTokenBatch<MediaMetadataSnapshot> GetPlaylistMetadataWithOpaqueTokens(
    ExoPlayerSdkPlayer* player) {
  MediaMetadataSnapshot metadata = player->GetPlaylistMetadata();
  return {metadata, CollectOpaqueObjectTokens(metadata)};
}

inline OpaqueTokenBatch<TracksSnapshot> GetTracksWithOpaqueTokens(
    ExoPlayerSdkPlayer* player) {
  TracksSnapshot tracks = player->GetTracks();
  return {tracks, CollectOpaqueObjectTokens(tracks)};
}

inline OpaqueTokenBatch<TimelineDetailsSnapshot> GetTimelineWithOpaqueTokens(
    ExoPlayerSdkPlayer* player) {
  TimelineDetailsSnapshot timeline = player->GetTimeline();
  return {timeline, CollectOpaqueObjectTokens(timeline)};
}

inline OpaqueTokenBatch<CueSnapshot> GetCurrentCuesWithOpaqueTokens(
    ExoPlayerSdkPlayer* player) {
  CueSnapshot cues = player->GetCurrentCues();
  return {cues, CollectOpaqueObjectTokens(cues)};
}

class ExoPlayerSdkPlayerBuilder {
 public:
  ExoPlayerSdkPlayerBuilder() = default;

  ExoPlayerSdkPlayerBuilder& SetHandleAudioFocus(bool handle_audio_focus);
  ExoPlayerSdkPlayerBuilder& SetHandleAudioBecomingNoisy(bool handle_audio_becoming_noisy);
  ExoPlayerSdkPlayerBuilder& SetUseLazyPreparation(bool use_lazy_preparation);
  ExoPlayerSdkPlayerBuilder& SetSeekBackIncrementMs(int64_t seek_back_increment_ms);
  ExoPlayerSdkPlayerBuilder& SetSeekForwardIncrementMs(int64_t seek_forward_increment_ms);
  ExoPlayerSdkPlayerBuilder& SetWakeMode(int wake_mode);
  ExoPlayerSdkPlayerBuilder& SetPriority(int priority);
  ExoPlayerSdkPlayerBuilder& SetUsePriorityTaskManager(bool use_priority_task_manager);
  ExoPlayerSdkPlayerBuilder& SetTargetPreloadDurationUs(int64_t target_preload_duration_us);
  ExoPlayerSdkPlayerBuilder& SetParseSubtitlesDuringExtraction(
      bool parse_subtitles_during_extraction);
  ExoPlayerSdkPlayerBuilder& SetLoadOnlySelectedTracks(bool load_only_selected_tracks);
  ExoPlayerSdkPlayerBuilder& SetDefaultRequestHeaders(
      const std::vector<std::string>& header_names,
      const std::vector<std::string>& header_values);
  ExoPlayerSdkPlayerBuilder& SetUserAgent(const std::string& user_agent);
  ExoPlayerSdkPlayerBuilder& SetConnectTimeoutMs(int connect_timeout_ms);
  ExoPlayerSdkPlayerBuilder& SetReadTimeoutMs(int read_timeout_ms);
  ExoPlayerSdkPlayerBuilder& SetAllowCrossProtocolRedirects(bool allow_cross_protocol_redirects);
  ExoPlayerSdkPlayerBuilder& SetLiveTargetOffsetMs(int64_t live_target_offset_ms);
  ExoPlayerSdkPlayerBuilder& SetLiveOffsetsMs(
      int64_t live_min_offset_ms,
      int64_t live_max_offset_ms);
  ExoPlayerSdkPlayerBuilder& SetLiveSpeeds(float live_min_speed, float live_max_speed);
  ExoPlayerSdkPlayerBuilder& SetMediaSourceFactoryToken(const std::string& factory_token);
  ExoPlayerSdkPlayerBuilder& SetMediaSourceFactoryConfig(
      const PlayerConfig::MediaSourceFactoryConfig& media_source_factory_config);

  const PlayerConfig& GetConfig() const;
  std::unique_ptr<ExoPlayerSdkPlayer> Build(JNIEnv* env, jobject context) const;

 private:
  PlayerConfig config_;
};

}  // namespace androidx::media3::cppbridge

#endif  // ANDROIDX_MEDIA3_EXOPLAYER_CPPBRIDGE_EXOPLAYER_SDK_H_
