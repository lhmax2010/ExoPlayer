package androidx.media3.exoplayer.cppbridge;

/** Reduced command-set descriptor used by the native bridge. */
public final class CppCommands {

  public final int[] commandCodes;

  public CppCommands(int[] commandCodes) {
    this.commandCodes = commandCodes;
  }
}
