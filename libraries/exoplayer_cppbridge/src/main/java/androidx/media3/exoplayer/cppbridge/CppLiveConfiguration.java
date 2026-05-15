package androidx.media3.exoplayer.cppbridge;

/** Live playback descriptor used by the native bridge. */
public final class CppLiveConfiguration {

  public final long targetOffsetMs;
  public final long minOffsetMs;
  public final long maxOffsetMs;
  public final float minPlaybackSpeed;
  public final float maxPlaybackSpeed;

  public CppLiveConfiguration(
      long targetOffsetMs,
      long minOffsetMs,
      long maxOffsetMs,
      float minPlaybackSpeed,
      float maxPlaybackSpeed) {
    this.targetOffsetMs = targetOffsetMs;
    this.minOffsetMs = minOffsetMs;
    this.maxOffsetMs = maxOffsetMs;
    this.minPlaybackSpeed = minPlaybackSpeed;
    this.maxPlaybackSpeed = maxPlaybackSpeed;
  }
}
