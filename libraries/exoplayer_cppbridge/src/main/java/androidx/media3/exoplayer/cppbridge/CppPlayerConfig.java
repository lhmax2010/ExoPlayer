package androidx.media3.exoplayer.cppbridge;

import androidx.media3.common.C;

/** Basic player configuration used by the phase-1 C++ bridge. */
public final class CppPlayerConfig {

  public static final CppPlayerConfig DEFAULT =
      new CppPlayerConfig(
          /* handleAudioFocus= */ true,
          /* handleAudioBecomingNoisy= */ true,
          /* useLazyPreparation= */ true,
          CppMediaSourceFactoryConfig.DEFAULT,
          /* seekBackIncrementMs= */ 5_000,
          /* seekForwardIncrementMs= */ 15_000,
          /* wakeMode= */ 0, // Media3 C.WAKE_MODE_NONE
          /* priority= */ 0, // No PriorityTaskManager priority unless explicitly configured.
          /* usePriorityTaskManager= */ false,
          /* targetPreloadDurationUs= */ C.TIME_UNSET);

  public final boolean handleAudioFocus;
  public final boolean handleAudioBecomingNoisy;
  public final boolean useLazyPreparation;
  public final CppMediaSourceFactoryConfig mediaSourceFactoryConfig;
  public final long seekBackIncrementMs;
  public final long seekForwardIncrementMs;
  /** One of the Media3 C.WAKE_MODE_* constants. */
  public final int wakeMode;

  /**
   * Priority passed to ExoPlayer#setPriority / PriorityTaskManager, for example
   * C.PRIORITY_PLAYBACK when the caller wants playback-priority resource arbitration.
   */
  public final int priority;
  public final boolean usePriorityTaskManager;
  public final long targetPreloadDurationUs;

  public CppPlayerConfig(
      boolean handleAudioFocus,
      boolean handleAudioBecomingNoisy,
      boolean useLazyPreparation,
      CppMediaSourceFactoryConfig mediaSourceFactoryConfig,
      long seekBackIncrementMs,
      long seekForwardIncrementMs,
      int wakeMode,
      int priority,
      boolean usePriorityTaskManager,
      long targetPreloadDurationUs) {
    this.handleAudioFocus = handleAudioFocus;
    this.handleAudioBecomingNoisy = handleAudioBecomingNoisy;
    this.useLazyPreparation = useLazyPreparation;
    this.mediaSourceFactoryConfig = mediaSourceFactoryConfig;
    this.seekBackIncrementMs = seekBackIncrementMs;
    this.seekForwardIncrementMs = seekForwardIncrementMs;
    this.wakeMode = wakeMode;
    this.priority = priority;
    this.usePriorityTaskManager = usePriorityTaskManager;
    this.targetPreloadDurationUs = targetPreloadDurationUs;
  }
}
