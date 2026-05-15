package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Reduced device info descriptor used by the native bridge. */
public final class CppDeviceInfo {

  public final int playbackType;
  public final int minVolume;
  public final int maxVolume;
  @Nullable public final String routingControllerId;

  public CppDeviceInfo(
      int playbackType, int minVolume, int maxVolume, @Nullable String routingControllerId) {
    this.playbackType = playbackType;
    this.minVolume = minVolume;
    this.maxVolume = maxVolume;
    this.routingControllerId = routingControllerId;
  }
}
