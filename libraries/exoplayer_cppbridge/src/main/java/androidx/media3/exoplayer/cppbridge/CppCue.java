package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Reduced cue descriptor used by the native bridge. */
public final class CppCue {

  @Nullable public final String text;
  @Nullable public final String textToken;
  @Nullable public final String bitmapToken;
  public final int textAlignment;
  public final int multiRowAlignment;
  public final float line;
  public final int lineType;
  public final int lineAnchor;
  public final float position;
  public final int positionAnchor;
  public final float size;
  public final float bitmapHeight;
  public final float textSize;
  public final int textSizeType;
  public final int verticalType;
  public final float shearDegrees;
  public final int zIndex;
  public final boolean windowColorSet;
  public final int windowColor;
  public final boolean hasBitmap;

  public CppCue(
      @Nullable String text,
      @Nullable String textToken,
      @Nullable String bitmapToken,
      int textAlignment,
      int multiRowAlignment,
      float line,
      int lineType,
      int lineAnchor,
      float position,
      int positionAnchor,
      float size,
      float bitmapHeight,
      float textSize,
      int textSizeType,
      int verticalType,
      float shearDegrees,
      int zIndex,
      boolean windowColorSet,
      int windowColor,
      boolean hasBitmap) {
    this.text = text;
    this.textToken = textToken;
    this.bitmapToken = bitmapToken;
    this.textAlignment = textAlignment;
    this.multiRowAlignment = multiRowAlignment;
    this.line = line;
    this.lineType = lineType;
    this.lineAnchor = lineAnchor;
    this.position = position;
    this.positionAnchor = positionAnchor;
    this.size = size;
    this.bitmapHeight = bitmapHeight;
    this.textSize = textSize;
    this.textSizeType = textSizeType;
    this.verticalType = verticalType;
    this.shearDegrees = shearDegrees;
    this.zIndex = zIndex;
    this.windowColorSet = windowColorSet;
    this.windowColor = windowColor;
    this.hasBitmap = hasBitmap;
  }
}
