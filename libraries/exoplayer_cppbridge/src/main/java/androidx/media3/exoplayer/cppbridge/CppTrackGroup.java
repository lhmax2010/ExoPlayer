package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Track group snapshot used by the native bridge. */
public final class CppTrackGroup {

  @Nullable public final String id;
  @Nullable public final String groupToken;
  public final int type;
  public final boolean adaptiveSupported;
  public final boolean selected;
  public final boolean supported;
  public final boolean supportedAllowingExceedsCapabilities;
  public final CppTrackInfo[] tracks;

  public CppTrackGroup(
      @Nullable String id,
      @Nullable String groupToken,
      int type,
      boolean adaptiveSupported,
      boolean selected,
      boolean supported,
      boolean supportedAllowingExceedsCapabilities,
      CppTrackInfo[] tracks) {
    this.id = id;
    this.groupToken = groupToken;
    this.type = type;
    this.adaptiveSupported = adaptiveSupported;
    this.selected = selected;
    this.supported = supported;
    this.supportedAllowingExceedsCapabilities = supportedAllowingExceedsCapabilities;
    this.tracks = tracks;
  }
}
