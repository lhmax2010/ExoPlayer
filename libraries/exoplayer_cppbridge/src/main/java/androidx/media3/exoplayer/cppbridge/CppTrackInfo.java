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
  public final int averageBitrate;
  public final int peakBitrate;
  public final int metadataEntryCount;
  public final int maxInputSize;
  public final int maxNumReorderSamples;
  public final int initializationDataCount;
  public final int initializationDataTotalBytes;
  public final int drmSchemeDataCount;
  public final long subsampleOffsetUs;
  public final boolean hasPrerollSamples;
  public final int width;
  public final int height;
  public final int decodedWidth;
  public final int decodedHeight;
  public final float frameRate;
  public final int rotationDegrees;
  public final float pixelWidthHeightRatio;
  public final int projectionDataLength;
  public final int stereoMode;
  public final int colorStandard;
  public final int colorRange;
  public final int colorTransfer;
  public final int maxSubLayers;
  public final int sampleRate;
  public final int channelCount;
  public final int pcmEncoding;
  public final int encoderDelay;
  public final int encoderPadding;
  public final int accessibilityChannel;
  public final int cueReplacementBehavior;
  public final int tileCountHorizontal;
  public final int tileCountVertical;
  public final int cryptoType;
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
      int averageBitrate,
      int peakBitrate,
      int metadataEntryCount,
      int maxInputSize,
      int maxNumReorderSamples,
      int initializationDataCount,
      int initializationDataTotalBytes,
      int drmSchemeDataCount,
      long subsampleOffsetUs,
      boolean hasPrerollSamples,
      int width,
      int height,
      int decodedWidth,
      int decodedHeight,
      float frameRate,
      int rotationDegrees,
      float pixelWidthHeightRatio,
      int projectionDataLength,
      int stereoMode,
      int colorStandard,
      int colorRange,
      int colorTransfer,
      int maxSubLayers,
      int sampleRate,
      int channelCount,
      int pcmEncoding,
      int encoderDelay,
      int encoderPadding,
      int accessibilityChannel,
      int cueReplacementBehavior,
      int tileCountHorizontal,
      int tileCountVertical,
      int cryptoType,
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
    this.averageBitrate = averageBitrate;
    this.peakBitrate = peakBitrate;
    this.metadataEntryCount = metadataEntryCount;
    this.maxInputSize = maxInputSize;
    this.maxNumReorderSamples = maxNumReorderSamples;
    this.initializationDataCount = initializationDataCount;
    this.initializationDataTotalBytes = initializationDataTotalBytes;
    this.drmSchemeDataCount = drmSchemeDataCount;
    this.subsampleOffsetUs = subsampleOffsetUs;
    this.hasPrerollSamples = hasPrerollSamples;
    this.width = width;
    this.height = height;
    this.decodedWidth = decodedWidth;
    this.decodedHeight = decodedHeight;
    this.frameRate = frameRate;
    this.rotationDegrees = rotationDegrees;
    this.pixelWidthHeightRatio = pixelWidthHeightRatio;
    this.projectionDataLength = projectionDataLength;
    this.stereoMode = stereoMode;
    this.colorStandard = colorStandard;
    this.colorRange = colorRange;
    this.colorTransfer = colorTransfer;
    this.maxSubLayers = maxSubLayers;
    this.sampleRate = sampleRate;
    this.channelCount = channelCount;
    this.pcmEncoding = pcmEncoding;
    this.encoderDelay = encoderDelay;
    this.encoderPadding = encoderPadding;
    this.accessibilityChannel = accessibilityChannel;
    this.cueReplacementBehavior = cueReplacementBehavior;
    this.tileCountHorizontal = tileCountHorizontal;
    this.tileCountVertical = tileCountVertical;
    this.cryptoType = cryptoType;
    this.roleFlags = roleFlags;
    this.selectionFlags = selectionFlags;
    this.formatSupport = formatSupport;
    this.selected = selected;
    this.supported = supported;
    this.supportedWithinCapabilities = supportedWithinCapabilities;
  }
}
