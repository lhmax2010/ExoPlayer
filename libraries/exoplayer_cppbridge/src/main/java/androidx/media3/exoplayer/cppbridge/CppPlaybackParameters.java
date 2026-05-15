package androidx.media3.exoplayer.cppbridge;

/** Reduced playback parameters descriptor used by the native bridge. */
public final class CppPlaybackParameters {

  public final float speed;
  public final float pitch;

  public CppPlaybackParameters(float speed, float pitch) {
    this.speed = speed;
    this.pitch = pitch;
  }
}
