package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** DRM descriptor used by the native bridge. */
public final class CppDrmConfiguration {

  @Nullable public final String schemeUuid;
  @Nullable public final String licenseUri;
  public final String[] licenseRequestHeaderNames;
  public final String[] licenseRequestHeaderValues;
  public final int[] forcedSessionTrackTypes;
  public final byte[] keySetId;
  public final boolean multiSession;
  public final boolean forceDefaultLicenseUri;
  public final boolean playClearContentWithoutKey;

  public CppDrmConfiguration(
      @Nullable String schemeUuid,
      @Nullable String licenseUri,
      String[] licenseRequestHeaderNames,
      String[] licenseRequestHeaderValues,
      int[] forcedSessionTrackTypes,
      byte[] keySetId,
      boolean multiSession,
      boolean forceDefaultLicenseUri,
      boolean playClearContentWithoutKey) {
    this.schemeUuid = schemeUuid;
    this.licenseUri = licenseUri;
    this.licenseRequestHeaderNames =
        licenseRequestHeaderNames != null ? licenseRequestHeaderNames : new String[0];
    this.licenseRequestHeaderValues =
        licenseRequestHeaderValues != null ? licenseRequestHeaderValues : new String[0];
    this.forcedSessionTrackTypes =
        forcedSessionTrackTypes != null ? forcedSessionTrackTypes : new int[0];
    this.keySetId = keySetId != null ? keySetId : new byte[0];
    this.multiSession = multiSession;
    this.forceDefaultLicenseUri = forceDefaultLicenseUri;
    this.playClearContentWithoutKey = playClearContentWithoutKey;
  }
}
