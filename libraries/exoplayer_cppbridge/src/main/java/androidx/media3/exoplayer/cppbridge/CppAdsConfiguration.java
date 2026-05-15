package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Reduced ads configuration used by the native bridge. */
public final class CppAdsConfiguration {

  public final String adTagUri;
  @Nullable public final String adsId;
  @Nullable public final String adsIdToken;

  public CppAdsConfiguration(
      String adTagUri, @Nullable String adsId, @Nullable String adsIdToken) {
    this.adTagUri = adTagUri;
    this.adsId = adsId;
    this.adsIdToken = adsIdToken;
  }
}
