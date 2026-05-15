package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Immutable media description used by the native bridge. */
public final class CppMediaItem {

  public final String uri;
  @Nullable public final String mediaId;
  @Nullable public final String mimeType;
  public final int sourceType;
  public final boolean tagPresent;
  @Nullable public final String tagString;
  @Nullable public final String tagToken;
  @Nullable public final CppMediaMetadata mediaMetadata;
  @Nullable public final CppRequestMetadata requestMetadata;
  @Nullable public final CppAdsConfiguration adsConfiguration;
  public final CppSubtitleConfiguration[] subtitleConfigurations;
  @Nullable public final CppClippingConfiguration clippingConfiguration;
  @Nullable public final CppLiveConfiguration liveConfiguration;
  @Nullable public final CppDrmConfiguration drmConfiguration;

  public CppMediaItem(
      String uri,
      @Nullable String mediaId,
      @Nullable String mimeType,
      int sourceType,
      boolean tagPresent,
      @Nullable String tagString,
      @Nullable String tagToken,
      @Nullable CppMediaMetadata mediaMetadata,
      @Nullable CppRequestMetadata requestMetadata,
      @Nullable CppAdsConfiguration adsConfiguration,
      CppSubtitleConfiguration[] subtitleConfigurations,
      @Nullable CppClippingConfiguration clippingConfiguration,
      @Nullable CppLiveConfiguration liveConfiguration,
      @Nullable CppDrmConfiguration drmConfiguration) {
    this.uri = uri;
    this.mediaId = mediaId;
    this.mimeType = mimeType;
    this.sourceType = sourceType;
    this.tagPresent = tagPresent;
    this.tagString = tagString;
    this.tagToken = tagToken;
    this.mediaMetadata = mediaMetadata;
    this.requestMetadata = requestMetadata;
    this.adsConfiguration = adsConfiguration;
    this.subtitleConfigurations = subtitleConfigurations;
    this.clippingConfiguration = clippingConfiguration;
    this.liveConfiguration = liveConfiguration;
    this.drmConfiguration = drmConfiguration;
  }
}
