#include "exoplayer_cppbridge_jni_internal.h"

namespace androidx::media3::cppbridge::internal {

namespace {

const char* GetTrackTypeName(int track_type) {
  switch (track_type) {
    // These numeric values mirror Media3 C.TRACK_TYPE_AUDIO/VIDEO/TEXT.
    case 1:
      return "Audio";
    case 2:
      return "Video";
    case 3:
      return "Text";
    default:
      return "Unknown";
  }
}

}  // namespace

std::string BuildTrackSummary(const std::vector<TrackGroupSnapshot>& groups) {
  if (groups.empty()) {
    return "No track groups";
  }
  std::string summary;
  for (size_t i = 0; i < groups.size(); ++i) {
    const auto& group = groups[i];
    if (!summary.empty()) {
      summary += "\n";
    }
    summary += GetTrackTypeName(group.type);
    summary += " group";
    summary += group.selected ? " [selected]" : "";
    summary += ": ";
    summary += std::to_string(group.tracks.size());
    summary += " tracks";
    for (size_t j = 0; j < group.tracks.size(); ++j) {
      const auto& track = group.tracks[j];
      summary += "\n  - ";
      if (!track.label.empty()) {
        summary += track.label;
      } else if (!track.language.empty()) {
        summary += track.language;
      } else if (!track.mime_type.empty()) {
        summary += track.mime_type;
      } else {
        summary += "track";
      }
      if (track.selected) {
        summary += " [selected]";
      }
    }
  }
  return summary;
}

std::string BuildPlaylistIdsSummary(const std::vector<MediaItemDescriptor>& media_items) {
  std::string summary;
  for (size_t i = 0; i < media_items.size(); ++i) {
    if (!summary.empty()) {
      summary += ",";
    }
    summary += media_items[i].media_id;
  }
  return summary;
}

std::string BuildSnapshotSummary(const PlaybackSnapshot& snapshot) {
  std::string summary = "state=" + std::to_string(static_cast<int>(snapshot.playback_state));
  summary += ",count=" + std::to_string(snapshot.media_item_count);
  summary += ",index=" + std::to_string(snapshot.current_media_item_index);
  summary += ",playWhenReady=" + std::to_string(snapshot.play_when_ready ? 1 : 0);
  summary += ",shuffle=" + std::to_string(snapshot.shuffle_mode_enabled ? 1 : 0);
  summary += ",repeat=" + std::to_string(static_cast<int>(snapshot.repeat_mode));
  summary += ",positionMs=" + std::to_string(snapshot.current_position_ms);
  summary += ",volume=" + std::to_string(snapshot.volume);
  summary += ",speed=" + std::to_string(snapshot.playback_speed);
  return summary;
}

std::string BuildTrackSelectionSummary(const TrackSelectionParametersDescriptor& parameters) {
  std::string summary = "audio=" + parameters.preferred_audio_language;
  summary += ",text=" + parameters.preferred_text_language;
  summary += ",maxAudioChannelCount=" + std::to_string(parameters.max_audio_channel_count);
  summary += ",maxAudioBitrate=" + std::to_string(parameters.max_audio_bitrate);
  summary += ",width=" + std::to_string(parameters.max_video_width);
  summary += ",height=" + std::to_string(parameters.max_video_height);
  summary += ",bitrate=" + std::to_string(parameters.max_video_bitrate);
  summary += ",viewportWidth=" + std::to_string(parameters.viewport_width);
  summary += ",viewportHeight=" + std::to_string(parameters.viewport_height);
  summary += ",viewportOrientationMayChange=" +
      std::to_string(parameters.viewport_orientation_may_change ? 1 : 0);
  summary += ",textDefault=" + std::to_string(parameters.select_text_by_default ? 1 : 0);
  summary += ",ignoredTextSelectionFlags=" +
      std::to_string(parameters.ignored_text_selection_flags);
  summary += ",selectUndeterminedTextLanguage=" +
      std::to_string(parameters.select_undetermined_text_language ? 1 : 0);
  summary += ",lowest=" + std::to_string(parameters.force_lowest_bitrate ? 1 : 0);
  summary += ",disableVideo=" + std::to_string(parameters.disable_video ? 1 : 0);
  summary += ",disableAudio=" + std::to_string(parameters.disable_audio ? 1 : 0);
  summary += ",disableText=" + std::to_string(parameters.disable_text ? 1 : 0);
  summary += ",disabledTrackTypeCount=" + std::to_string(parameters.DisabledTrackTypeCount());
  summary += ",audioTrackTypeDisabled=" + std::to_string(parameters.IsTrackTypeDisabled(1) ? 1 : 0);
  summary += ",videoTrackTypeDisabled=" + std::to_string(parameters.IsTrackTypeDisabled(2) ? 1 : 0);
  summary += ",textTrackTypeDisabled=" + std::to_string(parameters.IsTrackTypeDisabled(3) ? 1 : 0);
  summary += ",overrideCount=" + std::to_string(parameters.OverrideCount());
  return summary;
}

std::vector<MediaItemDescriptor> JStringArrayToMediaItems(JNIEnv* env, jobjectArray urls) {
  std::vector<MediaItemDescriptor> media_items;
  if (urls == nullptr) {
    return media_items;
  }
  jsize size = env->GetArrayLength(urls);
  media_items.reserve(static_cast<size_t>(size));
  for (jsize i = 0; i < size; ++i) {
    auto* value = static_cast<jstring>(env->GetObjectArrayElement(urls, i));
    if (ClearJniExceptionIfPresent(env, "GetObjectArrayElement(java/lang/String)") ||
        value == nullptr) {
      DeleteLocalRefIfNotNull(env, value);
      continue;
    }
    MediaItemDescriptor media_item;
    media_item.uri = JStringToString(env, value);
    // These ids are only demo/test placeholders derived from the original URL order.
    // Callers should not treat them as stable identifiers after later playlist reordering.
    media_item.media_id = "playlist-item-" + std::to_string(i);
    media_items.push_back(media_item);
    env->DeleteLocalRef(value);
  }
  return media_items;
}

}  // namespace androidx::media3::cppbridge::internal
