#include "exoplayer_cppbridge_jni_internal.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>

using namespace androidx::media3::cppbridge;
using namespace androidx::media3::cppbridge::internal;

namespace {

constexpr int kTrackTypeAudio = 1;
constexpr int kTrackTypeText = 3;
constexpr char kDemoHttpUrl[] =
    "https://storage.googleapis.com/exoplayer-test-media-0/BigBuckBunny_320x180.mp4";
constexpr char kDemoDashUrl[] =
    "https://storage.googleapis.com/wvmedia/clear/h264/tears/tears.mpd";
constexpr char kDemoHlsUrl[] =
    "https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_4x3/"
    "bipbop_4x3_variant.m3u8";

struct DemoTrackCandidate {
  std::string group_id;
  int track_type = 0;
  int track_index = 0;
  bool selected = false;
  std::string label;
};

MediaSourceType ToMediaSourceType(int source_type) {
  switch (source_type) {
    case 1:
      return MediaSourceType::kDash;
    case 2:
      return MediaSourceType::kHls;
    case 3:
      return MediaSourceType::kSmoothStreaming;
    case 4:
      return MediaSourceType::kRtsp;
    case 5:
      return MediaSourceType::kProgressive;
    default:
      return MediaSourceType::kDefault;
  }
}

std::string GetTrackTypeLabel(int track_type) {
  switch (track_type) {
    case kTrackTypeAudio:
      return "audio";
    case kTrackTypeText:
      return "text";
    default:
      return "track";
  }
}

std::string BuildTrackDisplayLabel(const TrackInfo& track, int track_index, int track_type) {
  std::string label;
  if (!track.label.empty()) {
    label = track.label;
  } else if (!track.language.empty()) {
    label = track.language;
  } else if (!track.mime_type.empty()) {
    label = track.mime_type;
  } else if (!track.id.empty()) {
    label = track.id;
  } else {
    label = GetTrackTypeLabel(track_type) + "-" + std::to_string(track_index);
  }
  if (track_type == kTrackTypeAudio && track.channel_count > 0) {
    label += ", " + std::to_string(track.channel_count) + "ch";
  }
  if (track.bitrate > 0) {
    label += ", " + std::to_string(track.bitrate) + "bps";
  }
  return label;
}

std::vector<DemoTrackCandidate> BuildTrackCandidates(
    const std::vector<TrackGroupSnapshot>& groups,
    int track_type) {
  std::vector<DemoTrackCandidate> candidates;
  for (const auto& group : groups) {
    if (group.type != track_type) {
      continue;
    }
    for (int i = 0; i < static_cast<int>(group.tracks.size()); ++i) {
      const auto& track = group.tracks[static_cast<size_t>(i)];
      if (!track.selected && !track.supported && !track.supported_within_capabilities) {
        continue;
      }
      DemoTrackCandidate candidate;
      candidate.group_id = group.id;
      candidate.track_type = track_type;
      candidate.track_index = i;
      candidate.selected = track.selected;
      candidate.label = BuildTrackDisplayLabel(track, i, track_type);
      candidates.push_back(candidate);
    }
  }
  return candidates;
}

void RemoveDisabledTrackType(
    TrackSelectionParametersDescriptor* parameters,
    int track_type) {
  parameters->disabled_track_types.erase(
      std::remove(
          parameters->disabled_track_types.begin(),
          parameters->disabled_track_types.end(),
          track_type),
      parameters->disabled_track_types.end());
}

void RemoveOverridesForTrackType(
    TrackSelectionParametersDescriptor* parameters,
    int track_type) {
  parameters->overrides.erase(
      std::remove_if(
          parameters->overrides.begin(),
          parameters->overrides.end(),
          [track_type](const TrackSelectionParametersDescriptor::OverrideDescriptor& override) {
            return override.track_type == track_type;
          }),
      parameters->overrides.end());
}

MediaItemDescriptor BuildDemoMediaItem(
    const std::string& uri,
    int source_type,
    const std::string& mime_type,
    const std::string& media_id,
    const std::string& title) {
  MediaItemDescriptor descriptor;
  descriptor.uri = uri;
  descriptor.media_id = media_id;
  descriptor.source_type = ToMediaSourceType(source_type);
  descriptor.mime_type = mime_type;
  descriptor.media_metadata.title = title;
  descriptor.media_metadata.display_title = title;
  descriptor.media_metadata.artist = "CppBridge Demo";
  descriptor.media_metadata.artwork_uri = "https://example.com/demo-artwork.jpg";
  return descriptor;
}

std::string BuildDemoCurrentItemSummary(const MediaItemDescriptor& item) {
  std::string summary = "mediaId=" + item.media_id;
  summary += ",uri=" + item.uri;
  summary += ",sourceType=" + std::to_string(static_cast<int>(item.source_type));
  summary += ",mimeType=" + item.mime_type;
  summary += ",tagPresent=" + std::to_string(item.tag_present ? 1 : 0);
  summary += ",tagString=" + item.tag_string;
  summary += ",subtitleCount=" + std::to_string(item.subtitle_configurations.size());
  if (!item.subtitle_configurations.empty()) {
    const auto& subtitle = item.subtitle_configurations.front();
    summary += ",subtitle0Language=" + subtitle.language;
    summary += ",subtitle0Label=" + subtitle.label;
  }
  return summary;
}

std::string BuildDemoCurrentMetadataSummary(const MediaMetadataSnapshot& metadata) {
  std::string summary = "title=" + metadata.title;
  summary += ",artist=" + metadata.artist;
  summary += ",albumTitle=" + metadata.album_title;
  summary += ",displayTitle=" + metadata.display_title;
  summary += ",subtitle=" + metadata.subtitle;
  summary += ",description=" + metadata.description;
  summary += ",genre=" + metadata.genre;
  summary += ",station=" + metadata.station;
  summary += ",artworkUri=" + metadata.artwork_uri;
  summary += ",artworkDataLength=" + std::to_string(metadata.artwork_data.size());
  summary += ",extrasPresent=" + std::to_string(metadata.extras_present ? 1 : 0);
  summary += ",extrasKeyCount=" + std::to_string(metadata.extras_key_count);
  return summary;
}

std::string BuildDemoTimelineSummary(const TimelineDetailsSnapshot& timeline) {
  std::string summary = "windowCount=" + std::to_string(timeline.summary.window_count);
  summary += ",periodCount=" + std::to_string(timeline.summary.period_count);
  summary += ",empty=" + std::to_string(timeline.summary.empty ? 1 : 0);
  if (!timeline.windows.empty()) {
    const auto& window = timeline.windows.front();
    summary += ",window0MediaId=" + window.media_item_id;
    summary += ",window0MediaUri=" + window.media_item_uri;
    summary += ",window0TagString=" + window.media_item_tag_string;
    summary += ",window0Seekable=" + std::to_string(window.is_seekable ? 1 : 0);
    summary += ",window0Live=" + std::to_string(window.is_live ? 1 : 0);
  }
  if (timeline.windows.size() > 1) {
    const auto& window = timeline.windows[1];
    summary += ",window1MediaId=" + window.media_item_id;
    summary += ",window1MediaUri=" + window.media_item_uri;
    summary += ",window1TagString=" + window.media_item_tag_string;
  }
  if (!timeline.periods.empty()) {
    const auto& period = timeline.periods.front();
    summary += ",period0Uid=" + period.uid;
    summary += ",period0DurationUs=" + std::to_string(period.duration_us);
  }
  return summary;
}

std::string BuildDemoCueSummary(const CueSnapshot& cues) {
  std::string summary = "cueCount=" + std::to_string(cues.cue_count);
  summary += ",presentationTimeUs=" + std::to_string(cues.presentation_time_us);
  if (!cues.cues.empty()) {
    summary += ",cue0Text=" + cues.cues[0].text;
    summary += ",cue0BitmapTokenPresent=" +
        std::to_string(cues.cues[0].bitmap_token.empty() ? 0 : 1);
  }
  if (cues.cues.size() > 1) {
    summary += ",cue1Text=" + cues.cues[1].text;
  }
  return summary;
}

std::string BuildDemoPlaybackSummary(ExoPlayerSdkPlayer* player) {
  PlaybackParametersSnapshot playback_parameters = player->GetPlaybackParameters();
  TracksSnapshot tracks = player->GetTracks();
  MediaItemDescriptor current_item = player->GetCurrentMediaItem();
  std::string summary = "state=" + std::to_string(static_cast<int>(player->GetPlaybackState()));
  summary += ",playing=" + std::to_string(player->IsPlaying() ? 1 : 0);
  summary += ",loading=" + std::to_string(player->IsLoading() ? 1 : 0);
  summary += ",positionMs=" + std::to_string(player->GetCurrentPosition());
  summary += ",bufferedMs=" + std::to_string(player->GetBufferedPosition());
  summary += ",durationMs=" + std::to_string(player->GetDuration());
  summary += ",itemIndex=" + std::to_string(player->GetCurrentMediaItemIndex());
  summary += ",itemCount=" + std::to_string(player->GetMediaItemCount());
  summary += ",speed=" + std::to_string(playback_parameters.speed);
  summary += ",pitch=" + std::to_string(playback_parameters.pitch);
  summary += ",repeat=" + std::to_string(static_cast<int>(player->GetRepeatMode()));
  summary += ",shuffle=" + std::to_string(player->GetShuffleModeEnabled() ? 1 : 0);
  summary += ",mediaId=" + current_item.media_id;
  summary += ",sourceType=" + std::to_string(static_cast<int>(current_item.source_type));
  summary += ",mimeType=" + current_item.mime_type;
  summary += ",audioSelected=" + std::to_string(tracks.audio_selected ? 1 : 0);
  summary += ",videoSelected=" + std::to_string(tracks.video_selected ? 1 : 0);
  summary += ",textSelected=" + std::to_string(tracks.text_selected ? 1 : 0);
  summary += "\nitemUri=" + current_item.uri;
  summary += "\ntrackGroups=" + std::to_string(tracks.groups.size());
  summary += ",hasNext=" + std::to_string(player->HasNextMediaItem() ? 1 : 0);
  summary += ",hasPrevious=" + std::to_string(player->HasPreviousMediaItem() ? 1 : 0);
  return summary;
}

std::string CycleTrackSelection(
    ExoPlayerSdkPlayer* player,
    int track_type,
    bool allow_disable_after_last) {
  TracksSnapshot tracks = player->GetTracks();
  std::vector<DemoTrackCandidate> candidates = BuildTrackCandidates(tracks.groups, track_type);
  TrackSelectionParametersDescriptor parameters = player->GetTrackSelectionParameters();
  RemoveOverridesForTrackType(&parameters, track_type);
  RemoveDisabledTrackType(&parameters, track_type);

  if (track_type == kTrackTypeAudio) {
    parameters.disable_audio = false;
    parameters.preferred_audio_language.clear();
    parameters.preferred_audio_languages.clear();
  } else if (track_type == kTrackTypeText) {
    parameters.disable_text = false;
    parameters.preferred_text_language.clear();
    parameters.preferred_text_languages.clear();
    parameters.select_text_by_default = true;
  }

  if (candidates.empty()) {
    if (track_type == kTrackTypeText) {
      parameters.disable_text = true;
      parameters.disabled_track_types.push_back(kTrackTypeText);
      player->SetTrackSelectionParameters(parameters);
      return "No text tracks are available; text remains disabled.";
    }
    return "No switchable audio tracks are available for the current stream.";
  }

  int selected_index = -1;
  for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
    if (candidates[static_cast<size_t>(i)].selected) {
      selected_index = i;
      break;
    }
  }

  if (allow_disable_after_last && selected_index == static_cast<int>(candidates.size()) - 1) {
    parameters.disable_text = true;
    parameters.disabled_track_types.push_back(kTrackTypeText);
    player->SetTrackSelectionParameters(parameters);
    return "Text tracks disabled.";
  }

  int target_index = selected_index < 0 ? 0 : (selected_index + 1) % candidates.size();
  const DemoTrackCandidate& target = candidates[static_cast<size_t>(target_index)];
  parameters.overrides.push_back(
      {target.group_id, target.track_type, {target.track_index}});
  player->SetTrackSelectionParameters(parameters);

  std::string message = GetTrackTypeLabel(track_type);
  message[0] = static_cast<char>(std::toupper(message[0]));
  message += " track -> ";
  message += target.label;
  return message;
}

}  // namespace

extern "C" {

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnPlaybackStateChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint playback_state) {
  BridgeOnPlaybackStateChanged(native_handle, playback_state);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnPlayWhenReadyChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jboolean play_when_ready,
    jint reason) {
  BridgeOnPlayWhenReadyChanged(native_handle, JNI_FALSE != play_when_ready, reason);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnIsPlayingChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jboolean is_playing) {
  BridgeOnIsPlayingChanged(native_handle, JNI_FALSE != is_playing);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnMediaItemTransition(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint media_item_index,
    jint reason) {
  BridgeOnMediaItemTransition(native_handle, media_item_index, reason);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnPlayerError(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jint error_code,
    jstring message) {
  BridgeOnPlayerError(env, native_handle, error_code, message);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnPlayerErrorChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jint error_code,
    jstring message) {
  BridgeOnPlayerErrorChanged(env, native_handle, error_code, message);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnTimelineChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint window_count,
    jint period_count,
    jint reason) {
  BridgeOnTimelineChanged(native_handle, window_count, period_count, reason);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnTracksChanged(
    JNIEnv*,
    jclass,
    jlong native_handle) {
  BridgeOnTracksChanged(native_handle);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnPositionDiscontinuity(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject old_position,
    jobject new_position,
    jint reason) {
  BridgeOnPositionDiscontinuity(env, native_handle, old_position, new_position, reason);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAudioAttributesChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint content_type,
    jint usage,
    jint flags,
    jint allowed_capture_policy,
    jint spatialization_behavior) {
  BridgeOnAudioAttributesChanged(
      native_handle,
      content_type,
      usage,
      flags,
      allowed_capture_policy,
      spatialization_behavior);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnCues(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint cue_count,
    jlong presentation_time_us) {
  BridgeOnCues(native_handle, cue_count, presentation_time_us);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnRepeatModeChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint repeat_mode) {
  BridgeOnRepeatModeChanged(native_handle, repeat_mode);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnShuffleModeEnabledChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jboolean shuffle_mode_enabled) {
  BridgeOnShuffleModeEnabledChanged(native_handle, JNI_FALSE != shuffle_mode_enabled);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnSeekBackIncrementChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jlong seek_back_increment_ms) {
  BridgeOnSeekBackIncrementChanged(native_handle, static_cast<int64_t>(seek_back_increment_ms));
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnSeekForwardIncrementChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jlong seek_forward_increment_ms) {
  BridgeOnSeekForwardIncrementChanged(
      native_handle, static_cast<int64_t>(seek_forward_increment_ms));
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnMaxSeekToPreviousPositionChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jlong max_seek_to_previous_position_ms) {
  BridgeOnMaxSeekToPreviousPositionChanged(
      native_handle, static_cast<int64_t>(max_seek_to_previous_position_ms));
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnTrackSelectionParametersChanged(
    JNIEnv*,
    jclass,
    jlong native_handle) {
  BridgeOnTrackSelectionParametersChanged(native_handle);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnPlaybackParametersChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jfloat speed,
    jfloat pitch) {
  BridgeOnPlaybackParametersChanged(native_handle, speed, pitch);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnPlaybackSuppressionReasonChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint playback_suppression_reason) {
  BridgeOnPlaybackSuppressionReasonChanged(native_handle, playback_suppression_reason);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAvailableCommandsChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject commands_object) {
  BridgeOnAvailableCommandsChanged(env, native_handle, commands_object);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnEvents(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject events_object) {
  BridgeOnEvents(env, native_handle, events_object);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnDeviceInfoChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject device_info_object) {
  BridgeOnDeviceInfoChanged(env, native_handle, device_info_object);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnDeviceVolumeChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint volume,
    jboolean muted) {
  BridgeOnDeviceVolumeChanged(native_handle, volume, JNI_FALSE != muted);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnSkipSilenceEnabledChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jboolean skip_silence_enabled) {
  BridgeOnSkipSilenceEnabledChanged(native_handle, JNI_FALSE != skip_silence_enabled);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnVideoSizeChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject video_size_object) {
  BridgeOnVideoSizeChanged(env, native_handle, video_size_object);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnSurfaceSizeChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint width,
    jint height) {
  BridgeOnSurfaceSizeChanged(native_handle, width, height);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnRenderedFirstFrame(
    JNIEnv*,
    jclass,
    jlong native_handle) {
  BridgeOnRenderedFirstFrame(native_handle);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnMediaMetadataChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject metadata_object) {
  BridgeOnMediaMetadataChanged(env, native_handle, metadata_object);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnPlaylistMetadataChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject metadata_object) {
  BridgeOnPlaylistMetadataChanged(env, native_handle, metadata_object);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsUpdated(
    JNIEnv*,
    jclass,
    jlong native_handle) {
  BridgeOnAnalyticsUpdated(native_handle);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsAudioUnderrun(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint buffer_size,
    jlong buffer_size_ms,
    jlong elapsed_since_last_feed_ms) {
  BridgeOnAudioUnderrun(
      native_handle, buffer_size, buffer_size_ms, elapsed_since_last_feed_ms);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsDroppedVideoFrames(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint dropped_frames,
    jlong elapsed_ms) {
  BridgeOnDroppedVideoFrames(native_handle, dropped_frames, elapsed_ms);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsBandwidthEstimate(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint elapsed_ms,
    jlong bytes_transferred,
    jlong bitrate_estimate) {
  BridgeOnBandwidthEstimate(
      native_handle, elapsed_ms, bytes_transferred, bitrate_estimate);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsLoadStarted(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jstring uri,
    jint data_type,
    jint track_type,
    jint retry_count) {
  BridgeOnLoadStarted(env, native_handle, uri, data_type, track_type, retry_count);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsLoadCompleted(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jstring uri,
    jint data_type,
    jint track_type) {
  BridgeOnLoadCompleted(env, native_handle, uri, data_type, track_type);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsAudioInputFormatChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jstring sample_mime_type,
    jstring codecs,
    jint channel_count,
    jint sample_rate) {
  BridgeOnAudioInputFormatChanged(
      env, native_handle, sample_mime_type, codecs, channel_count, sample_rate);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsAudioDecoderInitialized(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jstring decoder_name,
    jlong initialized_timestamp_ms,
    jlong initialization_duration_ms) {
  BridgeOnAudioDecoderInitialized(
      env,
      native_handle,
      decoder_name,
      initialized_timestamp_ms,
      initialization_duration_ms);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsVideoDecoderInitialized(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jstring decoder_name,
    jlong initialized_timestamp_ms,
    jlong initialization_duration_ms) {
  BridgeOnVideoDecoderInitialized(
      env,
      native_handle,
      decoder_name,
      initialized_timestamp_ms,
      initialization_duration_ms);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsAudioDecoderReleased(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jstring decoder_name) {
  BridgeOnAudioDecoderReleased(env, native_handle, decoder_name);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsVideoDecoderReleased(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jstring decoder_name) {
  BridgeOnVideoDecoderReleased(env, native_handle, decoder_name);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsRenderedFirstFrame(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jlong render_time_ms) {
  BridgeOnAnalyticsRenderedFirstFrame(native_handle, render_time_ms);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsVideoSizeChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint width,
    jint height,
    jfloat pixel_width_height_ratio) {
  BridgeOnAnalyticsVideoSizeChanged(
      native_handle, width, height, pixel_width_height_ratio);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsAudioPositionAdvancing(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jlong playout_start_system_time_ms) {
  BridgeOnAudioPositionAdvancing(native_handle, playout_start_system_time_ms);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsVideoFrameProcessingOffset(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jlong total_processing_offset_us,
    jint frame_count) {
  BridgeOnVideoFrameProcessingOffset(
      native_handle, total_processing_offset_us, frame_count);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsVolumeChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jfloat volume) {
  BridgeOnVolumeChanged(native_handle, volume);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsAudioSessionIdChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint audio_session_id) {
  BridgeOnAudioSessionIdChanged(native_handle, audio_session_id);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsSkipSilenceEnabledChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jboolean skip_silence_enabled) {
  BridgeOnAnalyticsSkipSilenceEnabledChanged(
      native_handle, JNI_FALSE != skip_silence_enabled);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsDeviceVolumeChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint volume,
    jboolean muted) {
  BridgeOnAnalyticsDeviceVolumeChanged(native_handle, volume, JNI_FALSE != muted);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsPlaybackStateChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint playback_state) {
  BridgeOnAnalyticsPlaybackStateChanged(native_handle, playback_state);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsIsPlayingChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jboolean is_playing) {
  BridgeOnAnalyticsIsPlayingChanged(native_handle, JNI_FALSE != is_playing);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsPlayWhenReadyChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jboolean play_when_ready,
    jint reason) {
  BridgeOnAnalyticsPlayWhenReadyChanged(
      native_handle, JNI_FALSE != play_when_ready, reason);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsPlaybackSuppressionReasonChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint playback_suppression_reason) {
  BridgeOnAnalyticsPlaybackSuppressionReasonChanged(
      native_handle, playback_suppression_reason);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsIsLoadingChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jboolean is_loading) {
  BridgeOnAnalyticsIsLoadingChanged(native_handle, JNI_FALSE != is_loading);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsRepeatModeChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint repeat_mode) {
  BridgeOnAnalyticsRepeatModeChanged(native_handle, repeat_mode);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsShuffleModeChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jboolean shuffle_mode_enabled) {
  BridgeOnAnalyticsShuffleModeChanged(native_handle, JNI_FALSE != shuffle_mode_enabled);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsPlaybackParametersChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jfloat speed,
    jfloat pitch) {
  BridgeOnAnalyticsPlaybackParametersChanged(native_handle, speed, pitch);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsAvailableCommandsChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject commands) {
  BridgeOnAnalyticsAvailableCommandsChanged(env, native_handle, commands);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsEvents(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject events) {
  BridgeOnAnalyticsEvents(env, native_handle, events);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsSeekBackIncrementChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jlong seek_back_increment_ms) {
  BridgeOnAnalyticsSeekBackIncrementChanged(native_handle, seek_back_increment_ms);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsSeekForwardIncrementChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jlong seek_forward_increment_ms) {
  BridgeOnAnalyticsSeekForwardIncrementChanged(native_handle, seek_forward_increment_ms);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsMaxSeekToPreviousPositionChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jlong max_seek_to_previous_position_ms) {
  BridgeOnAnalyticsMaxSeekToPreviousPositionChanged(
      native_handle, max_seek_to_previous_position_ms);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsTimelineChanged(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint reason) {
  BridgeOnAnalyticsTimelineChanged(native_handle, reason);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsPositionDiscontinuity(
    JNIEnv*,
    jclass,
    jlong native_handle,
    jint reason) {
  BridgeOnAnalyticsPositionDiscontinuity(native_handle, reason);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsSeekStarted(
    JNIEnv*,
    jclass,
    jlong native_handle) {
  BridgeOnAnalyticsSeekStarted(native_handle);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsPlayerError(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jint error_code,
    jstring message) {
  BridgeOnAnalyticsPlayerError(env, native_handle, error_code, message);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsPlayerErrorChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jint error_code,
    jstring message) {
  BridgeOnAnalyticsPlayerErrorChanged(env, native_handle, error_code, message);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsTracksChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject tracks) {
  BridgeOnAnalyticsTracksChanged(env, native_handle, tracks);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsMediaItemTransition(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject media_item,
    jint reason) {
  BridgeOnAnalyticsMediaItemTransition(env, native_handle, media_item, reason);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsCues(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobjectArray cues,
    jlong presentation_time_us) {
  BridgeOnAnalyticsCues(env, native_handle, cues, presentation_time_us);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsMetadata(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jint entry_count,
    jstring first_entry_type,
    jstring first_entry_text) {
  BridgeOnAnalyticsMetadata(
      env,
      native_handle,
      entry_count,
      first_entry_type,
      first_entry_text);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsLoadError(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jstring uri,
    jint data_type,
    jint track_type,
    jstring message,
    jboolean was_canceled) {
  BridgeOnAnalyticsLoadError(
      env,
      native_handle,
      uri,
      data_type,
      track_type,
      message,
      was_canceled);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsDeviceInfoChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject device_info) {
  BridgeOnAnalyticsDeviceInfoChanged(env, native_handle, device_info);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsMediaMetadataChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject metadata) {
  BridgeOnAnalyticsMediaMetadataChanged(env, native_handle, metadata);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsPlaylistMetadataChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jobject metadata) {
  BridgeOnAnalyticsPlaylistMetadataChanged(env, native_handle, metadata);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnAnalyticsVideoInputFormatChanged(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jstring sample_mime_type,
    jstring codecs,
    jint width,
    jint height,
    jfloat frame_rate) {
  BridgeOnVideoInputFormatChanged(
      env, native_handle, sample_mime_type, codecs, width, height, frame_rate);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnImageOutputAvailable(
    JNIEnv* env,
    jclass,
    jlong native_handle,
    jlong presentation_time_us,
    jint width,
    jint height,
    jint byte_count,
    jint allocation_byte_count,
    jint row_bytes,
    jboolean has_alpha,
    jboolean is_premultiplied,
    jboolean is_mutable,
    jstring bitmap_config) {
  BridgeOnImageOutputAvailable(
      env,
      native_handle,
      presentation_time_us,
      width,
      height,
      byte_count,
      allocation_byte_count,
      row_bytes,
      has_alpha == JNI_TRUE,
      is_premultiplied == JNI_TRUE,
      is_mutable == JNI_TRUE,
      bitmap_config);
}

JNIEXPORT void JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppExoPlayerBridge_nativeOnImageOutputDisabled(
    JNIEnv*,
    jclass,
    jlong native_handle) {
  BridgeOnImageOutputDisabled(native_handle);
}

JNIEXPORT jlong JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeCreatePlayer(
    JNIEnv* env,
    jobject,
    jobject context,
    jobject player_view) {
  PlayerConfig config;
  std::unique_ptr<ExoPlayerSdkPlayer> bridge = ExoPlayerSdkPlayer::Create(env, context, config);
  bridge->SetListener(&GetDemoLoggingPlayerListener());
  bridge->BindPlayerView(player_view);
  ExoPlayerSdkPlayer* released_bridge = bridge.release();
  RegisterDemoPlayer(released_bridge);
  return reinterpret_cast<jlong>(released_bridge);
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeLoadMedia(
    JNIEnv* env,
    jobject,
    jlong native_handle,
    jstring media_url,
    jint source_type,
    jstring mime_type) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player == nullptr) {
    return NewStringUtfChecked(env, "Player not initialized", "nativeLoadMedia.error");
  }
  MediaItemDescriptor descriptor = BuildDemoMediaItem(
      JStringToString(env, media_url),
      source_type,
      JStringToString(env, mime_type),
      "demo-item",
      "CppBridge Demo Item");
  player->SetMediaItem(descriptor);
  player->Prepare();
  std::string summary = "Loaded media via C++ API\n";
  summary += BuildDemoCurrentItemSummary(player->GetCurrentMediaItem());
  return NewStringUtfChecked(env, summary, "nativeLoadMedia");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeLoadMediaWithSubtitle(
    JNIEnv* env,
    jobject,
    jlong native_handle,
    jstring media_url,
    jint source_type,
    jstring mime_type,
    jstring subtitle_url) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env, "Player not initialized", "nativeLoadMediaWithSubtitle.error");
  }
  MediaItemDescriptor descriptor = BuildDemoMediaItem(
      JStringToString(env, media_url),
      source_type,
      JStringToString(env, mime_type),
      "subtitle-demo-item",
      "CppBridge Subtitle Demo");
  MediaItemDescriptor::SubtitleConfigurationDescriptor subtitle;
  subtitle.uri = JStringToString(env, subtitle_url);
  subtitle.mime_type = "text/vtt";
  subtitle.language = "en";
  subtitle.label = "English";
  descriptor.subtitle_configurations.push_back(subtitle);
  player->SetMediaItem(descriptor);
  player->Prepare();
  std::string summary = "Loaded media + subtitle via C++ API\n";
  summary += BuildDemoCurrentItemSummary(player->GetCurrentMediaItem());
  return NewStringUtfChecked(env, summary, "nativeLoadMediaWithSubtitle");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeLoadDemoPlaylist(
    JNIEnv* env,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player == nullptr) {
    return NewStringUtfChecked(env, "Player not initialized", "nativeLoadDemoPlaylist.error");
  }
  std::vector<MediaItemDescriptor> items;
  items.push_back(
      BuildDemoMediaItem(kDemoHttpUrl, 5, "video/mp4", "playlist-http", "HTTP Progressive"));
  items.push_back(BuildDemoMediaItem(
      kDemoDashUrl, 1, "application/dash+xml", "playlist-dash", "DASH Sample"));
  items.push_back(BuildDemoMediaItem(
      kDemoHlsUrl, 2, "application/x-mpegURL", "playlist-hls", "HLS Sample"));
  player->SetMediaItems(items, 0, 0);
  player->Prepare();
  std::string summary = "Loaded mixed playlist via C++ API\n";
  summary += BuildPlaylistIdsSummary(items);
  return NewStringUtfChecked(env, summary, "nativeLoadDemoPlaylist");
}

JNIEXPORT void JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativePlay(
    JNIEnv*,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player != nullptr) {
    player->Play();
  }
}

JNIEXPORT void JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativePause(
    JNIEnv*,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player != nullptr) {
    player->Pause();
  }
}

JNIEXPORT void JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeStop(
    JNIEnv*,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player != nullptr) {
    player->Stop();
  }
}

JNIEXPORT void JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeSeekTo(
    JNIEnv*,
    jobject,
    jlong native_handle,
    jlong position_ms) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player != nullptr) {
    player->SeekTo(position_ms);
  }
}

JNIEXPORT void JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeSeekBack(
    JNIEnv*,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player != nullptr) {
    player->SeekBack();
  }
}

JNIEXPORT void JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeSeekForward(
    JNIEnv*,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player != nullptr) {
    player->SeekForward();
  }
}

JNIEXPORT void JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeSeekToNext(
    JNIEnv*,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player != nullptr) {
    player->SeekToNextMediaItem();
  }
}

JNIEXPORT void JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeSeekToPrevious(
    JNIEnv*,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player != nullptr) {
    player->SeekToPreviousMediaItem();
  }
}

JNIEXPORT void JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeSetPlaybackSpeed(
    JNIEnv*,
    jobject,
    jlong native_handle,
    jfloat speed) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player != nullptr) {
    player->SetPlaybackSpeed(speed);
  }
}

JNIEXPORT void JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativePreferTextLanguage(
    JNIEnv* env,
    jobject,
    jlong native_handle,
    jstring language) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player == nullptr) {
    return;
  }
  TrackSelectionParametersDescriptor parameters = player->GetTrackSelectionParameters();
  parameters.preferred_text_language = JStringToString(env, language);
  parameters.preferred_text_languages = {parameters.preferred_text_language};
  parameters.disable_text = false;
  RemoveDisabledTrackType(&parameters, kTrackTypeText);
  RemoveOverridesForTrackType(&parameters, kTrackTypeText);
  parameters.select_text_by_default = true;
  player->SetTrackSelectionParameters(parameters);
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeCycleAudioTrack(
    JNIEnv* env,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player == nullptr) {
    return NewStringUtfChecked(env, "Player not initialized", "nativeCycleAudioTrack.error");
  }
  std::string summary = CycleTrackSelection(player, kTrackTypeAudio, false);
  return NewStringUtfChecked(env, summary, "nativeCycleAudioTrack");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeCycleTextTrack(
    JNIEnv* env,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player == nullptr) {
    return NewStringUtfChecked(env, "Player not initialized", "nativeCycleTextTrack.error");
  }
  std::string summary = CycleTrackSelection(player, kTrackTypeText, true);
  return NewStringUtfChecked(env, summary, "nativeCycleTextTrack");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeGetPlaybackSummary(
    JNIEnv* env,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player == nullptr) {
    return NewStringUtfChecked(env, "Player not initialized", "nativeGetPlaybackSummary.error");
  }
  std::string summary = BuildDemoPlaybackSummary(player);
  return NewStringUtfChecked(env, summary, "nativeGetPlaybackSummary");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeGetTrackSummary(
    JNIEnv* env,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player == nullptr) {
    return NewStringUtfChecked(env, "Player not initialized", "nativeGetTrackSummary.error");
  }
  std::string summary = BuildTrackSummary(player->GetTrackGroups());
  return NewStringUtfChecked(env, summary, "nativeGetTrackSummary");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeGetCurrentItemSummary(
    JNIEnv* env,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player == nullptr) {
    return NewStringUtfChecked(env, "Player not initialized", "nativeGetCurrentItemSummary.error");
  }
  std::string summary = BuildDemoCurrentItemSummary(player->GetCurrentMediaItem());
  return NewStringUtfChecked(env, summary, "nativeGetCurrentItemSummary");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeGetCurrentMetadataSummary(
    JNIEnv* env,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env, "Player not initialized", "nativeGetCurrentMetadataSummary.error");
  }
  std::string summary = BuildDemoCurrentMetadataSummary(player->GetMediaMetadata());
  return NewStringUtfChecked(env, summary, "nativeGetCurrentMetadataSummary");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeGetTimelineSummary(
    JNIEnv* env,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player == nullptr) {
    return NewStringUtfChecked(env, "Player not initialized", "nativeGetTimelineSummary.error");
  }
  std::string summary = BuildDemoTimelineSummary(player->GetTimeline());
  return NewStringUtfChecked(env, summary, "nativeGetTimelineSummary");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeGetCurrentCuesSummary(
    JNIEnv* env,
    jobject,
    jlong native_handle) {
  ExoPlayerSdkPlayer* player = AcquireDemoPlayer(native_handle);
  if (player == nullptr) {
    return NewStringUtfChecked(env, "Player not initialized", "nativeGetCurrentCuesSummary.error");
  }
  std::string summary = BuildDemoCueSummary(player->GetCurrentCues());
  return NewStringUtfChecked(env, summary, "nativeGetCurrentCuesSummary");
}

JNIEXPORT void JNICALL
Java_androidx_media3_demo_cppbridge_MainActivity_nativeRelease(
    JNIEnv*,
    jobject,
    jlong native_handle,
    jobject player_view) {
  ExoPlayerSdkPlayer* bridge = AcquireDemoPlayer(native_handle);
  if (bridge == nullptr) {
    return;
  }
  UnregisterDemoPlayer(bridge);
  bridge->UnbindPlayerView(player_view);
  bridge->Release();
  delete bridge;
}

}  // extern "C"
