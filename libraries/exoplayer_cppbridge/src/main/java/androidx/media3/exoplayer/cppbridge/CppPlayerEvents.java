package androidx.media3.exoplayer.cppbridge;

/** Reduced player-events descriptor used by the native bridge. */
public final class CppPlayerEvents {

  public final int[] eventCodes;

  public CppPlayerEvents(int[] eventCodes) {
    this.eventCodes = eventCodes;
  }
}
