package androidx.media3.exoplayer.cppbridge;

/** Clipping descriptor used by the native bridge. */
public final class CppClippingConfiguration {

  public final long startPositionMs;
  public final long endPositionMs;
  public final boolean relativeToLiveWindow;
  public final boolean relativeToDefaultPosition;
  public final boolean startsAtKeyFrame;
  public final boolean allowUnseekableMedia;

  public CppClippingConfiguration(
      long startPositionMs,
      long endPositionMs,
      boolean relativeToLiveWindow,
      boolean relativeToDefaultPosition,
      boolean startsAtKeyFrame,
      boolean allowUnseekableMedia) {
    this.startPositionMs = startPositionMs;
    this.endPositionMs = endPositionMs;
    this.relativeToLiveWindow = relativeToLiveWindow;
    this.relativeToDefaultPosition = relativeToDefaultPosition;
    this.startsAtKeyFrame = startsAtKeyFrame;
    this.allowUnseekableMedia = allowUnseekableMedia;
  }
}
