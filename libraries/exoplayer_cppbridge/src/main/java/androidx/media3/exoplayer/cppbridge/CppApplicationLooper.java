package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Reduced application looper descriptor used by the native bridge. */
public final class CppApplicationLooper {

  @Nullable public final String threadName;
  public final long threadId;
  public final boolean isCurrentThread;

  public CppApplicationLooper(
      @Nullable String threadName, long threadId, boolean isCurrentThread) {
    this.threadName = threadName;
    this.threadId = threadId;
    this.isCurrentThread = isCurrentThread;
  }
}
