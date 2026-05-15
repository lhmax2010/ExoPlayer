package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Reduced position discontinuity descriptor used by the native bridge. */
public final class CppPositionInfo {

  public final int mediaItemIndex;
  @Nullable public final CppMediaItem mediaItem;
  public final int periodIndex;
  public final long positionMs;
  public final long contentPositionMs;
  public final int adGroupIndex;
  public final int adIndexInAdGroup;

  public CppPositionInfo(
      int mediaItemIndex,
      @Nullable CppMediaItem mediaItem,
      int periodIndex,
      long positionMs,
      long contentPositionMs,
      int adGroupIndex,
      int adIndexInAdGroup) {
    this.mediaItemIndex = mediaItemIndex;
    this.mediaItem = mediaItem;
    this.periodIndex = periodIndex;
    this.positionMs = positionMs;
    this.contentPositionMs = contentPositionMs;
    this.adGroupIndex = adGroupIndex;
    this.adIndexInAdGroup = adIndexInAdGroup;
  }
}
