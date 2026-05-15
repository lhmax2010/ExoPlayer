package androidx.media3.exoplayer.cppbridge;

/** Reduced descriptor for a subset of ExoPlayer video effects. */
public final class CppVideoEffect {

  public static final int TYPE_SCALE_AND_ROTATE = 1;
  public static final int TYPE_RGB_ADJUSTMENT = 2;
  public static final int TYPE_PRESENTATION = 3;

  public final int type;
  public final float scaleX;
  public final float scaleY;
  public final float rotationDegrees;
  public final float redScale;
  public final float greenScale;
  public final float blueScale;
  public final int presentationWidth;
  public final int presentationHeight;
  public final int presentationLayout;

  public CppVideoEffect(
      int type,
      float scaleX,
      float scaleY,
      float rotationDegrees,
      float redScale,
      float greenScale,
      float blueScale,
      int presentationWidth,
      int presentationHeight,
      int presentationLayout) {
    this.type = type;
    this.scaleX = scaleX;
    this.scaleY = scaleY;
    this.rotationDegrees = rotationDegrees;
    this.redScale = redScale;
    this.greenScale = greenScale;
    this.blueScale = blueScale;
    this.presentationWidth = presentationWidth;
    this.presentationHeight = presentationHeight;
    this.presentationLayout = presentationLayout;
  }
}
