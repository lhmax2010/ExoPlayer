#ifndef ANDROIDX_MEDIA3_EXOPLAYER_CPPBRIDGE_EXOPLAYER_BRIDGE_H_
#define ANDROIDX_MEDIA3_EXOPLAYER_CPPBRIDGE_EXOPLAYER_BRIDGE_H_

#include <jni.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace androidx::media3::cppbridge {

enum class PlaybackState {
  kIdle = 1,
  kBuffering = 2,
  kReady = 3,
  kEnded = 4,
};

enum class RepeatMode {
  kOff = 0,
  kOne = 1,
  kAll = 2,
};

enum class PlaybackSuppressionReason {
  kNone = 0,
  kTransientAudioFocusLoss = 1,
  kUnsuitableAudioRoute = 2,
  kUnsuitableAudioOutput = 3,
  kScrubbing = 4,
};

enum class MediaSourceType {
  kDefault = 0,
  kDash = 1,
  kHls = 2,
  kSmoothStreaming = 3,
  kRtsp = 4,
  kProgressive = 5,
};

struct PlayerConfig {
  struct MediaSourceFactoryConfig {
    std::string factory_token;
    bool injected_factory_used_for_test = false;
    int injected_factory_identity_for_test = 0;
    bool parse_subtitles_during_extraction = true;
    bool load_only_selected_tracks = false;
    std::vector<std::string> default_request_header_names;
    std::vector<std::string> default_request_header_values;
    std::string user_agent;
    int connect_timeout_ms = -1;
    int read_timeout_ms = -1;
    bool allow_cross_protocol_redirects = false;
    int64_t live_target_offset_ms = -9223372036854775807LL;
    int64_t live_min_offset_ms = -9223372036854775807LL;
    int64_t live_max_offset_ms = -9223372036854775807LL;
    float live_min_speed = -3.4028235e38f;
    float live_max_speed = -3.4028235e38f;
  };
  bool handle_audio_focus = true;
  bool handle_audio_becoming_noisy = true;
  bool use_lazy_preparation = true;
  MediaSourceFactoryConfig media_source_factory_config;
  int64_t seek_back_increment_ms = 5000;
  int64_t seek_forward_increment_ms = 15000;
  int wake_mode = 0;
  int priority = 0;
  bool use_priority_task_manager = false;
  int64_t target_preload_duration_us = -9223372036854775807LL;
};

struct PlayerMessageDescriptor {
  enum class TargetType {
    kInternal = 0,
    kAudioRenderer = 1,
    kVideoRenderer = 2,
    kTextRenderer = 3,
    kImageRenderer = 4,
  };
  TargetType target_type = TargetType::kInternal;
  int type = 0;
  std::string payload;
  int media_item_index = -1;
  int64_t position_ms = -9223372036854775807LL;
  bool delete_after_delivery = true;
  bool cancel_after_send = false;
  int64_t block_timeout_ms = 1000;
};

struct PlayerMessageResult {
  bool delivered = false;
  bool timed_out = false;
  bool canceled = false;
  int delivery_count = 0;
  int type = 0;
  std::string payload;
  int media_item_index = -1;
  int64_t position_ms = -9223372036854775807LL;
  bool delete_after_delivery = true;
  std::string thread_name;
};

struct MediaMetadataSnapshot {
  std::string title;
  std::string title_token;
  std::string artist;
  std::string artist_token;
  std::string album_title;
  std::string album_title_token;
  std::string album_artist;
  std::string album_artist_token;
  std::string display_title;
  std::string display_title_token;
  std::string subtitle;
  std::string subtitle_token;
  std::string description;
  std::string description_token;
  std::string artwork_uri;
  std::vector<uint8_t> artwork_data;
  int artwork_data_type = -1;
  int64_t duration_ms = -1;
  int track_number = -1;
  int total_track_count = -1;
  int is_browsable = -1;
  int is_playable = -1;
  int folder_type = -1;
  int recording_year = -1;
  int recording_month = -1;
  int recording_day = -1;
  int release_year = -1;
  int release_month = -1;
  int release_day = -1;
  std::string writer;
  std::string writer_token;
  std::string author;
  std::string author_token;
  std::string composer;
  std::string composer_token;
  std::string conductor;
  std::string conductor_token;
  int disc_number = -1;
  int total_disc_count = -1;
  std::string genre;
  std::string genre_token;
  std::string compilation;
  std::string compilation_token;
  int media_type = -1;
  std::string station;
  std::string station_token;
  bool extras_present = false;
  int extras_key_count = 0;
  std::string extras_token;
};

struct MediaItemDescriptor {
  std::string uri;
  std::string media_id;
  std::string mime_type;
  std::string custom_cache_key;
  MediaSourceType source_type = MediaSourceType::kDefault;
  bool tag_present = false;
  std::string tag_string;
  std::string tag_token;
  MediaMetadataSnapshot media_metadata;
  struct RequestMetadataDescriptor {
    std::string media_uri;
    std::string search_query;
    bool extras_present = false;
    int extras_key_count = 0;
    std::string extras_token;
  };
  RequestMetadataDescriptor request_metadata;
  struct AdsConfigurationDescriptor {
    std::string ad_tag_uri;
    std::string ads_id;
    std::string ads_id_token;
  };
  AdsConfigurationDescriptor ads_configuration;
  struct SubtitleConfigurationDescriptor {
    std::string uri;
    std::string mime_type;
    std::string language;
    std::string label;
    std::string id;
    int selection_flags = 0;
    int role_flags = 0;
  };
  struct ClippingConfigurationDescriptor {
    int64_t start_position_ms = 0;
    int64_t end_position_ms = -9223372036854775807LL;
    bool relative_to_live_window = false;
    bool relative_to_default_position = false;
    bool starts_at_key_frame = false;
    bool allow_unseekable_media = false;
  };
  struct LiveConfigurationDescriptor {
    int64_t target_offset_ms = -9223372036854775807LL;
    int64_t min_offset_ms = -9223372036854775807LL;
    int64_t max_offset_ms = -9223372036854775807LL;
    float min_playback_speed = -3.4028235e38f;
    float max_playback_speed = -3.4028235e38f;
  };
  struct DrmConfigurationDescriptor {
    std::string scheme_uuid;
    std::string license_uri;
    std::vector<std::string> license_request_header_names;
    std::vector<std::string> license_request_header_values;
    std::vector<int> forced_session_track_types;
    std::vector<uint8_t> key_set_id;
    bool multi_session = false;
    bool force_default_license_uri = false;
    bool play_clear_content_without_key = true;
  };
  std::vector<SubtitleConfigurationDescriptor> subtitle_configurations;
  ClippingConfigurationDescriptor clipping_configuration;
  LiveConfigurationDescriptor live_configuration;
  DrmConfigurationDescriptor drm_configuration;
};

struct TrackSelectionParametersDescriptor {
  std::string preferred_audio_language;
  std::string preferred_text_language;
  std::vector<std::string> preferred_audio_languages;
  std::vector<std::string> preferred_text_languages;
  int preferred_audio_role_flags = 0;
  int preferred_text_role_flags = 0;
  int max_audio_channel_count = 2147483647;
  int max_audio_bitrate = 2147483647;
  int max_video_width = 2147483647;
  int max_video_height = 2147483647;
  int max_video_bitrate = 2147483647;
  int viewport_width = 2147483647;
  int viewport_height = 2147483647;
  bool viewport_orientation_may_change = true;
  bool select_text_by_default = false;
  int ignored_text_selection_flags = 0;
  bool select_undetermined_text_language = false;
  bool force_lowest_bitrate = false;
  bool force_highest_supported_bitrate = false;
  bool disable_video = false;
  bool disable_audio = false;
  bool disable_text = false;
  std::vector<int> disabled_track_types;
  struct OverrideDescriptor {
    std::string track_group_id;
    int track_type = 0;
    std::vector<int> track_indices;
  };
  std::vector<OverrideDescriptor> overrides;
  int DisabledTrackTypeCount() const { return static_cast<int>(disabled_track_types.size()); }
  bool IsTrackTypeDisabled(int track_type) const {
    return (disable_video && track_type == 2) ||
        (disable_audio && track_type == 1) ||
        (disable_text && track_type == 3) ||
        std::find(disabled_track_types.begin(), disabled_track_types.end(), track_type) !=
            disabled_track_types.end();
  }
  int OverrideCount() const { return static_cast<int>(overrides.size()); }
};

struct TrackInfo {
  std::string id;
  std::string language;
  std::string label;
  std::string label_token;
  std::string mime_type;
  std::string container_mime_type;
  std::string codecs;
  int bitrate = 0;
  int width = 0;
  int height = 0;
  float frame_rate = 0.0f;
  int sample_rate = 0;
  int channel_count = 0;
  int accessibility_channel = 0;
  int role_flags = 0;
  int selection_flags = 0;
  int format_support = 0;
  bool selected = false;
  bool supported = false;
  bool supported_within_capabilities = false;
};

struct TrackGroupSnapshot {
  std::string id;
  std::string group_token;
  int type = 0;
  bool adaptive_supported = false;
  bool selected = false;
  bool supported = false;
  bool supported_allowing_exceeds_capabilities = false;
  std::vector<TrackInfo> tracks;
};

struct TracksSnapshot {
  std::vector<TrackGroupSnapshot> groups;
  bool contains_audio = false;
  bool contains_video = false;
  bool contains_text = false;
  bool contains_image = false;
  bool audio_selected = false;
  bool video_selected = false;
  bool text_selected = false;
  bool image_selected = false;
  bool audio_supported = false;
  bool video_supported = false;
  bool text_supported = false;
  bool image_supported = false;
  bool audio_supported_allowing_exceeds_capabilities = false;
  bool video_supported_allowing_exceeds_capabilities = false;
  bool text_supported_allowing_exceeds_capabilities = false;
  bool image_supported_allowing_exceeds_capabilities = false;
};

struct AudioAttributesDescriptor {
  int content_type = 0;
  int flags = 0;
  int usage = 1;
  int allowed_capture_policy = 1;
  int spatialization_behavior = 0;
};

struct VideoEffectDescriptor {
  enum class Type {
    kScaleAndRotate = 1,
    kRgbAdjustment = 2,
    kPresentation = 3,
  };

  Type type = Type::kScaleAndRotate;
  float scale_x = 1.0f;
  float scale_y = 1.0f;
  float rotation_degrees = 0.0f;
  float red_scale = 1.0f;
  float green_scale = 1.0f;
  float blue_scale = 1.0f;
  int presentation_width = 0;
  int presentation_height = 0;
  int presentation_layout = 0;
};
struct AvailableCommandsSnapshot {
  std::vector<int> command_codes;
  int Count() const { return static_cast<int>(command_codes.size()); }
  bool Contains(int command_code) const {
    return std::find(command_codes.begin(), command_codes.end(), command_code) !=
        command_codes.end();
  }
};

struct TimelineSnapshot {
  int window_count = 0;
  int period_count = 0;
  bool empty = true;
  int current_media_item_index = 0;
  int next_media_item_index = -1;
  int previous_media_item_index = -1;
  bool has_next_media_item = false;
  bool has_previous_media_item = false;
  bool current_media_item_dynamic = false;
  bool current_media_item_live = false;
  bool current_media_item_seekable = false;
};

struct TimelineWindowSnapshot {
  int media_item_index = -1;
  std::string media_item_id;
  std::string media_item_uri;
  bool media_item_tag_present = false;
  std::string media_item_tag_string;
  std::string media_item_tag_token;
  std::string uid;
  std::string uid_token;
  bool live_configuration_present = false;
  int64_t live_target_offset_ms = -9223372036854775807LL;
  int64_t live_min_offset_ms = -9223372036854775807LL;
  int64_t live_max_offset_ms = -9223372036854775807LL;
  float live_min_playback_speed = -3.4028235e38f;
  float live_max_playback_speed = -3.4028235e38f;
  bool manifest_present = false;
  std::string manifest_string;
  std::string manifest_token;
  int first_period_index = -1;
  int last_period_index = -1;
  int64_t presentation_start_time_ms = -9223372036854775807LL;
  int64_t window_start_time_ms = -9223372036854775807LL;
  int64_t elapsed_realtime_epoch_offset_ms = -9223372036854775807LL;
  int64_t duration_ms = -9223372036854775807LL;
  int64_t duration_us = -9223372036854775807LL;
  int64_t default_position_ms = -9223372036854775807LL;
  int64_t default_position_us = -9223372036854775807LL;
  int64_t position_in_first_period_ms = 0;
  int64_t position_in_first_period_us = 0;
  bool is_seekable = false;
  bool is_dynamic = false;
  bool is_live = false;
  bool is_placeholder = false;
};

struct TimelinePeriodSnapshot {
  std::string id;
  std::string id_token;
  std::string uid;
  std::string uid_token;
  std::string ads_id;
  std::string ads_id_token;
  int window_index = -1;
  int ad_group_count = 0;
  int64_t duration_ms = -9223372036854775807LL;
  int64_t duration_us = -9223372036854775807LL;
  int64_t position_in_window_ms = 0;
  int64_t position_in_window_us = 0;
  bool is_placeholder = false;
};

struct TimelineDetailsSnapshot {
  TimelineSnapshot summary;
  std::vector<TimelineWindowSnapshot> windows;
  std::vector<TimelinePeriodSnapshot> periods;
};

struct PlaybackParametersSnapshot {
  float speed = 1.0f;
  float pitch = 1.0f;
};

struct SeekParametersDescriptor {
  int64_t tolerance_before_us = 0;
  int64_t tolerance_after_us = 0;
};

struct CueSnapshot {
  struct CueInfo {
    std::string text;
    std::string text_token;
    std::string bitmap_token;
    int text_alignment = 0;
    int multi_row_alignment = 0;
    float line = 0.0f;
    int line_type = 0;
    int line_anchor = 0;
    float position = 0.0f;
    int position_anchor = 0;
    float size = 0.0f;
    float bitmap_height = 0.0f;
    float text_size = 0.0f;
    int text_size_type = 0;
    int vertical_type = 0;
    float shear_degrees = 0.0f;
    int z_index = 0;
    bool window_color_set = false;
    int window_color = 0;
    bool has_bitmap = false;
  };
  int cue_count = 0;
  int64_t presentation_time_us = 0;
  std::vector<std::string> texts;
  std::vector<std::string> text_tokens;
  std::vector<std::string> bitmap_tokens;
  std::vector<CueInfo> cues;
};

struct PlayerEventsSnapshot {
  std::vector<int> event_codes;
  int Count() const { return static_cast<int>(event_codes.size()); }
  bool Contains(int event_code) const {
    return std::find(event_codes.begin(), event_codes.end(), event_code) != event_codes.end();
  }
};

struct ApplicationLooperDescriptor {
  std::string thread_name;
  int64_t thread_id = -1;
  bool is_current_thread = false;
};

struct DeviceInfoDescriptor {
  int playback_type = 0;
  int min_volume = 0;
  int max_volume = 0;
  std::string routing_controller_id;
};

struct VideoSizeSnapshot {
  int width = 0;
  int height = 0;
  int unapplied_rotation_degrees = 0;
  float pixel_width_height_ratio = 1.0f;
};

struct AnalyticsSnapshot {
  int64_t bitrate_estimate = 0;
  int dropped_video_frames = 0;
  int load_started_count = 0;
  int load_completed_count = 0;
  std::string audio_sample_mime_type;
  std::string video_sample_mime_type;
};

inline void AddOpaqueObjectToken(
    std::vector<std::string>* tokens,
    const std::string& token) {
  if (tokens != nullptr && !token.empty()) {
    tokens->push_back(token);
  }
}

inline void AppendOpaqueObjectTokens(
    const MediaMetadataSnapshot& metadata,
    std::vector<std::string>* tokens) {
  AddOpaqueObjectToken(tokens, metadata.title_token);
  AddOpaqueObjectToken(tokens, metadata.artist_token);
  AddOpaqueObjectToken(tokens, metadata.album_title_token);
  AddOpaqueObjectToken(tokens, metadata.album_artist_token);
  AddOpaqueObjectToken(tokens, metadata.display_title_token);
  AddOpaqueObjectToken(tokens, metadata.subtitle_token);
  AddOpaqueObjectToken(tokens, metadata.description_token);
  AddOpaqueObjectToken(tokens, metadata.writer_token);
  AddOpaqueObjectToken(tokens, metadata.author_token);
  AddOpaqueObjectToken(tokens, metadata.composer_token);
  AddOpaqueObjectToken(tokens, metadata.conductor_token);
  AddOpaqueObjectToken(tokens, metadata.genre_token);
  AddOpaqueObjectToken(tokens, metadata.compilation_token);
  AddOpaqueObjectToken(tokens, metadata.station_token);
  AddOpaqueObjectToken(tokens, metadata.extras_token);
}

inline void AppendOpaqueObjectTokens(
    const MediaItemDescriptor& media_item,
    std::vector<std::string>* tokens) {
  AddOpaqueObjectToken(tokens, media_item.tag_token);
  AppendOpaqueObjectTokens(media_item.media_metadata, tokens);
  AddOpaqueObjectToken(tokens, media_item.request_metadata.extras_token);
  AddOpaqueObjectToken(tokens, media_item.ads_configuration.ads_id_token);
}

inline void AppendOpaqueObjectTokens(
    const TrackInfo& track,
    std::vector<std::string>* tokens) {
  AddOpaqueObjectToken(tokens, track.label_token);
}

inline void AppendOpaqueObjectTokens(
    const TrackGroupSnapshot& group,
    std::vector<std::string>* tokens) {
  AddOpaqueObjectToken(tokens, group.group_token);
  for (const TrackInfo& track : group.tracks) {
    AppendOpaqueObjectTokens(track, tokens);
  }
}

inline void AppendOpaqueObjectTokens(
    const TracksSnapshot& tracks,
    std::vector<std::string>* tokens) {
  for (const TrackGroupSnapshot& group : tracks.groups) {
    AppendOpaqueObjectTokens(group, tokens);
  }
}

inline void AppendOpaqueObjectTokens(
    const TimelineWindowSnapshot& window,
    std::vector<std::string>* tokens) {
  AddOpaqueObjectToken(tokens, window.media_item_tag_token);
  AddOpaqueObjectToken(tokens, window.uid_token);
  AddOpaqueObjectToken(tokens, window.manifest_token);
}

inline void AppendOpaqueObjectTokens(
    const TimelinePeriodSnapshot& period,
    std::vector<std::string>* tokens) {
  AddOpaqueObjectToken(tokens, period.id_token);
  AddOpaqueObjectToken(tokens, period.uid_token);
  AddOpaqueObjectToken(tokens, period.ads_id_token);
}

inline void AppendOpaqueObjectTokens(
    const TimelineDetailsSnapshot& timeline,
    std::vector<std::string>* tokens) {
  for (const TimelineWindowSnapshot& window : timeline.windows) {
    AppendOpaqueObjectTokens(window, tokens);
  }
  for (const TimelinePeriodSnapshot& period : timeline.periods) {
    AppendOpaqueObjectTokens(period, tokens);
  }
}

inline void AppendOpaqueObjectTokens(
    const CueSnapshot& cues,
    std::vector<std::string>* tokens) {
  for (const CueSnapshot::CueInfo& cue : cues.cues) {
    AddOpaqueObjectToken(tokens, cue.text_token);
    AddOpaqueObjectToken(tokens, cue.bitmap_token);
  }
}

template <typename SnapshotType>
inline std::vector<std::string> CollectOpaqueObjectTokens(const SnapshotType& snapshot) {
  std::vector<std::string> tokens;
  AppendOpaqueObjectTokens(snapshot, &tokens);
  return tokens;
}

struct AudioUnderrunEvent {
  int buffer_size = 0;
  int64_t buffer_size_ms = 0;
  int64_t elapsed_since_last_feed_ms = 0;
};

struct DroppedVideoFramesEvent {
  int dropped_frames = 0;
  int64_t elapsed_ms = 0;
};

struct BandwidthEstimateEvent {
  int elapsed_ms = 0;
  int64_t bytes_transferred = 0;
  int64_t bitrate_estimate = 0;
};

struct LoadStartedEvent {
  std::string uri;
  int data_type = 0;
  int track_type = 0;
  int retry_count = 0;
};

struct LoadCompletedEvent {
  std::string uri;
  int data_type = 0;
  int track_type = 0;
};

struct AnalyticsLoadErrorEvent {
  std::string uri;
  int data_type = 0;
  int track_type = 0;
  std::string message;
  bool was_canceled = false;
};

struct AudioInputFormatChangedEvent {
  std::string sample_mime_type;
  std::string codecs;
  int channel_count = 0;
  int sample_rate = 0;
};

struct AudioDecoderInitializedEvent {
  std::string decoder_name;
  int64_t initialized_timestamp_ms = 0;
  int64_t initialization_duration_ms = 0;
};

struct VideoDecoderInitializedEvent {
  std::string decoder_name;
  int64_t initialized_timestamp_ms = 0;
  int64_t initialization_duration_ms = 0;
};

struct AudioDecoderReleasedEvent {
  std::string decoder_name;
};

struct VideoDecoderReleasedEvent {
  std::string decoder_name;
};

struct AnalyticsRenderedFirstFrameEvent {
  int64_t render_time_ms = 0;
};

struct AnalyticsVideoSizeChangedEvent {
  int width = 0;
  int height = 0;
  float pixel_width_height_ratio = 1.0f;
};

struct AudioPositionAdvancingEvent {
  int64_t playout_start_system_time_ms = 0;
};

struct VideoFrameProcessingOffsetEvent {
  int64_t total_processing_offset_us = 0;
  int frame_count = 0;
};

struct VolumeChangedEvent {
  float volume = 1.0f;
};

struct AudioSessionIdChangedEvent {
  int audio_session_id = 0;
};

struct AnalyticsSkipSilenceEnabledChangedEvent {
  bool skip_silence_enabled = false;
};

struct AnalyticsDeviceVolumeChangedEvent {
  int volume = 0;
  bool muted = false;
};

struct AnalyticsPlaybackStateChangedEvent {
  int playback_state = 0;
};

struct AnalyticsIsPlayingChangedEvent {
  bool is_playing = false;
};

struct AnalyticsPlayWhenReadyChangedEvent {
  bool play_when_ready = false;
  int reason = 0;
};

struct AnalyticsPlaybackSuppressionReasonChangedEvent {
  int playback_suppression_reason = 0;
};

struct AnalyticsIsLoadingChangedEvent {
  bool is_loading = false;
};

struct AnalyticsRepeatModeChangedEvent {
  int repeat_mode = 0;
};

struct AnalyticsShuffleModeChangedEvent {
  bool shuffle_mode_enabled = false;
};

struct AnalyticsPlaybackParametersChangedEvent {
  float speed = 1.0f;
  float pitch = 1.0f;
};

struct AnalyticsAvailableCommandsChangedEvent {
  std::vector<int> commands;
};

struct AnalyticsEventsEvent {
  std::vector<int> event_codes;
};

struct AnalyticsSeekBackIncrementChangedEvent {
  int64_t seek_back_increment_ms = 0;
};

struct AnalyticsSeekForwardIncrementChangedEvent {
  int64_t seek_forward_increment_ms = 0;
};

struct AnalyticsMaxSeekToPreviousPositionChangedEvent {
  int64_t max_seek_to_previous_position_ms = 0;
};

struct AnalyticsTimelineChangedEvent {
  int reason = 0;
};

struct AnalyticsPositionDiscontinuityEvent {
  int reason = 0;
};

struct AnalyticsSeekStartedEvent {
  bool started = true;
};

struct AnalyticsMediaItemTransitionEvent {
  MediaItemDescriptor media_item;
  int reason = 0;
};

struct AnalyticsMetadataEvent {
  int entry_count = 0;
  std::string first_entry_type;
  std::string first_entry_text;
};

struct VideoInputFormatChangedEvent {
  std::string sample_mime_type;
  std::string codecs;
  int width = 0;
  int height = 0;
  float frame_rate = 0.0f;
};

struct PlayerError {
  int error_code = 0;
  std::string message;
};

struct PlaybackSnapshot {
  PlaybackState playback_state = PlaybackState::kIdle;
  bool play_when_ready = false;
  bool is_playing = false;
  bool is_loading = false;
  bool shuffle_mode_enabled = false;
  int current_media_item_index = 0;
  int media_item_count = 0;
  RepeatMode repeat_mode = RepeatMode::kOff;
  int64_t current_position_ms = 0;
  int64_t buffered_position_ms = 0;
  int64_t duration_ms = 0;
  int64_t seek_back_increment_ms = 0;
  int64_t seek_forward_increment_ms = 0;
  int64_t max_seek_to_previous_position_ms = 0;
  float volume = 1.0f;
  float playback_speed = 1.0f;
  PlayerError last_error;
};

struct PositionInfoSnapshot {
  int media_item_index = 0;
  MediaItemDescriptor media_item;
  int period_index = 0;
  int64_t position_ms = 0;
  int64_t content_position_ms = 0;
  int ad_group_index = -1;
  int ad_index_in_ad_group = -1;
};

struct ImageFrameSnapshot {
  int64_t presentation_time_us = 0;
  int width = 0;
  int height = 0;
  int byte_count = 0;
  int allocation_byte_count = 0;
  int row_bytes = 0;
  bool has_alpha = false;
  bool is_premultiplied = false;
  bool is_mutable = false;
  std::string bitmap_config;
};

class ImageOutputListener {
 public:
  virtual ~ImageOutputListener() = default;

  virtual void OnImageAvailable(const ImageFrameSnapshot& image_frame) = 0;
  virtual void OnDisabled() {}
};

class PlayerListener {
 public:
  virtual ~PlayerListener() = default;

  virtual void OnPlaybackStateChanged(const PlaybackSnapshot& snapshot) = 0;
  virtual void OnPlayWhenReadyChanged(const PlaybackSnapshot& snapshot, int reason) = 0;
  virtual void OnIsPlayingChanged(const PlaybackSnapshot& snapshot) = 0;
  virtual void OnMediaItemTransition(const PlaybackSnapshot& snapshot, int reason) = 0;
  virtual void OnPlayerError(const PlaybackSnapshot& snapshot) = 0;
  virtual void OnPlayerErrorChanged(const PlaybackSnapshot& snapshot) {}
  virtual void OnTimelineChanged(
      const PlaybackSnapshot& snapshot,
      const TimelineDetailsSnapshot& timeline,
      int reason) {}
  virtual void OnTracksChanged(
      const PlaybackSnapshot& snapshot,
      const TracksSnapshot& tracks) {}
  virtual void OnPositionDiscontinuity(
      const PlaybackSnapshot& snapshot,
      const PositionInfoSnapshot& old_position,
      const PositionInfoSnapshot& new_position,
      int reason) {}
  virtual void OnAudioAttributesChanged(
      const PlaybackSnapshot& snapshot,
      const AudioAttributesDescriptor& attributes) {}
  virtual void OnCues(const PlaybackSnapshot& snapshot, const CueSnapshot& cues) {}
  virtual void OnRepeatModeChanged(const PlaybackSnapshot& snapshot) {}
  virtual void OnShuffleModeEnabledChanged(const PlaybackSnapshot& snapshot) {}
  virtual void OnSeekBackIncrementChanged(
      const PlaybackSnapshot& snapshot,
      int64_t seek_back_increment_ms) {}
  virtual void OnSeekForwardIncrementChanged(
      const PlaybackSnapshot& snapshot,
      int64_t seek_forward_increment_ms) {}
  virtual void OnMaxSeekToPreviousPositionChanged(
      const PlaybackSnapshot& snapshot,
      int64_t max_seek_to_previous_position_ms) {}
  virtual void OnTrackSelectionParametersChanged(
      const PlaybackSnapshot& snapshot,
      const TrackSelectionParametersDescriptor& parameters) {}
  virtual void OnPlaybackParametersChanged(
      const PlaybackSnapshot& snapshot,
      const PlaybackParametersSnapshot& parameters) {}
  virtual void OnPlaybackSuppressionReasonChanged(
      const PlaybackSnapshot& snapshot,
      PlaybackSuppressionReason suppression_reason) {}
  virtual void OnAvailableCommandsChanged(
      const PlaybackSnapshot& snapshot,
      const AvailableCommandsSnapshot& commands) {}
  virtual void OnEvents(
      const PlaybackSnapshot& snapshot,
      const PlayerEventsSnapshot& events) {}
  virtual void OnDeviceInfoChanged(
      const PlaybackSnapshot& snapshot,
      const DeviceInfoDescriptor& device_info) {}
  virtual void OnDeviceVolumeChanged(
      const PlaybackSnapshot& snapshot,
      int device_volume,
      bool muted) {}
  virtual void OnSkipSilenceEnabledChanged(
      const PlaybackSnapshot& snapshot,
      bool skip_silence_enabled) {}
  virtual void OnVideoSizeChanged(
      const PlaybackSnapshot& snapshot,
      const VideoSizeSnapshot& video_size) {}
  virtual void OnSurfaceSizeChanged(
      const PlaybackSnapshot& snapshot,
      int width,
      int height) {}
  virtual void OnRenderedFirstFrame(const PlaybackSnapshot& snapshot) {}
  virtual void OnMediaMetadataChanged(
      const PlaybackSnapshot& snapshot,
      const MediaMetadataSnapshot& metadata) {}
  virtual void OnPlaylistMetadataChanged(
      const PlaybackSnapshot& snapshot,
      const MediaMetadataSnapshot& metadata) {}
  virtual void OnAnalyticsUpdated(
      const PlaybackSnapshot& snapshot,
      const AnalyticsSnapshot& analytics) {}
  virtual void OnAudioUnderrun(
      const PlaybackSnapshot& snapshot,
      const AudioUnderrunEvent& audio_underrun) {}
  virtual void OnDroppedVideoFrames(
      const PlaybackSnapshot& snapshot,
      const DroppedVideoFramesEvent& dropped_video_frames) {}
  virtual void OnBandwidthEstimate(
      const PlaybackSnapshot& snapshot,
      const BandwidthEstimateEvent& bandwidth_estimate) {}
  virtual void OnLoadStarted(
      const PlaybackSnapshot& snapshot,
      const LoadStartedEvent& load_started) {}
  virtual void OnLoadCompleted(
      const PlaybackSnapshot& snapshot,
      const LoadCompletedEvent& load_completed) {}
  virtual void OnAnalyticsLoadError(
      const PlaybackSnapshot& snapshot,
      const AnalyticsLoadErrorEvent& load_error) {}
  virtual void OnAudioInputFormatChanged(
      const PlaybackSnapshot& snapshot,
      const AudioInputFormatChangedEvent& audio_input_format_changed) {}
  virtual void OnAudioDecoderInitialized(
      const PlaybackSnapshot& snapshot,
      const AudioDecoderInitializedEvent& audio_decoder_initialized) {}
  virtual void OnVideoDecoderInitialized(
      const PlaybackSnapshot& snapshot,
      const VideoDecoderInitializedEvent& video_decoder_initialized) {}
  virtual void OnAudioDecoderReleased(
      const PlaybackSnapshot& snapshot,
      const AudioDecoderReleasedEvent& audio_decoder_released) {}
  virtual void OnVideoDecoderReleased(
      const PlaybackSnapshot& snapshot,
      const VideoDecoderReleasedEvent& video_decoder_released) {}
  virtual void OnAnalyticsRenderedFirstFrame(
      const PlaybackSnapshot& snapshot,
      const AnalyticsRenderedFirstFrameEvent& rendered_first_frame) {}
  virtual void OnAnalyticsVideoSizeChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsVideoSizeChangedEvent& analytics_video_size) {}
  virtual void OnAudioPositionAdvancing(
      const PlaybackSnapshot& snapshot,
      const AudioPositionAdvancingEvent& audio_position_advancing) {}
  virtual void OnVideoFrameProcessingOffset(
      const PlaybackSnapshot& snapshot,
      const VideoFrameProcessingOffsetEvent& video_frame_processing_offset) {}
  virtual void OnVolumeChanged(
      const PlaybackSnapshot& snapshot,
      const VolumeChangedEvent& volume_changed) {}
  virtual void OnAudioSessionIdChanged(
      const PlaybackSnapshot& snapshot,
      const AudioSessionIdChangedEvent& audio_session_id_changed) {}
  virtual void OnAnalyticsSkipSilenceEnabledChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsSkipSilenceEnabledChangedEvent& skip_silence_enabled_changed) {}
  virtual void OnAnalyticsDeviceVolumeChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsDeviceVolumeChangedEvent& device_volume_changed) {}
  virtual void OnAnalyticsPlaybackStateChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsPlaybackStateChangedEvent& playback_state_changed) {}
  virtual void OnAnalyticsIsPlayingChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsIsPlayingChangedEvent& is_playing_changed) {}
  virtual void OnAnalyticsPlayWhenReadyChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsPlayWhenReadyChangedEvent& play_when_ready_changed) {}
  virtual void OnAnalyticsPlaybackSuppressionReasonChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsPlaybackSuppressionReasonChangedEvent& suppression_reason_changed) {}
  virtual void OnAnalyticsIsLoadingChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsIsLoadingChangedEvent& is_loading_changed) {}
  virtual void OnAnalyticsRepeatModeChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsRepeatModeChangedEvent& repeat_mode_changed) {}
  virtual void OnAnalyticsShuffleModeChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsShuffleModeChangedEvent& shuffle_mode_changed) {}
  virtual void OnAnalyticsPlaybackParametersChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsPlaybackParametersChangedEvent& playback_parameters_changed) {}
  virtual void OnAnalyticsAvailableCommandsChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsAvailableCommandsChangedEvent& available_commands_changed) {}
  virtual void OnAnalyticsEvents(
      const PlaybackSnapshot& snapshot,
      const AnalyticsEventsEvent& analytics_events) {}
  virtual void OnAnalyticsSeekBackIncrementChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsSeekBackIncrementChangedEvent& seek_back_increment_changed) {}
  virtual void OnAnalyticsSeekForwardIncrementChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsSeekForwardIncrementChangedEvent& seek_forward_increment_changed) {}
  virtual void OnAnalyticsMaxSeekToPreviousPositionChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsMaxSeekToPreviousPositionChangedEvent&
          max_seek_to_previous_position_changed) {}
  virtual void OnAnalyticsTimelineChanged(
      const PlaybackSnapshot& snapshot,
      const AnalyticsTimelineChangedEvent& timeline_changed) {}
  virtual void OnAnalyticsPositionDiscontinuity(
      const PlaybackSnapshot& snapshot,
      const AnalyticsPositionDiscontinuityEvent& position_discontinuity) {}
  virtual void OnAnalyticsSeekStarted(
      const PlaybackSnapshot& snapshot,
      const AnalyticsSeekStartedEvent& seek_started) {}
  virtual void OnAnalyticsPlayerError(
      const PlaybackSnapshot& snapshot,
      const PlayerError& error) {}
  virtual void OnAnalyticsPlayerErrorChanged(
      const PlaybackSnapshot& snapshot,
      const PlayerError& error) {}
  virtual void OnAnalyticsTracksChanged(
      const PlaybackSnapshot& snapshot,
      const TracksSnapshot& tracks) {}
  virtual void OnAnalyticsMediaItemTransition(
      const PlaybackSnapshot& snapshot,
      const AnalyticsMediaItemTransitionEvent& media_item_transition) {}
  virtual void OnAnalyticsCues(
      const PlaybackSnapshot& snapshot,
      const CueSnapshot& cues) {}
  virtual void OnAnalyticsMetadata(
      const PlaybackSnapshot& snapshot,
      const AnalyticsMetadataEvent& metadata) {}
  virtual void OnAnalyticsDeviceInfoChanged(
      const PlaybackSnapshot& snapshot,
      const DeviceInfoDescriptor& device_info) {}
  virtual void OnAnalyticsMediaMetadataChanged(
      const PlaybackSnapshot& snapshot,
      const MediaMetadataSnapshot& metadata) {}
  virtual void OnAnalyticsPlaylistMetadataChanged(
      const PlaybackSnapshot& snapshot,
      const MediaMetadataSnapshot& metadata) {}
  virtual void OnVideoInputFormatChanged(
      const PlaybackSnapshot& snapshot,
      const VideoInputFormatChangedEvent& video_input_format_changed) {}
};

class ExoPlayerBridge {
 public:
  static std::shared_ptr<ExoPlayerBridge> Create(
      JNIEnv* env,
      jobject context,
      const PlayerConfig& config);

  virtual ~ExoPlayerBridge() = default;

  virtual void SetListener(PlayerListener* listener) = 0;
  virtual void RemoveListener(PlayerListener* listener) = 0;
  virtual void SetImageOutputListener(ImageOutputListener* listener) = 0;
  virtual void RemoveImageOutputListener(ImageOutputListener* listener) = 0;
  virtual void BindPlayerView(JNIEnv* env, jobject player_view) = 0;
  virtual void UnbindPlayerView(JNIEnv* env, jobject player_view) = 0;
  virtual void SetVideoSurface(JNIEnv* env, jobject surface) = 0;
  virtual void ClearVideoSurface(JNIEnv* env) = 0;
  virtual void ClearVideoSurface(JNIEnv* env, jobject surface) = 0;
  virtual void SetVideoSurfaceHolder(JNIEnv* env, jobject surface_holder) = 0;
  virtual void ClearVideoSurfaceHolder(JNIEnv* env, jobject surface_holder) = 0;
  virtual void SetVideoSurfaceView(JNIEnv* env, jobject surface_view) = 0;
  virtual void ClearVideoSurfaceView(JNIEnv* env, jobject surface_view) = 0;
  virtual void SetVideoTextureView(JNIEnv* env, jobject texture_view) = 0;
  virtual void ClearVideoTextureView(JNIEnv* env, jobject texture_view) = 0;
  virtual void SetVideoEffects(
      JNIEnv* env,
      const std::vector<VideoEffectDescriptor>& video_effects) = 0;
  virtual void SetMediaItem(JNIEnv* env, const MediaItemDescriptor& media_item) = 0;
  virtual void SetMediaItem(
      JNIEnv* env,
      const MediaItemDescriptor& media_item,
      bool reset_position) = 0;
  virtual void SetMediaItem(
      JNIEnv* env,
      const MediaItemDescriptor& media_item,
      int64_t start_position_ms) = 0;
  virtual void SetMediaItems(
      JNIEnv* env,
      const std::vector<MediaItemDescriptor>& media_items,
      int start_index,
      int64_t start_position_ms) = 0;
  virtual void SetMediaItems(
      JNIEnv* env,
      const std::vector<MediaItemDescriptor>& media_items,
      bool reset_position) = 0;
  virtual void AddMediaItem(JNIEnv* env, const MediaItemDescriptor& media_item) = 0;
  virtual void AddMediaItem(JNIEnv* env, int index, const MediaItemDescriptor& media_item) = 0;
  virtual void AddMediaItems(
      JNIEnv* env,
      const std::vector<MediaItemDescriptor>& media_items) = 0;
  virtual void AddMediaItems(
      JNIEnv* env,
      int index,
      const std::vector<MediaItemDescriptor>& media_items) = 0;
  virtual void RemoveMediaItem(JNIEnv* env, int index) = 0;
  virtual void RemoveMediaItems(JNIEnv* env, int from_index, int to_index) = 0;
  virtual void MoveMediaItem(JNIEnv* env, int current_index, int new_index) = 0;
  virtual void MoveMediaItems(
      JNIEnv* env,
      int from_index,
      int to_index,
      int new_index) = 0;
  virtual void ReplaceMediaItems(
      JNIEnv* env,
      int from_index,
      int to_index,
      const std::vector<MediaItemDescriptor>& media_items) = 0;
  virtual void ReplaceMediaItem(
      JNIEnv* env,
      int index,
      const MediaItemDescriptor& media_item) = 0;
  virtual void ClearMediaItems(JNIEnv* env) = 0;
  virtual void Prepare(JNIEnv* env) = 0;
  virtual void Play(JNIEnv* env) = 0;
  virtual void Pause(JNIEnv* env) = 0;
  virtual void Stop(JNIEnv* env) = 0;
  virtual void SeekTo(JNIEnv* env, int64_t position_ms) = 0;
  virtual void SeekToMediaItem(JNIEnv* env, int media_item_index, int64_t position_ms) = 0;
  virtual void SeekBack(JNIEnv* env) = 0;
  virtual void SeekForward(JNIEnv* env) = 0;
  virtual void SeekToDefaultPosition(JNIEnv* env) = 0;
  virtual void SeekToDefaultPosition(JNIEnv* env, int media_item_index) = 0;
  virtual void SetSeekParameters(
      JNIEnv* env,
      const SeekParametersDescriptor& seek_parameters) = 0;
  virtual SeekParametersDescriptor GetSeekParameters(JNIEnv* env) = 0;
  virtual void SeekToNext(JNIEnv* env) = 0;
  virtual void SeekToPrevious(JNIEnv* env) = 0;
  virtual void SeekToNextMediaItem(JNIEnv* env) = 0;
  virtual void SeekToPreviousMediaItem(JNIEnv* env) = 0;
  virtual void SetWakeMode(JNIEnv* env, int wake_mode) = 0;
  virtual void SetPriority(JNIEnv* env, int priority) = 0;
  virtual void SetPriorityTaskManager(JNIEnv* env, jobject priority_task_manager) = 0;
  virtual void SetPriorityTaskManagerEnabled(JNIEnv* env, bool enabled) = 0;
  virtual void SetPreloadConfiguration(JNIEnv* env, int64_t target_preload_duration_us) = 0;
  virtual PlayerMessageResult SendPlayerMessage(
      JNIEnv* env,
      const PlayerMessageDescriptor& message) = 0;
  virtual void SetImageOutputEnabled(JNIEnv* env, bool enabled) = 0;
  virtual void SetAudioAttributes(
      JNIEnv* env,
      const AudioAttributesDescriptor& attributes,
      bool handle_audio_focus) = 0;
  virtual void SetDeviceVolume(JNIEnv* env, int volume, int flags) = 0;
  virtual void AdjustDeviceVolume(JNIEnv* env, int direction, int flags) = 0;
  virtual void IncreaseDeviceVolume(JNIEnv* env, int flags) = 0;
  virtual void DecreaseDeviceVolume(JNIEnv* env, int flags) = 0;
  virtual void SetDeviceMuted(JNIEnv* env, bool muted, int flags) = 0;
  virtual void SetSkipSilenceEnabled(JNIEnv* env, bool skip_silence_enabled) = 0;
  virtual void SetPlayWhenReady(JNIEnv* env, bool play_when_ready) = 0;
  virtual void SetRepeatMode(JNIEnv* env, RepeatMode repeat_mode) = 0;
  virtual void SetShuffleModeEnabled(JNIEnv* env, bool shuffle_mode_enabled) = 0;
  virtual void SetVolume(JNIEnv* env, float volume) = 0;
  virtual void SetPlaybackSpeed(JNIEnv* env, float speed) = 0;
  virtual void SetPlaybackParameters(
      JNIEnv* env,
      const PlaybackParametersSnapshot& parameters) = 0;
  virtual void SetPauseAtEndOfMediaItems(JNIEnv* env, bool pause_at_end_of_media_items) = 0;
  virtual void SetTrackSelectionParameters(
      JNIEnv* env,
      const TrackSelectionParametersDescriptor& parameters) = 0;
  virtual TrackSelectionParametersDescriptor GetTrackSelectionParameters(JNIEnv* env) = 0;
  virtual TracksSnapshot GetTracksSnapshot(JNIEnv* env) = 0;
  virtual std::vector<TrackGroupSnapshot> GetTrackGroups(JNIEnv* env) = 0;
  virtual PlaybackState GetPlaybackState(JNIEnv* env) = 0;
  virtual bool GetPlayWhenReady(JNIEnv* env) = 0;
  virtual bool IsPlaying(JNIEnv* env) = 0;
  virtual bool IsLoading(JNIEnv* env) = 0;
  virtual PlayerError GetPlayerError(JNIEnv* env) = 0;
  virtual int64_t GetCurrentPosition(JNIEnv* env) = 0;
  virtual int64_t GetBufferedPosition(JNIEnv* env) = 0;
  virtual int64_t GetDuration(JNIEnv* env) = 0;
  virtual int GetCurrentMediaItemIndex(JNIEnv* env) = 0;
  virtual int GetMediaItemCount(JNIEnv* env) = 0;
  virtual RepeatMode GetRepeatMode(JNIEnv* env) = 0;
  virtual bool GetShuffleModeEnabled(JNIEnv* env) = 0;
  virtual float GetVolume(JNIEnv* env) = 0;
  virtual AudioAttributesDescriptor GetAudioAttributes(JNIEnv* env) = 0;
  virtual DeviceInfoDescriptor GetDeviceInfo(JNIEnv* env) = 0;
  virtual int GetDeviceVolume(JNIEnv* env) = 0;
  virtual bool IsDeviceMuted(JNIEnv* env) = 0;
  virtual bool GetSkipSilenceEnabled(JNIEnv* env) = 0;
  virtual VideoSizeSnapshot GetVideoSize(JNIEnv* env) = 0;
  virtual int GetNextMediaItemIndex(JNIEnv* env) = 0;
  virtual int GetPreviousMediaItemIndex(JNIEnv* env) = 0;
  virtual bool HasNextMediaItem(JNIEnv* env) = 0;
  virtual bool HasPreviousMediaItem(JNIEnv* env) = 0;
  virtual int GetBufferedPercentage(JNIEnv* env) = 0;
  virtual int64_t GetContentBufferedPosition(JNIEnv* env) = 0;
  virtual int64_t GetContentDuration(JNIEnv* env) = 0;
  virtual int64_t GetContentPosition(JNIEnv* env) = 0;
  virtual int64_t GetCurrentLiveOffset(JNIEnv* env) = 0;
  virtual int GetCurrentPeriodIndex(JNIEnv* env) = 0;
  virtual int64_t GetMaxSeekToPreviousPosition(JNIEnv* env) = 0;
  virtual PlaybackSuppressionReason GetPlaybackSuppressionReason(JNIEnv* env) = 0;
  virtual int64_t GetSeekBackIncrement(JNIEnv* env) = 0;
  virtual int64_t GetSeekForwardIncrement(JNIEnv* env) = 0;
  virtual int64_t GetTotalBufferedDuration(JNIEnv* env) = 0;
  virtual int64_t GetTargetPreloadDurationUs(JNIEnv* env) = 0;
  virtual bool IsCommandAvailable(JNIEnv* env, int command_code) = 0;
  virtual bool CanAdvertiseSession(JNIEnv* env) = 0;
  virtual ApplicationLooperDescriptor GetApplicationLooper(JNIEnv* env) = 0;
  virtual int GetCurrentAdGroupIndex(JNIEnv* env) = 0;
  virtual int GetCurrentAdIndexInAdGroup(JNIEnv* env) = 0;
  virtual bool IsCurrentMediaItemDynamic(JNIEnv* env) = 0;
  virtual bool IsCurrentMediaItemLive(JNIEnv* env) = 0;
  virtual bool IsCurrentMediaItemSeekable(JNIEnv* env) = 0;
  virtual bool IsPlayingAd(JNIEnv* env) = 0;
  virtual void SetPlaylistMetadata(
      JNIEnv* env,
      const MediaMetadataSnapshot& metadata) = 0;
  virtual MediaMetadataSnapshot GetMediaMetadata(JNIEnv* env) = 0;
  virtual MediaMetadataSnapshot GetPlaylistMetadata(JNIEnv* env) = 0;
  virtual PlaybackParametersSnapshot GetPlaybackParameters(JNIEnv* env) = 0;
  virtual MediaItemDescriptor GetMediaItemAt(JNIEnv* env, int index) = 0;
  virtual MediaItemDescriptor GetCurrentMediaItem(JNIEnv* env) = 0;
  virtual void ReleaseOpaqueObjectTokens(
      JNIEnv* env,
      const std::vector<std::string>& tokens) = 0;
  virtual AnalyticsSnapshot GetAnalyticsSnapshot(JNIEnv* env) = 0;
  virtual void SimulateAnalyticsUpdateForTest(
      JNIEnv* env,
      const AnalyticsSnapshot& analytics) = 0;
  virtual void SimulateAudioUnderrunForTest(
      JNIEnv* env,
      const AudioUnderrunEvent& audio_underrun) = 0;
  virtual void SimulateDroppedVideoFramesForTest(
      JNIEnv* env,
      const DroppedVideoFramesEvent& dropped_video_frames) = 0;
  virtual void SimulateBandwidthEstimateForTest(
      JNIEnv* env,
      const BandwidthEstimateEvent& bandwidth_estimate) = 0;
  virtual void SimulateLoadStartedForTest(
      JNIEnv* env,
      const LoadStartedEvent& load_started) = 0;
  virtual void SimulateLoadCompletedForTest(
      JNIEnv* env,
      const LoadCompletedEvent& load_completed) = 0;
  virtual void SimulateAnalyticsLoadErrorForTest(
      JNIEnv* env,
      const AnalyticsLoadErrorEvent& load_error) = 0;
  virtual void SimulateAudioInputFormatChangedForTest(
      JNIEnv* env,
      const AudioInputFormatChangedEvent& audio_input_format_changed) = 0;
  virtual void SimulateAudioDecoderInitializedForTest(
      JNIEnv* env,
      const AudioDecoderInitializedEvent& audio_decoder_initialized) = 0;
  virtual void SimulateVideoDecoderInitializedForTest(
      JNIEnv* env,
      const VideoDecoderInitializedEvent& video_decoder_initialized) = 0;
  virtual void SimulateAudioDecoderReleasedForTest(
      JNIEnv* env,
      const AudioDecoderReleasedEvent& audio_decoder_released) = 0;
  virtual void SimulateVideoDecoderReleasedForTest(
      JNIEnv* env,
      const VideoDecoderReleasedEvent& video_decoder_released) = 0;
  virtual void SimulateAnalyticsRenderedFirstFrameForTest(
      JNIEnv* env,
      const AnalyticsRenderedFirstFrameEvent& rendered_first_frame) = 0;
  virtual void SimulateAnalyticsVideoSizeChangedForTest(
      JNIEnv* env,
      const AnalyticsVideoSizeChangedEvent& analytics_video_size) = 0;
  virtual void SimulateAudioPositionAdvancingForTest(
      JNIEnv* env,
      const AudioPositionAdvancingEvent& audio_position_advancing) = 0;
  virtual void SimulateVideoFrameProcessingOffsetForTest(
      JNIEnv* env,
      const VideoFrameProcessingOffsetEvent& video_frame_processing_offset) = 0;
  virtual void SimulateVolumeChangedForTest(
      JNIEnv* env,
      const VolumeChangedEvent& volume_changed) = 0;
  virtual void SimulateAudioSessionIdChangedForTest(
      JNIEnv* env,
      const AudioSessionIdChangedEvent& audio_session_id_changed) = 0;
  virtual void SimulateAnalyticsSkipSilenceEnabledChangedForTest(
      JNIEnv* env,
      const AnalyticsSkipSilenceEnabledChangedEvent& skip_silence_enabled_changed) = 0;
  virtual void SimulateAnalyticsDeviceVolumeChangedForTest(
      JNIEnv* env,
      const AnalyticsDeviceVolumeChangedEvent& device_volume_changed) = 0;
  virtual void SimulateAnalyticsPlaybackStateChangedForTest(
      JNIEnv* env,
      const AnalyticsPlaybackStateChangedEvent& playback_state_changed) = 0;
  virtual void SimulateAnalyticsIsPlayingChangedForTest(
      JNIEnv* env,
      const AnalyticsIsPlayingChangedEvent& is_playing_changed) = 0;
  virtual void SimulateAnalyticsPlayWhenReadyChangedForTest(
      JNIEnv* env,
      const AnalyticsPlayWhenReadyChangedEvent& play_when_ready_changed) = 0;
  virtual void SimulateAnalyticsPlaybackSuppressionReasonChangedForTest(
      JNIEnv* env,
      const AnalyticsPlaybackSuppressionReasonChangedEvent& suppression_reason_changed) = 0;
  virtual void SimulateAnalyticsIsLoadingChangedForTest(
      JNIEnv* env,
      const AnalyticsIsLoadingChangedEvent& is_loading_changed) = 0;
  virtual void SimulateAnalyticsRepeatModeChangedForTest(
      JNIEnv* env,
      const AnalyticsRepeatModeChangedEvent& repeat_mode_changed) = 0;
  virtual void SimulateAnalyticsShuffleModeChangedForTest(
      JNIEnv* env,
      const AnalyticsShuffleModeChangedEvent& shuffle_mode_changed) = 0;
  virtual void SimulateAnalyticsPlaybackParametersChangedForTest(
      JNIEnv* env,
      const AnalyticsPlaybackParametersChangedEvent& playback_parameters_changed) = 0;
  virtual void SimulateAnalyticsAvailableCommandsChangedForTest(
      JNIEnv* env,
      const AnalyticsAvailableCommandsChangedEvent& available_commands_changed) = 0;
  virtual void SimulateAnalyticsEventsForTest(
      JNIEnv* env,
      const AnalyticsEventsEvent& analytics_events) = 0;
  virtual void SimulateSeekBackIncrementChangedForTest(
      JNIEnv* env,
      int64_t seek_back_increment_ms) = 0;
  virtual void SimulateSeekForwardIncrementChangedForTest(
      JNIEnv* env,
      int64_t seek_forward_increment_ms) = 0;
  virtual void SimulateMaxSeekToPreviousPositionChangedForTest(
      JNIEnv* env,
      int64_t max_seek_to_previous_position_ms) = 0;
  virtual void SimulateAnalyticsSeekBackIncrementChangedForTest(
      JNIEnv* env,
      const AnalyticsSeekBackIncrementChangedEvent& seek_back_increment_changed) = 0;
  virtual void SimulateAnalyticsSeekForwardIncrementChangedForTest(
      JNIEnv* env,
      const AnalyticsSeekForwardIncrementChangedEvent& seek_forward_increment_changed) = 0;
  virtual void SimulateAnalyticsMaxSeekToPreviousPositionChangedForTest(
      JNIEnv* env,
      const AnalyticsMaxSeekToPreviousPositionChangedEvent&
          max_seek_to_previous_position_changed) = 0;
  virtual void SimulateAnalyticsTimelineChangedForTest(
      JNIEnv* env,
      const AnalyticsTimelineChangedEvent& timeline_changed) = 0;
  virtual void SimulateAnalyticsPositionDiscontinuityForTest(
      JNIEnv* env,
      const AnalyticsPositionDiscontinuityEvent& position_discontinuity) = 0;
  virtual void SimulateAnalyticsSeekStartedForTest(
      JNIEnv* env,
      const AnalyticsSeekStartedEvent& seek_started) = 0;
  virtual void SimulateAnalyticsPlayerErrorForTest(
      JNIEnv* env,
      const PlayerError& error) = 0;
  virtual void SimulateAnalyticsPlayerErrorChangedForTest(
      JNIEnv* env,
      const PlayerError& error) = 0;
  virtual void SimulateAnalyticsTracksChangedForTest(
      JNIEnv* env,
      const TracksSnapshot& tracks) = 0;
  virtual void SimulateAnalyticsMediaItemTransitionForTest(
      JNIEnv* env,
      const AnalyticsMediaItemTransitionEvent& media_item_transition) = 0;
  virtual void SimulateAnalyticsCuesForTest(
      JNIEnv* env,
      const CueSnapshot& cues) = 0;
  virtual void SimulateCurrentCuesForTest(
      JNIEnv* env,
      const CueSnapshot& cues) = 0;
  virtual void SimulateAnalyticsMetadataForTest(
      JNIEnv* env,
      const AnalyticsMetadataEvent& metadata) = 0;
  virtual void SimulateAnalyticsDeviceInfoChangedForTest(
      JNIEnv* env,
      const DeviceInfoDescriptor& device_info) = 0;
  virtual void SimulateAnalyticsMediaMetadataChangedForTest(
      JNIEnv* env,
      const MediaMetadataSnapshot& metadata) = 0;
  virtual void SimulateAnalyticsPlaylistMetadataChangedForTest(
      JNIEnv* env,
      const MediaMetadataSnapshot& metadata) = 0;
  virtual void SimulateVideoInputFormatChangedForTest(
      JNIEnv* env,
      const VideoInputFormatChangedEvent& video_input_format_changed) = 0;
  virtual void SimulateImageOutputForTest(
      JNIEnv* env,
      const ImageFrameSnapshot& image_frame) = 0;
  virtual PlayerConfig::MediaSourceFactoryConfig GetMediaSourceFactoryConfig(JNIEnv* env) = 0;
  virtual int GetAvailableCommandCount(JNIEnv* env) = 0;
  virtual AvailableCommandsSnapshot GetAvailableCommands(JNIEnv* env) = 0;
  virtual TimelineSnapshot GetTimelineSnapshot(JNIEnv* env) = 0;
  virtual std::vector<TimelineWindowSnapshot> GetTimelineWindows(JNIEnv* env) = 0;
  virtual std::vector<TimelinePeriodSnapshot> GetTimelinePeriods(JNIEnv* env) = 0;
  virtual CueSnapshot GetCurrentCues(JNIEnv* env) = 0;
  virtual std::string GetCurrentMediaItemDebugSummary(JNIEnv* env) = 0;
  virtual PlaybackSnapshot GetSnapshot(JNIEnv* env) = 0;
  virtual void Release(JNIEnv* env) = 0;
};

}  // namespace androidx::media3::cppbridge

#endif  // ANDROIDX_MEDIA3_EXOPLAYER_CPPBRIDGE_EXOPLAYER_BRIDGE_H_
