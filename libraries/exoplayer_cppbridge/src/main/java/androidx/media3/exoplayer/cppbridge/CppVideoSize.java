package androidx.media3.exoplayer.cppbridge;

/** Reduced video size descriptor used by the native bridge. */
public final class CppVideoSize {

  public final int width;
  public final int height;
  public final int unappliedRotationDegrees;
  public final float pixelWidthHeightRatio;

  public CppVideoSize(
      int width, int height, int unappliedRotationDegrees, float pixelWidthHeightRatio) {
    this.width = width;
    this.height = height;
    this.unappliedRotationDegrees = unappliedRotationDegrees;
    this.pixelWidthHeightRatio = pixelWidthHeightRatio;
  }
}
