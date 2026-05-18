package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Reduced Bundle entry used by the native bridge for stable primitive value transport. */
public final class CppBundleValue {

  public static final int TYPE_STRING = 1;
  public static final int TYPE_LONG = 2;
  public static final int TYPE_DOUBLE = 3;
  public static final int TYPE_BOOLEAN = 4;
  public static final int TYPE_BYTE_ARRAY = 5;

  public final String key;
  public final int valueType;
  @Nullable public final String stringValue;
  public final long longValue;
  public final double doubleValue;
  public final boolean booleanValue;
  public final byte[] byteArrayValue;

  public CppBundleValue(
      String key,
      int valueType,
      @Nullable String stringValue,
      long longValue,
      double doubleValue,
      boolean booleanValue,
      @Nullable byte[] byteArrayValue) {
    this.key = key;
    this.valueType = valueType;
    this.stringValue = stringValue;
    this.longValue = longValue;
    this.doubleValue = doubleValue;
    this.booleanValue = booleanValue;
    this.byteArrayValue = byteArrayValue != null ? byteArrayValue : new byte[0];
  }
}
