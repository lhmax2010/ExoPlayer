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
  @Nullable public final String metadataToken;
  public final String[] labelLanguages;
  public final String[] labelValues;
  @Nullable public final String customDataToken;
  public final int maxInputSize;
  public final int maxNumReorderSamples;
  public final int initializationDataCount;
  public final int initializationDataTotalBytes;
  public final byte[][] initializationData;
  @Nullable public final String drmSchemeType;
  public final int drmSchemeDataCount;
  public final String[] drmSchemeUuids;
  public final String[] drmSchemeLicenseServerUrls;
  public final String[] drmSchemeMimeTypes;
  public final byte[][] drmSchemeData;
  public final int[] drmSchemeDataHasData;
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
  @Nullable public final byte[] projectionData;
  public final int stereoMode;
  public final int colorStandard;
  public final int colorRange;
  public final int colorTransfer;
  @Nullable public final byte[] colorHdrStaticInfo;
  public final int colorLumaBitdepth;
  public final int colorChromaBitdepth;
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
  public final int auxiliaryTrackType;
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
      boolean supportedWithinCapabilities,
      @Nullable String metadataToken,
      @Nullable String[] labelLanguages,
      @Nullable String[] labelValues,
      @Nullable String customDataToken,
      @Nullable byte[][] initializationData,
      @Nullable String drmSchemeType,
      @Nullable String[] drmSchemeUuids,
      @Nullable String[] drmSchemeLicenseServerUrls,
      @Nullable String[] drmSchemeMimeTypes,
      @Nullable byte[][] drmSchemeData,
      @Nullable byte[] colorHdrStaticInfo,
      int colorLumaBitdepth,
      int colorChromaBitdepth,
      @Nullable byte[] projectionData,
      int auxiliaryTrackType,
      @Nullable int[] drmSchemeDataHasData) {
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
    this.metadataToken = metadataToken;
    this.labelLanguages = labelLanguages != null ? labelLanguages : new String[0];
    this.labelValues = labelValues != null ? labelValues : new String[0];
    this.customDataToken = customDataToken;
    this.maxInputSize = maxInputSize;
    this.maxNumReorderSamples = maxNumReorderSamples;
    this.initializationDataCount = initializationDataCount;
    this.initializationDataTotalBytes = initializationDataTotalBytes;
    this.initializationData = initializationData != null ? initializationData : new byte[0][];
    this.drmSchemeType = drmSchemeType;
    this.drmSchemeDataCount = drmSchemeDataCount;
    this.drmSchemeUuids = drmSchemeUuids != null ? drmSchemeUuids : new String[0];
    this.drmSchemeLicenseServerUrls =
        drmSchemeLicenseServerUrls != null ? drmSchemeLicenseServerUrls : new String[0];
    this.drmSchemeMimeTypes = drmSchemeMimeTypes != null ? drmSchemeMimeTypes : new String[0];
    this.drmSchemeData = drmSchemeData != null ? drmSchemeData : new byte[0][];
    this.drmSchemeDataHasData = drmSchemeDataHasData != null ? drmSchemeDataHasData : new int[0];
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
    this.projectionData = projectionData;
    this.stereoMode = stereoMode;
    this.colorStandard = colorStandard;
    this.colorRange = colorRange;
    this.colorTransfer = colorTransfer;
    this.colorHdrStaticInfo = colorHdrStaticInfo;
    this.colorLumaBitdepth = colorLumaBitdepth;
    this.colorChromaBitdepth = colorChromaBitdepth;
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
    this.auxiliaryTrackType = auxiliaryTrackType;
    this.formatSupport = formatSupport;
    this.selected = selected;
    this.supported = supported;
    this.supportedWithinCapabilities = supportedWithinCapabilities;
  }
}
