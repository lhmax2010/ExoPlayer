package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Single track info snapshot used by the native bridge. */
public final class CppTrackInfo {

  @Nullable public final String id;
  @Nullable public final String language;
  @Nullable public final String label;
  @Nullable public final String labelToken;
  @Nullable public final String mimeType;
  @Nullable public final String containerMimeType;
  @Nullable public final String codecs;
  public final int bitrate;
  public final int width;
  public final int height;
  public final float frameRate;
  public final int sampleRate;
  public final int channelCount;
  public final int accessibilityChannel;
  public final int roleFlags;
  public final int selectionFlags;
  public final int formatSupport;
  public final boolean selected;
  public final boolean supported;
  public final boolean supportedWithinCapabilities;

  public CppTrackInfo(
      @Nullable String id,
      @Nullable String language,
      @Nullable String label,
      @Nullable String labelToken,
      @Nullable String mimeType,
      @Nullable String containerMimeType,
      @Nullable String codecs,
      int bitrate,
      int width,
      int height,
      float frameRate,
      int sampleRate,
      int channelCount,
      int accessibilityChannel,
      int roleFlags,
      int selectionFlags,
      int formatSupport,
      boolean selected,
      boolean supported,
      boolean supportedWithinCapabilities) {
    this.id = id;
    this.language = language;
    this.label = label;
    this.labelToken = labelToken;
    this.mimeType = mimeType;
    this.containerMimeType = containerMimeType;
    this.codecs = codecs;
    this.bitrate = bitrate;
    this.width = width;
    this.height = height;
    this.frameRate = frameRate;
    this.sampleRate = sampleRate;
    this.channelCount = channelCount;
    this.accessibilityChannel = accessibilityChannel;
    this.roleFlags = roleFlags;
    this.selectionFlags = selectionFlags;
    this.formatSupport = formatSupport;
    this.selected = selected;
    this.supported = supported;
    this.supportedWithinCapabilities = supportedWithinCapabilities;
  }
}
