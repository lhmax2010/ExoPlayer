package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Track selection preferences used by the native bridge. */
public final class CppTrackSelectionParameters {

  @Nullable public final String preferredAudioLanguage;
  @Nullable public final String preferredTextLanguage;
  public final String[] preferredAudioLanguages;
  public final String[] preferredTextLanguages;
  public final int preferredAudioRoleFlags;
  public final int preferredTextRoleFlags;
  public final int maxAudioChannelCount;
  public final int maxAudioBitrate;
  public final int maxVideoWidth;
  public final int maxVideoHeight;
  public final int maxVideoBitrate;
  public final int viewportWidth;
  public final int viewportHeight;
  public final boolean viewportOrientationMayChange;
  public final boolean selectTextByDefault;
  public final int ignoredTextSelectionFlags;
  public final boolean selectUndeterminedTextLanguage;
  public final boolean forceLowestBitrate;
  public final boolean forceHighestSupportedBitrate;
  public final boolean disableVideo;
  public final boolean disableAudio;
  public final boolean disableText;
  public final int[] disabledTrackTypes;
  public final CppTrackSelectionOverride[] overrides;

  public CppTrackSelectionParameters(
      @Nullable String preferredAudioLanguage,
      @Nullable String preferredTextLanguage,
      String[] preferredAudioLanguages,
      String[] preferredTextLanguages,
      int preferredAudioRoleFlags,
      int preferredTextRoleFlags,
      int maxAudioChannelCount,
      int maxAudioBitrate,
      int maxVideoWidth,
      int maxVideoHeight,
      int maxVideoBitrate,
      int viewportWidth,
      int viewportHeight,
      boolean viewportOrientationMayChange,
      boolean selectTextByDefault,
      int ignoredTextSelectionFlags,
      boolean selectUndeterminedTextLanguage,
      boolean forceLowestBitrate,
      boolean forceHighestSupportedBitrate,
      boolean disableVideo,
      boolean disableAudio,
      boolean disableText,
      int[] disabledTrackTypes,
      CppTrackSelectionOverride[] overrides) {
    this.preferredAudioLanguage = preferredAudioLanguage;
    this.preferredTextLanguage = preferredTextLanguage;
    this.preferredAudioLanguages = preferredAudioLanguages;
    this.preferredTextLanguages = preferredTextLanguages;
    this.preferredAudioRoleFlags = preferredAudioRoleFlags;
    this.preferredTextRoleFlags = preferredTextRoleFlags;
    this.maxAudioChannelCount = maxAudioChannelCount;
    this.maxAudioBitrate = maxAudioBitrate;
    this.maxVideoWidth = maxVideoWidth;
    this.maxVideoHeight = maxVideoHeight;
    this.maxVideoBitrate = maxVideoBitrate;
    this.viewportWidth = viewportWidth;
    this.viewportHeight = viewportHeight;
    this.viewportOrientationMayChange = viewportOrientationMayChange;
    this.selectTextByDefault = selectTextByDefault;
    this.ignoredTextSelectionFlags = ignoredTextSelectionFlags;
    this.selectUndeterminedTextLanguage = selectUndeterminedTextLanguage;
    this.forceLowestBitrate = forceLowestBitrate;
    this.forceHighestSupportedBitrate = forceHighestSupportedBitrate;
    this.disableVideo = disableVideo;
    this.disableAudio = disableAudio;
    this.disableText = disableText;
    this.disabledTrackTypes = disabledTrackTypes;
    this.overrides = overrides;
  }
}
