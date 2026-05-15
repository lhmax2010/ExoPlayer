package androidx.media3.exoplayer.cppbridge;

/** Reduced track-selection override used by the native bridge. */
public final class CppTrackSelectionOverride {

  public final String trackGroupId;
  public final int trackType;
  public final int[] trackIndices;

  public CppTrackSelectionOverride(String trackGroupId, int trackType, int[] trackIndices) {
    this.trackGroupId = trackGroupId;
    this.trackType = trackType;
    this.trackIndices = trackIndices;
  }
}
