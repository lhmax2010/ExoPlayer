package androidx.media3.exoplayer.cppbridge;

import android.content.Context;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.TextureView;
import androidx.media3.ui.PlayerView;

/** Native-backed helpers for instrumentation tests that exercise the C++ SDK player path. */
public final class CppBridgeNativePlayerTestHelper {

  static {
    // Player-focused JNI test entrypoints are loaded into the core bridge
    // library so listener implementations and the forwarding layer stay in the
    // same .so during instrumentation runs.
    System.loadLibrary("exoplayer_cppbridge_jni");
  }

  private CppBridgeNativePlayerTestHelper() {}

  public static native String nativeCreateConfiguredPlayerSnapshotForTest(Context context);

  public static native String nativeCreatePlaylistSnapshotForTest(Context context);

  public static native String nativeLifecycleSmokeTest(Context context);

  public static native String nativePostReleaseCallSafetySmokeTest(Context context);

  public static native String nativeTrackSelectionRoundTripForTest(Context context);

  public static native String nativeSubtitleSmokeTest(Context context);

  public static native String nativeDrmSmokeTest(Context context);

  public static native String nativeClippingSmokeTest(Context context);

  public static native String nativeLiveConfigurationSmokeTest(Context context);

  public static native String nativeMultiSubtitleSmokeTest(Context context);

  public static native String nativePlaylistMutationSmokeTest(Context context);

  public static native String nativePlaylistMutationSmokeTestV2(Context context);

  public static native String nativeSeekNavigationSmokeTest(Context context);

  public static native String nativeSeekAliasSmokeTest(Context context);

  public static native String nativeSeekParametersSmokeTest(Context context);

  public static native String nativeAudioAndQuerySmokeTest(Context context);

  public static native String nativeCurrentTracksSmokeTest(Context context);

  public static native String nativeCurrentTimelineSmokeTest(Context context);

  public static native String nativeAvailableCommandsSmokeTest(Context context);

  public static native String nativeSurfaceBridgeSmokeTest(
      Context context, Surface surface, SurfaceView surfaceView, TextureView textureView);

  public static native String nativePlayerViewBridgeSmokeTest(Context context, PlayerView playerView);

  public static native String nativeListenerSmokeTest(Context context);

  public static native String nativeListenerCallbackDetailSmokeTest(Context context);

  public static native String nativeListenerMetadataCueDetailSmokeTest(Context context);

  public static native String nativeListenerDetachSmokeTest(Context context);

  public static native String nativeDeviceAndSkipSilenceSmokeTest(Context context);

  public static native String nativeVideoAndMetadataSmokeTest(Context context);

  public static native String nativeAnalyticsSmokeTest(Context context);

  public static native String nativeAnalyticsCallbackSmokeTest(Context context);

  public static native String nativeAnalyticsListenerRegistrationSmokeTest(Context context);

  public static native String nativeAnalyticsAudioUnderrunSmokeTest(Context context);

  public static native String nativeAnalyticsDroppedVideoFramesSmokeTest(Context context);

  public static native String nativeAnalyticsBandwidthEstimateSmokeTest(Context context);

  public static native String nativeAnalyticsLoadStartedSmokeTest(Context context);

  public static native String nativeAnalyticsLoadCompletedSmokeTest(Context context);

  public static native String nativeAnalyticsAudioInputFormatChangedSmokeTest(Context context);

  public static native String nativeAnalyticsAudioDecoderInitializedSmokeTest(Context context);

  public static native String nativeAnalyticsVideoDecoderInitializedSmokeTest(Context context);

  public static native String nativeAnalyticsAudioDecoderReleasedSmokeTest(Context context);

  public static native String nativeAnalyticsVideoDecoderReleasedSmokeTest(Context context);

  public static native String nativeAnalyticsRenderedFirstFrameSmokeTest(Context context);

  public static native String nativeAnalyticsVideoSizeChangedSmokeTest(Context context);

  public static native String nativeAnalyticsAudioPositionAdvancingSmokeTest(Context context);

  public static native String nativeAnalyticsVideoFrameProcessingOffsetSmokeTest(Context context);

  public static native String nativeAnalyticsVolumeChangedSmokeTest(Context context);

  public static native String nativeAnalyticsAudioSessionIdChangedSmokeTest(Context context);

  public static native String nativeAnalyticsAudioAttributesChangedSmokeTest(Context context);

  public static native String nativeAnalyticsSkipSilenceEnabledChangedSmokeTest(Context context);

  public static native String nativeAnalyticsDeviceVolumeChangedSmokeTest(Context context);

  public static native String nativeAnalyticsPlaybackStateChangedSmokeTest(Context context);

  public static native String nativeAnalyticsIsPlayingChangedSmokeTest(Context context);

  public static native String nativeAnalyticsPlayWhenReadyChangedSmokeTest(Context context);

  public static native String nativeAnalyticsPlaybackSuppressionReasonChangedSmokeTest(
      Context context);

  public static native String nativeAnalyticsIsLoadingChangedSmokeTest(Context context);

  public static native String nativeAnalyticsRepeatModeChangedSmokeTest(Context context);

  public static native String nativeAnalyticsShuffleModeChangedSmokeTest(Context context);

  public static native String nativeAnalyticsPlaybackParametersChangedSmokeTest(Context context);

  public static native String nativeAnalyticsAvailableCommandsChangedSmokeTest(Context context);

  public static native String nativeAnalyticsEventsSmokeTest(Context context);

  public static native String nativeAnalyticsSeekBackIncrementChangedSmokeTest(Context context);

  public static native String nativeAnalyticsSeekForwardIncrementChangedSmokeTest(
      Context context);

  public static native String nativeAnalyticsMaxSeekToPreviousPositionChangedSmokeTest(
      Context context);

  public static native String nativeAnalyticsTimelineChangedSmokeTest(Context context);

  public static native String nativeAnalyticsPositionDiscontinuitySmokeTest(Context context);

  public static native String nativeAnalyticsSeekStartedSmokeTest(Context context);

  public static native String nativeAnalyticsPlayerErrorSmokeTest(Context context);

  public static native String nativeAnalyticsPlayerErrorChangedSmokeTest(Context context);

  public static native String nativeAnalyticsTracksChangedSmokeTest(Context context);

  public static native String nativeAnalyticsMediaItemTransitionSmokeTest(Context context);

  public static native String nativeAnalyticsCuesSmokeTest(Context context);

  public static native String nativeAnalyticsMetadataSmokeTest(Context context);

  public static native String nativeAnalyticsLoadErrorSmokeTest(Context context);

  public static native String nativeAnalyticsDeviceInfoChangedSmokeTest(Context context);

  public static native String nativeAnalyticsMediaMetadataChangedSmokeTest(Context context);

  public static native String nativeAnalyticsPlaylistMetadataChangedSmokeTest(Context context);

  public static native String nativeAnalyticsVideoInputFormatChangedSmokeTest(Context context);

  public static native String nativeImageOutputSmokeTest(Context context);

  public static native String nativeSourceTypeSmokeTest(Context context);

  public static native String nativeHttpHlsDashPlaybackSmokeTest(
      Context context, String httpUrl, String hlsUrl, String dashUrl);

  public static native String nativeMediaSourceFactoryConfigSmokeTest(Context context);

  public static native String nativeMediaSourceFactoryInjectionSmokeTest(Context context);

  public static native String nativeMediaSourceFactoryInjectionFallbackSmokeTest(Context context);

  public static native String nativeMediaSourceFactoryInjectionReplacementSmokeTest(Context context);

  public static native String nativeMediaSourceFactoryInjectionMultiTokenSmokeTest(Context context);

  public static native String nativeMediaSourceFactoryGeneratedTokenSmokeTest(
      Context context, String token);

  public static native String nativePlayerConfigFlagsSmokeTest(Context context);

  public static native String nativeWakeModeRuntimeSmokeTest(Context context);

  public static native String nativeRuntimeControlParitySmokeTest(Context context);

  public static native String nativeAudioAndScrubbingParitySmokeTest(Context context);

  public static native String nativeCodecParametersParitySmokeTest(Context context);

  public static native String nativeAuxiliaryCallbackParitySmokeTest(Context context);

  public static native String nativeVideoFrameMetadataSimulationFallbackSmokeTest(Context context);

  public static native String nativeCodecParametersMultiListenerParitySmokeTest(Context context);

  public static native String nativeRendererAndDeviceStateGetterSmokeTest(Context context);

  public static native String nativePreloadConfigurationSmokeTest(Context context);

  public static native String nativePreloadRoundTripSmokeTest(Context context);

  public static native String nativePreloadBridgeRuntimeSmokeTest(Context context);

  public static native String nativePriorityTaskManagerSmokeTest(Context context);

  public static native String nativePlayerMessageSmokeTest(Context context);

  public static native String nativeTimedPlayerMessageSmokeTest(Context context);

  public static native String nativeRendererPlayerMessageSmokeTest(Context context);

  public static native String nativePlayerMessageCancelSmokeTest(Context context);

  public static native String nativeCurrentMediaItemQuerySmokeTest(Context context);

  public static native String nativePlaylistMetadataSmokeTest(Context context);

  public static native String nativePlaylistMetadataOpaqueTokenSmokeTest(
      Context context,
      String titleToken,
      String artistToken,
      String albumTitleToken,
      String albumArtistToken,
      String displayTitleToken,
      String subtitleToken,
      String descriptionToken,
      String writerToken,
      String authorToken,
      String composerToken,
      String conductorToken,
      String genreToken,
      String compilationToken,
      String stationToken,
      String extrasToken);

  public static native String nativeMediaItemAtSmokeTest(Context context);

  public static native String nativeMediaItemOpaqueTokenSmokeTest(
      Context context, String tagToken, String adsIdToken, String extrasToken);

  public static native String nativeMediaSetOverloadsSmokeTest(Context context);
}
