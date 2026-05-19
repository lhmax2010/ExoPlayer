package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Reduced request metadata used by the native bridge. */
public final class CppRequestMetadata {

  @Nullable public final String mediaUri;
  @Nullable public final String searchQuery;
  public final boolean extrasPresent;
  public final int extrasKeyCount;
  @Nullable public final String extrasToken;
  public final CppBundleValue[] extrasValues;

  public CppRequestMetadata(
      @Nullable String mediaUri,
      @Nullable String searchQuery,
      boolean extrasPresent,
      int extrasKeyCount,
      @Nullable String extrasToken) {
    this(mediaUri, searchQuery, extrasPresent, extrasKeyCount, extrasToken, null);
  }

  public CppRequestMetadata(
      @Nullable String mediaUri,
      @Nullable String searchQuery,
      boolean extrasPresent,
      int extrasKeyCount,
      @Nullable String extrasToken,
      @Nullable CppBundleValue[] extrasValues) {
    this.mediaUri = mediaUri;
    this.searchQuery = searchQuery;
    this.extrasPresent = extrasPresent;
    this.extrasKeyCount = extrasKeyCount;
    this.extrasToken = extrasToken;
    this.extrasValues = extrasValues != null ? extrasValues : new CppBundleValue[0];
  }
}
