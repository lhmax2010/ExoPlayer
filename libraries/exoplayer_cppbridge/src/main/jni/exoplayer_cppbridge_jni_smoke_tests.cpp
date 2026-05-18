#include "exoplayer_cppbridge_jni_internal.h"

using namespace androidx::media3::cppbridge;
using namespace androidx::media3::cppbridge::internal;

class CapturingPlayerListener : public PlayerListener {
 public:
  void OnPlaybackStateChanged(const PlaybackSnapshot&) override {}
  void OnPlayWhenReadyChanged(const PlaybackSnapshot&, int) override {}
  void OnIsPlayingChanged(const PlaybackSnapshot&) override {}
  void OnMediaItemTransition(const PlaybackSnapshot&, int) override {}
  void OnPlayerError(const PlaybackSnapshot&) override {}

  void OnTimelineChanged(
      const PlaybackSnapshot&,
      const TimelineDetailsSnapshot& timeline,
      int reason) override {
    timeline_window_count = timeline.summary.window_count;
    timeline_period_count = timeline.summary.period_count;
    timeline_current_media_item_index = timeline.summary.current_media_item_index;
    timeline_change_reason = reason;
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
    first_timeline_window_uid = timeline.windows.empty() ? "" : timeline.windows[0].uid;
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
    first_timeline_window_is_dynamic =
        !timeline.windows.empty() && timeline.windows[0].is_dynamic;
    first_timeline_window_presentation_start_time_ms =
        timeline.windows.empty() ? 0 : timeline.windows[0].presentation_start_time_ms;
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
    second_timeline_window_is_dynamic =
        timeline.windows.size() > 1 && timeline.windows[1].is_dynamic;
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
  }

  void OnTracksChanged(const PlaybackSnapshot&, const TracksSnapshot& tracks) override {
    track_group_count = static_cast<int>(tracks.groups.size());
    first_track_group_type = tracks.groups.empty() ? -1 : tracks.groups[0].type;
    first_track_group_id = tracks.groups.empty() ? "" : tracks.groups[0].id;
    first_track_group_token_present =
        !tracks.groups.empty() && !tracks.groups[0].group_token.empty();
    first_track_count =
        tracks.groups.empty() ? 0 : static_cast<int>(tracks.groups[0].tracks.size());
    first_track_selected =
        !tracks.groups.empty() && !tracks.groups[0].tracks.empty() && tracks.groups[0].tracks[0].selected;
    first_track_supported =
        !tracks.groups.empty() && !tracks.groups[0].tracks.empty() && tracks.groups[0].tracks[0].supported;
    first_track_supported_within_capabilities =
        !tracks.groups.empty() && !tracks.groups[0].tracks.empty() &&
        tracks.groups[0].tracks[0].supported_within_capabilities;
    first_track_label_token_present =
        !tracks.groups.empty() && !tracks.groups[0].tracks.empty() &&
        !tracks.groups[0].tracks[0].label_token.empty();
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
    contains_audio = tracks.contains_audio;
    contains_video = tracks.contains_video;
    contains_text = tracks.contains_text;
    audio_selected = tracks.audio_selected;
    video_selected = tracks.video_selected;
    video_supported = tracks.video_supported;
    video_supported_allowing_exceeds = tracks.video_supported_allowing_exceeds_capabilities;
    tracks_changed_callback_count++;
  }

  void OnPositionDiscontinuity(
      const PlaybackSnapshot&,
      const PositionInfoSnapshot& old_position,
      const PositionInfoSnapshot& new_position,
      int reason) override {
    old_position_media_id = old_position.media_item.media_id;
    old_position_tag_token_present = !old_position.media_item.tag_token.empty();
    new_position_media_id = new_position.media_item.media_id;
    new_position_tag_token_present = !new_position.media_item.tag_token.empty();
    position_discontinuity_reason = reason;
    position_discontinuity_callback_count++;
  }

  void OnAvailableCommandsChanged(
      const PlaybackSnapshot&,
      const AvailableCommandsSnapshot& commands) override {
    available_commands_count = static_cast<int>(commands.command_codes.size());
    available_commands_callback_count++;
  }

  void OnEvents(const PlaybackSnapshot&, const PlayerEventsSnapshot& events) override {
    last_event_count = static_cast<int>(events.event_codes.size());
    events_callback_count++;
  }

  int timeline_window_count = 0;
  int timeline_period_count = 0;
  int timeline_current_media_item_index = 0;
  int timeline_change_reason = 0;
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
  bool first_timeline_window_is_dynamic = false;
  int64_t first_timeline_window_presentation_start_time_ms = 0;
  std::string second_timeline_window_media_item_id;
  std::string second_timeline_window_media_item_uri;
  bool second_timeline_window_media_item_tag_present = false;
  std::string second_timeline_window_media_item_tag_string;
  bool second_timeline_window_media_item_tag_token_present = false;
  bool second_timeline_window_is_dynamic = false;
  std::string first_timeline_period_id;
  bool first_timeline_period_id_token_present = false;
  std::string first_timeline_period_uid;
  bool first_timeline_period_uid_token_present = false;
  std::string first_timeline_period_ads_id;
  bool first_timeline_period_ads_id_token_present = false;
  int first_timeline_period_ad_group_count = 0;
  int64_t first_timeline_period_duration_ms = 0;
  int64_t first_timeline_period_duration_us = 0;
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
  bool first_track_label_token_present = false;
  bool first_track_selected = false;
  bool first_track_supported = false;
  bool first_track_supported_within_capabilities = false;
  std::string second_track_group_id;
  bool second_track_group_token_present = false;
  int second_track_count = 0;
  std::string second_track_label;
  bool second_track_label_token_present = false;
  bool contains_audio = false;
  bool contains_video = false;
  bool contains_text = false;
  bool audio_selected = false;
  bool video_selected = false;
  bool video_supported = false;
  bool video_supported_allowing_exceeds = false;
  std::string old_position_media_id;
  bool old_position_tag_token_present = false;
  std::string new_position_media_id;
  bool new_position_tag_token_present = false;
  int position_discontinuity_reason = 0;
  int available_commands_count = 0;
  int last_event_count = 0;
  int timeline_callback_count = 0;
  int tracks_changed_callback_count = 0;
  int position_discontinuity_callback_count = 0;
  int available_commands_callback_count = 0;
  int events_callback_count = 0;
};

extern "C" {

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeBuildTrackSummaryForTest(
    JNIEnv* env,
    jclass) {
  TrackGroupSnapshot video_group;
  video_group.type = 2;
  video_group.selected = true;
  video_group.adaptive_supported = true;
  TrackInfo hd_track;
  hd_track.id = "video-hd";
  hd_track.label = "English";
  hd_track.mime_type = "video/avc";
  hd_track.selected = true;
  TrackInfo sd_track;
  sd_track.id = "video-sd";
  sd_track.mime_type = "video/avc";
  video_group.tracks = {hd_track, sd_track};

  TrackGroupSnapshot audio_group;
  audio_group.type = 1;
  audio_group.selected = false;
  TrackInfo audio_track;
  audio_track.id = "audio-main";
  audio_track.language = "en";
  audio_track.mime_type = "audio/mp4a-latm";
  audio_group.tracks = {audio_track};

  std::vector<TrackGroupSnapshot> groups = {video_group, audio_group};
  std::string summary = BuildTrackSummary(groups);
  return NewStringUtfChecked(env, summary, "nativeBuildTrackSummaryForTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeTracksSnapshotConversionSmokeTest(
    JNIEnv* env,
    jclass) {
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
      "Ljava/lang/String;IIIIIIIIIJZIIIIFIF"
      "IIIIIIIIIIIIIIIIIIIZZZ)V");
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

  jstring video_hd_id = NewStringUtfChecked(env, "video-hd", "CppTrackInfo.id");
  jstring video_hd_label = NewStringUtfChecked(env, "Main Video", "CppTrackInfo.label");
  jstring video_hd_label_token =
      NewStringUtfChecked(env, "generated-opaque-object-token-track-label", "CppTrackInfo.labelToken");
  jstring video_hd_mime = NewStringUtfChecked(env, "video/avc", "CppTrackInfo.mimeType");
  jstring audio_id = NewStringUtfChecked(env, "audio-main", "CppTrackInfo.id");
  jstring audio_label = NewStringUtfChecked(env, "Main Audio", "CppTrackInfo.label");
  jstring audio_label_token = NewStringUtfChecked(
      env, "generated-opaque-object-token-audio-track-label", "CppTrackInfo.labelToken");
  jstring audio_language = NewStringUtfChecked(env, "en", "CppTrackInfo.language");
  jstring audio_mime = NewStringUtfChecked(env, "audio/mp4a-latm", "CppTrackInfo.mimeType");
  jstring video_group_id = NewStringUtfChecked(env, "video-group", "CppTrackGroup.id");
  jstring video_group_token = NewStringUtfChecked(
      env, "generated-opaque-object-token-track-group-video", "CppTrackGroup.groupToken");
  jstring audio_group_id = NewStringUtfChecked(env, "audio-group", "CppTrackGroup.id");
  jstring audio_group_token = NewStringUtfChecked(
      env, "generated-opaque-object-token-track-group-audio", "CppTrackGroup.groupToken");
  if (video_hd_id == nullptr || video_hd_label == nullptr || video_hd_label_token == nullptr ||
      video_hd_mime == nullptr ||
      audio_id == nullptr || audio_label == nullptr || audio_label_token == nullptr ||
      audio_language == nullptr || audio_mime == nullptr ||
      video_group_id == nullptr || video_group_token == nullptr ||
      audio_group_id == nullptr || audio_group_token == nullptr) {
    DeleteLocalRefIfNotNull(env, video_hd_id);
    DeleteLocalRefIfNotNull(env, video_hd_label);
    DeleteLocalRefIfNotNull(env, video_hd_label_token);
    DeleteLocalRefIfNotNull(env, video_hd_mime);
    DeleteLocalRefIfNotNull(env, audio_id);
    DeleteLocalRefIfNotNull(env, audio_label);
    DeleteLocalRefIfNotNull(env, audio_label_token);
    DeleteLocalRefIfNotNull(env, audio_language);
    DeleteLocalRefIfNotNull(env, audio_mime);
    DeleteLocalRefIfNotNull(env, video_group_id);
    DeleteLocalRefIfNotNull(env, video_group_token);
    DeleteLocalRefIfNotNull(env, audio_group_id);
    DeleteLocalRefIfNotNull(env, audio_group_token);
    DeleteLocalRefIfNotNull(env, track_info_class);
    DeleteLocalRefIfNotNull(env, track_group_class);
    DeleteLocalRefIfNotNull(env, tracks_class);
    return nullptr;
  }

  jobject video_hd = NewObjectChecked(
      env,
      track_info_class,
      track_info_ctor,
      "CppTrackInfo",
      video_hd_id,
      nullptr,
      video_hd_label,
      video_hd_label_token,
      video_hd_mime,
      nullptr,
      nullptr,
      static_cast<jint>(2500000),
      static_cast<jint>(2000000),
      static_cast<jint>(2500000),
      static_cast<jint>(2),
      static_cast<jint>(4096),
      static_cast<jint>(3),
      static_cast<jint>(2),
      static_cast<jint>(7),
      static_cast<jint>(1),
      static_cast<jlong>(987654),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jint>(1920),
      static_cast<jint>(1080),
      static_cast<jint>(1936),
      static_cast<jint>(1096),
      static_cast<jfloat>(30.0f),
      static_cast<jint>(90),
      static_cast<jfloat>(1.25f),
      static_cast<jint>(4),
      static_cast<jint>(2),
      static_cast<jint>(1),
      static_cast<jint>(2),
      static_cast<jint>(3),
      static_cast<jint>(4),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(-1),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(1),
      static_cast<jint>(5),
      static_cast<jint>(6),
      static_cast<jint>(2),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(1),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_TRUE));
  jobject video_sd = NewObjectChecked(
      env,
      track_info_class,
      track_info_ctor,
      "CppTrackInfo",
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      video_hd_mime,
      nullptr,
      nullptr,
      static_cast<jint>(1200000),
      static_cast<jint>(1000000),
      static_cast<jint>(1200000),
      static_cast<jint>(0),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jlong>(9223372036854775807LL),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jint>(1280),
      static_cast<jint>(720),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jfloat>(30.0f),
      static_cast<jint>(0),
      static_cast<jfloat>(1.0f),
      static_cast<jint>(0),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(-1),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(1),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(1),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_TRUE));
  jobject audio_main = NewObjectChecked(
      env,
      track_info_class,
      track_info_ctor,
      "CppTrackInfo",
      audio_id,
      audio_language,
      audio_label,
      audio_label_token,
      audio_mime,
      nullptr,
      nullptr,
      static_cast<jint>(192000),
      static_cast<jint>(160000),
      static_cast<jint>(192000),
      static_cast<jint>(1),
      static_cast<jint>(1024),
      static_cast<jint>(-1),
      static_cast<jint>(1),
      static_cast<jint>(3),
      static_cast<jint>(0),
      static_cast<jlong>(9223372036854775807LL),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jfloat>(0.0f),
      static_cast<jint>(0),
      static_cast<jfloat>(1.0f),
      static_cast<jint>(0),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jint>(48000),
      static_cast<jint>(2),
      static_cast<jint>(2),
      static_cast<jint>(12),
      static_cast<jint>(34),
      static_cast<jint>(0),
      static_cast<jint>(1),
      static_cast<jint>(-1),
      static_cast<jint>(-1),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(0),
      static_cast<jint>(1),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_TRUE));
  if (video_hd == nullptr || video_sd == nullptr || audio_main == nullptr) {
    DeleteLocalRefIfNotNull(env, video_hd);
    DeleteLocalRefIfNotNull(env, video_sd);
    DeleteLocalRefIfNotNull(env, audio_main);
    DeleteLocalRefIfNotNull(env, video_hd_id);
    DeleteLocalRefIfNotNull(env, video_hd_label);
    DeleteLocalRefIfNotNull(env, video_hd_label_token);
    DeleteLocalRefIfNotNull(env, video_hd_mime);
    DeleteLocalRefIfNotNull(env, audio_id);
    DeleteLocalRefIfNotNull(env, audio_label);
    DeleteLocalRefIfNotNull(env, audio_label_token);
    DeleteLocalRefIfNotNull(env, audio_language);
    DeleteLocalRefIfNotNull(env, audio_mime);
    DeleteLocalRefIfNotNull(env, video_group_id);
    DeleteLocalRefIfNotNull(env, video_group_token);
    DeleteLocalRefIfNotNull(env, audio_group_id);
    DeleteLocalRefIfNotNull(env, audio_group_token);
    DeleteLocalRefIfNotNull(env, track_info_class);
    DeleteLocalRefIfNotNull(env, track_group_class);
    DeleteLocalRefIfNotNull(env, tracks_class);
    return nullptr;
  }

  jobjectArray video_tracks = env->NewObjectArray(2, track_info_class, nullptr);
  jobjectArray audio_tracks = env->NewObjectArray(1, track_info_class, nullptr);
  if (ClearJniExceptionIfPresent(env, "NewObjectArray(CppTrackInfo)")) {
    DeleteLocalRefIfNotNull(env, video_tracks);
    DeleteLocalRefIfNotNull(env, audio_tracks);
    DeleteLocalRefIfNotNull(env, video_hd);
    DeleteLocalRefIfNotNull(env, video_sd);
    DeleteLocalRefIfNotNull(env, audio_main);
    DeleteLocalRefIfNotNull(env, video_hd_id);
    DeleteLocalRefIfNotNull(env, video_hd_label);
    DeleteLocalRefIfNotNull(env, video_hd_label_token);
    DeleteLocalRefIfNotNull(env, video_hd_mime);
    DeleteLocalRefIfNotNull(env, audio_id);
    DeleteLocalRefIfNotNull(env, audio_label);
    DeleteLocalRefIfNotNull(env, audio_label_token);
    DeleteLocalRefIfNotNull(env, audio_language);
    DeleteLocalRefIfNotNull(env, audio_mime);
    DeleteLocalRefIfNotNull(env, video_group_id);
    DeleteLocalRefIfNotNull(env, video_group_token);
    DeleteLocalRefIfNotNull(env, audio_group_id);
    DeleteLocalRefIfNotNull(env, audio_group_token);
    DeleteLocalRefIfNotNull(env, track_info_class);
    DeleteLocalRefIfNotNull(env, track_group_class);
    DeleteLocalRefIfNotNull(env, tracks_class);
    return nullptr;
  }
  env->SetObjectArrayElement(video_tracks, 0, video_hd);
  env->SetObjectArrayElement(video_tracks, 1, video_sd);
  env->SetObjectArrayElement(audio_tracks, 0, audio_main);
  if (ClearJniExceptionIfPresent(env, "SetObjectArrayElement(CppTrackInfo)")) {
    DeleteLocalRefIfNotNull(env, video_tracks);
    DeleteLocalRefIfNotNull(env, audio_tracks);
    DeleteLocalRefIfNotNull(env, video_hd);
    DeleteLocalRefIfNotNull(env, video_sd);
    DeleteLocalRefIfNotNull(env, audio_main);
    DeleteLocalRefIfNotNull(env, video_hd_id);
    DeleteLocalRefIfNotNull(env, video_hd_label);
    DeleteLocalRefIfNotNull(env, video_hd_label_token);
    DeleteLocalRefIfNotNull(env, video_hd_mime);
    DeleteLocalRefIfNotNull(env, audio_id);
    DeleteLocalRefIfNotNull(env, audio_label);
    DeleteLocalRefIfNotNull(env, audio_label_token);
    DeleteLocalRefIfNotNull(env, audio_language);
    DeleteLocalRefIfNotNull(env, audio_mime);
    DeleteLocalRefIfNotNull(env, video_group_id);
    DeleteLocalRefIfNotNull(env, video_group_token);
    DeleteLocalRefIfNotNull(env, audio_group_id);
    DeleteLocalRefIfNotNull(env, audio_group_token);
    DeleteLocalRefIfNotNull(env, track_info_class);
    DeleteLocalRefIfNotNull(env, track_group_class);
    DeleteLocalRefIfNotNull(env, tracks_class);
    return nullptr;
  }

  jobject video_group = NewObjectChecked(
      env,
      track_group_class,
      track_group_ctor,
      "CppTrackGroup",
      video_group_id,
      video_group_token,
      static_cast<jint>(2),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_TRUE),
      video_tracks);
  jobject audio_group = NewObjectChecked(
      env,
      track_group_class,
      track_group_ctor,
      "CppTrackGroup",
      audio_group_id,
      audio_group_token,
      static_cast<jint>(1),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_TRUE),
      audio_tracks);
  if (video_group == nullptr || audio_group == nullptr) {
    DeleteLocalRefIfNotNull(env, video_group);
    DeleteLocalRefIfNotNull(env, audio_group);
    DeleteLocalRefIfNotNull(env, video_tracks);
    DeleteLocalRefIfNotNull(env, audio_tracks);
    DeleteLocalRefIfNotNull(env, video_hd);
    DeleteLocalRefIfNotNull(env, video_sd);
    DeleteLocalRefIfNotNull(env, audio_main);
    DeleteLocalRefIfNotNull(env, video_hd_id);
    DeleteLocalRefIfNotNull(env, video_hd_label);
    DeleteLocalRefIfNotNull(env, video_hd_label_token);
    DeleteLocalRefIfNotNull(env, video_hd_mime);
    DeleteLocalRefIfNotNull(env, audio_id);
    DeleteLocalRefIfNotNull(env, audio_label);
    DeleteLocalRefIfNotNull(env, audio_label_token);
    DeleteLocalRefIfNotNull(env, audio_language);
    DeleteLocalRefIfNotNull(env, audio_mime);
    DeleteLocalRefIfNotNull(env, video_group_id);
    DeleteLocalRefIfNotNull(env, video_group_token);
    DeleteLocalRefIfNotNull(env, audio_group_id);
    DeleteLocalRefIfNotNull(env, audio_group_token);
    DeleteLocalRefIfNotNull(env, track_info_class);
    DeleteLocalRefIfNotNull(env, track_group_class);
    DeleteLocalRefIfNotNull(env, tracks_class);
    return nullptr;
  }

  jobjectArray groups = env->NewObjectArray(2, track_group_class, nullptr);
  if (ClearJniExceptionIfPresent(env, "NewObjectArray(CppTrackGroup)")) {
    DeleteLocalRefIfNotNull(env, groups);
    DeleteLocalRefIfNotNull(env, video_group);
    DeleteLocalRefIfNotNull(env, audio_group);
    DeleteLocalRefIfNotNull(env, video_tracks);
    DeleteLocalRefIfNotNull(env, audio_tracks);
    DeleteLocalRefIfNotNull(env, video_hd);
    DeleteLocalRefIfNotNull(env, video_sd);
    DeleteLocalRefIfNotNull(env, audio_main);
    DeleteLocalRefIfNotNull(env, video_hd_id);
    DeleteLocalRefIfNotNull(env, video_hd_label);
    DeleteLocalRefIfNotNull(env, video_hd_label_token);
    DeleteLocalRefIfNotNull(env, video_hd_mime);
    DeleteLocalRefIfNotNull(env, audio_id);
    DeleteLocalRefIfNotNull(env, audio_label);
    DeleteLocalRefIfNotNull(env, audio_label_token);
    DeleteLocalRefIfNotNull(env, audio_language);
    DeleteLocalRefIfNotNull(env, audio_mime);
    DeleteLocalRefIfNotNull(env, video_group_id);
    DeleteLocalRefIfNotNull(env, video_group_token);
    DeleteLocalRefIfNotNull(env, audio_group_id);
    DeleteLocalRefIfNotNull(env, audio_group_token);
    DeleteLocalRefIfNotNull(env, track_info_class);
    DeleteLocalRefIfNotNull(env, track_group_class);
    DeleteLocalRefIfNotNull(env, tracks_class);
    return nullptr;
  }
  env->SetObjectArrayElement(groups, 0, video_group);
  env->SetObjectArrayElement(groups, 1, audio_group);
  if (ClearJniExceptionIfPresent(env, "SetObjectArrayElement(CppTrackGroup)")) {
    DeleteLocalRefIfNotNull(env, groups);
    DeleteLocalRefIfNotNull(env, video_group);
    DeleteLocalRefIfNotNull(env, audio_group);
    DeleteLocalRefIfNotNull(env, video_tracks);
    DeleteLocalRefIfNotNull(env, audio_tracks);
    DeleteLocalRefIfNotNull(env, video_hd);
    DeleteLocalRefIfNotNull(env, video_sd);
    DeleteLocalRefIfNotNull(env, audio_main);
    DeleteLocalRefIfNotNull(env, video_hd_id);
    DeleteLocalRefIfNotNull(env, video_hd_label);
    DeleteLocalRefIfNotNull(env, video_hd_label_token);
    DeleteLocalRefIfNotNull(env, video_hd_mime);
    DeleteLocalRefIfNotNull(env, audio_id);
    DeleteLocalRefIfNotNull(env, audio_label);
    DeleteLocalRefIfNotNull(env, audio_label_token);
    DeleteLocalRefIfNotNull(env, audio_language);
    DeleteLocalRefIfNotNull(env, audio_mime);
    DeleteLocalRefIfNotNull(env, video_group_id);
    DeleteLocalRefIfNotNull(env, video_group_token);
    DeleteLocalRefIfNotNull(env, audio_group_id);
    DeleteLocalRefIfNotNull(env, audio_group_token);
    DeleteLocalRefIfNotNull(env, track_info_class);
    DeleteLocalRefIfNotNull(env, track_group_class);
    DeleteLocalRefIfNotNull(env, tracks_class);
    return nullptr;
  }

  jobject java_tracks = NewObjectChecked(
      env,
      tracks_class,
      tracks_ctor,
      "CppTracks",
      groups,
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jboolean>(JNI_FALSE));
  if (java_tracks == nullptr) {
    DeleteLocalRefIfNotNull(env, groups);
    DeleteLocalRefIfNotNull(env, video_group);
    DeleteLocalRefIfNotNull(env, audio_group);
    DeleteLocalRefIfNotNull(env, video_tracks);
    DeleteLocalRefIfNotNull(env, audio_tracks);
    DeleteLocalRefIfNotNull(env, video_hd);
    DeleteLocalRefIfNotNull(env, video_sd);
    DeleteLocalRefIfNotNull(env, audio_main);
    DeleteLocalRefIfNotNull(env, video_hd_id);
    DeleteLocalRefIfNotNull(env, video_hd_label);
    DeleteLocalRefIfNotNull(env, video_hd_label_token);
    DeleteLocalRefIfNotNull(env, video_hd_mime);
    DeleteLocalRefIfNotNull(env, audio_id);
    DeleteLocalRefIfNotNull(env, audio_label);
    DeleteLocalRefIfNotNull(env, audio_label_token);
    DeleteLocalRefIfNotNull(env, audio_language);
    DeleteLocalRefIfNotNull(env, audio_mime);
    DeleteLocalRefIfNotNull(env, video_group_id);
    DeleteLocalRefIfNotNull(env, video_group_token);
    DeleteLocalRefIfNotNull(env, audio_group_id);
    DeleteLocalRefIfNotNull(env, audio_group_token);
    DeleteLocalRefIfNotNull(env, track_info_class);
    DeleteLocalRefIfNotNull(env, track_group_class);
    DeleteLocalRefIfNotNull(env, tracks_class);
    return nullptr;
  }

  TracksSnapshot snapshot = FromJavaTracks(env, java_tracks);
  std::string summary = "groupCount=" + std::to_string(snapshot.groups.size());
  summary += ",containsAudio=" + std::to_string(snapshot.contains_audio ? 1 : 0);
  summary += ",containsVideo=" + std::to_string(snapshot.contains_video ? 1 : 0);
  summary += ",containsText=" + std::to_string(snapshot.contains_text ? 1 : 0);
  summary += ",audioSelected=" + std::to_string(snapshot.audio_selected ? 1 : 0);
  summary += ",videoSelected=" + std::to_string(snapshot.video_selected ? 1 : 0);
  summary += ",audioSupported=" + std::to_string(snapshot.audio_supported ? 1 : 0);
  summary += ",videoSupported=" + std::to_string(snapshot.video_supported ? 1 : 0);
  summary += ",audioSupportedAllowingExceeds=" +
      std::to_string(snapshot.audio_supported_allowing_exceeds_capabilities ? 1 : 0);
  summary += ",videoSupportedAllowingExceeds=" +
      std::to_string(snapshot.video_supported_allowing_exceeds_capabilities ? 1 : 0);
  summary += ",textSupportedAllowingExceeds=" +
      std::to_string(snapshot.text_supported_allowing_exceeds_capabilities ? 1 : 0);
  if (!snapshot.groups.empty()) {
    const auto& group0 = snapshot.groups[0];
    summary += ",group0Id=" + group0.id;
    summary += ",group0TokenPresent=" +
        std::to_string(group0.group_token.empty() ? 0 : 1);
    summary += ",group0Type=" + std::to_string(group0.type);
    summary += ",group0Adaptive=" + std::to_string(group0.adaptive_supported ? 1 : 0);
    summary += ",group0Selected=" + std::to_string(group0.selected ? 1 : 0);
    summary += ",group0Supported=" + std::to_string(group0.supported ? 1 : 0);
    summary += ",group0SupportedAllowingExceeds=" +
        std::to_string(group0.supported_allowing_exceeds_capabilities ? 1 : 0);
    summary += ",group0TrackCount=" + std::to_string(group0.tracks.size());
    if (!group0.tracks.empty()) {
      const auto& track0 = group0.tracks[0];
      summary += ",track0Id=" + track0.id;
      summary += ",track0Label=" + track0.label;
      summary += ",track0LabelTokenPresent=" +
          std::to_string(track0.label_token.empty() ? 0 : 1);
      summary += ",track0Language=" + track0.language;
      summary += ",track0MimeType=" + track0.mime_type;
      summary += ",track0ContainerMimeType=" + track0.container_mime_type;
      summary += ",track0Codecs=" + track0.codecs;
      summary += ",track0Bitrate=" + std::to_string(track0.bitrate);
      summary += ",track0AverageBitrate=" + std::to_string(track0.average_bitrate);
      summary += ",track0PeakBitrate=" + std::to_string(track0.peak_bitrate);
      summary += ",track0MetadataEntryCount=" +
          std::to_string(track0.metadata_entry_count);
      summary += ",track0MaxInputSize=" + std::to_string(track0.max_input_size);
      summary += ",track0MaxNumReorderSamples=" +
          std::to_string(track0.max_num_reorder_samples);
      summary += ",track0InitializationData=" +
          std::to_string(track0.initialization_data_count) + ":" +
          std::to_string(track0.initialization_data_total_bytes);
      summary += ",track0DrmSchemeDataCount=" +
          std::to_string(track0.drm_scheme_data_count);
      summary += ",track0SubsampleOffsetUs=" +
          std::to_string(track0.subsample_offset_us);
      summary += ",track0HasPrerollSamples=" +
          std::to_string(track0.has_preroll_samples ? 1 : 0);
      summary += ",track0Width=" + std::to_string(track0.width);
      summary += ",track0Height=" + std::to_string(track0.height);
      summary += ",track0DecodedSize=" + std::to_string(track0.decoded_width) + "x" +
          std::to_string(track0.decoded_height);
      summary += ",track0FrameRate=" + std::to_string(track0.frame_rate);
      summary += ",track0RotationDegrees=" + std::to_string(track0.rotation_degrees);
      summary += ",track0PixelRatio=" +
          std::to_string(track0.pixel_width_height_ratio);
      summary += ",track0ProjectionDataLength=" +
          std::to_string(track0.projection_data_length);
      summary += ",track0StereoMode=" + std::to_string(track0.stereo_mode);
      summary += ",track0Color=" + std::to_string(track0.color_standard) + ":" +
          std::to_string(track0.color_range) + ":" +
          std::to_string(track0.color_transfer);
      summary += ",track0MaxSubLayers=" + std::to_string(track0.max_sub_layers);
      summary += ",track0PcmEncoding=" + std::to_string(track0.pcm_encoding);
      summary += ",track0EncoderTrim=" + std::to_string(track0.encoder_delay) + ":" +
          std::to_string(track0.encoder_padding);
      summary += ",track0AccessibilityChannel=" + std::to_string(track0.accessibility_channel);
      summary += ",track0CueReplacementBehavior=" +
          std::to_string(track0.cue_replacement_behavior);
      summary += ",track0Tiles=" + std::to_string(track0.tile_count_horizontal) + "x" +
          std::to_string(track0.tile_count_vertical);
      summary += ",track0CryptoType=" + std::to_string(track0.crypto_type);
      summary += ",track0RoleFlags=" + std::to_string(track0.role_flags);
      summary += ",track0SelectionFlags=" + std::to_string(track0.selection_flags);
      summary += ",track0Selected=" + std::to_string(track0.selected ? 1 : 0);
      summary += ",track0Supported=" + std::to_string(track0.supported ? 1 : 0);
      summary += ",track0SupportedWithinCapabilities=" +
          std::to_string(track0.supported_within_capabilities ? 1 : 0);
    }
    if (group0.tracks.size() > 1) {
      const auto& track1 = group0.tracks[1];
      summary += ",track1Id=" + track1.id;
      summary += ",track1Selected=" + std::to_string(track1.selected ? 1 : 0);
      summary += ",track1Supported=" + std::to_string(track1.supported ? 1 : 0);
      summary += ",track1SupportedWithinCapabilities=" +
          std::to_string(track1.supported_within_capabilities ? 1 : 0);
    }
  }
  if (snapshot.groups.size() > 1) {
    const auto& group1 = snapshot.groups[1];
    summary += ",group1Id=" + group1.id;
    summary += ",group1TokenPresent=" +
        std::to_string(group1.group_token.empty() ? 0 : 1);
    summary += ",group1Type=" + std::to_string(group1.type);
    summary += ",group1Selected=" + std::to_string(group1.selected ? 1 : 0);
    summary += ",group1Supported=" + std::to_string(group1.supported ? 1 : 0);
    summary += ",group1SupportedAllowingExceeds=" +
        std::to_string(group1.supported_allowing_exceeds_capabilities ? 1 : 0);
    summary += ",group1TrackCount=" + std::to_string(group1.tracks.size());
    if (!group1.tracks.empty()) {
      const auto& track0 = group1.tracks[0];
      summary += ",group1Track0Label=" + track0.label;
      summary += ",group1Track0LabelTokenPresent=" +
          std::to_string(track0.label_token.empty() ? 0 : 1);
      summary += ",group1Track0Language=" + track0.language;
      summary += ",group1Track0MimeType=" + track0.mime_type;
      summary += ",group1Track0AverageBitrate=" +
          std::to_string(track0.average_bitrate);
      summary += ",group1Track0PeakBitrate=" + std::to_string(track0.peak_bitrate);
      summary += ",group1Track0MetadataEntryCount=" +
          std::to_string(track0.metadata_entry_count);
      summary += ",group1Track0InitializationData=" +
          std::to_string(track0.initialization_data_count) + ":" +
          std::to_string(track0.initialization_data_total_bytes);
      summary += ",group1Track0PcmEncoding=" + std::to_string(track0.pcm_encoding);
      summary += ",group1Track0EncoderTrim=" +
          std::to_string(track0.encoder_delay) + ":" +
          std::to_string(track0.encoder_padding);
      summary += ",group1Track0ChannelCount=" + std::to_string(track0.channel_count);
      summary += ",group1Track0SampleRate=" + std::to_string(track0.sample_rate);
      summary += ",group1Track0RoleFlags=" + std::to_string(track0.role_flags);
      summary += ",group1Track0SelectionFlags=" + std::to_string(track0.selection_flags);
    }
  }

  DeleteLocalRefIfNotNull(env, java_tracks);
  DeleteLocalRefIfNotNull(env, groups);
  DeleteLocalRefIfNotNull(env, video_group);
  DeleteLocalRefIfNotNull(env, audio_group);
  DeleteLocalRefIfNotNull(env, video_tracks);
  DeleteLocalRefIfNotNull(env, audio_tracks);
  DeleteLocalRefIfNotNull(env, video_hd);
  DeleteLocalRefIfNotNull(env, video_sd);
  DeleteLocalRefIfNotNull(env, audio_main);
  DeleteLocalRefIfNotNull(env, video_hd_id);
  DeleteLocalRefIfNotNull(env, video_hd_label);
  DeleteLocalRefIfNotNull(env, video_hd_label_token);
  DeleteLocalRefIfNotNull(env, video_hd_mime);
  DeleteLocalRefIfNotNull(env, audio_id);
  DeleteLocalRefIfNotNull(env, audio_label);
  DeleteLocalRefIfNotNull(env, audio_label_token);
  DeleteLocalRefIfNotNull(env, audio_language);
  DeleteLocalRefIfNotNull(env, audio_mime);
  DeleteLocalRefIfNotNull(env, video_group_id);
  DeleteLocalRefIfNotNull(env, video_group_token);
  DeleteLocalRefIfNotNull(env, audio_group_id);
  DeleteLocalRefIfNotNull(env, audio_group_token);
  DeleteLocalRefIfNotNull(env, track_info_class);
  DeleteLocalRefIfNotNull(env, track_group_class);
  DeleteLocalRefIfNotNull(env, tracks_class);
  return NewStringUtfChecked(env, summary, "nativeTracksSnapshotConversionSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeCueSnapshotConversionSmokeTest(
    JNIEnv* env,
    jclass) {
  jclass cue_class = FindClassChecked(env, "androidx/media3/exoplayer/cppbridge/CppCue");
  if (cue_class == nullptr) {
    DeleteLocalRefIfNotNull(env, cue_class);
    return nullptr;
  }
  jmethodID cue_ctor = GetMethodChecked(
      env,
      cue_class,
      "CppCue",
      "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IIFIIFIFFFIIFIZIZ)V");
  if (cue_ctor == nullptr) {
    DeleteLocalRefIfNotNull(env, cue_class);
    return nullptr;
  }

  jstring cue0_text = NewStringUtfChecked(env, "Hello Cue", "CppCue.text");
  jstring cue1_text = NewStringUtfChecked(env, "Second Cue", "CppCue.text");
  jstring cue0_text_token =
      NewStringUtfChecked(env, "cue0-text-token", "CppCue.textToken");
  jstring cue0_bitmap_token =
      NewStringUtfChecked(env, "cue0-bitmap-token", "CppCue.bitmapToken");
  jstring cue1_text_token =
      NewStringUtfChecked(env, "cue1-text-token", "CppCue.textToken");
  if (cue0_text == nullptr || cue1_text == nullptr || cue0_text_token == nullptr ||
      cue0_bitmap_token == nullptr || cue1_text_token == nullptr) {
    DeleteLocalRefIfNotNull(env, cue0_text);
    DeleteLocalRefIfNotNull(env, cue1_text);
    DeleteLocalRefIfNotNull(env, cue0_text_token);
    DeleteLocalRefIfNotNull(env, cue0_bitmap_token);
    DeleteLocalRefIfNotNull(env, cue1_text_token);
    DeleteLocalRefIfNotNull(env, cue_class);
    return nullptr;
  }

  jobject cue0 = NewObjectChecked(
      env,
      cue_class,
      cue_ctor,
      "CppCue",
      cue0_text,
      cue0_text_token,
      cue0_bitmap_token,
      static_cast<jint>(2),
      static_cast<jint>(1),
      static_cast<jfloat>(0.25f),
      static_cast<jint>(0),
      static_cast<jint>(1),
      static_cast<jfloat>(0.5f),
      static_cast<jint>(2),
      static_cast<jfloat>(0.6f),
      static_cast<jfloat>(0.75f),
      static_cast<jfloat>(18.0f),
      static_cast<jint>(2),
      static_cast<jint>(0),
      static_cast<jfloat>(12.5f),
      static_cast<jint>(4),
      static_cast<jboolean>(JNI_TRUE),
      static_cast<jint>(0x00FF00),
      static_cast<jboolean>(JNI_TRUE));
  jobject cue1 = NewObjectChecked(
      env,
      cue_class,
      cue_ctor,
      "CppCue",
      cue1_text,
      cue1_text_token,
      (jstring)nullptr,  // bitmapToken
      static_cast<jint>(1),
      static_cast<jint>(0),
      static_cast<jfloat>(0.1f),
      static_cast<jint>(1),
      static_cast<jint>(0),
      static_cast<jfloat>(0.2f),
      static_cast<jint>(1),
      static_cast<jfloat>(0.9f),
      static_cast<jfloat>(0.0f),
      static_cast<jfloat>(22.0f),
      static_cast<jint>(3),
      static_cast<jint>(1),
      static_cast<jfloat>(0.0f),
      static_cast<jint>(0),
      static_cast<jboolean>(JNI_FALSE),
      static_cast<jint>(0),
      static_cast<jboolean>(JNI_FALSE));
  if (cue0 == nullptr || cue1 == nullptr) {
    DeleteLocalRefIfNotNull(env, cue0);
    DeleteLocalRefIfNotNull(env, cue1);
    DeleteLocalRefIfNotNull(env, cue0_text);
    DeleteLocalRefIfNotNull(env, cue1_text);
    DeleteLocalRefIfNotNull(env, cue0_text_token);
    DeleteLocalRefIfNotNull(env, cue0_bitmap_token);
    DeleteLocalRefIfNotNull(env, cue1_text_token);
    DeleteLocalRefIfNotNull(env, cue_class);
    return nullptr;
  }

  jobjectArray cues = env->NewObjectArray(2, cue_class, nullptr);
  if (ClearJniExceptionIfPresent(env, "NewObjectArray(CppCue)")) {
    DeleteLocalRefIfNotNull(env, cues);
    DeleteLocalRefIfNotNull(env, cue0);
    DeleteLocalRefIfNotNull(env, cue1);
    DeleteLocalRefIfNotNull(env, cue0_text);
    DeleteLocalRefIfNotNull(env, cue1_text);
    DeleteLocalRefIfNotNull(env, cue0_text_token);
    DeleteLocalRefIfNotNull(env, cue0_bitmap_token);
    DeleteLocalRefIfNotNull(env, cue1_text_token);
    DeleteLocalRefIfNotNull(env, cue_class);
    return nullptr;
  }
  env->SetObjectArrayElement(cues, 0, cue0);
  env->SetObjectArrayElement(cues, 1, cue1);
  if (ClearJniExceptionIfPresent(env, "SetObjectArrayElement(CppCue)")) {
    DeleteLocalRefIfNotNull(env, cues);
    DeleteLocalRefIfNotNull(env, cue0);
    DeleteLocalRefIfNotNull(env, cue1);
    DeleteLocalRefIfNotNull(env, cue0_text);
    DeleteLocalRefIfNotNull(env, cue1_text);
    DeleteLocalRefIfNotNull(env, cue0_text_token);
    DeleteLocalRefIfNotNull(env, cue0_bitmap_token);
    DeleteLocalRefIfNotNull(env, cue1_text_token);
    DeleteLocalRefIfNotNull(env, cue_class);
    return nullptr;
  }

  CueSnapshot snapshot = FromJavaCues(env, cues, /*presentation_time_us=*/987654);
  std::string summary = "cueCount=" + std::to_string(snapshot.cue_count);
  summary += ",presentationTimeUs=" + std::to_string(snapshot.presentation_time_us);
  summary += ",textsCount=" + std::to_string(snapshot.texts.size());
  if (!snapshot.texts.empty()) {
    summary += ",text0=" + snapshot.texts[0];
    summary += ",text0TokenPresent=" +
        std::to_string(snapshot.text_tokens.empty() || snapshot.text_tokens[0].empty() ? 0 : 1);
    summary += ",bitmap0TokenPresent=" +
        std::to_string(snapshot.bitmap_tokens.empty() || snapshot.bitmap_tokens[0].empty() ? 0 : 1);
  }
  if (snapshot.texts.size() > 1) {
    summary += ",text1=" + snapshot.texts[1];
    summary += ",text1TokenPresent=" +
        std::to_string(snapshot.text_tokens.size() < 2 || snapshot.text_tokens[1].empty() ? 0 : 1);
  }
  if (!snapshot.cues.empty()) {
    const auto& cue0_info = snapshot.cues[0];
    summary += ",cue0Text=" + cue0_info.text;
    summary += ",cue0TextTokenPresent=" +
        std::to_string(cue0_info.text_token.empty() ? 0 : 1);
    summary += ",cue0BitmapTokenPresent=" +
        std::to_string(cue0_info.bitmap_token.empty() ? 0 : 1);
    summary += ",cue0TextAlignment=" + std::to_string(cue0_info.text_alignment);
    summary += ",cue0MultiRowAlignment=" + std::to_string(cue0_info.multi_row_alignment);
    summary += ",cue0Line=" + std::to_string(cue0_info.line);
    summary += ",cue0LineType=" + std::to_string(cue0_info.line_type);
    summary += ",cue0LineAnchor=" + std::to_string(cue0_info.line_anchor);
    summary += ",cue0Position=" + std::to_string(cue0_info.position);
    summary += ",cue0PositionAnchor=" + std::to_string(cue0_info.position_anchor);
    summary += ",cue0Size=" + std::to_string(cue0_info.size);
    summary += ",cue0BitmapHeight=" + std::to_string(cue0_info.bitmap_height);
    summary += ",cue0TextSize=" + std::to_string(cue0_info.text_size);
    summary += ",cue0ShearDegrees=" + std::to_string(cue0_info.shear_degrees);
    summary += ",cue0ZIndex=" + std::to_string(cue0_info.z_index);
    summary += ",cue0WindowColorSet=" + std::to_string(cue0_info.window_color_set ? 1 : 0);
    summary += ",cue0WindowColor=" + std::to_string(cue0_info.window_color);
    summary += ",cue0HasBitmap=" + std::to_string(cue0_info.has_bitmap ? 1 : 0);
  }
  if (snapshot.cues.size() > 1) {
    const auto& cue1_info = snapshot.cues[1];
    summary += ",cue1Text=" + cue1_info.text;
    summary += ",cue1TextTokenPresent=" +
        std::to_string(cue1_info.text_token.empty() ? 0 : 1);
    summary += ",cue1BitmapTokenPresent=" +
        std::to_string(cue1_info.bitmap_token.empty() ? 0 : 1);
    summary += ",cue1LineType=" + std::to_string(cue1_info.line_type);
    summary += ",cue1PositionAnchor=" + std::to_string(cue1_info.position_anchor);
    summary += ",cue1TextSize=" + std::to_string(cue1_info.text_size);
    summary += ",cue1TextSizeType=" + std::to_string(cue1_info.text_size_type);
    summary += ",cue1VerticalType=" + std::to_string(cue1_info.vertical_type);
  }

  DeleteLocalRefIfNotNull(env, cues);
  DeleteLocalRefIfNotNull(env, cue0);
  DeleteLocalRefIfNotNull(env, cue1);
  DeleteLocalRefIfNotNull(env, cue0_text);
  DeleteLocalRefIfNotNull(env, cue1_text);
  DeleteLocalRefIfNotNull(env, cue0_text_token);
  DeleteLocalRefIfNotNull(env, cue0_bitmap_token);
  DeleteLocalRefIfNotNull(env, cue1_text_token);
  DeleteLocalRefIfNotNull(env, cue_class);
  return NewStringUtfChecked(env, summary, "nativeCueSnapshotConversionSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeListenerPayloadCaptureSmokeTest(
    JNIEnv* env,
    jclass) {
  CapturingPlayerListener listener;
  PlaybackSnapshot snapshot;

  TimelineDetailsSnapshot timeline;
  timeline.summary.window_count = 2;
  timeline.summary.period_count = 3;
  timeline.summary.empty = false;
  timeline.summary.current_media_item_index = 1;
  TimelineWindowSnapshot window0;
  window0.media_item_id = "payload-window-0";
  window0.media_item_uri = "https://example.com/payload-window-0.m3u8";
  window0.media_item_tag_present = true;
  window0.media_item_tag_string = "payload-window-tag";
  window0.media_item_tag_token = "generated-opaque-object-token-payload-window-tag";
  window0.uid = "payload-window-uid";
  window0.uid_token = "generated-opaque-object-token-payload-window-uid";
  window0.live_configuration_present = true;
  window0.live_target_offset_ms = 3333;
  window0.live_min_offset_ms = 2222;
  window0.live_max_offset_ms = 5555;
  window0.live_min_playback_speed = 0.95f;
  window0.live_max_playback_speed = 1.05f;
  window0.manifest_present = true;
  window0.manifest_string = "payload-window-manifest";
  window0.manifest_token = "generated-opaque-object-token-payload-window";
  window0.is_dynamic = true;
  window0.presentation_start_time_ms = 111000;
  window0.default_position_us = 123456;
  timeline.windows.push_back(window0);
  TimelineWindowSnapshot window1;
  window1.media_item_id = "payload-window-1";
  window1.media_item_uri = "https://example.com/payload-window-1.mp4";
  window1.media_item_tag_present = true;
  window1.media_item_tag_string = "payload-window-tag-2";
  window1.media_item_tag_token = "generated-opaque-object-token-payload-window-tag-2";
  window1.uid = "payload-window-uid-2";
  window1.uid_token = "generated-opaque-object-token-payload-window-uid-2";
  window1.is_dynamic = false;
  timeline.windows.push_back(window1);
  TimelinePeriodSnapshot period0;
  period0.id = "payload-period-id";
  period0.id_token = "generated-opaque-object-token-payload-period-id";
  period0.uid = "payload-period-0";
  period0.uid_token = "generated-opaque-object-token-payload-period";
  period0.ads_id = "payload-period-ads-id";
  period0.ads_id_token = "generated-opaque-object-token-payload-period-ads-id";
  period0.ad_group_count = 2;
  period0.duration_ms = 777;
  period0.duration_us = 777000;
  period0.position_in_window_us = 456000;
  timeline.periods.push_back(period0);
  TimelinePeriodSnapshot period1;
  period1.id = "payload-period-id-2";
  period1.id_token = "generated-opaque-object-token-payload-period-id-2";
  period1.uid = "payload-period-1";
  period1.uid_token = "generated-opaque-object-token-payload-period-1";
  period1.ads_id = "payload-period-ads-id-2";
  period1.ads_id_token = "generated-opaque-object-token-payload-period-ads-id-2";
  period1.ad_group_count = 1;
  period1.duration_ms = 888;
  period1.duration_us = 888000;
  period1.position_in_window_us = 654000;
  timeline.periods.push_back(period1);
  listener.OnTimelineChanged(snapshot, timeline, /*reason=*/9);

  TracksSnapshot tracks;
  TrackGroupSnapshot video_group;
  video_group.id = "payload-video";
  video_group.group_token = "generated-opaque-object-token-payload-track-group";
  video_group.type = 2;
  video_group.selected = true;
  TrackInfo video_track;
  video_track.id = "payload-video-track";
  video_track.label = "Payload Video";
  video_track.label_token = "generated-opaque-object-token-payload-track-label";
  video_track.selected = true;
  video_track.supported = true;
  video_track.supported_within_capabilities = true;
  video_group.tracks.push_back(video_track);
  TrackGroupSnapshot audio_group;
  audio_group.id = "payload-audio";
  audio_group.group_token = "generated-opaque-object-token-payload-audio-track-group";
  audio_group.type = 1;
  audio_group.selected = true;
  TrackInfo audio_track;
  audio_track.id = "payload-audio-track";
  audio_track.label = "Payload Audio";
  audio_track.label_token = "generated-opaque-object-token-payload-audio-track-label";
  audio_track.selected = true;
  audio_track.supported = true;
  audio_track.supported_within_capabilities = true;
  audio_group.tracks.push_back(audio_track);
  video_group.supported = true;
  video_group.supported_allowing_exceeds_capabilities = true;
  audio_group.supported = true;
  tracks.video_supported = true;
  tracks.video_supported_allowing_exceeds_capabilities = true;
  tracks.groups = {video_group, audio_group};
  tracks.contains_audio = true;
  tracks.contains_video = true;
  tracks.audio_selected = true;
  tracks.video_selected = true;
  listener.OnTracksChanged(snapshot, tracks);

  PositionInfoSnapshot old_position;
  old_position.media_item_index = 0;
  old_position.media_item.media_id = "payload-old";
  old_position.media_item.tag_present = true;
  old_position.media_item.tag_token = "generated-opaque-object-token-old-position-tag";
  PositionInfoSnapshot new_position;
  new_position.media_item_index = 1;
  new_position.media_item.media_id = "payload-new";
  new_position.media_item.tag_present = true;
  new_position.media_item.tag_token = "generated-opaque-object-token-new-position-tag";
  listener.OnPositionDiscontinuity(snapshot, old_position, new_position, /*reason=*/12);

  AvailableCommandsSnapshot commands;
  commands.command_codes = {1, 2, 3};
  listener.OnAvailableCommandsChanged(snapshot, commands);

  PlayerEventsSnapshot events;
  events.event_codes = {10, 11};
  listener.OnEvents(snapshot, events);

  std::string summary = "timelineCb=" + std::to_string(listener.timeline_callback_count);
  summary += ",timelineWindowCount=" + std::to_string(listener.timeline_window_count);
  summary += ",timelinePeriodCount=" + std::to_string(listener.timeline_period_count);
  summary += ",timelineCurrentMediaItemIndex=" +
      std::to_string(listener.timeline_current_media_item_index);
  summary += ",timelineReason=" + std::to_string(listener.timeline_change_reason);
  summary += ",timelineWindow0MediaId=" + listener.first_timeline_window_media_item_id;
  summary += ",timelineWindow0MediaUri=" + listener.first_timeline_window_media_item_uri;
  summary += ",timelineWindow0TagPresent=" +
      std::to_string(listener.first_timeline_window_media_item_tag_present ? 1 : 0);
  summary += ",timelineWindow0TagString=" + listener.first_timeline_window_media_item_tag_string;
  summary += ",timelineWindow0TagTokenPresent=" +
      std::to_string(listener.first_timeline_window_media_item_tag_token_present ? 1 : 0);
  summary += ",timelineWindow0Uid=" + listener.first_timeline_window_uid;
  summary += ",timelineWindow0UidTokenPresent=" +
      std::to_string(listener.first_timeline_window_uid_token_present ? 1 : 0);
  summary += ",timelineWindow0LiveConfigurationPresent=" +
      std::to_string(listener.first_timeline_window_live_configuration_present ? 1 : 0);
  summary += ",timelineWindow0LiveTargetOffsetMs=" +
      std::to_string(listener.first_timeline_window_live_target_offset_ms);
  summary += ",timelineWindow0LiveMinOffsetMs=" +
      std::to_string(listener.first_timeline_window_live_min_offset_ms);
  summary += ",timelineWindow0LiveMaxOffsetMs=" +
      std::to_string(listener.first_timeline_window_live_max_offset_ms);
  summary += ",timelineWindow0LiveMinSpeed=" +
      std::to_string(listener.first_timeline_window_live_min_playback_speed);
  summary += ",timelineWindow0LiveMaxSpeed=" +
      std::to_string(listener.first_timeline_window_live_max_playback_speed);
  summary += ",timelineWindow0ManifestPresent=" +
      std::to_string(listener.first_timeline_window_manifest_present ? 1 : 0);
  summary += ",timelineWindow0ManifestString=" + listener.first_timeline_window_manifest_string;
  summary += ",timelineWindow0ManifestTokenPresent=" +
      std::to_string(listener.first_timeline_window_manifest_token_present ? 1 : 0);
  summary += ",timelineWindow0Dynamic=" +
      std::to_string(listener.first_timeline_window_is_dynamic ? 1 : 0);
  summary += ",timelineWindow1MediaId=" + listener.second_timeline_window_media_item_id;
  summary += ",timelineWindow1MediaUri=" + listener.second_timeline_window_media_item_uri;
  summary += ",timelineWindow1TagPresent=" +
      std::to_string(listener.second_timeline_window_media_item_tag_present ? 1 : 0);
  summary += ",timelineWindow1TagString=" + listener.second_timeline_window_media_item_tag_string;
  summary += ",timelineWindow1TagTokenPresent=" +
      std::to_string(listener.second_timeline_window_media_item_tag_token_present ? 1 : 0);
  summary += ",timelineWindow1Dynamic=" +
      std::to_string(listener.second_timeline_window_is_dynamic ? 1 : 0);
  summary += ",timelineWindow0PresentationStartTimeMs=" +
      std::to_string(listener.first_timeline_window_presentation_start_time_ms);
  summary += ",timelinePeriod0Id=" + listener.first_timeline_period_id;
  summary += ",timelinePeriod0IdTokenPresent=" +
      std::to_string(listener.first_timeline_period_id_token_present ? 1 : 0);
  summary += ",timelinePeriod0Uid=" + listener.first_timeline_period_uid;
  summary += ",timelinePeriod0UidTokenPresent=" +
      std::to_string(listener.first_timeline_period_uid_token_present ? 1 : 0);
  summary += ",timelinePeriod0AdsId=" + listener.first_timeline_period_ads_id;
  summary += ",timelinePeriod0AdsIdTokenPresent=" +
      std::to_string(listener.first_timeline_period_ads_id_token_present ? 1 : 0);
  summary += ",timelinePeriod0AdGroupCount=" +
      std::to_string(listener.first_timeline_period_ad_group_count);
  summary += ",timelinePeriod0DurationMs=" +
      std::to_string(listener.first_timeline_period_duration_ms);
  summary += ",timelinePeriod0DurationUs=" +
      std::to_string(listener.first_timeline_period_duration_us);
  summary += ",timelinePeriod1Id=" + listener.second_timeline_period_id;
  summary += ",timelinePeriod1IdTokenPresent=" +
      std::to_string(listener.second_timeline_period_id_token_present ? 1 : 0);
  summary += ",timelinePeriod1Uid=" + listener.second_timeline_period_uid;
  summary += ",timelinePeriod1UidTokenPresent=" +
      std::to_string(listener.second_timeline_period_uid_token_present ? 1 : 0);
  summary += ",timelinePeriod1AdsId=" + listener.second_timeline_period_ads_id;
  summary += ",timelinePeriod1AdsIdTokenPresent=" +
      std::to_string(listener.second_timeline_period_ads_id_token_present ? 1 : 0);
  summary += ",timelinePeriod1DurationUs=" +
      std::to_string(listener.second_timeline_period_duration_us);
  summary += ",tracksChangedCb=" + std::to_string(listener.tracks_changed_callback_count);
  summary += ",trackGroupCount=" + std::to_string(listener.track_group_count);
  summary += ",firstTrackGroupType=" + std::to_string(listener.first_track_group_type);
  summary += ",firstTrackGroupId=" + listener.first_track_group_id;
  summary += ",firstTrackGroupTokenPresent=" +
      std::to_string(listener.first_track_group_token_present ? 1 : 0);
  summary += ",firstTrackCount=" + std::to_string(listener.first_track_count);
  summary += ",secondTrackGroupId=" + listener.second_track_group_id;
  summary += ",secondTrackGroupTokenPresent=" +
      std::to_string(listener.second_track_group_token_present ? 1 : 0);
  summary += ",secondTrackCount=" + std::to_string(listener.second_track_count);
  summary += ",secondTrackLabel=" + listener.second_track_label;
  summary += ",secondTrackLabelTokenPresent=" +
      std::to_string(listener.second_track_label_token_present ? 1 : 0);
  summary += ",firstTrackLabelTokenPresent=" +
      std::to_string(listener.first_track_label_token_present ? 1 : 0);
  summary += ",firstTrackSelected=" + std::to_string(listener.first_track_selected ? 1 : 0);
  summary += ",firstTrackSupported=" + std::to_string(listener.first_track_supported ? 1 : 0);
  summary += ",firstTrackSupportedWithinCapabilities=" +
      std::to_string(listener.first_track_supported_within_capabilities ? 1 : 0);
  summary += ",containsAudio=" + std::to_string(listener.contains_audio ? 1 : 0);
  summary += ",containsVideo=" + std::to_string(listener.contains_video ? 1 : 0);
  summary += ",containsText=" + std::to_string(listener.contains_text ? 1 : 0);
  summary += ",audioSelected=" + std::to_string(listener.audio_selected ? 1 : 0);
  summary += ",videoSelected=" + std::to_string(listener.video_selected ? 1 : 0);
  summary += ",videoSupported=" + std::to_string(listener.video_supported ? 1 : 0);
  summary += ",videoSupportedAllowingExceeds=" +
      std::to_string(listener.video_supported_allowing_exceeds ? 1 : 0);
  summary += ",positionDiscontinuityCb=" +
      std::to_string(listener.position_discontinuity_callback_count);
  summary += ",positionDiscontinuityReason=" +
      std::to_string(listener.position_discontinuity_reason);
  summary += ",oldMediaId=" + listener.old_position_media_id;
  summary += ",oldTagTokenPresent=" +
      std::to_string(listener.old_position_tag_token_present ? 1 : 0);
  summary += ",newMediaId=" + listener.new_position_media_id;
  summary += ",newTagTokenPresent=" +
      std::to_string(listener.new_position_tag_token_present ? 1 : 0);
  summary += ",availableCommandsCb=" +
      std::to_string(listener.available_commands_callback_count);
  summary += ",availableCommandsCount=" + std::to_string(listener.available_commands_count);
  summary += ",eventsCb=" + std::to_string(listener.events_callback_count);
  summary += ",eventCount=" + std::to_string(listener.last_event_count);
  return NewStringUtfChecked(env, summary, "nativeListenerPayloadCaptureSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeBuilderConfigSmokeTest(
    JNIEnv* env,
    jclass) {
  ExoPlayerSdkPlayerBuilder builder;
  builder.SetHandleAudioFocus(false)
      .SetHandleAudioBecomingNoisy(false)
      .SetUseLazyPreparation(false)
      .SetSeekBackIncrementMs(1111)
      .SetSeekForwardIncrementMs(2222)
      .SetWakeMode(1)
      .SetPriority(77)
      .SetUsePriorityTaskManager(true)
      .SetTargetPreloadDurationUs(987654)
      .SetParseSubtitlesDuringExtraction(false)
      .SetLoadOnlySelectedTracks(true)
      .SetDefaultRequestHeaders({"X-Test"}, {"1"})
      .SetUserAgent("Builder UA")
      .SetConnectTimeoutMs(3333)
      .SetReadTimeoutMs(4444)
      .SetAllowCrossProtocolRedirects(true)
      .SetLiveTargetOffsetMs(5555)
      .SetLiveOffsetsMs(4444, 6666)
      .SetLiveSpeeds(0.95f, 1.05f);

  const PlayerConfig& config = builder.GetConfig();
  std::string summary =
      "handleAudioFocus=" + std::to_string(config.handle_audio_focus ? 1 : 0);
  summary += ",handleAudioBecomingNoisy=" +
      std::to_string(config.handle_audio_becoming_noisy ? 1 : 0);
  summary += ",useLazyPreparation=" +
      std::to_string(config.use_lazy_preparation ? 1 : 0);
  summary += ",seekBackIncrementMs=" + std::to_string(config.seek_back_increment_ms);
  summary += ",seekForwardIncrementMs=" + std::to_string(config.seek_forward_increment_ms);
  summary += ",wakeMode=" + std::to_string(config.wake_mode);
  summary += ",priority=" + std::to_string(config.priority);
  summary += ",usePriorityTaskManager=" +
      std::to_string(config.use_priority_task_manager ? 1 : 0);
  summary += ",targetPreloadDurationUs=" +
      std::to_string(config.target_preload_duration_us);
  summary += ",parseSubtitlesDuringExtraction=" + std::to_string(
      config.media_source_factory_config.parse_subtitles_during_extraction ? 1 : 0);
  summary += ",loadOnlySelectedTracks=" +
      std::to_string(config.media_source_factory_config.load_only_selected_tracks ? 1 : 0);
  summary += ",userAgent=" + config.media_source_factory_config.user_agent;
  summary += ",headerCount=" + std::to_string(std::min(
      config.media_source_factory_config.default_request_header_names.size(),
      config.media_source_factory_config.default_request_header_values.size()));
  if (!config.media_source_factory_config.default_request_header_names.empty() &&
      !config.media_source_factory_config.default_request_header_values.empty()) {
    summary += ",header0=" + config.media_source_factory_config.default_request_header_names[0] +
        ":" + config.media_source_factory_config.default_request_header_values[0];
  }
  summary += ",connectTimeoutMs=" +
      std::to_string(config.media_source_factory_config.connect_timeout_ms);
  summary += ",readTimeoutMs=" +
      std::to_string(config.media_source_factory_config.read_timeout_ms);
  summary += ",allowCrossProtocolRedirects=" + std::to_string(
      config.media_source_factory_config.allow_cross_protocol_redirects ? 1 : 0);
  summary += ",liveTargetOffsetMs=" +
      std::to_string(config.media_source_factory_config.live_target_offset_ms);
  summary += ",liveMinOffsetMs=" +
      std::to_string(config.media_source_factory_config.live_min_offset_ms);
  summary += ",liveMaxOffsetMs=" +
      std::to_string(config.media_source_factory_config.live_max_offset_ms);
  summary += ",liveMinSpeed=" +
      std::to_string(config.media_source_factory_config.live_min_speed);
  summary += ",liveMaxSpeed=" +
      std::to_string(config.media_source_factory_config.live_max_speed);
  return NewStringUtfChecked(env, summary, "nativeBuilderConfigSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeBuilderBuildSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  ExoPlayerSdkPlayerBuilder builder;
  builder.SetHandleAudioFocus(false)
      .SetHandleAudioBecomingNoisy(false)
      .SetUseLazyPreparation(false)
      .SetSeekBackIncrementMs(1357)
      .SetSeekForwardIncrementMs(2468)
      .SetWakeMode(1)
      .SetTargetPreloadDurationUs(321654)
      .SetParseSubtitlesDuringExtraction(false)
      .SetLoadOnlySelectedTracks(true)
      .SetDefaultRequestHeaders({"X-Build", "X-Build-Trace"}, {"yes", "trace-build"})
      .SetUserAgent("Builder Build UA")
      .SetConnectTimeoutMs(5555)
      .SetReadTimeoutMs(6666)
      .SetAllowCrossProtocolRedirects(true)
      .SetLiveTargetOffsetMs(7777)
      .SetLiveOffsetsMs(7000, 8000)
      .SetLiveSpeeds(0.98f, 1.02f);

  std::unique_ptr<ExoPlayerSdkPlayer> player = builder.Build(env, context);
  if (player == nullptr) {
    return NewStringUtfChecked(env, "builderBuild=0", "nativeBuilderBuildSmokeTest.error");
  }

  PlayerConfig::MediaSourceFactoryConfig config = player->GetMediaSourceFactoryConfig();
  std::string summary = "builderBuild=1";
  summary += ",targetPreloadDurationUs=" + std::to_string(player->GetTargetPreloadDurationUs());
  summary += ",seekBackIncrementMs=" + std::to_string(player->GetSeekBackIncrement());
  summary += ",seekForwardIncrementMs=" + std::to_string(player->GetSeekForwardIncrement());
  summary += ",parseSubtitlesDuringExtraction=" +
      std::to_string(config.parse_subtitles_during_extraction ? 1 : 0);
  summary += ",loadOnlySelectedTracks=" +
      std::to_string(config.load_only_selected_tracks ? 1 : 0);
  summary += ",headerCount=" + std::to_string(std::min(
      config.default_request_header_names.size(), config.default_request_header_values.size()));
  if (!config.default_request_header_names.empty() && !config.default_request_header_values.empty()) {
    summary += ",header0=" + config.default_request_header_names[0] + ":" +
        config.default_request_header_values[0];
  }
  if (config.default_request_header_names.size() > 1 && config.default_request_header_values.size() > 1) {
    summary += ",header1=" + config.default_request_header_names[1] + ":" +
        config.default_request_header_values[1];
  }
  summary += ",userAgent=" + config.user_agent;
  summary += ",connectTimeoutMs=" + std::to_string(config.connect_timeout_ms);
  summary += ",readTimeoutMs=" + std::to_string(config.read_timeout_ms);
  summary += ",allowCrossProtocolRedirects=" +
      std::to_string(config.allow_cross_protocol_redirects ? 1 : 0);
  summary += ",liveTargetOffsetMs=" + std::to_string(config.live_target_offset_ms);
  summary += ",liveMinOffsetMs=" + std::to_string(config.live_min_offset_ms);
  summary += ",liveMaxOffsetMs=" + std::to_string(config.live_max_offset_ms);
  summary += ",liveMinSpeed=" + std::to_string(config.live_min_speed);
  summary += ",liveMaxSpeed=" + std::to_string(config.live_max_speed);
  player->Release();
  return NewStringUtfChecked(env, summary, "nativeBuilderBuildSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativePlayerDoubleReleaseSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  ExoPlayerSdkPlayerBuilder builder;
  std::unique_ptr<ExoPlayerSdkPlayer> player = builder.Build(env, context);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env, "playerBuild=0,doubleReleaseSafe=0", "nativePlayerDoubleReleaseSmokeTest.error");
  }

  player->Release();
  player->Release();

  return NewStringUtfChecked(
      env,
      "playerBuild=1,doubleReleaseSafe=1",
      "nativePlayerDoubleReleaseSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeListenerLifecycleNegativeSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  ExoPlayerSdkPlayerBuilder builder;
  std::unique_ptr<ExoPlayerSdkPlayer> player = builder.Build(env, context);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "playerBuild=0,listenerMutationSequenceSafe=0,analyticsNullClearSafe=0,"
        "releaseAfterMutationSafe=0",
        "nativeListenerLifecycleNegativeSmokeTest.error");
  }

  CapturingPlayerListener primary_listener;
  CapturingPlayerListener secondary_listener;

  player->SetListener(&primary_listener);
  player->RemoveListener(&secondary_listener);
  player->RemoveListener(nullptr);
  player->SetListener(nullptr);

  player->AddAnalyticsListener(&primary_listener);
  player->AddAnalyticsListener(&primary_listener);
  player->AddAnalyticsListener(&secondary_listener);
  player->RemoveAnalyticsListener(&secondary_listener);
  player->RemoveAnalyticsListener(nullptr);

  player->Release();

  return NewStringUtfChecked(
      env,
      "playerBuild=1,listenerMutationSequenceSafe=1,analyticsNullClearSafe=1,"
      "releaseAfterMutationSafe=1",
      "nativeListenerLifecycleNegativeSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeOpaqueTokenReleaseSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context,
    jobjectArray java_tokens) {
  ExoPlayerSdkPlayerBuilder builder;
  std::unique_ptr<ExoPlayerSdkPlayer> player = builder.Build(env, context);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "playerBuild=0,releaseCallSafe=0,tokenCount=0",
        "nativeOpaqueTokenReleaseSmokeTest.error");
  }

  std::vector<std::string> tokens = JStringArrayToVector(env, java_tokens);
  player->ReleaseOpaqueObjectTokens(tokens);
  player->Release();

  std::string summary =
      "playerBuild=1,releaseCallSafe=1,tokenCount=" + std::to_string(tokens.size());
  return NewStringUtfChecked(env, summary, "nativeOpaqueTokenReleaseSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeBuilderPreloadRoundTripSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  ExoPlayerSdkPlayerBuilder builder;
  builder.SetTargetPreloadDurationUs(111111);

  std::unique_ptr<ExoPlayerSdkPlayer> player = builder.Build(env, context);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env, "builderPreloadRoundTrip=0", "nativeBuilderPreloadRoundTripSmokeTest.error");
  }

  int64_t initial = player->GetTargetPreloadDurationUs();
  player->SetPreloadConfiguration(222222);
  int64_t after_update = player->GetTargetPreloadDurationUs();
  player->SetPreloadConfiguration(-9223372036854775807LL);
  int64_t after_unset = player->GetTargetPreloadDurationUs();
  player->SetPreloadConfiguration(333333);
  int64_t after_reset = player->GetTargetPreloadDurationUs();

  std::string summary = "builderPreloadRoundTrip=1";
  summary += ",initialTargetPreloadDurationUs=" + std::to_string(initial);
  summary += ",afterUpdateTargetPreloadDurationUs=" + std::to_string(after_update);
  summary += ",afterUnsetTargetPreloadDurationUs=" + std::to_string(after_unset);
  summary += ",afterResetTargetPreloadDurationUs=" + std::to_string(after_reset);
  summary += ",updateApplied=" + std::to_string(after_update == 222222 ? 1 : 0);
  summary += ",unsetApplied=" +
      std::to_string(after_unset == -9223372036854775807LL ? 1 : 0);
  summary += ",resetApplied=" + std::to_string(after_reset == 333333 ? 1 : 0);

  player->Release();
  return NewStringUtfChecked(env, summary, "nativeBuilderPreloadRoundTripSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeBuilderMediaSourceFactoryInjectionSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  ExoPlayerSdkPlayerBuilder builder;
  PlayerConfig::MediaSourceFactoryConfig media_source_factory_config;
  media_source_factory_config.factory_token = "test-injected-media-source-factory";
  media_source_factory_config.user_agent = "Builder Injected Factory UA";
  builder.SetMediaSourceFactoryConfig(media_source_factory_config);

  const PlayerConfig& config = builder.GetConfig();
  std::unique_ptr<ExoPlayerSdkPlayer> player = builder.Build(env, context);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "builderInjectedFactoryBuild=0",
        "nativeBuilderMediaSourceFactoryInjectionSmokeTest.error");
  }

  PlayerConfig::MediaSourceFactoryConfig resolved = player->GetMediaSourceFactoryConfig();
  std::string summary = "builderInjectedFactoryBuild=1";
  summary += ",configFactoryToken=" + config.media_source_factory_config.factory_token;
  summary += ",resolvedFactoryToken=" + resolved.factory_token;
  summary += ",resolvedInjectedFactoryUsed=" +
      std::to_string(resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",resolvedFactoryIdentity=" +
      std::to_string(resolved.injected_factory_identity_for_test);
  summary += ",userAgent=" + resolved.user_agent;
  player->Release();
  return NewStringUtfChecked(
      env, summary, "nativeBuilderMediaSourceFactoryInjectionSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeBuilderMediaSourceFactoryFallbackSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  ExoPlayerSdkPlayerBuilder builder;
  builder.SetMediaSourceFactoryToken("missing-media-source-factory-token")
      .SetUserAgent("Builder Fallback UA");

  const PlayerConfig& config = builder.GetConfig();
  std::unique_ptr<ExoPlayerSdkPlayer> player = builder.Build(env, context);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "builderFallbackFactoryBuild=0",
        "nativeBuilderMediaSourceFactoryFallbackSmokeTest.error");
  }

  PlayerConfig::MediaSourceFactoryConfig resolved = player->GetMediaSourceFactoryConfig();
  std::string summary = "builderFallbackFactoryBuild=1";
  summary += ",configFactoryToken=" + config.media_source_factory_config.factory_token;
  summary += ",resolvedFactoryToken=" + resolved.factory_token;
  summary += ",resolvedInjectedFactoryUsed=" +
      std::to_string(resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",resolvedFactoryIdentity=" +
      std::to_string(resolved.injected_factory_identity_for_test);
  summary += ",fallbackApplied=" +
      std::to_string(!resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",userAgent=" + resolved.user_agent;
  player->Release();
  return NewStringUtfChecked(
      env, summary, "nativeBuilderMediaSourceFactoryFallbackSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeBuilderMediaSourceFactoryInjectionReplacementSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  ExoPlayerSdkPlayerBuilder builder;
  builder.SetMediaSourceFactoryToken("replaceable-media-source-factory-token")
      .SetUserAgent("Builder Replacement Factory UA");

  const PlayerConfig& config = builder.GetConfig();
  std::unique_ptr<ExoPlayerSdkPlayer> player = builder.Build(env, context);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "builderReplacementFactoryBuild=0",
        "nativeBuilderMediaSourceFactoryInjectionReplacementSmokeTest.error");
  }

  PlayerConfig::MediaSourceFactoryConfig resolved = player->GetMediaSourceFactoryConfig();
  std::string summary = "builderReplacementFactoryBuild=1";
  summary += ",configFactoryToken=" + config.media_source_factory_config.factory_token;
  summary += ",resolvedFactoryToken=" + resolved.factory_token;
  summary += ",resolvedInjectedFactoryUsed=" +
      std::to_string(resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",resolvedFactoryIdentity=" +
      std::to_string(resolved.injected_factory_identity_for_test);
  summary += ",userAgent=" + resolved.user_agent;
  player->Release();
  return NewStringUtfChecked(
      env, summary, "nativeBuilderMediaSourceFactoryInjectionReplacementSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeBuilderMediaSourceFactoryInjectionMultiTokenSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  ExoPlayerSdkPlayerBuilder first_builder;
  first_builder.SetMediaSourceFactoryToken("multi-token-media-source-factory-a")
      .SetUserAgent("Builder Multi Token A");
  std::unique_ptr<ExoPlayerSdkPlayer> first_player = first_builder.Build(env, context);
  if (first_player == nullptr) {
    return NewStringUtfChecked(
        env,
        "builderMultiTokenBuild=0",
        "nativeBuilderMediaSourceFactoryInjectionMultiTokenSmokeTest.error");
  }
  PlayerConfig::MediaSourceFactoryConfig first_resolved =
      first_player->GetMediaSourceFactoryConfig();

  ExoPlayerSdkPlayerBuilder second_builder;
  second_builder.SetMediaSourceFactoryToken("multi-token-media-source-factory-b")
      .SetUserAgent("Builder Multi Token B");
  std::unique_ptr<ExoPlayerSdkPlayer> second_player = second_builder.Build(env, context);
  if (second_player == nullptr) {
    first_player->Release();
    return NewStringUtfChecked(
        env,
        "builderMultiTokenBuild=0",
        "nativeBuilderMediaSourceFactoryInjectionMultiTokenSmokeTest.error");
  }
  PlayerConfig::MediaSourceFactoryConfig second_resolved =
      second_player->GetMediaSourceFactoryConfig();

  std::string summary = "builderMultiTokenBuild=1";
  summary += ",firstFactoryToken=" + first_resolved.factory_token;
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

  first_player->Release();
  second_player->Release();
  return NewStringUtfChecked(
      env, summary, "nativeBuilderMediaSourceFactoryInjectionMultiTokenSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeBuilderMediaSourceFactoryGeneratedTokenSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context,
    jstring token) {
  ExoPlayerSdkPlayerBuilder builder;
  builder.SetMediaSourceFactoryToken(JStringToString(env, token))
      .SetUserAgent("Builder Generated Token UA");

  std::unique_ptr<ExoPlayerSdkPlayer> player = builder.Build(env, context);
  if (player == nullptr) {
    return NewStringUtfChecked(
        env,
        "builderGeneratedTokenBuild=0",
        "nativeBuilderMediaSourceFactoryGeneratedTokenSmokeTest.error");
  }

  PlayerConfig::MediaSourceFactoryConfig resolved = player->GetMediaSourceFactoryConfig();
  std::string summary = "builderGeneratedTokenBuild=1";
  summary += ",resolvedFactoryToken=" + resolved.factory_token;
  summary += ",resolvedInjectedFactoryUsed=" +
      std::to_string(resolved.injected_factory_used_for_test ? 1 : 0);
  summary += ",resolvedFactoryIdentity=" +
      std::to_string(resolved.injected_factory_identity_for_test);
  summary += ",generatedTokenPath=1";
  summary += ",userAgent=" + resolved.user_agent;
  player->Release();
  return NewStringUtfChecked(
      env, summary, "nativeBuilderMediaSourceFactoryGeneratedTokenSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativePriorityTaskManagerWrapperSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  std::unique_ptr<ExoPlayerSdkPriorityTaskManager> priority_task_manager =
      ExoPlayerSdkPriorityTaskManager::Create(env);
  if (priority_task_manager == nullptr) {
    return NewStringUtfChecked(
        env,
        "priority-task-manager-wrapper-error:create",
        "nativePriorityTaskManagerWrapperSmokeTest.error");
  }
  priority_task_manager->Add(77);
  bool proceed_77 = priority_task_manager->ProceedNonBlocking(77);
  bool proceed_76 = priority_task_manager->ProceedNonBlocking(76);

  PlayerConfig config;
  std::shared_ptr<ExoPlayerBridge> bridge = ExoPlayerBridge::Create(env, context, config);
  if (bridge == nullptr) {
    return NewStringUtfChecked(
        env,
        "priority-task-manager-wrapper-error:createBridge",
        "nativePriorityTaskManagerWrapperSmokeTest.error");
  }
  jobject manager = priority_task_manager->GetJavaObjectLocalRef(env);
  bridge->SetPriorityTaskManager(env, manager);
  DeleteLocalRefIfNotNull(env, manager);
  std::vector<std::string> attached = BridgeGetPriorityTaskManagerStateForTest(env, bridge);
  bridge->SetPriorityTaskManager(env, nullptr);
  std::vector<std::string> cleared = BridgeGetPriorityTaskManagerStateForTest(env, bridge);
  bridge->Release(env);
  PlayerConfig player_config;
  std::unique_ptr<ExoPlayerSdkPlayer> player =
      ExoPlayerSdkPlayer::Create(env, context, player_config);
  bool sdk_clear_priority_task_manager_safe = false;
  if (player != nullptr) {
    player->SetPriorityTaskManager(priority_task_manager.get());
    player->SetPriorityTaskManagerEnabled(true);
    player->ClearPriorityTaskManager();
    player->Release();
    sdk_clear_priority_task_manager_safe = true;
  }
  priority_task_manager->Remove(77);
  bool proceed_after_remove = priority_task_manager->ProceedNonBlocking(77);
  priority_task_manager->Release();

  std::string summary = "proceed77=" + std::to_string(proceed_77 ? 1 : 0);
  summary += ",proceed76=" + std::to_string(proceed_76 ? 1 : 0);
  summary += ",attachedEnabled=";
  summary += attached.size() > 0 ? attached[0] : "";
  summary += ",attachedState=";
  summary += attached.size() > 1 ? attached[1] : "";
  summary += ",attachedRegistered=";
  summary += attached.size() > 2 ? attached[2] : "";
  summary += ",attachedPriority=";
  summary += attached.size() > 3 ? attached[3] : "";
  summary += ",clearedEnabled=";
  summary += cleared.size() > 0 ? cleared[0] : "";
  summary += ",clearedState=";
  summary += cleared.size() > 1 ? cleared[1] : "";
  summary += ",clearedRegistered=";
  summary += cleared.size() > 2 ? cleared[2] : "";
  summary += ",clearedPriority=";
  summary += cleared.size() > 3 ? cleared[3] : "";
  summary += ",proceedAfterRemove=" + std::to_string(proceed_after_remove ? 1 : 0);
  summary += ",sdkClearPriorityTaskManagerSafe=" +
      std::to_string(sdk_clear_priority_task_manager_safe ? 1 : 0);
  return NewStringUtfChecked(env, summary, "nativePriorityTaskManagerWrapperSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeVideoEffectsConversionSmokeTest(
    JNIEnv* env,
    jclass,
    jobject context) {
  PlayerConfig config;
  std::shared_ptr<ExoPlayerBridge> bridge = ExoPlayerBridge::Create(env, context, config);
  if (bridge == nullptr) {
    return NewStringUtfChecked(
        env,
        "video-effects-error:createBridge",
        "nativeVideoEffectsConversionSmokeTest.error");
  }
  std::vector<VideoEffectDescriptor> video_effects;
  VideoEffectDescriptor scale_and_rotate;
  scale_and_rotate.type = VideoEffectDescriptor::Type::kScaleAndRotate;
  scale_and_rotate.scale_x = 1.5f;
  scale_and_rotate.scale_y = 0.75f;
  scale_and_rotate.rotation_degrees = 45.0f;
  video_effects.push_back(scale_and_rotate);
  VideoEffectDescriptor rgb_adjustment;
  rgb_adjustment.type = VideoEffectDescriptor::Type::kRgbAdjustment;
  rgb_adjustment.red_scale = 1.2f;
  rgb_adjustment.green_scale = 0.8f;
  rgb_adjustment.blue_scale = 1.1f;
  video_effects.push_back(rgb_adjustment);
  bridge->SetVideoEffects(env, video_effects);
  std::string summary = BridgeSummarizeVideoEffectsForTest(env, bridge, video_effects);
  std::vector<VideoEffectDescriptor> cleared_video_effects;
  bridge->SetVideoEffects(env, cleared_video_effects);
  std::string cleared_summary =
      BridgeSummarizeVideoEffectsForTest(env, bridge, cleared_video_effects);
  summary += ",afterClear=" + cleared_summary;
  std::vector<VideoEffectDescriptor> reapplied_video_effects;
  VideoEffectDescriptor reapply_rgb_adjustment;
  reapply_rgb_adjustment.type = VideoEffectDescriptor::Type::kRgbAdjustment;
  reapply_rgb_adjustment.red_scale = 0.9f;
  reapply_rgb_adjustment.green_scale = 1.05f;
  reapply_rgb_adjustment.blue_scale = 1.15f;
  reapplied_video_effects.push_back(reapply_rgb_adjustment);
  VideoEffectDescriptor reapply_scale_and_rotate_first;
  reapply_scale_and_rotate_first.type = VideoEffectDescriptor::Type::kScaleAndRotate;
  reapply_scale_and_rotate_first.scale_x = 2.0f;
  reapply_scale_and_rotate_first.scale_y = 0.5f;
  reapply_scale_and_rotate_first.rotation_degrees = 90.0f;
  reapplied_video_effects.push_back(reapply_scale_and_rotate_first);
  VideoEffectDescriptor reapply_scale_and_rotate_second;
  reapply_scale_and_rotate_second.type = VideoEffectDescriptor::Type::kScaleAndRotate;
  reapply_scale_and_rotate_second.scale_x = 1.0f;
  reapply_scale_and_rotate_second.scale_y = 1.0f;
  reapply_scale_and_rotate_second.rotation_degrees = 180.0f;
  reapplied_video_effects.push_back(reapply_scale_and_rotate_second);
  VideoEffectDescriptor reapply_rgb_identity;
  reapply_rgb_identity.type = VideoEffectDescriptor::Type::kRgbAdjustment;
  reapply_rgb_identity.red_scale = 1.0f;
  reapply_rgb_identity.green_scale = 1.0f;
  reapply_rgb_identity.blue_scale = 1.0f;
  reapplied_video_effects.push_back(reapply_rgb_identity);
  VideoEffectDescriptor reapply_scale_identity;
  reapply_scale_identity.type = VideoEffectDescriptor::Type::kScaleAndRotate;
  reapply_scale_identity.scale_x = 1.0f;
  reapply_scale_identity.scale_y = 1.0f;
  reapply_scale_identity.rotation_degrees = 0.0f;
  reapplied_video_effects.push_back(reapply_scale_identity);
  VideoEffectDescriptor presentation;
  presentation.type = VideoEffectDescriptor::Type::kPresentation;
  presentation.presentation_width = 640;
  presentation.presentation_height = 360;
  presentation.presentation_layout = 2;
  reapplied_video_effects.push_back(presentation);
  VideoEffectDescriptor presentation_second_layout;
  presentation_second_layout.type = VideoEffectDescriptor::Type::kPresentation;
  presentation_second_layout.presentation_width = 320;
  presentation_second_layout.presentation_height = 240;
  presentation_second_layout.presentation_layout = 1;
  reapplied_video_effects.push_back(presentation_second_layout);
  bridge->SetVideoEffects(env, reapplied_video_effects);
  std::string reapplied_summary =
      BridgeSummarizeVideoEffectsForTest(env, bridge, reapplied_video_effects);
  summary += ",afterReapply=" + reapplied_summary;
  bridge->Release(env);
  return NewStringUtfChecked(env, summary, "nativeVideoEffectsConversionSmokeTest");
}

JNIEXPORT jstring JNICALL
Java_androidx_media3_exoplayer_cppbridge_CppBridgeNativeSmokeTestHelper_nativeBuildPlaylistIdsForTest(
    JNIEnv* env,
    jclass,
    jobjectArray urls) {
  std::vector<MediaItemDescriptor> media_items = JStringArrayToMediaItems(env, urls);
  std::string summary = BuildPlaylistIdsSummary(media_items);
  return NewStringUtfChecked(env, summary, "nativeBuildPlaylistIdsForTest");
}

}  // extern "C"
