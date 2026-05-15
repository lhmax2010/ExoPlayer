package androidx.media3.exoplayer.cppbridge;

/** Seek parameters descriptor used by the native bridge. */
public final class CppSeekParameters {

  public final long toleranceBeforeUs;
  public final long toleranceAfterUs;

  public CppSeekParameters(long toleranceBeforeUs, long toleranceAfterUs) {
    this.toleranceBeforeUs = toleranceBeforeUs;
    this.toleranceAfterUs = toleranceAfterUs;
  }
}
