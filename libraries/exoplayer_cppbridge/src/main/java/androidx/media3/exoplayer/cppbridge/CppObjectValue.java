package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Reduced object value metadata used by the native bridge. */
public final class CppObjectValue {

  public static final int TYPE_NULL = 0;
  public static final int TYPE_STRING = 1;
  public static final int TYPE_LONG = 2;
  public static final int TYPE_DOUBLE = 3;
  public static final int TYPE_BOOLEAN = 4;
  public static final int TYPE_OTHER = 5;

  public final boolean present;
  @Nullable public final String className;
  public final int valueType;
  @Nullable public final String stringValue;
  public final long longValue;
  public final double doubleValue;
  public final boolean booleanValue;

  public CppObjectValue(
      boolean present,
      @Nullable String className,
      int valueType,
      @Nullable String stringValue,
      long longValue,
      double doubleValue,
      boolean booleanValue) {
    this.present = present;
    this.className = className;
    this.valueType = valueType;
    this.stringValue = stringValue;
    this.longValue = longValue;
    this.doubleValue = doubleValue;
    this.booleanValue = booleanValue;
  }

  public static CppObjectValue nullValue() {
    return new CppObjectValue(false, null, TYPE_NULL, null, 0L, 0.0, false);
  }
}
