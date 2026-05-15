package androidx.media3.exoplayer.cppbridge;

import androidx.media3.common.C;

/** Java-side configuration for DefaultMediaSourceFactory used by the C++ bridge. */
public final class CppMediaSourceFactoryConfig {

  public static final CppMediaSourceFactoryConfig DEFAULT =
      new CppMediaSourceFactoryConfig(
          /* factoryToken= */ null,
          /* parseSubtitlesDuringExtraction= */ true,
          /* loadOnlySelectedTracks= */ false,
          /* defaultRequestHeaderNames= */ new String[0],
          /* defaultRequestHeaderValues= */ new String[0],
          /* userAgent= */ null,
          /* connectTimeoutMs= */ -1,
          /* readTimeoutMs= */ -1,
          /* allowCrossProtocolRedirects= */ false,
          /* liveTargetOffsetMs= */ C.TIME_UNSET,
          /* liveMinOffsetMs= */ C.TIME_UNSET,
          /* liveMaxOffsetMs= */ C.TIME_UNSET,
          /* liveMinSpeed= */ C.RATE_UNSET,
          /* liveMaxSpeed= */ C.RATE_UNSET);

  public final String factoryToken;
  public final boolean parseSubtitlesDuringExtraction;
  public final boolean loadOnlySelectedTracks;
  public final String[] defaultRequestHeaderNames;
  public final String[] defaultRequestHeaderValues;
  public final String userAgent;
  public final int connectTimeoutMs;
  public final int readTimeoutMs;
  public final boolean allowCrossProtocolRedirects;
  public final long liveTargetOffsetMs;
  public final long liveMinOffsetMs;
  public final long liveMaxOffsetMs;
  public final float liveMinSpeed;
  public final float liveMaxSpeed;

  public CppMediaSourceFactoryConfig(
      String factoryToken,
      boolean parseSubtitlesDuringExtraction,
      boolean loadOnlySelectedTracks,
      String[] defaultRequestHeaderNames,
      String[] defaultRequestHeaderValues,
      String userAgent,
      int connectTimeoutMs,
      int readTimeoutMs,
      boolean allowCrossProtocolRedirects,
      long liveTargetOffsetMs,
      long liveMinOffsetMs,
      long liveMaxOffsetMs,
      float liveMinSpeed,
      float liveMaxSpeed) {
    this.factoryToken = factoryToken;
    this.parseSubtitlesDuringExtraction = parseSubtitlesDuringExtraction;
    this.loadOnlySelectedTracks = loadOnlySelectedTracks;
    this.defaultRequestHeaderNames =
        defaultRequestHeaderNames != null ? defaultRequestHeaderNames : new String[0];
    this.defaultRequestHeaderValues =
        defaultRequestHeaderValues != null ? defaultRequestHeaderValues : new String[0];
    this.userAgent = userAgent;
    this.connectTimeoutMs = connectTimeoutMs;
    this.readTimeoutMs = readTimeoutMs;
    this.allowCrossProtocolRedirects = allowCrossProtocolRedirects;
    this.liveTargetOffsetMs = liveTargetOffsetMs;
    this.liveMinOffsetMs = liveMinOffsetMs;
    this.liveMaxOffsetMs = liveMaxOffsetMs;
    this.liveMinSpeed = liveMinSpeed;
    this.liveMaxSpeed = liveMaxSpeed;
  }
}
