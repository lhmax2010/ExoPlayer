package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Reduced ads configuration used by the native bridge. */
public final class CppAdsConfiguration {

  public final String adTagUri;
  @Nullable public final String adsId;
  @Nullable public final String adsIdToken;
  public final CppObjectValue adsIdValue;

  public CppAdsConfiguration(
      String adTagUri, @Nullable String adsId, @Nullable String adsIdToken) {
    this(adTagUri, adsId, adsIdToken, null);
  }

  public CppAdsConfiguration(
      String adTagUri,
      @Nullable String adsId,
      @Nullable String adsIdToken,
      @Nullable CppObjectValue adsIdValue) {
    this.adTagUri = adTagUri;
    this.adsId = adsId;
    this.adsIdToken = adsIdToken;
    this.adsIdValue = adsIdValue != null ? adsIdValue : CppObjectValue.nullValue();
  }
}
