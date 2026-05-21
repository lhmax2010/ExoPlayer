package androidx.media3.exoplayer.cppbridge;

import android.content.Context;

/** Small native-backed helpers used by instrumentation smoke tests for the bridge JNI layer. */
public final class CppBridgeNativeSmokeTestHelper {

  static {
    // Smoke-only JNI helpers still live in the testhooks library, which links
    // against the core bridge library.
    System.loadLibrary("exoplayer_cppbridge_jni_testhooks");
  }

  private CppBridgeNativeSmokeTestHelper() {}

  public static native String nativeBuildTrackSummaryForTest();

  public static native String nativeObjectValueInfoParsingSmokeTest();

  public static native String nativeMediaItemObjectValueConversionSmokeTest();

  public static native String nativeMediaMetadataObjectValueConversionSmokeTest();

  public static native String nativeTracksSnapshotConversionSmokeTest();

  public static native String nativeTracksFullPayloadConversionSmokeTest();

  public static native String nativeCueSnapshotConversionSmokeTest();

  public static native String nativeListenerPayloadCaptureSmokeTest();

  public static native String nativeBuilderConfigSmokeTest();

  public static native String nativeBuilderBuildSmokeTest(Context context);

  public static native String nativePlayerDoubleReleaseSmokeTest(Context context);

  public static native String nativeListenerLifecycleNegativeSmokeTest(Context context);

  public static native String nativeOpaqueTokenReleaseSmokeTest(
      Context context, String[] tokens);

  public static native String nativeOpaqueTokenBatchReleaseSmokeTest(
      Context context, String firstToken, String secondToken);

  public static native String nativeBuilderPreloadRoundTripSmokeTest(Context context);

  public static native String nativeBuilderMediaSourceFactoryInjectionSmokeTest(Context context);

  public static native String nativeBuilderMediaSourceFactoryFallbackSmokeTest(Context context);

  public static native String nativeBuilderMediaSourceFactoryInjectionReplacementSmokeTest(
      Context context);

  public static native String nativeBuilderMediaSourceFactoryInjectionMultiTokenSmokeTest(
      Context context);

  public static native String nativeBuilderMediaSourceFactoryGeneratedTokenSmokeTest(
      Context context, String token);

  public static native String nativeBuilderAudioOutputProviderInjectionSmokeTest(
      Context context, String token);

  public static native String nativePriorityTaskManagerWrapperSmokeTest(Context context);

  public static native String nativeVideoEffectsConversionSmokeTest(Context context);

  public static native String nativeBuildPlaylistIdsForTest(String[] urls);

  public static native String nativeBuildSubtitleConfigurationsForTest(
      String[] urls, String[] mimeTypes, String[] languages, String[] labels);
}
