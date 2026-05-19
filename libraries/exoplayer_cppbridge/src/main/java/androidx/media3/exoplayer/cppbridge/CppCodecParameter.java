package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Java transport object for a single C++ codec parameter. */
public final class CppCodecParameter {
  public static final int TYPE_INTEGER = 0;
  public static final int TYPE_LONG = 1;
  public static final int TYPE_FLOAT = 2;
  public static final int TYPE_STRING = 3;
  public static final int TYPE_BYTE_BUFFER = 4;
  public static final int TYPE_NULL = 5;

  public final String key;
  public final int type;
  public final int intValue;
  public final long longValue;
  public final float floatValue;
  @Nullable public final String stringValue;
  @Nullable public final byte[] byteBufferValue;

  public CppCodecParameter(
      String key,
      int type,
      int intValue,
      long longValue,
      float floatValue,
      @Nullable String stringValue,
      @Nullable byte[] byteBufferValue) {
    this.key = key;
    this.type = type;
    this.intValue = intValue;
    this.longValue = longValue;
    this.floatValue = floatValue;
    this.stringValue = stringValue;
    this.byteBufferValue = byteBufferValue;
  }
}
