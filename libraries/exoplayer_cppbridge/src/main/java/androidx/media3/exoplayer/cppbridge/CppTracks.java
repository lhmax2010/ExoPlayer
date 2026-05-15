package androidx.media3.exoplayer.cppbridge;

/** Reduced tracks snapshot used by the native bridge. */
public final class CppTracks {

  public final CppTrackGroup[] groups;
  public final boolean containsAudio;
  public final boolean containsVideo;
  public final boolean containsText;
  public final boolean containsImage;
  public final boolean audioSelected;
  public final boolean videoSelected;
  public final boolean textSelected;
  public final boolean imageSelected;
  public final boolean audioSupported;
  public final boolean videoSupported;
  public final boolean textSupported;
  public final boolean imageSupported;
  public final boolean audioSupportedAllowingExceedsCapabilities;
  public final boolean videoSupportedAllowingExceedsCapabilities;
  public final boolean textSupportedAllowingExceedsCapabilities;
  public final boolean imageSupportedAllowingExceedsCapabilities;

  public CppTracks(
      CppTrackGroup[] groups,
      boolean containsAudio,
      boolean containsVideo,
      boolean containsText,
      boolean containsImage,
      boolean audioSelected,
      boolean videoSelected,
      boolean textSelected,
      boolean imageSelected,
      boolean audioSupported,
      boolean videoSupported,
      boolean textSupported,
      boolean imageSupported,
      boolean audioSupportedAllowingExceedsCapabilities,
      boolean videoSupportedAllowingExceedsCapabilities,
      boolean textSupportedAllowingExceedsCapabilities,
      boolean imageSupportedAllowingExceedsCapabilities) {
    this.groups = groups;
    this.containsAudio = containsAudio;
    this.containsVideo = containsVideo;
    this.containsText = containsText;
    this.containsImage = containsImage;
    this.audioSelected = audioSelected;
    this.videoSelected = videoSelected;
    this.textSelected = textSelected;
    this.imageSelected = imageSelected;
    this.audioSupported = audioSupported;
    this.videoSupported = videoSupported;
    this.textSupported = textSupported;
    this.imageSupported = imageSupported;
    this.audioSupportedAllowingExceedsCapabilities =
        audioSupportedAllowingExceedsCapabilities;
    this.videoSupportedAllowingExceedsCapabilities =
        videoSupportedAllowingExceedsCapabilities;
    this.textSupportedAllowingExceedsCapabilities =
        textSupportedAllowingExceedsCapabilities;
    this.imageSupportedAllowingExceedsCapabilities =
        imageSupportedAllowingExceedsCapabilities;
  }
}
