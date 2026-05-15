package androidx.media3.exoplayer.cppbridge;

import android.content.Context;
import android.graphics.Bitmap;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import androidx.annotation.Nullable;
import androidx.media3.common.AudioAttributes;
import androidx.media3.common.C;
import androidx.media3.common.DeviceInfo;
import androidx.media3.common.MediaItem;
import androidx.media3.common.Metadata;
import androidx.media3.common.MediaMetadata;
import androidx.media3.common.PlaybackParameters;
import androidx.media3.common.PlaybackException;
import androidx.media3.common.Player;
import androidx.media3.common.PriorityTaskManager;
import androidx.media3.common.Timeline;
import androidx.media3.common.VideoSize;
import androidx.media3.common.text.Cue;
import androidx.media3.common.text.CueGroup;
import androidx.media3.effect.Presentation;
import androidx.media3.effect.RgbAdjustment;
import androidx.media3.effect.ScaleAndRotateTransformation;
import androidx.media3.datasource.DefaultDataSource;
import androidx.media3.datasource.DefaultHttpDataSource;
import androidx.media3.exoplayer.ExoPlayer;
import androidx.media3.exoplayer.ExoPlaybackException;
import androidx.media3.exoplayer.PlayerMessage;
import androidx.media3.exoplayer.Renderer;
import androidx.media3.exoplayer.SeekParameters;
import java.util.Arrays;
import androidx.media3.exoplayer.analytics.AnalyticsListener;
import androidx.media3.exoplayer.image.ImageOutput;
import androidx.media3.exoplayer.source.DefaultMediaSourceFactory;
import androidx.media3.exoplayer.source.LoadEventInfo;
import androidx.media3.exoplayer.source.MediaSource;
import androidx.media3.exoplayer.source.MediaLoadData;
import androidx.media3.ui.PlayerView;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicReference;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/** Java-side ExoPlayer owner used by the C++ bridge. */
public final class CppExoPlayerBridge implements Player.Listener, AnalyticsListener {

  private static final String TAG = "cppbridge";

  private final ExoPlayer player;
  private final Handler playerHandler;
  private final AtomicLong nativeHandle;
  private final AtomicBoolean released;
  private volatile int configuredPriorityForTest;
  @Nullable private volatile PriorityTaskManager priorityTaskManagerForTest;
  private volatile boolean priorityTaskManagerAttachedForTest;
  private volatile boolean priorityTaskManagerRegisteredForTest;
  @Nullable private volatile ImageOutput imageOutputForTest;
  @Nullable private volatile CppCue[] currentCuesForTest;
  private volatile long currentCuesPresentationTimeUsForTest;
  private final boolean configuredHandleAudioFocusForTest;
  private final boolean configuredHandleAudioBecomingNoisyForTest;
  private final boolean configuredUseLazyPreparationForTest;
  private final long configuredSeekBackIncrementMsForTest;
  private final long configuredSeekForwardIncrementMsForTest;
  private final int configuredWakeModeForTest;
  private final CppMediaSourceFactoryConfig mediaSourceFactoryConfig;
  private final String injectedMediaSourceFactoryTokenForTest;
  private final boolean injectedMediaSourceFactoryUsedForTest;
  private final int injectedMediaSourceFactoryIdentityForTest;
  private volatile long latestBitrateEstimate;
  private volatile int droppedVideoFrames;
  private volatile int loadStartedCount;
  private volatile int loadCompletedCount;
  private volatile String latestAudioSampleMimeType = "";
  private volatile String latestVideoSampleMimeType = "";
  private int playerMessageDeliveryCount;
  private int lastPlayerMessageType;
  private String lastPlayerMessagePayload = "";
  private int lastPlayerMessageMediaItemIndex = -1;
  private long lastPlayerMessagePositionMs = C.TIME_UNSET;
  private boolean lastPlayerMessageDeleteAfterDelivery = true;
  private String lastPlayerMessageThreadName = "";

  public CppExoPlayerBridge(Context context, long nativeHandle, @Nullable CppPlayerConfig config) {
    this.nativeHandle = new AtomicLong(nativeHandle);
    released = new AtomicBoolean(false);
    CppPlayerConfig resolvedConfig = config != null ? config : CppPlayerConfig.DEFAULT;
    configuredPriorityForTest = resolvedConfig.priority;
    configuredHandleAudioFocusForTest = resolvedConfig.handleAudioFocus;
    configuredHandleAudioBecomingNoisyForTest = resolvedConfig.handleAudioBecomingNoisy;
    configuredUseLazyPreparationForTest = resolvedConfig.useLazyPreparation;
    configuredSeekBackIncrementMsForTest = resolvedConfig.seekBackIncrementMs;
    configuredSeekForwardIncrementMsForTest = resolvedConfig.seekForwardIncrementMs;
    configuredWakeModeForTest = resolvedConfig.wakeMode;
    priorityTaskManagerForTest =
        resolvedConfig.usePriorityTaskManager ? new PriorityTaskManager() : null;
    priorityTaskManagerAttachedForTest = priorityTaskManagerForTest != null;
    priorityTaskManagerRegisteredForTest = priorityTaskManagerForTest != null;
    mediaSourceFactoryConfig =
        resolvedConfig.mediaSourceFactoryConfig != null
            ? resolvedConfig.mediaSourceFactoryConfig
            : CppMediaSourceFactoryConfig.DEFAULT;
    injectedMediaSourceFactoryTokenForTest =
        mediaSourceFactoryConfig.factoryToken != null ? mediaSourceFactoryConfig.factoryToken : "";
    DefaultHttpDataSource.Factory httpDataSourceFactory = new DefaultHttpDataSource.Factory();
    if (mediaSourceFactoryConfig.defaultRequestHeaderNames.length > 0) {
      Map<String, String> requestProperties = new LinkedHashMap<>();
      int pairCount =
          Math.min(
              mediaSourceFactoryConfig.defaultRequestHeaderNames.length,
              mediaSourceFactoryConfig.defaultRequestHeaderValues.length);
      for (int i = 0; i < pairCount; i++) {
        String headerName = mediaSourceFactoryConfig.defaultRequestHeaderNames[i];
        String headerValue = mediaSourceFactoryConfig.defaultRequestHeaderValues[i];
        if (headerName != null && !headerName.isEmpty() && headerValue != null) {
          requestProperties.put(headerName, headerValue);
        }
      }
      if (!requestProperties.isEmpty()) {
        httpDataSourceFactory.setDefaultRequestProperties(requestProperties);
      }
    }
    if (mediaSourceFactoryConfig.userAgent != null && !mediaSourceFactoryConfig.userAgent.isEmpty()) {
      httpDataSourceFactory.setUserAgent(mediaSourceFactoryConfig.userAgent);
    }
    if (mediaSourceFactoryConfig.connectTimeoutMs >= 0) {
      httpDataSourceFactory.setConnectTimeoutMs(mediaSourceFactoryConfig.connectTimeoutMs);
    }
    if (mediaSourceFactoryConfig.readTimeoutMs >= 0) {
      httpDataSourceFactory.setReadTimeoutMs(mediaSourceFactoryConfig.readTimeoutMs);
    }
    httpDataSourceFactory.setAllowCrossProtocolRedirects(
        mediaSourceFactoryConfig.allowCrossProtocolRedirects);
    DefaultDataSource.Factory dataSourceFactory =
        new DefaultDataSource.Factory(context, httpDataSourceFactory);
    DefaultMediaSourceFactory defaultMediaSourceFactory =
        new DefaultMediaSourceFactory(context)
            .setDataSourceFactory(dataSourceFactory)
            .experimentalParseSubtitlesDuringExtraction(
                mediaSourceFactoryConfig.parseSubtitlesDuringExtraction)
            .setLoadOnlySelectedTracks(mediaSourceFactoryConfig.loadOnlySelectedTracks);
    if (mediaSourceFactoryConfig.liveTargetOffsetMs != C.TIME_UNSET) {
      defaultMediaSourceFactory.setLiveTargetOffsetMs(mediaSourceFactoryConfig.liveTargetOffsetMs);
    }
    if (mediaSourceFactoryConfig.liveMinOffsetMs != C.TIME_UNSET) {
      defaultMediaSourceFactory.setLiveMinOffsetMs(mediaSourceFactoryConfig.liveMinOffsetMs);
    }
    if (mediaSourceFactoryConfig.liveMaxOffsetMs != C.TIME_UNSET) {
      defaultMediaSourceFactory.setLiveMaxOffsetMs(mediaSourceFactoryConfig.liveMaxOffsetMs);
    }
    if (mediaSourceFactoryConfig.liveMinSpeed != C.RATE_UNSET) {
      defaultMediaSourceFactory.setLiveMinSpeed(mediaSourceFactoryConfig.liveMinSpeed);
    }
    if (mediaSourceFactoryConfig.liveMaxSpeed != C.RATE_UNSET) {
      defaultMediaSourceFactory.setLiveMaxSpeed(mediaSourceFactoryConfig.liveMaxSpeed);
    }
    MediaSource.Factory injectedFactory =
        CppMediaSourceFactoryRegistry.resolve(mediaSourceFactoryConfig.factoryToken);
    injectedMediaSourceFactoryUsedForTest = injectedFactory != null;
    injectedMediaSourceFactoryIdentityForTest =
        injectedFactory != null ? System.identityHashCode(injectedFactory) : 0;
    MediaSource.Factory mediaSourceFactory =
        injectedFactory != null ? injectedFactory : defaultMediaSourceFactory;
    player =
        new ExoPlayer.Builder(context)
            .setMediaSourceFactory(mediaSourceFactory)
            .setAudioAttributes(AudioAttributes.DEFAULT, resolvedConfig.handleAudioFocus)
            .setHandleAudioBecomingNoisy(resolvedConfig.handleAudioBecomingNoisy)
            .setUseLazyPreparation(resolvedConfig.useLazyPreparation)
            .setSeekBackIncrementMs(resolvedConfig.seekBackIncrementMs)
            .setSeekForwardIncrementMs(resolvedConfig.seekForwardIncrementMs)
            .setWakeMode(resolvedConfig.wakeMode)
            .setPriority(resolvedConfig.priority)
            .setPriorityTaskManager(priorityTaskManagerForTest)
            .build();
    playerHandler = new Handler(player.getApplicationLooper());
    if (resolvedConfig.targetPreloadDurationUs != C.TIME_UNSET) {
      long targetPreloadDurationUs = resolvedConfig.targetPreloadDurationUs;
      runOnPlayerThread(
          () -> player.setPreloadConfiguration(new ExoPlayer.PreloadConfiguration(targetPreloadDurationUs)));
    }
    player.addListener(this);
    player.addAnalyticsListener(this);
  }

  private ImageOutput createNativeBackedImageOutput() {
    return new ImageOutput() {
      @Override
      public void onImageAvailable(long presentationTimeUs, Bitmap bitmap) {
        long handle = getNativeHandle();
        if (handle != 0L && bitmap != null) {
          Bitmap.Config config = bitmap.getConfig();
          nativeOnImageOutputAvailable(
              handle,
              presentationTimeUs,
              bitmap.getWidth(),
              bitmap.getHeight(),
              bitmap.getByteCount(),
              bitmap.getAllocationByteCount(),
              bitmap.getRowBytes(),
              bitmap.hasAlpha(),
              bitmap.isPremultiplied(),
              bitmap.isMutable(),
              config != null ? config.name() : "");
        }
      }

      @Override
      public void onDisabled() {
        long handle = getNativeHandle();
        if (handle != 0L) {
          nativeOnImageOutputDisabled(handle);
        }
      }
    };
  }

  private long getNativeHandle() {
    return nativeHandle.get();
  }

  private static void debugLog(String message) {
    Log.i(TAG, "CppExoPlayerBridge " + message);
  }

  private static void debugLogWaiting(String message) {
    Log.w(TAG, "CppExoPlayerBridge " + message);
  }

  private static long elapsedMs(long startNs) {
    return TimeUnit.NANOSECONDS.toMillis(System.nanoTime() - startNs);
  }

  private String playerThreadName() {
    Thread thread = player.getApplicationLooper().getThread();
    return thread != null ? thread.getName() : "unknown";
  }

  private static String resolveBridgeCaller() {
    StackTraceElement[] stackTrace = Thread.currentThread().getStackTrace();
    for (StackTraceElement element : stackTrace) {
      if (!CppExoPlayerBridge.class.getName().equals(element.getClassName())) {
        continue;
      }
      String methodName = element.getMethodName();
      if (methodName.equals("resolveBridgeCaller")
          || methodName.equals("runOnPlayerThread")
          || methodName.equals("queryOnPlayerThread")
          || methodName.equals("awaitTask")
          || methodName.equals("debugLog")
          || methodName.equals("debugLogWaiting")
          || methodName.equals("elapsedMs")) {
        continue;
      }
      return methodName;
    }
    return "unknown";
  }

  private void runOnPlayerThread(Runnable action) {
    runOnPlayerThread(resolveBridgeCaller(), action, /* allowAfterRelease= */ false);
  }

  private void runOnPlayerThread(Runnable action, boolean allowAfterRelease) {
    runOnPlayerThread(resolveBridgeCaller(), action, allowAfterRelease);
  }

  private void runOnPlayerThread(String caller, Runnable action, boolean allowAfterRelease) {
    long startNs = System.nanoTime();
    debugLog(
        "runOnPlayerThread start caller="
            + caller
            + " currentThread="
            + Thread.currentThread().getName()
            + " playerThread="
            + playerThreadName()
            + " allowAfterRelease="
            + allowAfterRelease
            + " released="
            + released.get());
    if (!allowAfterRelease && released.get()) {
      debugLog("runOnPlayerThread skip caller=" + caller + " reason=released");
      return;
    }
    if (Looper.myLooper() == player.getApplicationLooper()) {
      debugLog("runOnPlayerThread inline caller=" + caller);
      action.run();
      debugLog(
          "runOnPlayerThread done caller="
              + caller
              + " mode=inline"
              + " totalMs="
              + elapsedMs(startNs));
      return;
    }
    FutureTask<Void> task =
        new FutureTask<>(
            () -> {
              if (!allowAfterRelease && released.get()) {
                debugLog("runOnPlayerThread task skipped caller=" + caller + " reason=released");
                return null;
              }
              debugLog(
                  "runOnPlayerThread task begin caller="
                      + caller
                      + " thread="
                      + Thread.currentThread().getName());
              action.run();
              debugLog(
                  "runOnPlayerThread task end caller="
                      + caller
                      + " thread="
                      + Thread.currentThread().getName());
              return null;
            });
    if (!playerHandler.post(task)) {
      if (released.get()) {
        debugLog("runOnPlayerThread post failed caller=" + caller + " reason=released");
        return;
      }
      throw new IllegalStateException("Failed to post action to player thread");
    }
    debugLog("runOnPlayerThread posted caller=" + caller + " playerThread=" + playerThreadName());
    awaitTask(task, "runOnPlayerThread", caller, playerThreadName());
    debugLog(
        "runOnPlayerThread done caller="
            + caller
            + " mode=posted"
            + " totalMs="
            + elapsedMs(startNs));
  }

  private <T> T queryOnPlayerThread(Callable<T> action) {
    String caller = resolveBridgeCaller();
    long startNs = System.nanoTime();
    debugLog(
        "queryOnPlayerThread start caller="
            + caller
            + " currentThread="
            + Thread.currentThread().getName()
            + " playerThread="
            + playerThreadName()
            + " released="
            + released.get());
    if (released.get()) {
      throw new IllegalStateException("Player bridge already released");
    }
    if (Looper.myLooper() == player.getApplicationLooper()) {
      try {
        T result = action.call();
        debugLog(
            "queryOnPlayerThread done caller="
                + caller
                + " mode=inline"
                + " totalMs="
                + elapsedMs(startNs));
        return result;
      } catch (Exception e) {
        throw new IllegalStateException("Failed to execute player action", e);
      }
    }
    FutureTask<T> task = new FutureTask<>(action);
    if (!playerHandler.post(task)) {
      throw new IllegalStateException("Failed to post query to player thread");
    }
    debugLog("queryOnPlayerThread posted caller=" + caller + " playerThread=" + playerThreadName());
    T result = awaitTask(task, "queryOnPlayerThread", caller, playerThreadName());
    debugLog(
        "queryOnPlayerThread done caller="
            + caller
            + " mode=posted"
            + " totalMs="
            + elapsedMs(startNs));
    return result;
  }

  private static <T> T awaitTask(
      FutureTask<T> task, String operation, String caller, String playerThreadName) {
    long startNs = System.nanoTime();
    try {
      while (true) {
        try {
          return task.get(2, TimeUnit.SECONDS);
        } catch (TimeoutException e) {
          debugLogWaiting(
              operation
                  + " waiting caller="
                  + caller
                  + " waitMs="
                  + elapsedMs(startNs)
                  + " playerThread="
                  + playerThreadName);
        }
      }
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
      throw new IllegalStateException("Interrupted while waiting for player thread", e);
    } catch (ExecutionException e) {
      throw new IllegalStateException("Player thread action failed", e.getCause());
    }
  }

  public void bindPlayerView(PlayerView playerView) {
    runOnPlayerThread(() -> playerView.setPlayer(player));
  }

  public void unbindPlayerView(PlayerView playerView) {
    runOnPlayerThread(
        () -> {
          if (playerView.getPlayer() == player) {
            playerView.setPlayer(null);
          }
        });
  }

  public void setVideoSurface(@Nullable Surface surface) {
    runOnPlayerThread(() -> player.setVideoSurface(surface));
  }

  public void clearVideoSurface() {
    runOnPlayerThread(player::clearVideoSurface);
  }

  public void clearVideoSurface(@Nullable Surface surface) {
    runOnPlayerThread(() -> player.clearVideoSurface(surface));
  }

  public void setVideoSurfaceHolder(@Nullable SurfaceHolder surfaceHolder) {
    runOnPlayerThread(() -> player.setVideoSurfaceHolder(surfaceHolder));
  }

  public void clearVideoSurfaceHolder(@Nullable SurfaceHolder surfaceHolder) {
    runOnPlayerThread(() -> player.clearVideoSurfaceHolder(surfaceHolder));
  }

  public void setVideoSurfaceView(@Nullable SurfaceView surfaceView) {
    runOnPlayerThread(() -> player.setVideoSurfaceView(surfaceView));
  }

  public void clearVideoSurfaceView(@Nullable SurfaceView surfaceView) {
    runOnPlayerThread(() -> player.clearVideoSurfaceView(surfaceView));
  }

  public void setVideoTextureView(@Nullable TextureView textureView) {
    runOnPlayerThread(() -> player.setVideoTextureView(textureView));
  }

  public void clearVideoTextureView(@Nullable TextureView textureView) {
    runOnPlayerThread(() -> player.clearVideoTextureView(textureView));
  }

  public void setVideoEffects(CppVideoEffect[] videoEffects) {
    List<androidx.media3.common.Effect> converted = CppBridgeConverters.toVideoEffects(videoEffects);
    runOnPlayerThread(() -> player.setVideoEffects(converted));
  }

  public void setImageOutputEnabled(boolean enabled) {
    setImageOutputObject(enabled ? ensureImageOutput() : null);
  }

  public void setImageOutputObject(@Nullable ImageOutput imageOutput) {
    imageOutputForTest = imageOutput;
    runOnPlayerThread(() -> player.setImageOutput(imageOutput));
  }

  public void setMediaItem(CppMediaItem mediaItem) {
    runOnPlayerThread(() -> player.setMediaItem(CppBridgeConverters.toMediaItem(mediaItem)));
  }

  public void setMediaItem(CppMediaItem mediaItem, boolean resetPosition) {
    runOnPlayerThread(
        () -> player.setMediaItem(CppBridgeConverters.toMediaItem(mediaItem), resetPosition));
  }

  public void setMediaItem(CppMediaItem mediaItem, long startPositionMs) {
    runOnPlayerThread(
        () -> player.setMediaItem(CppBridgeConverters.toMediaItem(mediaItem), startPositionMs));
  }

  public void setMediaItems(CppMediaItem[] mediaItems, int startIndex, long startPositionMs) {
    List<MediaItem> converted = new ArrayList<>(mediaItems.length);
    for (CppMediaItem mediaItem : mediaItems) {
      converted.add(CppBridgeConverters.toMediaItem(mediaItem));
    }
    runOnPlayerThread(() -> player.setMediaItems(converted, startIndex, startPositionMs));
  }

  public void setMediaItems(CppMediaItem[] mediaItems, boolean resetPosition) {
    List<MediaItem> converted = new ArrayList<>(mediaItems.length);
    for (CppMediaItem mediaItem : mediaItems) {
      converted.add(CppBridgeConverters.toMediaItem(mediaItem));
    }
    runOnPlayerThread(() -> player.setMediaItems(converted, resetPosition));
  }

  public void addMediaItem(CppMediaItem mediaItem) {
    runOnPlayerThread(() -> player.addMediaItem(CppBridgeConverters.toMediaItem(mediaItem)));
  }

  public void addMediaItem(int index, CppMediaItem mediaItem) {
    runOnPlayerThread(
        () -> player.addMediaItem(index, CppBridgeConverters.toMediaItem(mediaItem)));
  }

  public void addMediaItems(CppMediaItem[] mediaItems) {
    List<MediaItem> converted = new ArrayList<>(mediaItems.length);
    for (CppMediaItem mediaItem : mediaItems) {
      converted.add(CppBridgeConverters.toMediaItem(mediaItem));
    }
    runOnPlayerThread(() -> player.addMediaItems(converted));
  }

  public void addMediaItems(int index, CppMediaItem[] mediaItems) {
    List<MediaItem> converted = new ArrayList<>(mediaItems.length);
    for (CppMediaItem mediaItem : mediaItems) {
      converted.add(CppBridgeConverters.toMediaItem(mediaItem));
    }
    runOnPlayerThread(() -> player.addMediaItems(index, converted));
  }

  public void removeMediaItem(int index) {
    runOnPlayerThread(() -> player.removeMediaItem(index));
  }

  public void removeMediaItems(int fromIndex, int toIndex) {
    runOnPlayerThread(() -> player.removeMediaItems(fromIndex, toIndex));
  }

  public void moveMediaItem(int currentIndex, int newIndex) {
    runOnPlayerThread(() -> player.moveMediaItem(currentIndex, newIndex));
  }

  public void moveMediaItems(int fromIndex, int toIndex, int newIndex) {
    runOnPlayerThread(() -> player.moveMediaItems(fromIndex, toIndex, newIndex));
  }

  public void replaceMediaItems(int fromIndex, int toIndex, CppMediaItem[] mediaItems) {
    List<MediaItem> converted = new ArrayList<>(mediaItems.length);
    for (CppMediaItem mediaItem : mediaItems) {
      converted.add(CppBridgeConverters.toMediaItem(mediaItem));
    }
    runOnPlayerThread(() -> player.replaceMediaItems(fromIndex, toIndex, converted));
  }

  public void replaceMediaItem(int index, CppMediaItem mediaItem) {
    runOnPlayerThread(
        () -> player.replaceMediaItem(index, CppBridgeConverters.toMediaItem(mediaItem)));
  }

  public void clearMediaItems() {
    runOnPlayerThread(player::clearMediaItems);
  }

  public void prepare() {
    runOnPlayerThread(player::prepare);
  }

  public void play() {
    runOnPlayerThread(player::play);
  }

  public void pause() {
    runOnPlayerThread(player::pause);
  }

  public void stop() {
    runOnPlayerThread(player::stop);
  }

  public void seekTo(long positionMs) {
    runOnPlayerThread(() -> player.seekTo(positionMs));
  }

  public void seekToMediaItem(int mediaItemIndex, long positionMs) {
    runOnPlayerThread(() -> player.seekTo(mediaItemIndex, positionMs));
  }

  public void seekBack() {
    runOnPlayerThread(player::seekBack);
  }

  public void seekForward() {
    runOnPlayerThread(player::seekForward);
  }

  public void seekToDefaultPosition() {
    runOnPlayerThread(player::seekToDefaultPosition);
  }

  public void seekToDefaultPosition(int mediaItemIndex) {
    runOnPlayerThread(() -> player.seekToDefaultPosition(mediaItemIndex));
  }

  public void setSeekParameters(CppSeekParameters seekParameters) {
    runOnPlayerThread(
        () ->
            player.setSeekParameters(
                new SeekParameters(
                    seekParameters.toleranceBeforeUs, seekParameters.toleranceAfterUs)));
  }

  public CppSeekParameters getSeekParameters() {
    SeekParameters seekParameters = queryOnPlayerThread(player::getSeekParameters);
    return new CppSeekParameters(
        seekParameters.toleranceBeforeUs, seekParameters.toleranceAfterUs);
  }

  public void seekToNext() {
    runOnPlayerThread(player::seekToNext);
  }

  public void seekToPrevious() {
    runOnPlayerThread(player::seekToPrevious);
  }

  public void seekToNextMediaItem() {
    runOnPlayerThread(player::seekToNextMediaItem);
  }

  public void seekToPreviousMediaItem() {
    runOnPlayerThread(player::seekToPreviousMediaItem);
  }

  public void setWakeMode(int wakeMode) {
    runOnPlayerThread(() -> player.setWakeMode(wakeMode));
  }

  public void setPriority(int priority) {
    configuredPriorityForTest = priority;
    runOnPlayerThread(() -> player.setPriority(priority));
  }

  public void setPriorityTaskManagerEnabled(boolean enabled) {
    setPriorityTaskManagerObject(enabled ? ensurePriorityTaskManager() : null);
  }

  public void setPreloadConfiguration(long targetPreloadDurationUs) {
    runOnPlayerThread(
        () ->
            player.setPreloadConfiguration(
                new ExoPlayer.PreloadConfiguration(targetPreloadDurationUs)));
  }

  public String[] sendPlayerMessageForTest(
      int targetType,
      int type,
      @Nullable String payload,
      int mediaItemIndex,
      long positionMs,
      boolean deleteAfterDelivery,
      boolean cancelAfterSend,
      long blockTimeoutMs) {
    // Reflection below intentionally targets internal ExoPlayer fields for test-only routing.
    // Keep this method limited to validation code paths rather than production control flow.
    if (released.get()) {
      return new String[] {
        "0", "0", "0", "0", "0", "", "-1", Long.toString(C.TIME_UNSET), "1", ""
      };
    }
    if (positionMs == C.TIME_UNSET && !deleteAfterDelivery) {
      throw new IllegalArgumentException(
          "Immediate player messages must be delete-after-delivery");
    }
    playerMessageDeliveryCount = 0;
    lastPlayerMessageType = 0;
    lastPlayerMessagePayload = "";
    lastPlayerMessageMediaItemIndex = -1;
    lastPlayerMessagePositionMs = C.TIME_UNSET;
    lastPlayerMessageDeleteAfterDelivery = true;
    lastPlayerMessageThreadName = "";
    final int resolvedMediaItemIndex = mediaItemIndex;
    final long resolvedPositionMs = positionMs;
    final boolean resolvedDeleteAfterDelivery = deleteAfterDelivery;
    AtomicReference<PlayerMessage> messageRef = new AtomicReference<>();
    runOnPlayerThread(
        () -> {
          PlayerMessage.Target target =
              resolvePlayerMessageTarget(
                  targetType,
                  resolvedMediaItemIndex,
                  resolvedPositionMs,
                  resolvedDeleteAfterDelivery);
          if (target == null) {
            return;
          }
          PlayerMessage message =
              player
                  .createMessage(target)
                  .setType(type)
                  .setPayload(payload)
                  .setDeleteAfterDelivery(deleteAfterDelivery);
          if (positionMs != C.TIME_UNSET) {
            if (mediaItemIndex >= 0) {
              message.setPosition(mediaItemIndex, positionMs);
            } else {
              message.setPosition(positionMs);
            }
          }
          message.send();
          if (cancelAfterSend) {
            message.cancel();
          }
          messageRef.set(message);
        });
    PlayerMessage message = messageRef.get();
    if (message == null) {
      return new String[] {
        "0", "0", "0", "0", "0", "", "-1", Long.toString(C.TIME_UNSET), "1", ""
      };
    }
    boolean delivered = false;
    boolean timedOut = false;
    boolean canceled = false;
    try {
      delivered =
          blockTimeoutMs >= 0
              ? message.blockUntilDelivered(blockTimeoutMs)
              : message.blockUntilDelivered();
    } catch (TimeoutException e) {
      timedOut = true;
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
      throw new IllegalStateException("Interrupted while waiting for player message", e);
    }
    canceled = message.isCanceled();
    return new String[] {
      delivered ? "1" : "0",
      timedOut ? "1" : "0",
      canceled ? "1" : "0",
      Integer.toString(playerMessageDeliveryCount),
      Integer.toString(lastPlayerMessageType),
      lastPlayerMessagePayload,
      Integer.toString(lastPlayerMessageMediaItemIndex),
      Long.toString(lastPlayerMessagePositionMs),
      lastPlayerMessageDeleteAfterDelivery ? "1" : "0",
      lastPlayerMessageThreadName
    };
  }

  @Nullable
  private PlayerMessage.Target resolvePlayerMessageTarget(
      int targetType, int mediaItemIndex, long positionMs, boolean deleteAfterDelivery) {
    if (targetType == 0) {
      return createCapturingMessageTarget(mediaItemIndex, positionMs, deleteAfterDelivery);
    }
    Renderer[] renderers = getRenderersForTest();
    if (renderers == null) {
      return null;
    }
    int trackType;
    switch (targetType) {
      case 1:
        trackType = C.TRACK_TYPE_AUDIO;
        break;
      case 2:
        trackType = C.TRACK_TYPE_VIDEO;
        break;
      case 3:
        trackType = C.TRACK_TYPE_TEXT;
        break;
      case 4:
        trackType = C.TRACK_TYPE_IMAGE;
        break;
      default:
        return null;
    }
    for (Renderer renderer : renderers) {
      if (renderer != null && renderer.getTrackType() == trackType) {
        return new CapturingRendererMessageTarget(
            renderer, mediaItemIndex, positionMs, deleteAfterDelivery);
      }
    }
    return null;
  }

  private PlayerMessage.Target createCapturingMessageTarget(
      int mediaItemIndex, long positionMs, boolean deleteAfterDelivery) {
    return (messageType, messagePayload) ->
        captureDeliveredPlayerMessage(
            messageType, messagePayload, mediaItemIndex, positionMs, deleteAfterDelivery);
  }

  private void captureDeliveredPlayerMessage(
      int messageType,
      @Nullable Object messagePayload,
      int mediaItemIndex,
      long positionMs,
      boolean deleteAfterDelivery) {
    playerMessageDeliveryCount++;
    lastPlayerMessageType = messageType;
    lastPlayerMessagePayload =
        messagePayload instanceof String
            ? (String) messagePayload
            : messagePayload != null ? String.valueOf(messagePayload) : "";
    lastPlayerMessageMediaItemIndex = mediaItemIndex;
    lastPlayerMessagePositionMs = positionMs;
    lastPlayerMessageDeleteAfterDelivery = deleteAfterDelivery;
    lastPlayerMessageThreadName = Thread.currentThread().getName();
  }

  private final class CapturingRendererMessageTarget implements PlayerMessage.Target {
    private final Renderer renderer;
    private final int mediaItemIndex;
    private final long positionMs;
    private final boolean deleteAfterDelivery;

    private CapturingRendererMessageTarget(
        Renderer renderer, int mediaItemIndex, long positionMs, boolean deleteAfterDelivery) {
      this.renderer = renderer;
      this.mediaItemIndex = mediaItemIndex;
      this.positionMs = positionMs;
      this.deleteAfterDelivery = deleteAfterDelivery;
    }

    @Override
    public void handleMessage(int messageType, @Nullable Object messagePayload)
        throws ExoPlaybackException {
      captureDeliveredPlayerMessage(
          messageType, messagePayload, mediaItemIndex, positionMs, deleteAfterDelivery);
      renderer.handleMessage(messageType, messagePayload);
    }
  }

  public void setPriorityTaskManagerObject(@Nullable PriorityTaskManager priorityTaskManager) {
    priorityTaskManagerForTest = priorityTaskManager;
    priorityTaskManagerAttachedForTest = priorityTaskManager != null;
    runOnPlayerThread(
        () -> {
          player.setPriorityTaskManager(priorityTaskManager);
          priorityTaskManagerRegisteredForTest = priorityTaskManager != null;
        });
  }

  public void setAudioAttributesConfig(
      int contentType,
      int usage,
      int flags,
      int allowedCapturePolicy,
      int spatializationBehavior,
      boolean handleAudioFocus) {
    AudioAttributes audioAttributes =
        new AudioAttributes.Builder()
            .setContentType(contentType)
            .setUsage(usage)
            .setFlags(flags)
            .setAllowedCapturePolicy(allowedCapturePolicy)
            .setSpatializationBehavior(spatializationBehavior)
            .build();
    runOnPlayerThread(() -> player.setAudioAttributes(audioAttributes, handleAudioFocus));
  }

  public int[] getAudioAttributesConfig() {
    AudioAttributes attributes = queryOnPlayerThread(player::getAudioAttributes);
    return new int[] {
      attributes.contentType,
      attributes.usage,
      attributes.flags,
      attributes.allowedCapturePolicy,
      attributes.spatializationBehavior
    };
  }

  public void setDeviceVolumeWithFlags(int volume, int flags) {
    runOnPlayerThread(() -> player.setDeviceVolume(volume, flags));
  }

  public void adjustDeviceVolumeWithFlags(int direction, int flags) {
    runOnPlayerThread(
        () -> {
          if (direction == AudioManager.ADJUST_RAISE) {
            player.increaseDeviceVolume(flags);
          } else if (direction == AudioManager.ADJUST_LOWER) {
            player.decreaseDeviceVolume(flags);
          } else if (direction == AudioManager.ADJUST_MUTE) {
            player.setDeviceMuted(true, flags);
          } else if (direction == AudioManager.ADJUST_UNMUTE) {
            player.setDeviceMuted(false, flags);
          } else if (direction == AudioManager.ADJUST_TOGGLE_MUTE) {
            player.setDeviceMuted(!player.isDeviceMuted(), flags);
          }
        });
  }

  public void increaseDeviceVolumeWithFlags(int flags) {
    runOnPlayerThread(() -> player.increaseDeviceVolume(flags));
  }

  public void decreaseDeviceVolumeWithFlags(int flags) {
    runOnPlayerThread(() -> player.decreaseDeviceVolume(flags));
  }

  public void setDeviceMutedWithFlags(boolean muted, int flags) {
    runOnPlayerThread(() -> player.setDeviceMuted(muted, flags));
  }

  public void setSkipSilenceEnabled(boolean skipSilenceEnabled) {
    runOnPlayerThread(() -> player.setSkipSilenceEnabled(skipSilenceEnabled));
  }

  public boolean getSkipSilenceEnabled() {
    return queryOnPlayerThread(player::getSkipSilenceEnabled);
  }

  public CppDeviceInfo getDeviceInfo() {
    DeviceInfo deviceInfo = queryOnPlayerThread(player::getDeviceInfo);
    return new CppDeviceInfo(
        deviceInfo.playbackType,
        deviceInfo.minVolume,
        deviceInfo.maxVolume,
        deviceInfo.routingControllerId);
  }

  public int getDeviceVolumeValue() {
    return queryOnPlayerThread(player::getDeviceVolume);
  }

  public boolean isDeviceMutedValue() {
    return queryOnPlayerThread(player::isDeviceMuted);
  }

  public CppVideoSize getVideoSize() {
    VideoSize videoSize = queryOnPlayerThread(player::getVideoSize);
    return new CppVideoSize(
        videoSize.width,
        videoSize.height,
        videoSize.unappliedRotationDegrees,
        videoSize.pixelWidthHeightRatio);
  }

  public CppMediaMetadata getMediaMetadata() {
    return CppBridgeConverters.fromMediaMetadata(queryOnPlayerThread(player::getMediaMetadata));
  }

  public CppMediaMetadata getPlaylistMetadata() {
    return CppBridgeConverters.fromMediaMetadata(queryOnPlayerThread(player::getPlaylistMetadata));
  }

  public void setPlaylistMetadata(CppMediaMetadata metadata) {
    runOnPlayerThread(() -> player.setPlaylistMetadata(CppBridgeConverters.toMediaMetadata(metadata)));
  }

  public String[] getAnalyticsStrings() {
    return new String[] {
      Long.toString(latestBitrateEstimate),
      Integer.toString(droppedVideoFrames),
      Integer.toString(loadStartedCount),
      Integer.toString(loadCompletedCount),
      latestAudioSampleMimeType,
      latestVideoSampleMimeType
    };
  }

  public void simulateAnalyticsUpdateForTest(
      long bitrateEstimate,
      int droppedFrames,
      int loadStarted,
      int loadCompleted,
      String audioSampleMimeType,
      String videoSampleMimeType) {
    runOnPlayerThread(
        () -> {
          latestBitrateEstimate = bitrateEstimate;
          droppedVideoFrames = droppedFrames;
          loadStartedCount = loadStarted;
          loadCompletedCount = loadCompleted;
          latestAudioSampleMimeType = audioSampleMimeType != null ? audioSampleMimeType : "";
          latestVideoSampleMimeType = videoSampleMimeType != null ? videoSampleMimeType : "";
          long handle = getNativeHandle();
          if (handle != 0L) {
            nativeOnAnalyticsUpdated(handle);
          }
        });
  }

  public void simulateAudioUnderrunForTest(
      int bufferSize, long bufferSizeMs, long elapsedSinceLastFeedMs) {
    runOnPlayerThread(
        () -> dispatchAudioUnderrun(bufferSize, bufferSizeMs, elapsedSinceLastFeedMs));
  }

  public void simulateDroppedVideoFramesForTest(int droppedFrames, long elapsedMs) {
    runOnPlayerThread(() -> dispatchDroppedVideoFrames(droppedFrames, elapsedMs));
  }

  public void simulateBandwidthEstimateForTest(
      int elapsedMs, long bytesTransferred, long bitrateEstimate) {
    runOnPlayerThread(
        () -> dispatchBandwidthEstimate(elapsedMs, bytesTransferred, bitrateEstimate));
  }

  public void simulateLoadStartedForTest(
      String uri, int dataType, int trackType, int retryCount) {
    runOnPlayerThread(() -> dispatchLoadStarted(uri, dataType, trackType, retryCount));
  }

  public void simulateLoadCompletedForTest(String uri, int dataType, int trackType) {
    runOnPlayerThread(() -> dispatchLoadCompleted(uri, dataType, trackType));
  }

  public void simulateAudioInputFormatChangedForTest(
      String sampleMimeType, String codecs, int channelCount, int sampleRate) {
    runOnPlayerThread(
        () ->
            dispatchAudioInputFormatChanged(
                sampleMimeType, codecs, channelCount, sampleRate));
  }

  public void simulateAudioDecoderInitializedForTest(
      String decoderName, long initializedTimestampMs, long initializationDurationMs) {
    runOnPlayerThread(
        () ->
            dispatchAudioDecoderInitialized(
                decoderName, initializedTimestampMs, initializationDurationMs));
  }

  public void simulateVideoDecoderInitializedForTest(
      String decoderName, long initializedTimestampMs, long initializationDurationMs) {
    runOnPlayerThread(
        () ->
            dispatchVideoDecoderInitialized(
                decoderName, initializedTimestampMs, initializationDurationMs));
  }

  public void simulateAudioDecoderReleasedForTest(String decoderName) {
    runOnPlayerThread(() -> dispatchAudioDecoderReleased(decoderName));
  }

  public void simulateVideoDecoderReleasedForTest(String decoderName) {
    runOnPlayerThread(() -> dispatchVideoDecoderReleased(decoderName));
  }

  public void simulateAnalyticsRenderedFirstFrameForTest(long renderTimeMs) {
    runOnPlayerThread(() -> dispatchAnalyticsRenderedFirstFrame(renderTimeMs));
  }

  public void simulateAnalyticsVideoSizeChangedForTest(
      int width, int height, float pixelWidthHeightRatio) {
    runOnPlayerThread(
        () -> dispatchAnalyticsVideoSizeChanged(width, height, pixelWidthHeightRatio));
  }

  public void simulateAudioPositionAdvancingForTest(long playoutStartSystemTimeMs) {
    runOnPlayerThread(
        () -> dispatchAudioPositionAdvancing(playoutStartSystemTimeMs));
  }

  public void simulateVideoFrameProcessingOffsetForTest(
      long totalProcessingOffsetUs, int frameCount) {
    runOnPlayerThread(
        () -> dispatchVideoFrameProcessingOffset(totalProcessingOffsetUs, frameCount));
  }

  public void simulateVolumeChangedForTest(float volume) {
    runOnPlayerThread(() -> dispatchVolumeChanged(volume));
  }

  public void simulateAudioSessionIdChangedForTest(int audioSessionId) {
    runOnPlayerThread(() -> dispatchAudioSessionIdChanged(audioSessionId));
  }

  public void simulateAnalyticsSkipSilenceEnabledChangedForTest(boolean skipSilenceEnabled) {
    runOnPlayerThread(() -> dispatchAnalyticsSkipSilenceEnabledChanged(skipSilenceEnabled));
  }

  public void simulateAnalyticsDeviceVolumeChangedForTest(int volume, boolean muted) {
    runOnPlayerThread(() -> dispatchAnalyticsDeviceVolumeChanged(volume, muted));
  }

  public void simulateAnalyticsPlaybackStateChangedForTest(int playbackState) {
    runOnPlayerThread(() -> dispatchAnalyticsPlaybackStateChanged(playbackState));
  }

  public void simulateAnalyticsIsPlayingChangedForTest(boolean isPlaying) {
    runOnPlayerThread(() -> dispatchAnalyticsIsPlayingChanged(isPlaying));
  }

  public void simulateAnalyticsPlayWhenReadyChangedForTest(boolean playWhenReady, int reason) {
    runOnPlayerThread(() -> dispatchAnalyticsPlayWhenReadyChanged(playWhenReady, reason));
  }

  public void simulateAnalyticsPlaybackSuppressionReasonChangedForTest(
      int playbackSuppressionReason) {
    runOnPlayerThread(
        () -> dispatchAnalyticsPlaybackSuppressionReasonChanged(playbackSuppressionReason));
  }

  public void simulateAnalyticsIsLoadingChangedForTest(boolean isLoading) {
    runOnPlayerThread(() -> dispatchAnalyticsIsLoadingChanged(isLoading));
  }

  public void simulateAnalyticsRepeatModeChangedForTest(int repeatMode) {
    runOnPlayerThread(() -> dispatchAnalyticsRepeatModeChanged(repeatMode));
  }

  public void simulateAnalyticsShuffleModeChangedForTest(boolean shuffleModeEnabled) {
    runOnPlayerThread(() -> dispatchAnalyticsShuffleModeChanged(shuffleModeEnabled));
  }

  public void simulateAnalyticsPlaybackParametersChangedForTest(float speed, float pitch) {
    runOnPlayerThread(() -> dispatchAnalyticsPlaybackParametersChanged(speed, pitch));
  }

  public void simulateAnalyticsAvailableCommandsChangedForTest(int[] commandCodes) {
    runOnPlayerThread(
        () -> dispatchAnalyticsAvailableCommandsChanged(new CppCommands(commandCodes)));
  }

  public void simulateAnalyticsEventsForTest(int[] eventCodes) {
    runOnPlayerThread(() -> dispatchAnalyticsEvents(new CppPlayerEvents(eventCodes)));
  }

  public void simulateSeekBackIncrementChangedForTest(long seekBackIncrementMs) {
    runOnPlayerThread(() -> dispatchSeekBackIncrementChanged(seekBackIncrementMs));
  }

  public void simulateSeekForwardIncrementChangedForTest(long seekForwardIncrementMs) {
    runOnPlayerThread(() -> dispatchSeekForwardIncrementChanged(seekForwardIncrementMs));
  }

  public void simulateMaxSeekToPreviousPositionChangedForTest(long maxSeekToPreviousPositionMs) {
    runOnPlayerThread(
        () -> dispatchMaxSeekToPreviousPositionChanged(maxSeekToPreviousPositionMs));
  }

  public void simulateAnalyticsSeekBackIncrementChangedForTest(long seekBackIncrementMs) {
    runOnPlayerThread(
        () -> dispatchAnalyticsSeekBackIncrementChanged(seekBackIncrementMs));
  }

  public void simulateAnalyticsSeekForwardIncrementChangedForTest(long seekForwardIncrementMs) {
    runOnPlayerThread(
        () -> dispatchAnalyticsSeekForwardIncrementChanged(seekForwardIncrementMs));
  }

  public void simulateAnalyticsMaxSeekToPreviousPositionChangedForTest(
      long maxSeekToPreviousPositionMs) {
    runOnPlayerThread(
        () ->
            dispatchAnalyticsMaxSeekToPreviousPositionChanged(
                maxSeekToPreviousPositionMs));
  }

  public void simulateAnalyticsTimelineChangedForTest(int reason) {
    runOnPlayerThread(() -> dispatchAnalyticsTimelineChanged(reason));
  }

  public void simulateAnalyticsPositionDiscontinuityForTest(int reason) {
    runOnPlayerThread(() -> dispatchAnalyticsPositionDiscontinuity(reason));
  }

  public void simulateAnalyticsSeekStartedForTest() {
    runOnPlayerThread(this::dispatchAnalyticsSeekStarted);
  }

  public void simulateAnalyticsPlayerErrorForTest(int errorCode, @Nullable String message) {
    runOnPlayerThread(() -> dispatchAnalyticsPlayerError(errorCode, message));
  }

  public void simulateAnalyticsPlayerErrorChangedForTest(
      int errorCode, @Nullable String message) {
    runOnPlayerThread(() -> dispatchAnalyticsPlayerErrorChanged(errorCode, message));
  }

  public void simulateAnalyticsTracksChangedForTest(CppTracks tracks) {
    runOnPlayerThread(() -> dispatchAnalyticsTracksChanged(tracks));
  }

  public void simulateAnalyticsMediaItemTransitionForTest(
      @Nullable CppMediaItem mediaItem, int reason) {
    runOnPlayerThread(() -> dispatchAnalyticsMediaItemTransition(mediaItem, reason));
  }

  public void simulateAnalyticsCuesForTest(CppCue[] cues, long presentationTimeUs) {
    runOnPlayerThread(() -> dispatchAnalyticsCues(cues, presentationTimeUs));
  }

  public void simulateCurrentCuesForTest(CppCue[] cues, long presentationTimeUs) {
    CppCue[] safeCues = cues != null ? cues : new CppCue[0];
    runOnPlayerThread(
        () -> {
          currentCuesForTest = Arrays.copyOf(safeCues, safeCues.length);
          currentCuesPresentationTimeUsForTest = presentationTimeUs;
          long handle = getNativeHandle();
          if (handle != 0L) {
            nativeOnCues(handle, safeCues.length, presentationTimeUs);
          }
        });
  }

  public void simulateAnalyticsMetadataForTest(
      int entryCount, @Nullable String firstEntryType, @Nullable String firstEntryText) {
    runOnPlayerThread(
        () -> dispatchAnalyticsMetadata(entryCount, firstEntryType, firstEntryText));
  }

  public void simulateAnalyticsLoadErrorForTest(
      @Nullable String uri,
      int dataType,
      int trackType,
      @Nullable String message,
      boolean wasCanceled) {
    runOnPlayerThread(
        () ->
            dispatchAnalyticsLoadError(
                uri != null ? uri : "",
                dataType,
                trackType,
                message != null ? message : "",
                wasCanceled));
  }

  public void simulateAnalyticsDeviceInfoChangedForTest(
      int playbackType, int minVolume, int maxVolume, @Nullable String routingControllerId) {
    runOnPlayerThread(
        () ->
            dispatchAnalyticsDeviceInfoChanged(
                new CppDeviceInfo(playbackType, minVolume, maxVolume, routingControllerId)));
  }

  public void simulateAnalyticsMediaMetadataChangedForTest(CppMediaMetadata metadata) {
    runOnPlayerThread(() -> dispatchAnalyticsMediaMetadataChanged(metadata));
  }

  public void simulateAnalyticsPlaylistMetadataChangedForTest(CppMediaMetadata metadata) {
    runOnPlayerThread(() -> dispatchAnalyticsPlaylistMetadataChanged(metadata));
  }

  public void simulateVideoInputFormatChangedForTest(
      String sampleMimeType, String codecs, int width, int height, float frameRate) {
    runOnPlayerThread(
        () ->
            dispatchVideoInputFormatChanged(
                sampleMimeType, codecs, width, height, frameRate));
  }

  public void simulateImageOutputForTest(long presentationTimeUs, int width, int height) {
    ImageOutput imageOutput = imageOutputForTest;
    if (imageOutput == null) {
      return;
    }
    int safeWidth = Math.max(1, width);
    int safeHeight = Math.max(1, height);
    runOnPlayerThread(
        () -> imageOutput.onImageAvailable(
            presentationTimeUs,
            Bitmap.createBitmap(safeWidth, safeHeight, Bitmap.Config.ARGB_8888)));
  }

  public String[] getMediaSourceFactoryConfigStrings() {
    return new String[] {
      mediaSourceFactoryConfig.parseSubtitlesDuringExtraction ? "1" : "0",
      mediaSourceFactoryConfig.loadOnlySelectedTracks ? "1" : "0",
      mediaSourceFactoryConfig.userAgent != null ? mediaSourceFactoryConfig.userAgent : "",
      Integer.toString(mediaSourceFactoryConfig.connectTimeoutMs),
      Integer.toString(mediaSourceFactoryConfig.readTimeoutMs),
      mediaSourceFactoryConfig.allowCrossProtocolRedirects ? "1" : "0",
      Long.toString(mediaSourceFactoryConfig.liveTargetOffsetMs),
      Long.toString(mediaSourceFactoryConfig.liveMinOffsetMs),
      Long.toString(mediaSourceFactoryConfig.liveMaxOffsetMs),
      Float.toString(mediaSourceFactoryConfig.liveMinSpeed),
      Float.toString(mediaSourceFactoryConfig.liveMaxSpeed)
    };
  }

  public String[] getPlayerConfigFlagsForTest() {
    return queryOnPlayerThread(
        () ->
            new String[] {
              configuredHandleAudioFocusForTest ? "1" : "0",
              configuredHandleAudioBecomingNoisyForTest ? "1" : "0",
              configuredUseLazyPreparationForTest ? "1" : "0",
              Long.toString(configuredSeekBackIncrementMsForTest),
              Long.toString(configuredSeekForwardIncrementMsForTest),
              Integer.toString(configuredWakeModeForTest),
              Integer.toString(configuredPriorityForTest),
              priorityTaskManagerForTest != null ? "1" : "0",
              Long.toString(player.getPreloadConfiguration().targetPreloadDurationUs)
            });
  }

  public long getTargetPreloadDurationUs() {
    return queryOnPlayerThread(() -> player.getPreloadConfiguration().targetPreloadDurationUs);
  }

  public String[] getPriorityTaskManagerStateForTest() {
    return queryOnPlayerThread(
        () -> {
          return new String[] {
            priorityTaskManagerForTest != null ? "1" : "0",
            priorityTaskManagerAttachedForTest ? "1" : "0",
            priorityTaskManagerRegisteredForTest ? "1" : "0",
            Integer.toString(configuredPriorityForTest)
          };
        });
  }

  public String summarizeVideoEffectsForTest(CppVideoEffect[] videoEffects) {
    List<androidx.media3.common.Effect> converted = CppBridgeConverters.toVideoEffects(videoEffects);
    StringBuilder summary = new StringBuilder();
    summary.append("effectCount=").append(converted.size());
    for (int i = 0; i < converted.size(); i++) {
      CppVideoEffect sourceEffect = videoEffects[i];
      androidx.media3.common.Effect effect = converted.get(i);
      summary.append(",effect").append(i).append("=");
      if (effect instanceof ScaleAndRotateTransformation) {
        ScaleAndRotateTransformation transformation = (ScaleAndRotateTransformation) effect;
        summary
            .append("scaleAndRotate")
            .append(":scaleX=")
            .append(transformation.scaleX)
            .append(":scaleY=")
            .append(transformation.scaleY)
            .append(":rotationDegrees=")
            .append(transformation.rotationDegrees);
      } else if (effect instanceof RgbAdjustment) {
        summary
            .append("rgbAdjustment")
            .append(":redScale=")
            .append(sourceEffect.redScale)
            .append(":greenScale=")
            .append(sourceEffect.greenScale)
            .append(":blueScale=")
            .append(sourceEffect.blueScale);
      } else if (effect instanceof Presentation) {
        summary
            .append("presentation")
            .append(":width=")
            .append(sourceEffect.presentationWidth)
            .append(":height=")
            .append(sourceEffect.presentationHeight)
            .append(":layout=")
            .append(sourceEffect.presentationLayout);
      } else {
        summary.append(effect.getClass().getSimpleName());
      }
    }
    return summary.toString();
  }

  private PriorityTaskManager ensurePriorityTaskManager() {
    if (priorityTaskManagerForTest == null) {
      priorityTaskManagerForTest = new PriorityTaskManager();
    }
    return priorityTaskManagerForTest;
  }

  private ImageOutput ensureImageOutput() {
    if (imageOutputForTest == null) {
      imageOutputForTest = createNativeBackedImageOutput();
    }
    return imageOutputForTest;
  }

  @Nullable
  private Renderer[] getRenderersForTest() {
    // ExoPlayer does not expose public renderer access. Keep the reflective lookup contained to
    // this single test-only helper so renderer-targeted player-message smoke can stay isolated.
    Object renderersValue = getDeclaredFieldValueForTest(player, "renderers");
    return renderersValue instanceof Renderer[] ? (Renderer[]) renderersValue : null;
  }

  @Nullable
  private static Object getDeclaredFieldValueForTest(Object target, String fieldName) {
    // Test-only reflection helper for smoke/demo inspection paths. Avoid using this from
    // production bridge behavior because the targeted fields are private ExoPlayer internals.
    Class<?> currentClass = target.getClass();
    while (currentClass != null) {
      try {
        java.lang.reflect.Field field = currentClass.getDeclaredField(fieldName);
        field.setAccessible(true);
        return field.get(target);
      } catch (NoSuchFieldException e) {
        currentClass = currentClass.getSuperclass();
      } catch (IllegalAccessException e) {
        return null;
      }
    }
    return null;
  }

  public String[] getMediaSourceFactoryHeaderNames() {
    return mediaSourceFactoryConfig.defaultRequestHeaderNames;
  }

  public String[] getMediaSourceFactoryHeaderValues() {
    return mediaSourceFactoryConfig.defaultRequestHeaderValues;
  }

  public void setPlayWhenReady(boolean playWhenReady) {
    runOnPlayerThread(() -> player.setPlayWhenReady(playWhenReady));
  }

  public void setRepeatMode(int repeatMode) {
    runOnPlayerThread(() -> player.setRepeatMode(repeatMode));
  }

  public void setShuffleModeEnabled(boolean shuffleModeEnabled) {
    runOnPlayerThread(() -> player.setShuffleModeEnabled(shuffleModeEnabled));
  }

  public boolean getShuffleModeEnabled() {
    return queryOnPlayerThread(player::getShuffleModeEnabled);
  }

  public int getRepeatMode() {
    return queryOnPlayerThread(player::getRepeatMode);
  }

  public void setVolume(float volume) {
    runOnPlayerThread(() -> player.setVolume(volume));
  }

  public float getVolume() {
    return queryOnPlayerThread(player::getVolume);
  }

  public void setPlaybackSpeed(float speed) {
    runOnPlayerThread(() -> player.setPlaybackParameters(new PlaybackParameters(speed)));
  }

  public void setPlaybackParametersConfig(float speed, float pitch) {
    runOnPlayerThread(() -> player.setPlaybackParameters(new PlaybackParameters(speed, pitch)));
  }

  public float getPlaybackSpeed() {
    return queryOnPlayerThread(() -> player.getPlaybackParameters().speed);
  }

  public CppPlaybackParameters getPlaybackParameters() {
    PlaybackParameters parameters = queryOnPlayerThread(player::getPlaybackParameters);
    return new CppPlaybackParameters(parameters.speed, parameters.pitch);
  }

  public void setPauseAtEndOfMediaItems(boolean pauseAtEndOfMediaItems) {
    runOnPlayerThread(() -> player.setPauseAtEndOfMediaItems(pauseAtEndOfMediaItems));
  }

  public boolean getIsLoading() {
    return queryOnPlayerThread(player::isLoading);
  }

  public int getMediaItemCount() {
    return queryOnPlayerThread(player::getMediaItemCount);
  }

  public int getNextMediaItemIndex() {
    return queryOnPlayerThread(player::getNextMediaItemIndex);
  }

  public int getPreviousMediaItemIndex() {
    return queryOnPlayerThread(player::getPreviousMediaItemIndex);
  }

  public boolean hasNextMediaItem() {
    return queryOnPlayerThread(player::hasNextMediaItem);
  }

  public boolean hasPreviousMediaItem() {
    return queryOnPlayerThread(player::hasPreviousMediaItem);
  }

  public int getBufferedPercentage() {
    return queryOnPlayerThread(player::getBufferedPercentage);
  }

  public long getContentBufferedPosition() {
    return queryOnPlayerThread(player::getContentBufferedPosition);
  }

  public long getContentDuration() {
    return queryOnPlayerThread(player::getContentDuration);
  }

  public long getContentPosition() {
    return queryOnPlayerThread(player::getContentPosition);
  }

  public long getCurrentLiveOffset() {
    return queryOnPlayerThread(player::getCurrentLiveOffset);
  }

  public int getCurrentPeriodIndex() {
    return queryOnPlayerThread(player::getCurrentPeriodIndex);
  }

  public long getMaxSeekToPreviousPosition() {
    return queryOnPlayerThread(player::getMaxSeekToPreviousPosition);
  }

  public int getPlaybackSuppressionReason() {
    return queryOnPlayerThread(player::getPlaybackSuppressionReason);
  }

  public long getSeekBackIncrement() {
    return queryOnPlayerThread(player::getSeekBackIncrement);
  }

  public long getSeekForwardIncrement() {
    return queryOnPlayerThread(player::getSeekForwardIncrement);
  }

  public long getTotalBufferedDuration() {
    return queryOnPlayerThread(player::getTotalBufferedDuration);
  }

  public boolean isCommandAvailable(int commandCode) {
    return queryOnPlayerThread(() -> player.isCommandAvailable(commandCode));
  }

  public boolean canAdvertiseSession() {
    return queryOnPlayerThread(player::canAdvertiseSession);
  }

  public CppApplicationLooper getApplicationLooper() {
    Looper looper = queryOnPlayerThread(player::getApplicationLooper);
    if (looper == null || looper.getThread() == null) {
      return new CppApplicationLooper(null, -1L, false);
    }
    return new CppApplicationLooper(
        looper.getThread().getName(),
        looper.getThread().getId(),
        Looper.myLooper() == looper);
  }

  public int getCurrentAdGroupIndex() {
    return queryOnPlayerThread(player::getCurrentAdGroupIndex);
  }

  public int getCurrentAdIndexInAdGroup() {
    return queryOnPlayerThread(player::getCurrentAdIndexInAdGroup);
  }

  public boolean isCurrentMediaItemDynamicValue() {
    return queryOnPlayerThread(player::isCurrentMediaItemDynamic);
  }

  public boolean isCurrentMediaItemLiveValue() {
    return queryOnPlayerThread(player::isCurrentMediaItemLive);
  }

  public boolean isCurrentMediaItemSeekableValue() {
    return queryOnPlayerThread(player::isCurrentMediaItemSeekable);
  }

  public boolean isPlayingAdValue() {
    return queryOnPlayerThread(player::isPlayingAd);
  }

  public void setTrackSelectionParameters(CppTrackSelectionParameters parameters) {
    runOnPlayerThread(
        () ->
            player.setTrackSelectionParameters(
                CppBridgeConverters.toTrackSelectionParameters(
                    player.getTrackSelectionParameters(), player.getCurrentTracks(), parameters)));
  }

  public CppTrackSelectionParameters getTrackSelectionParameters() {
    return queryOnPlayerThread(
        () -> CppBridgeConverters.fromTrackSelectionParameters(player.getTrackSelectionParameters()));
  }

  public CppTracks getTracks() {
    return queryOnPlayerThread(() -> CppBridgeConverters.toCppTracks(player.getCurrentTracks()));
  }

  public CppTrackGroup[] getTrackGroups() {
    return getTracks().groups;
  }

  public CppCommands getAvailableCommands() {
    Player.Commands commands = queryOnPlayerThread(player::getAvailableCommands);
    int[] result = new int[commands.size()];
    for (int i = 0; i < commands.size(); i++) {
      result[i] = commands.get(i);
    }
    return new CppCommands(result);
  }

  public int getAvailableCommandCount() {
    return queryOnPlayerThread(() -> player.getAvailableCommands().size());
  }

  public int[] getTimelineSnapshotData() {
    return queryOnPlayerThread(
        () -> {
          Timeline timeline = player.getCurrentTimeline();
          return new int[] {
            timeline.getWindowCount(),
            timeline.getPeriodCount(),
            timeline.isEmpty() ? 1 : 0,
            player.getCurrentMediaItemIndex(),
            player.getNextMediaItemIndex(),
            player.getPreviousMediaItemIndex(),
            player.hasNextMediaItem() ? 1 : 0,
            player.hasPreviousMediaItem() ? 1 : 0,
            player.isCurrentMediaItemDynamic() ? 1 : 0,
            player.isCurrentMediaItemLive() ? 1 : 0,
            player.isCurrentMediaItemSeekable() ? 1 : 0
          };
        });
  }

  public String[] getTimelineWindowRows() {
    return queryOnPlayerThread(
        () -> {
          Timeline timeline = player.getCurrentTimeline();
          Timeline.Window window = new Timeline.Window();
          String[] rows = new String[timeline.getWindowCount()];
          for (int i = 0; i < rows.length; i++) {
            timeline.getWindow(i, window);
            rows[i] =
                i
                    + "|"
                    + (window.mediaItem != null ? window.mediaItem.mediaId : "")
                    + "|"
                    + (window.mediaItem != null
                            && window.mediaItem.localConfiguration != null
                            && window.mediaItem.localConfiguration.uri != null
                        ? window.mediaItem.localConfiguration.uri.toString()
                        : "")
                    + "|"
                    + (window.mediaItem != null
                            && window.mediaItem.localConfiguration != null
                            && window.mediaItem.localConfiguration.tag != null
                        ? 1
                        : 0)
                    + "|"
                    + (window.mediaItem != null
                            && window.mediaItem.localConfiguration != null
                            && window.mediaItem.localConfiguration.tag != null
                        ? window.mediaItem.localConfiguration.tag.toString()
                        : "")
                    + "|"
                    + (window.mediaItem != null
                            && window.mediaItem.localConfiguration != null
                            && window.mediaItem.localConfiguration.tag != null
                        ? CppOpaqueObjectRegistry.register(window.mediaItem.localConfiguration.tag)
                        : "")
                    + "|"
                    + String.valueOf(window.uid)
                    + "|"
                    + (window.uid != null ? CppOpaqueObjectRegistry.register(window.uid) : "")
                    + "|"
                    + (window.liveConfiguration != null ? 1 : 0)
                    + "|"
                    + (window.liveConfiguration != null
                        ? window.liveConfiguration.targetOffsetMs
                        : C.TIME_UNSET)
                    + "|"
                    + (window.liveConfiguration != null
                        ? window.liveConfiguration.minOffsetMs
                        : C.TIME_UNSET)
                    + "|"
                    + (window.liveConfiguration != null
                        ? window.liveConfiguration.maxOffsetMs
                        : C.TIME_UNSET)
                    + "|"
                    + (window.liveConfiguration != null
                        ? window.liveConfiguration.minPlaybackSpeed
                        : C.RATE_UNSET)
                    + "|"
                    + (window.liveConfiguration != null
                        ? window.liveConfiguration.maxPlaybackSpeed
                        : C.RATE_UNSET)
                    + "|"
                    + (window.manifest != null ? 1 : 0)
                    + "|"
                    + (window.manifest != null ? String.valueOf(window.manifest) : "")
                    + "|"
                    + (window.manifest != null ? CppOpaqueObjectRegistry.register(window.manifest) : "")
                    + "|"
                    + window.firstPeriodIndex
                    + "|"
                    + window.lastPeriodIndex
                    + "|"
                    + window.presentationStartTimeMs
                    + "|"
                    + window.windowStartTimeMs
                    + "|"
                    + window.elapsedRealtimeEpochOffsetMs
                    + "|"
                    + window.getDurationMs()
                    + "|"
                    + window.getDurationUs()
                    + "|"
                    + window.getDefaultPositionMs()
                    + "|"
                    + window.getDefaultPositionUs()
                    + "|"
                    + window.getPositionInFirstPeriodMs()
                    + "|"
                    + window.getPositionInFirstPeriodUs()
                    + "|"
                    + (window.isSeekable ? 1 : 0)
                    + "|"
                    + (window.isDynamic ? 1 : 0)
                    + "|"
                    + (window.isLive() ? 1 : 0)
                    + "|"
                    + (window.isPlaceholder ? 1 : 0);
          }
          return rows;
        });
  }

  public String[] getTimelinePeriodRows() {
    return queryOnPlayerThread(
        () -> {
          Timeline timeline = player.getCurrentTimeline();
          Timeline.Period period = new Timeline.Period();
          String[] rows = new String[timeline.getPeriodCount()];
          for (int i = 0; i < rows.length; i++) {
            timeline.getPeriod(i, period);
            rows[i] =
                String.valueOf(period.id)
                    + "|"
                    + (period.id != null ? CppOpaqueObjectRegistry.register(period.id) : "")
                    + "|"
                    + String.valueOf(period.uid)
                    + "|"
                    + (period.uid != null ? CppOpaqueObjectRegistry.register(period.uid) : "")
                    + "|"
                    + String.valueOf(period.getAdsId())
                    + "|"
                    + (period.getAdsId() != null
                        ? CppOpaqueObjectRegistry.register(period.getAdsId())
                        : "")
                    + "|"
                    + period.windowIndex
                    + "|"
                    + period.getAdGroupCount()
                    + "|"
                    + period.getDurationMs()
                    + "|"
                    + period.getDurationUs()
                    + "|"
                    + period.getPositionInWindowMs()
                    + "|"
                    + period.getPositionInWindowUs()
                    + "|"
                    + (period.isPlaceholder ? 1 : 0);
          }
          return rows;
        });
  }

  public CppCue[] getCurrentCues() {
    return queryOnPlayerThread(
        () -> {
          @Nullable CppCue[] overrideCues = currentCuesForTest;
          if (overrideCues != null) {
            return Arrays.copyOf(overrideCues, overrideCues.length);
          }
          CueGroup cueGroup = player.getCurrentCues();
          CppCue[] result = new CppCue[cueGroup.cues.size()];
          for (int i = 0; i < cueGroup.cues.size(); i++) {
            Cue cue = cueGroup.cues.get(i);
            result[i] = CppBridgeConverters.fromCue(cue);
          }
          return result;
        });
  }

  public long getCurrentCuesPresentationTimeUs() {
    return queryOnPlayerThread(
        () -> {
          if (currentCuesForTest != null) {
            return currentCuesPresentationTimeUsForTest;
          }
          return player.getCurrentCues().presentationTimeUs;
        });
  }

  private static int[] getEventCodes(Player.Events events) {
    int[] result = new int[events.size()];
    for (int i = 0; i < events.size(); i++) {
      result[i] = events.get(i);
    }
    return result;
  }

  private static int[] getEventCodes(AnalyticsListener.Events events) {
    int[] result = new int[events.size()];
    for (int i = 0; i < events.size(); i++) {
      result[i] = events.get(i);
    }
    return result;
  }

  private static int[] getCommandCodes(Player.Commands commands) {
    int[] result = new int[commands.size()];
    for (int i = 0; i < commands.size(); i++) {
      result[i] = commands.get(i);
    }
    return result;
  }

  public long getCurrentPosition() {
    return queryOnPlayerThread(player::getCurrentPosition);
  }

  public long getDuration() {
    return queryOnPlayerThread(player::getDuration);
  }

  public long getBufferedPosition() {
    return queryOnPlayerThread(player::getBufferedPosition);
  }

  public int getPlaybackState() {
    return queryOnPlayerThread(player::getPlaybackState);
  }

  public boolean getPlayWhenReady() {
    return queryOnPlayerThread(player::getPlayWhenReady);
  }

  public boolean isPlaying() {
    return queryOnPlayerThread(player::isPlaying);
  }

  public int getCurrentMediaItemIndex() {
    return queryOnPlayerThread(player::getCurrentMediaItemIndex);
  }

  public String[] getPlayerErrorData() {
    @Nullable PlaybackException error = queryOnPlayerThread(player::getPlayerError);
    return new String[] {
      Integer.toString(error != null ? error.errorCode : 0),
      error != null && error.getMessage() != null ? error.getMessage() : ""
    };
  }

  public @Nullable CppMediaItem getCurrentMediaItem() {
    @Nullable MediaItem mediaItem = queryOnPlayerThread(player::getCurrentMediaItem);
    return mediaItem != null ? CppBridgeConverters.fromMediaItem(mediaItem) : null;
  }

  public @Nullable CppMediaItem getMediaItemAt(int index) {
    @Nullable MediaItem mediaItem = getMediaItemAtOrNull(index);
    return mediaItem != null ? CppBridgeConverters.fromMediaItem(mediaItem) : null;
  }

  private @Nullable MediaItem getMediaItemAtOrNull(int index) {
    return queryOnPlayerThread(
        () -> index >= 0 && index < player.getMediaItemCount() ? player.getMediaItemAt(index) : null);
  }

  public void releaseOpaqueObjectTokens(@Nullable String[] tokens) {
    if (tokens == null) {
      return;
    }
    for (String token : tokens) {
      if (token != null && !token.isEmpty()) {
        CppOpaqueObjectRegistry.unregister(token);
      }
    }
  }

  private static String[] toOpaqueTokenArray(LinkedHashSet<String> tokens) {
    return tokens.toArray(new String[0]);
  }

  private static void addOpaqueToken(LinkedHashSet<String> tokens, @Nullable String token) {
    if (token != null && !token.isEmpty()) {
      tokens.add(token);
    }
  }

  private static void collectOpaqueTokens(
      LinkedHashSet<String> tokens, @Nullable CppRequestMetadata requestMetadata) {
    if (requestMetadata == null) {
      return;
    }
    addOpaqueToken(tokens, requestMetadata.extrasToken);
  }

  private static void collectOpaqueTokens(
      LinkedHashSet<String> tokens, @Nullable CppAdsConfiguration adsConfiguration) {
    if (adsConfiguration == null) {
      return;
    }
    addOpaqueToken(tokens, adsConfiguration.adsIdToken);
  }

  private static void collectOpaqueTokens(
      LinkedHashSet<String> tokens, @Nullable CppMediaMetadata metadata) {
    if (metadata == null) {
      return;
    }
    addOpaqueToken(tokens, metadata.titleToken);
    addOpaqueToken(tokens, metadata.artistToken);
    addOpaqueToken(tokens, metadata.albumTitleToken);
    addOpaqueToken(tokens, metadata.albumArtistToken);
    addOpaqueToken(tokens, metadata.displayTitleToken);
    addOpaqueToken(tokens, metadata.subtitleToken);
    addOpaqueToken(tokens, metadata.descriptionToken);
    addOpaqueToken(tokens, metadata.writerToken);
    addOpaqueToken(tokens, metadata.authorToken);
    addOpaqueToken(tokens, metadata.composerToken);
    addOpaqueToken(tokens, metadata.conductorToken);
    addOpaqueToken(tokens, metadata.genreToken);
    addOpaqueToken(tokens, metadata.compilationToken);
    addOpaqueToken(tokens, metadata.stationToken);
    addOpaqueToken(tokens, metadata.extrasToken);
  }

  private static void collectOpaqueTokens(LinkedHashSet<String> tokens, @Nullable CppMediaItem item) {
    if (item == null) {
      return;
    }
    addOpaqueToken(tokens, item.tagToken);
    collectOpaqueTokens(tokens, item.mediaMetadata);
    collectOpaqueTokens(tokens, item.requestMetadata);
    collectOpaqueTokens(tokens, item.adsConfiguration);
  }

  private static void collectOpaqueTokens(
      LinkedHashSet<String> tokens, @Nullable CppPositionInfo positionInfo) {
    if (positionInfo == null) {
      return;
    }
    collectOpaqueTokens(tokens, positionInfo.mediaItem);
  }

  private static void collectOpaqueTokens(LinkedHashSet<String> tokens, @Nullable CppCue cue) {
    if (cue == null) {
      return;
    }
    addOpaqueToken(tokens, cue.textToken);
    addOpaqueToken(tokens, cue.bitmapToken);
  }

  private static void collectOpaqueTokens(LinkedHashSet<String> tokens, @Nullable CppCue[] cues) {
    if (cues == null) {
      return;
    }
    for (CppCue cue : cues) {
      collectOpaqueTokens(tokens, cue);
    }
  }

  private static void collectOpaqueTokens(
      LinkedHashSet<String> tokens, @Nullable CppTrackInfo trackInfo) {
    if (trackInfo == null) {
      return;
    }
    addOpaqueToken(tokens, trackInfo.labelToken);
  }

  private static void collectOpaqueTokens(
      LinkedHashSet<String> tokens, @Nullable CppTrackGroup trackGroup) {
    if (trackGroup == null) {
      return;
    }
    addOpaqueToken(tokens, trackGroup.groupToken);
    if (trackGroup.tracks == null) {
      return;
    }
    for (CppTrackInfo track : trackGroup.tracks) {
      collectOpaqueTokens(tokens, track);
    }
  }

  private static void collectOpaqueTokens(LinkedHashSet<String> tokens, @Nullable CppTracks tracks) {
    if (tracks == null || tracks.groups == null) {
      return;
    }
    for (CppTrackGroup group : tracks.groups) {
      collectOpaqueTokens(tokens, group);
    }
  }

  public String getCurrentMediaItemDebugSummary() {
    return queryOnPlayerThread(
        () -> {
          @Nullable MediaItem mediaItem = player.getCurrentMediaItem();
          if (mediaItem == null) {
            return "mediaItem=null";
          }
          int subtitleCount = 0;
          String subtitleLanguages = "";
          if (mediaItem.localConfiguration != null) {
            subtitleCount = mediaItem.localConfiguration.subtitleConfigurations.size();
            StringBuilder subtitleLanguageBuilder = new StringBuilder();
            for (int i = 0; i < mediaItem.localConfiguration.subtitleConfigurations.size(); i++) {
              @Nullable String language =
                  mediaItem.localConfiguration.subtitleConfigurations.get(i).language;
              if (i > 0) {
                subtitleLanguageBuilder.append('|');
              }
              subtitleLanguageBuilder.append(language != null ? language : "");
            }
            subtitleLanguages = subtitleLanguageBuilder.toString();
          }
          long clippingStartMs = mediaItem.clippingConfiguration.startPositionMs;
          long clippingEndMs = mediaItem.clippingConfiguration.endPositionMs;
          long liveTargetOffsetMs = mediaItem.liveConfiguration.targetOffsetMs;
          long liveMinOffsetMs = mediaItem.liveConfiguration.minOffsetMs;
          long liveMaxOffsetMs = mediaItem.liveConfiguration.maxOffsetMs;
          float liveMinPlaybackSpeed = mediaItem.liveConfiguration.minPlaybackSpeed;
          float liveMaxPlaybackSpeed = mediaItem.liveConfiguration.maxPlaybackSpeed;
          @Nullable String drmScheme =
              mediaItem.localConfiguration != null
                      && mediaItem.localConfiguration.drmConfiguration != null
                  ? mediaItem.localConfiguration.drmConfiguration.scheme.toString()
                  : "";
          @Nullable String localMimeType =
              mediaItem.localConfiguration != null ? mediaItem.localConfiguration.mimeType : "";
          @Nullable String drmLicenseUri =
              mediaItem.localConfiguration != null
                      && mediaItem.localConfiguration.drmConfiguration != null
                      && mediaItem.localConfiguration.drmConfiguration.licenseUri != null
                  ? mediaItem.localConfiguration.drmConfiguration.licenseUri.toString()
                  : "";
          int drmHeaderCount =
              mediaItem.localConfiguration != null
                      && mediaItem.localConfiguration.drmConfiguration != null
                  ? mediaItem.localConfiguration.drmConfiguration.licenseRequestHeaders.size()
                  : 0;
          String drmHeader0 = "";
          String drmForcedSessionTrackTypes = "";
          int drmKeySetIdLength =
              mediaItem.localConfiguration != null
                          && mediaItem.localConfiguration.drmConfiguration != null
                          && mediaItem.localConfiguration.drmConfiguration.getKeySetId() != null
                  ? mediaItem.localConfiguration.drmConfiguration.getKeySetId().length
                  : 0;
          if (mediaItem.localConfiguration != null
              && mediaItem.localConfiguration.drmConfiguration != null
              && !mediaItem.localConfiguration.drmConfiguration.licenseRequestHeaders.isEmpty()) {
            Map.Entry<String, String> firstDrmHeader =
                mediaItem.localConfiguration.drmConfiguration.licenseRequestHeaders.entrySet()
                    .iterator()
                    .next();
            drmHeader0 = firstDrmHeader.getKey() + ":" + firstDrmHeader.getValue();
          }
          if (mediaItem.localConfiguration != null
              && mediaItem.localConfiguration.drmConfiguration != null) {
            StringBuilder forcedSessionTrackTypesBuilder = new StringBuilder();
            for (int i = 0;
                i < mediaItem.localConfiguration.drmConfiguration.forcedSessionTrackTypes.size();
                i++) {
              if (i > 0) {
                forcedSessionTrackTypesBuilder.append('|');
              }
              forcedSessionTrackTypesBuilder.append(
                  mediaItem.localConfiguration.drmConfiguration.forcedSessionTrackTypes.get(i));
            }
            drmForcedSessionTrackTypes = forcedSessionTrackTypesBuilder.toString();
          }
          boolean playClearWithoutKey =
              mediaItem.localConfiguration != null
                  && mediaItem.localConfiguration.drmConfiguration != null
                  && mediaItem.localConfiguration.drmConfiguration.playClearContentWithoutKey;
          return "mediaId="
              + mediaItem.mediaId
              + ",subtitleCount="
              + subtitleCount
              + ",subtitleLanguages="
              + subtitleLanguages
              + ",clipStartMs="
              + clippingStartMs
              + ",clipEndMs="
              + clippingEndMs
              + ",liveTargetOffsetMs="
              + liveTargetOffsetMs
              + ",liveMinOffsetMs="
              + liveMinOffsetMs
              + ",liveMaxOffsetMs="
              + liveMaxOffsetMs
              + ",liveMinSpeed="
              + liveMinPlaybackSpeed
              + ",liveMaxSpeed="
              + liveMaxPlaybackSpeed
              + ",mimeType="
              + localMimeType
              + ",drmScheme="
              + drmScheme
              + ",drmLicenseUri="
              + drmLicenseUri
              + ",drmHeaderCount="
              + drmHeaderCount
              + ",drmHeader0="
              + drmHeader0
              + ",drmForcedSessionTrackTypes="
              + drmForcedSessionTrackTypes
              + ",drmKeySetIdLength="
              + drmKeySetIdLength
              + ",playClearWithoutKey="
              + (playClearWithoutKey ? 1 : 0);
        });
  }

  public String getMediaSourceFactoryDebugSummary() {
    return "injectedFactoryUsed="
        + (injectedMediaSourceFactoryUsedForTest ? 1 : 0)
        + ",factoryToken="
        + injectedMediaSourceFactoryTokenForTest
        + ",factoryIdentity="
        + injectedMediaSourceFactoryIdentityForTest
        + ",parseSubtitlesDuringExtraction="
        + (mediaSourceFactoryConfig.parseSubtitlesDuringExtraction ? 1 : 0)
        + ",loadOnlySelectedTracks="
        + (mediaSourceFactoryConfig.loadOnlySelectedTracks ? 1 : 0)
        + ",headerCount="
        + Math.min(
            mediaSourceFactoryConfig.defaultRequestHeaderNames.length,
            mediaSourceFactoryConfig.defaultRequestHeaderValues.length)
        + ",userAgent="
        + (mediaSourceFactoryConfig.userAgent != null ? mediaSourceFactoryConfig.userAgent : "")
        + ",connectTimeoutMs="
        + mediaSourceFactoryConfig.connectTimeoutMs
        + ",readTimeoutMs="
        + mediaSourceFactoryConfig.readTimeoutMs
        + ",allowCrossProtocolRedirects="
        + (mediaSourceFactoryConfig.allowCrossProtocolRedirects ? 1 : 0)
        + ",liveTargetOffsetMs="
        + mediaSourceFactoryConfig.liveTargetOffsetMs
        + ",liveMinOffsetMs="
        + mediaSourceFactoryConfig.liveMinOffsetMs
        + ",liveMaxOffsetMs="
        + mediaSourceFactoryConfig.liveMaxOffsetMs
        + ",liveMinSpeed="
        + mediaSourceFactoryConfig.liveMinSpeed
        + ",liveMaxSpeed="
        + mediaSourceFactoryConfig.liveMaxSpeed;
  }

  public void release() {
    long releasedHandle = nativeHandle.getAndSet(0L);
    if (releasedHandle == 0L) {
      debugLog("release skip reason=emptyHandle");
      return;
    }
    if (!released.compareAndSet(false, true)) {
      debugLog("release skip reason=alreadyReleased");
      return;
    }
    debugLog(
        "release start releasedHandle="
            + releasedHandle
            + " currentThread="
            + Thread.currentThread().getName()
            + " playerThread="
            + playerThreadName());
    runOnPlayerThread(
        "release",
        () -> {
          debugLog("release task removeListener");
          player.removeListener(this);
          debugLog("release task removeAnalyticsListener");
          player.removeAnalyticsListener(this);
          debugLog("release task player.release begin");
          player.release();
          debugLog("release task player.release end");
        },
        /* allowAfterRelease= */ true);
    debugLog("release done releasedHandle=" + releasedHandle);
  }

  @Override
  public void onPlaybackStateChanged(int playbackState) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnPlaybackStateChanged(handle, playbackState);
  }

  @Override
  public void onPlayWhenReadyChanged(boolean playWhenReady, int reason) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnPlayWhenReadyChanged(handle, playWhenReady, reason);
  }

  @Override
  public void onIsPlayingChanged(boolean isPlaying) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnIsPlayingChanged(handle, isPlaying);
  }

  @Override
  public void onMediaItemTransition(@Nullable MediaItem mediaItem, int reason) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnMediaItemTransition(handle, player.getCurrentMediaItemIndex(), reason);
  }

  @Override
  public void onPlayerError(PlaybackException error) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnPlayerError(
        handle, error.errorCode, error.getMessage() != null ? error.getMessage() : "");
  }

  @Override
  public void onPlayerErrorChanged(@Nullable PlaybackException error) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnPlayerErrorChanged(
        handle,
        error != null ? error.errorCode : 0,
        error != null && error.getMessage() != null ? error.getMessage() : "");
  }

  @Override
  public void onTimelineChanged(Timeline timeline, int reason) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnTimelineChanged(handle, timeline.getWindowCount(), timeline.getPeriodCount(), reason);
  }

  @Override
  public void onTracksChanged(androidx.media3.common.Tracks tracks) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnTracksChanged(handle);
  }

  @Override
  public void onPositionDiscontinuity(
      Player.PositionInfo oldPosition, Player.PositionInfo newPosition, int reason) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnPositionDiscontinuity(
        handle,
        CppBridgeConverters.fromPositionInfo(oldPosition),
        CppBridgeConverters.fromPositionInfo(newPosition),
        reason);
  }

  @Override
  public void onAudioAttributesChanged(AudioAttributes audioAttributes) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAudioAttributesChanged(
        handle,
        audioAttributes.contentType,
        audioAttributes.usage,
        audioAttributes.flags,
        audioAttributes.allowedCapturePolicy,
        audioAttributes.spatializationBehavior);
  }

  @Override
  public void onCues(CueGroup cueGroup) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnCues(handle, cueGroup.cues.size(), cueGroup.presentationTimeUs);
  }

  @Override
  public void onRepeatModeChanged(int repeatMode) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnRepeatModeChanged(handle, repeatMode);
  }

  @Override
  public void onShuffleModeEnabledChanged(boolean shuffleModeEnabled) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnShuffleModeEnabledChanged(handle, shuffleModeEnabled);
  }

  @Override
  public void onSeekBackIncrementChanged(long seekBackIncrementMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnSeekBackIncrementChanged(handle, seekBackIncrementMs);
  }

  @Override
  public void onSeekForwardIncrementChanged(long seekForwardIncrementMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnSeekForwardIncrementChanged(handle, seekForwardIncrementMs);
  }

  @Override
  public void onMaxSeekToPreviousPositionChanged(long maxSeekToPreviousPositionMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnMaxSeekToPreviousPositionChanged(handle, maxSeekToPreviousPositionMs);
  }

  @Override
  public void onTrackSelectionParametersChanged(
      androidx.media3.common.TrackSelectionParameters parameters) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnTrackSelectionParametersChanged(handle);
  }

  @Override
  public void onPlaybackParametersChanged(PlaybackParameters playbackParameters) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnPlaybackParametersChanged(
        handle, playbackParameters.speed, playbackParameters.pitch);
  }

  @Override
  public void onPlaybackSuppressionReasonChanged(int playbackSuppressionReason) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnPlaybackSuppressionReasonChanged(handle, playbackSuppressionReason);
  }

  @Override
  public void onAvailableCommandsChanged(Player.Commands availableCommands) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAvailableCommandsChanged(handle, new CppCommands(getCommandCodes(availableCommands)));
  }

  @Override
  public void onEvents(Player player, Player.Events events) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnEvents(handle, new CppPlayerEvents(getEventCodes(events)));
  }

  @Override
  public void onDeviceInfoChanged(DeviceInfo deviceInfo) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnDeviceInfoChanged(
        handle,
        new CppDeviceInfo(
            deviceInfo.playbackType,
            deviceInfo.minVolume,
            deviceInfo.maxVolume,
            deviceInfo.routingControllerId));
  }

  @Override
  public void onDeviceVolumeChanged(int volume, boolean muted) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnDeviceVolumeChanged(handle, volume, muted);
  }

  @Override
  public void onSkipSilenceEnabledChanged(boolean skipSilenceEnabled) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnSkipSilenceEnabledChanged(handle, skipSilenceEnabled);
  }

  @Override
  public void onVideoSizeChanged(VideoSize videoSize) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnVideoSizeChanged(
        handle,
        new CppVideoSize(
            videoSize.width,
            videoSize.height,
            videoSize.unappliedRotationDegrees,
            videoSize.pixelWidthHeightRatio));
  }

  @Override
  public void onSurfaceSizeChanged(int width, int height) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnSurfaceSizeChanged(handle, width, height);
  }

  @Override
  public void onRenderedFirstFrame() {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnRenderedFirstFrame(handle);
  }

  @Override
  public void onMediaMetadataChanged(MediaMetadata mediaMetadata) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnMediaMetadataChanged(handle, CppBridgeConverters.fromMediaMetadata(mediaMetadata));
  }

  @Override
  public void onPlaylistMetadataChanged(MediaMetadata mediaMetadata) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnPlaylistMetadataChanged(handle, CppBridgeConverters.fromMediaMetadata(mediaMetadata));
  }

  @Override
  public void onBandwidthEstimate(
      EventTime eventTime, int elapsedMs, long bytesTransferred, long bitrateEstimate) {
    latestBitrateEstimate = bitrateEstimate;
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsUpdated(handle);
    nativeOnAnalyticsBandwidthEstimate(handle, elapsedMs, bytesTransferred, bitrateEstimate);
  }

  @Override
  public void onDroppedVideoFrames(EventTime eventTime, int droppedFrames, long elapsedMs) {
    droppedVideoFrames += droppedFrames;
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsUpdated(handle);
    nativeOnAnalyticsDroppedVideoFrames(handle, droppedFrames, elapsedMs);
  }

  @Override
  public void onLoadStarted(
      EventTime eventTime, LoadEventInfo loadEventInfo, MediaLoadData mediaLoadData, int retryCount) {
    loadStartedCount++;
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsUpdated(handle);
    nativeOnAnalyticsLoadStarted(
        handle,
        loadEventInfo.uri != null ? loadEventInfo.uri.toString() : "",
        mediaLoadData.dataType,
        mediaLoadData.trackType,
        retryCount);
  }

  @Override
  public void onLoadCompleted(
      EventTime eventTime, LoadEventInfo loadEventInfo, MediaLoadData mediaLoadData) {
    loadCompletedCount++;
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsUpdated(handle);
    nativeOnAnalyticsLoadCompleted(
        handle,
        loadEventInfo.uri != null ? loadEventInfo.uri.toString() : "",
        mediaLoadData.dataType,
        mediaLoadData.trackType);
  }

  @Override
  public void onAudioInputFormatChanged(
      EventTime eventTime,
      androidx.media3.common.Format format,
      @Nullable androidx.media3.exoplayer.DecoderReuseEvaluation decoderReuseEvaluation) {
    latestAudioSampleMimeType = format.sampleMimeType != null ? format.sampleMimeType : "";
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsUpdated(handle);
    nativeOnAnalyticsAudioInputFormatChanged(
        handle,
        format.sampleMimeType != null ? format.sampleMimeType : "",
        format.codecs != null ? format.codecs : "",
        format.channelCount,
        format.sampleRate);
  }

  @Override
  public void onAudioDecoderInitialized(
      EventTime eventTime,
      String decoderName,
      long initializedTimestampMs,
      long initializationDurationMs) {
    dispatchAudioDecoderInitialized(
        decoderName, initializedTimestampMs, initializationDurationMs);
  }

  @Override
  public void onVideoDecoderInitialized(
      EventTime eventTime,
      String decoderName,
      long initializedTimestampMs,
      long initializationDurationMs) {
    dispatchVideoDecoderInitialized(
        decoderName, initializedTimestampMs, initializationDurationMs);
  }

  @Override
  public void onAudioDecoderReleased(EventTime eventTime, String decoderName) {
    dispatchAudioDecoderReleased(decoderName);
  }

  @Override
  public void onVideoDecoderReleased(EventTime eventTime, String decoderName) {
    dispatchVideoDecoderReleased(decoderName);
  }

  @Override
  public void onRenderedFirstFrame(EventTime eventTime, Object output, long renderTimeMs) {
    dispatchAnalyticsRenderedFirstFrame(renderTimeMs);
  }

  @Override
  public void onVideoSizeChanged(EventTime eventTime, VideoSize videoSize) {
    dispatchAnalyticsVideoSizeChanged(
        videoSize.width, videoSize.height, videoSize.pixelWidthHeightRatio);
  }

  @Override
  public void onAudioPositionAdvancing(EventTime eventTime, long playoutStartSystemTimeMs) {
    dispatchAudioPositionAdvancing(playoutStartSystemTimeMs);
  }

  @Override
  public void onVideoFrameProcessingOffset(
      EventTime eventTime, long totalProcessingOffsetUs, int frameCount) {
    dispatchVideoFrameProcessingOffset(totalProcessingOffsetUs, frameCount);
  }

  @Override
  public void onVolumeChanged(EventTime eventTime, float volume) {
    dispatchVolumeChanged(volume);
  }

  @Override
  public void onAudioSessionIdChanged(EventTime eventTime, int audioSessionId) {
    dispatchAudioSessionIdChanged(audioSessionId);
  }

  @Override
  public void onSkipSilenceEnabledChanged(EventTime eventTime, boolean skipSilenceEnabled) {
    dispatchAnalyticsSkipSilenceEnabledChanged(skipSilenceEnabled);
  }

  @Override
  public void onDeviceVolumeChanged(EventTime eventTime, int volume, boolean muted) {
    dispatchAnalyticsDeviceVolumeChanged(volume, muted);
  }

  @Override
  public void onPlaybackStateChanged(EventTime eventTime, int state) {
    dispatchAnalyticsPlaybackStateChanged(state);
  }

  @Override
  public void onIsPlayingChanged(EventTime eventTime, boolean isPlaying) {
    dispatchAnalyticsIsPlayingChanged(isPlaying);
  }

  @Override
  public void onPlayWhenReadyChanged(
      EventTime eventTime, boolean playWhenReady, int reason) {
    dispatchAnalyticsPlayWhenReadyChanged(playWhenReady, reason);
  }

  @Override
  public void onPlaybackSuppressionReasonChanged(
      EventTime eventTime, int playbackSuppressionReason) {
    dispatchAnalyticsPlaybackSuppressionReasonChanged(playbackSuppressionReason);
  }

  @Override
  public void onIsLoadingChanged(EventTime eventTime, boolean isLoading) {
    dispatchAnalyticsIsLoadingChanged(isLoading);
  }

  @Override
  public void onRepeatModeChanged(EventTime eventTime, int repeatMode) {
    dispatchAnalyticsRepeatModeChanged(repeatMode);
  }

  @Override
  public void onShuffleModeChanged(EventTime eventTime, boolean shuffleModeEnabled) {
    dispatchAnalyticsShuffleModeChanged(shuffleModeEnabled);
  }

  @Override
  public void onPlaybackParametersChanged(EventTime eventTime, PlaybackParameters playbackParameters) {
    dispatchAnalyticsPlaybackParametersChanged(playbackParameters.speed, playbackParameters.pitch);
  }

  @Override
  public void onAvailableCommandsChanged(EventTime eventTime, Player.Commands availableCommands) {
    dispatchAnalyticsAvailableCommandsChanged(new CppCommands(getCommandCodes(availableCommands)));
  }

  @Override
  public void onEvents(Player player, AnalyticsListener.Events events) {
    dispatchAnalyticsEvents(new CppPlayerEvents(getEventCodes(events)));
  }

  @Override
  public void onSeekBackIncrementChanged(EventTime eventTime, long seekBackIncrementMs) {
    dispatchAnalyticsSeekBackIncrementChanged(seekBackIncrementMs);
  }

  @Override
  public void onSeekForwardIncrementChanged(EventTime eventTime, long seekForwardIncrementMs) {
    dispatchAnalyticsSeekForwardIncrementChanged(seekForwardIncrementMs);
  }

  @Override
  public void onMaxSeekToPreviousPositionChanged(
      EventTime eventTime, long maxSeekToPreviousPositionMs) {
    dispatchAnalyticsMaxSeekToPreviousPositionChanged(maxSeekToPreviousPositionMs);
  }

  @Override
  public void onTimelineChanged(EventTime eventTime, int reason) {
    dispatchAnalyticsTimelineChanged(reason);
  }

  @Override
  public void onPositionDiscontinuity(
      EventTime eventTime,
      Player.PositionInfo oldPosition,
      Player.PositionInfo newPosition,
      int reason) {
    dispatchAnalyticsPositionDiscontinuity(reason);
  }

  @Override
  public void onSeekStarted(EventTime eventTime) {
    dispatchAnalyticsSeekStarted();
  }

  @Override
  public void onPlayerError(EventTime eventTime, PlaybackException error) {
    dispatchAnalyticsPlayerError(
        error.errorCode, error.getMessage() != null ? error.getMessage() : "");
  }

  @Override
  public void onPlayerErrorChanged(EventTime eventTime, @Nullable PlaybackException error) {
    dispatchAnalyticsPlayerErrorChanged(
        error != null ? error.errorCode : 0,
        error != null && error.getMessage() != null ? error.getMessage() : "");
  }

  @Override
  public void onTracksChanged(EventTime eventTime, androidx.media3.common.Tracks tracks) {
    dispatchAnalyticsTracksChanged(CppBridgeConverters.toCppTracks(tracks));
  }

  @Override
  public void onMediaItemTransition(
      EventTime eventTime, @Nullable MediaItem mediaItem, int reason) {
    dispatchAnalyticsMediaItemTransition(
        mediaItem != null ? CppBridgeConverters.fromMediaItem(mediaItem) : null, reason);
  }

  @Override
  public void onCues(EventTime eventTime, CueGroup cueGroup) {
    CppCue[] result = new CppCue[cueGroup.cues.size()];
    for (int i = 0; i < cueGroup.cues.size(); i++) {
      Cue cue = cueGroup.cues.get(i);
      result[i] = CppBridgeConverters.fromCue(cue);
    }
    dispatchAnalyticsCues(result, cueGroup.presentationTimeUs);
  }

  @Override
  public void onMetadata(EventTime eventTime, Metadata metadata) {
    int entryCount = metadata.length();
    String firstEntryType = "";
    String firstEntryText = "";
    if (entryCount > 0) {
      Metadata.Entry firstEntry = metadata.get(0);
      firstEntryType = firstEntry.getClass().getSimpleName();
      firstEntryText = String.valueOf(firstEntry);
    }
    dispatchAnalyticsMetadata(entryCount, firstEntryType, firstEntryText);
  }

  @Override
  public void onDeviceInfoChanged(EventTime eventTime, DeviceInfo deviceInfo) {
    dispatchAnalyticsDeviceInfoChanged(
        new CppDeviceInfo(
            deviceInfo.playbackType,
            deviceInfo.minVolume,
            deviceInfo.maxVolume,
            deviceInfo.routingControllerId));
  }

  @Override
  public void onMediaMetadataChanged(EventTime eventTime, MediaMetadata mediaMetadata) {
    dispatchAnalyticsMediaMetadataChanged(CppBridgeConverters.fromMediaMetadata(mediaMetadata));
  }

  @Override
  public void onPlaylistMetadataChanged(EventTime eventTime, MediaMetadata playlistMetadata) {
    dispatchAnalyticsPlaylistMetadataChanged(
        CppBridgeConverters.fromMediaMetadata(playlistMetadata));
  }

  @Override
  public void onLoadError(
      EventTime eventTime,
      LoadEventInfo loadEventInfo,
      MediaLoadData mediaLoadData,
      IOException error,
      boolean wasCanceled) {
    dispatchAnalyticsLoadError(
        loadEventInfo.uri != null ? loadEventInfo.uri.toString() : "",
        mediaLoadData.dataType,
        mediaLoadData.trackType,
        error.getMessage() != null ? error.getMessage() : "",
        wasCanceled);
  }

  @Override
  public void onVideoInputFormatChanged(
      EventTime eventTime,
      androidx.media3.common.Format format,
      @Nullable androidx.media3.exoplayer.DecoderReuseEvaluation decoderReuseEvaluation) {
    dispatchVideoInputFormatChanged(
        format.sampleMimeType != null ? format.sampleMimeType : "",
        format.codecs != null ? format.codecs : "",
        format.width,
        format.height,
        format.frameRate);
  }

  @Override
  public void onAudioUnderrun(
      EventTime eventTime, int bufferSize, long bufferSizeMs, long elapsedSinceLastFeedMs) {
    dispatchAudioUnderrun(bufferSize, bufferSizeMs, elapsedSinceLastFeedMs);
  }

  private void dispatchAudioUnderrun(
      int bufferSize, long bufferSizeMs, long elapsedSinceLastFeedMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsAudioUnderrun(
        handle, bufferSize, bufferSizeMs, elapsedSinceLastFeedMs);
  }

  private void dispatchDroppedVideoFrames(int droppedFrames, long elapsedMs) {
    droppedVideoFrames += droppedFrames;
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsUpdated(handle);
    nativeOnAnalyticsDroppedVideoFrames(handle, droppedFrames, elapsedMs);
  }

  private void dispatchBandwidthEstimate(
      int elapsedMs, long bytesTransferred, long bitrateEstimate) {
    latestBitrateEstimate = bitrateEstimate;
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsUpdated(handle);
    nativeOnAnalyticsBandwidthEstimate(handle, elapsedMs, bytesTransferred, bitrateEstimate);
  }

  private void dispatchLoadStarted(String uri, int dataType, int trackType, int retryCount) {
    loadStartedCount++;
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsUpdated(handle);
    nativeOnAnalyticsLoadStarted(
        handle, uri != null ? uri : "", dataType, trackType, retryCount);
  }

  private void dispatchLoadCompleted(String uri, int dataType, int trackType) {
    loadCompletedCount++;
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsUpdated(handle);
    nativeOnAnalyticsLoadCompleted(handle, uri != null ? uri : "", dataType, trackType);
  }

  private void dispatchAudioInputFormatChanged(
      String sampleMimeType, String codecs, int channelCount, int sampleRate) {
    latestAudioSampleMimeType = sampleMimeType != null ? sampleMimeType : "";
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsUpdated(handle);
    nativeOnAnalyticsAudioInputFormatChanged(
        handle,
        sampleMimeType != null ? sampleMimeType : "",
        codecs != null ? codecs : "",
        channelCount,
        sampleRate);
  }

  private void dispatchAudioDecoderInitialized(
      String decoderName, long initializedTimestampMs, long initializationDurationMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsAudioDecoderInitialized(
        handle,
        decoderName != null ? decoderName : "",
        initializedTimestampMs,
        initializationDurationMs);
  }

  private void dispatchVideoDecoderInitialized(
      String decoderName, long initializedTimestampMs, long initializationDurationMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsVideoDecoderInitialized(
        handle,
        decoderName != null ? decoderName : "",
        initializedTimestampMs,
        initializationDurationMs);
  }

  private void dispatchAudioDecoderReleased(String decoderName) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsAudioDecoderReleased(handle, decoderName != null ? decoderName : "");
  }

  private void dispatchVideoDecoderReleased(String decoderName) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsVideoDecoderReleased(handle, decoderName != null ? decoderName : "");
  }

  private void dispatchAnalyticsRenderedFirstFrame(long renderTimeMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsRenderedFirstFrame(handle, renderTimeMs);
  }

  private void dispatchAnalyticsVideoSizeChanged(
      int width, int height, float pixelWidthHeightRatio) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsVideoSizeChanged(handle, width, height, pixelWidthHeightRatio);
  }

  private void dispatchAudioPositionAdvancing(long playoutStartSystemTimeMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsAudioPositionAdvancing(handle, playoutStartSystemTimeMs);
  }

  private void dispatchVideoFrameProcessingOffset(
      long totalProcessingOffsetUs, int frameCount) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsVideoFrameProcessingOffset(
        handle, totalProcessingOffsetUs, frameCount);
  }

  private void dispatchVolumeChanged(float volume) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsVolumeChanged(handle, volume);
  }

  private void dispatchAudioSessionIdChanged(int audioSessionId) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsAudioSessionIdChanged(handle, audioSessionId);
  }

  private void dispatchAnalyticsSkipSilenceEnabledChanged(boolean skipSilenceEnabled) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsSkipSilenceEnabledChanged(handle, skipSilenceEnabled);
  }

  private void dispatchAnalyticsDeviceVolumeChanged(int volume, boolean muted) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsDeviceVolumeChanged(handle, volume, muted);
  }

  private void dispatchAnalyticsPlaybackStateChanged(int playbackState) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsPlaybackStateChanged(handle, playbackState);
  }

  private void dispatchAnalyticsIsPlayingChanged(boolean isPlaying) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsIsPlayingChanged(handle, isPlaying);
  }

  private void dispatchAnalyticsPlayWhenReadyChanged(boolean playWhenReady, int reason) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsPlayWhenReadyChanged(handle, playWhenReady, reason);
  }

  private void dispatchAnalyticsPlaybackSuppressionReasonChanged(
      int playbackSuppressionReason) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsPlaybackSuppressionReasonChanged(handle, playbackSuppressionReason);
  }

  private void dispatchAnalyticsIsLoadingChanged(boolean isLoading) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsIsLoadingChanged(handle, isLoading);
  }

  private void dispatchAnalyticsRepeatModeChanged(int repeatMode) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsRepeatModeChanged(handle, repeatMode);
  }

  private void dispatchAnalyticsShuffleModeChanged(boolean shuffleModeEnabled) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsShuffleModeChanged(handle, shuffleModeEnabled);
  }

  private void dispatchAnalyticsPlaybackParametersChanged(float speed, float pitch) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsPlaybackParametersChanged(handle, speed, pitch);
  }

  private void dispatchAnalyticsAvailableCommandsChanged(CppCommands commands) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsAvailableCommandsChanged(handle, commands);
  }

  private void dispatchAnalyticsEvents(CppPlayerEvents events) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsEvents(handle, events);
  }

  private void dispatchSeekBackIncrementChanged(long seekBackIncrementMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnSeekBackIncrementChanged(handle, seekBackIncrementMs);
  }

  private void dispatchSeekForwardIncrementChanged(long seekForwardIncrementMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnSeekForwardIncrementChanged(handle, seekForwardIncrementMs);
  }

  private void dispatchMaxSeekToPreviousPositionChanged(long maxSeekToPreviousPositionMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnMaxSeekToPreviousPositionChanged(handle, maxSeekToPreviousPositionMs);
  }

  private void dispatchAnalyticsSeekBackIncrementChanged(long seekBackIncrementMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsSeekBackIncrementChanged(handle, seekBackIncrementMs);
  }

  private void dispatchAnalyticsSeekForwardIncrementChanged(long seekForwardIncrementMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsSeekForwardIncrementChanged(handle, seekForwardIncrementMs);
  }

  private void dispatchAnalyticsMaxSeekToPreviousPositionChanged(
      long maxSeekToPreviousPositionMs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsMaxSeekToPreviousPositionChanged(handle, maxSeekToPreviousPositionMs);
  }

  private void dispatchAnalyticsTimelineChanged(int reason) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsTimelineChanged(handle, reason);
  }

  private void dispatchAnalyticsPositionDiscontinuity(int reason) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsPositionDiscontinuity(handle, reason);
  }

  private void dispatchAnalyticsSeekStarted() {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsSeekStarted(handle);
  }

  private void dispatchAnalyticsPlayerError(int errorCode, @Nullable String message) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsPlayerError(handle, errorCode, message != null ? message : "");
  }

  private void dispatchAnalyticsPlayerErrorChanged(int errorCode, @Nullable String message) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsPlayerErrorChanged(handle, errorCode, message != null ? message : "");
  }

  private void dispatchAnalyticsTracksChanged(CppTracks tracks) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsTracksChanged(handle, tracks);
  }

  private void dispatchAnalyticsMediaItemTransition(
      @Nullable CppMediaItem mediaItem, int reason) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsMediaItemTransition(handle, mediaItem, reason);
  }

  private void dispatchAnalyticsCues(CppCue[] cues, long presentationTimeUs) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsCues(handle, cues, presentationTimeUs);
  }

  private void dispatchAnalyticsMetadata(
      int entryCount, @Nullable String firstEntryType, @Nullable String firstEntryText) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsMetadata(
        handle,
        entryCount,
        firstEntryType != null ? firstEntryType : "",
        firstEntryText != null ? firstEntryText : "");
  }

  private void dispatchAnalyticsLoadError(
      String uri, int dataType, int trackType, String message, boolean wasCanceled) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsLoadError(
        handle,
        uri != null ? uri : "",
        dataType,
        trackType,
        message != null ? message : "",
        wasCanceled);
  }

  private void dispatchAnalyticsDeviceInfoChanged(CppDeviceInfo deviceInfo) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsDeviceInfoChanged(handle, deviceInfo);
  }

  private void dispatchAnalyticsMediaMetadataChanged(CppMediaMetadata metadata) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsMediaMetadataChanged(handle, metadata);
  }

  private void dispatchAnalyticsPlaylistMetadataChanged(CppMediaMetadata metadata) {
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsPlaylistMetadataChanged(handle, metadata);
  }

  private void dispatchVideoInputFormatChanged(
      String sampleMimeType, String codecs, int width, int height, float frameRate) {
    latestVideoSampleMimeType = sampleMimeType != null ? sampleMimeType : "";
    long handle = getNativeHandle();
    if (handle == 0L) {
      return;
    }
    nativeOnAnalyticsUpdated(handle);
    nativeOnAnalyticsVideoInputFormatChanged(
        handle,
        sampleMimeType != null ? sampleMimeType : "",
        codecs != null ? codecs : "",
        width,
        height,
        frameRate);
  }

  private static native void nativeOnPlaybackStateChanged(long nativeHandle, int playbackState);

  private static native void nativeOnPlayWhenReadyChanged(
      long nativeHandle, boolean playWhenReady, int reason);

  private static native void nativeOnIsPlayingChanged(long nativeHandle, boolean isPlaying);

  private static native void nativeOnMediaItemTransition(
      long nativeHandle, int mediaItemIndex, int reason);

  private static native void nativeOnPlayerError(
      long nativeHandle, int errorCode, String message);

  private static native void nativeOnPlayerErrorChanged(
      long nativeHandle, int errorCode, String message);

  private static native void nativeOnTimelineChanged(
      long nativeHandle, int windowCount, int periodCount, int reason);

  private static native void nativeOnTracksChanged(long nativeHandle);

  private static native void nativeOnPositionDiscontinuity(
      long nativeHandle, CppPositionInfo oldPosition, CppPositionInfo newPosition, int reason);

  private static native void nativeOnAudioAttributesChanged(
      long nativeHandle,
      int contentType,
      int usage,
      int flags,
      int allowedCapturePolicy,
      int spatializationBehavior);

  private static native void nativeOnCues(
      long nativeHandle, int cueCount, long presentationTimeUs);

  private static native void nativeOnRepeatModeChanged(long nativeHandle, int repeatMode);

  private static native void nativeOnShuffleModeEnabledChanged(
      long nativeHandle, boolean shuffleModeEnabled);

  private static native void nativeOnSeekBackIncrementChanged(
      long nativeHandle, long seekBackIncrementMs);

  private static native void nativeOnSeekForwardIncrementChanged(
      long nativeHandle, long seekForwardIncrementMs);

  private static native void nativeOnMaxSeekToPreviousPositionChanged(
      long nativeHandle, long maxSeekToPreviousPositionMs);

  private static native void nativeOnTrackSelectionParametersChanged(long nativeHandle);

  private static native void nativeOnPlaybackParametersChanged(
      long nativeHandle, float speed, float pitch);

  private static native void nativeOnPlaybackSuppressionReasonChanged(
      long nativeHandle, int playbackSuppressionReason);

  private static native void nativeOnAvailableCommandsChanged(
      long nativeHandle, CppCommands commands);

  private static native void nativeOnEvents(long nativeHandle, CppPlayerEvents events);

  private static native void nativeOnDeviceInfoChanged(
      long nativeHandle, CppDeviceInfo deviceInfo);

  private static native void nativeOnDeviceVolumeChanged(
      long nativeHandle, int volume, boolean muted);

  private static native void nativeOnSkipSilenceEnabledChanged(
      long nativeHandle, boolean skipSilenceEnabled);

  private static native void nativeOnVideoSizeChanged(
      long nativeHandle, CppVideoSize videoSize);

  private static native void nativeOnSurfaceSizeChanged(
      long nativeHandle, int width, int height);

  private static native void nativeOnRenderedFirstFrame(long nativeHandle);

  private static native void nativeOnMediaMetadataChanged(
      long nativeHandle, CppMediaMetadata metadata);

  private static native void nativeOnPlaylistMetadataChanged(
      long nativeHandle, CppMediaMetadata metadata);

  private static native void nativeOnAnalyticsUpdated(long nativeHandle);

  private static native void nativeOnAnalyticsAudioUnderrun(
      long nativeHandle,
      int bufferSize,
      long bufferSizeMs,
      long elapsedSinceLastFeedMs);

  private static native void nativeOnAnalyticsDroppedVideoFrames(
      long nativeHandle, int droppedFrames, long elapsedMs);

  private static native void nativeOnAnalyticsBandwidthEstimate(
      long nativeHandle, int elapsedMs, long bytesTransferred, long bitrateEstimate);

  private static native void nativeOnAnalyticsLoadStarted(
      long nativeHandle, String uri, int dataType, int trackType, int retryCount);

  private static native void nativeOnAnalyticsLoadCompleted(
      long nativeHandle, String uri, int dataType, int trackType);

  private static native void nativeOnAnalyticsAudioInputFormatChanged(
      long nativeHandle,
      String sampleMimeType,
      String codecs,
      int channelCount,
      int sampleRate);

  private static native void nativeOnAnalyticsAudioDecoderInitialized(
      long nativeHandle,
      String decoderName,
      long initializedTimestampMs,
      long initializationDurationMs);

  private static native void nativeOnAnalyticsVideoDecoderInitialized(
      long nativeHandle,
      String decoderName,
      long initializedTimestampMs,
      long initializationDurationMs);

  private static native void nativeOnAnalyticsAudioDecoderReleased(
      long nativeHandle, String decoderName);

  private static native void nativeOnAnalyticsVideoDecoderReleased(
      long nativeHandle, String decoderName);

  private static native void nativeOnAnalyticsRenderedFirstFrame(
      long nativeHandle, long renderTimeMs);

  private static native void nativeOnAnalyticsVideoSizeChanged(
      long nativeHandle, int width, int height, float pixelWidthHeightRatio);

  private static native void nativeOnAnalyticsAudioPositionAdvancing(
      long nativeHandle, long playoutStartSystemTimeMs);

  private static native void nativeOnAnalyticsVideoFrameProcessingOffset(
      long nativeHandle, long totalProcessingOffsetUs, int frameCount);

  private static native void nativeOnAnalyticsVolumeChanged(
      long nativeHandle, float volume);

  private static native void nativeOnAnalyticsAudioSessionIdChanged(
      long nativeHandle, int audioSessionId);

  private static native void nativeOnAnalyticsSkipSilenceEnabledChanged(
      long nativeHandle, boolean skipSilenceEnabled);

  private static native void nativeOnAnalyticsDeviceVolumeChanged(
      long nativeHandle, int volume, boolean muted);

  private static native void nativeOnAnalyticsPlaybackStateChanged(
      long nativeHandle, int playbackState);

  private static native void nativeOnAnalyticsIsPlayingChanged(
      long nativeHandle, boolean isPlaying);

  private static native void nativeOnAnalyticsPlayWhenReadyChanged(
      long nativeHandle, boolean playWhenReady, int reason);

  private static native void nativeOnAnalyticsPlaybackSuppressionReasonChanged(
      long nativeHandle, int playbackSuppressionReason);

  private static native void nativeOnAnalyticsIsLoadingChanged(
      long nativeHandle, boolean isLoading);

  private static native void nativeOnAnalyticsRepeatModeChanged(
      long nativeHandle, int repeatMode);

  private static native void nativeOnAnalyticsShuffleModeChanged(
      long nativeHandle, boolean shuffleModeEnabled);

  private static native void nativeOnAnalyticsPlaybackParametersChanged(
      long nativeHandle, float speed, float pitch);

  private static native void nativeOnAnalyticsAvailableCommandsChanged(
      long nativeHandle, CppCommands commands);

  private static native void nativeOnAnalyticsEvents(
      long nativeHandle, CppPlayerEvents events);

  private static native void nativeOnAnalyticsSeekBackIncrementChanged(
      long nativeHandle, long seekBackIncrementMs);

  private static native void nativeOnAnalyticsSeekForwardIncrementChanged(
      long nativeHandle, long seekForwardIncrementMs);

  private static native void nativeOnAnalyticsMaxSeekToPreviousPositionChanged(
      long nativeHandle, long maxSeekToPreviousPositionMs);

  private static native void nativeOnAnalyticsTimelineChanged(
      long nativeHandle, int reason);

  private static native void nativeOnAnalyticsPositionDiscontinuity(
      long nativeHandle, int reason);

  private static native void nativeOnAnalyticsSeekStarted(long nativeHandle);

  private static native void nativeOnAnalyticsPlayerError(
      long nativeHandle, int errorCode, String message);

  private static native void nativeOnAnalyticsPlayerErrorChanged(
      long nativeHandle, int errorCode, String message);

  private static native void nativeOnAnalyticsTracksChanged(
      long nativeHandle, CppTracks tracks);

  private static native void nativeOnAnalyticsMediaItemTransition(
      long nativeHandle, @Nullable CppMediaItem mediaItem, int reason);

  private static native void nativeOnAnalyticsCues(
      long nativeHandle, CppCue[] cues, long presentationTimeUs);

  private static native void nativeOnAnalyticsMetadata(
      long nativeHandle, int entryCount, String firstEntryType, String firstEntryText);

  private static native void nativeOnAnalyticsLoadError(
      long nativeHandle,
      String uri,
      int dataType,
      int trackType,
      String message,
      boolean wasCanceled);

  private static native void nativeOnAnalyticsDeviceInfoChanged(
      long nativeHandle, CppDeviceInfo deviceInfo);

  private static native void nativeOnAnalyticsMediaMetadataChanged(
      long nativeHandle, CppMediaMetadata metadata);

  private static native void nativeOnAnalyticsPlaylistMetadataChanged(
      long nativeHandle, CppMediaMetadata metadata);

  private static native void nativeOnAnalyticsVideoInputFormatChanged(
      long nativeHandle,
      String sampleMimeType,
      String codecs,
      int width,
      int height,
      float frameRate);

  private static native void nativeOnImageOutputAvailable(
      long nativeHandle,
      long presentationTimeUs,
      int width,
      int height,
      int byteCount,
      int allocationByteCount,
      int rowBytes,
      boolean hasAlpha,
      boolean isPremultiplied,
      boolean isMutable,
      String bitmapConfig);

  private static native void nativeOnImageOutputDisabled(long nativeHandle);
}


