package androidx.media3.exoplayer.cppbridge;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.os.Bundle;
import android.text.SpannableString;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.TextureView;
import androidx.media3.common.C;
import androidx.media3.exoplayer.source.DefaultMediaSourceFactory;
import androidx.media3.test.utils.TestUtil;
import androidx.media3.test.utils.WebServerDispatcher;
import androidx.media3.ui.PlayerView;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import java.io.IOException;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicReference;
import okhttp3.mockwebserver.MockWebServer;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public final class CppBridgeNativePlayerInstrumentationTest {

  private static int extractIntMarker(String summary, String marker) {
    int start = summary.indexOf(marker);
    assertThat(start).isAtLeast(0);
    start += marker.length();
    int end = summary.indexOf(',', start);
    String value = end >= 0 ? summary.substring(start, end) : summary.substring(start);
    return Integer.parseInt(value);
  }

  private static void assertCallbackStoppedAfterRemove(String summary) {
    int beforeRemoveCb = extractIntMarker(summary, "beforeRemoveCb=");
    int afterRemoveCb = extractIntMarker(summary, "afterRemoveCb=");
    assertThat(beforeRemoveCb).isAtLeast(2);
    assertThat(afterRemoveCb).isEqualTo(beforeRemoveCb);
    assertThat(summary).contains("callbackStopped=1");
  }

  private static WebServerDispatcher.Resource assetResource(
      Context context, String path, String assetName) throws IOException {
    return new WebServerDispatcher.Resource.Builder()
        .setPath(path)
        .setData(TestUtil.getByteArray(context, assetName))
        .supportsRangeRequests(true)
        .build();
  }

  private static MockWebServer createStreamPlaybackServer(Context context) throws IOException {
    MockWebServer server = new MockWebServer();
    server.setDispatcher(
        WebServerDispatcher.forResources(
            Arrays.asList(
                assetResource(context, "/http/sample.audio.mp4", "sample.audio.mp4"),
                assetResource(context, "/dash/sample.mpd", "sample.mpd"),
                assetResource(context, "/dash/sample.audio.mp4", "sample.audio.mp4"),
                assetResource(context, "/hls/manifest.m3u8", "manifest.m3u8"),
                assetResource(context, "/hls/sd-hls.m3u8", "sd-hls.m3u8"),
                assetResource(context, "/hls/sd-hls0000000000.ts", "sd-hls0000000000.ts"))));
    server.start();
    return server;
  }

  @Test
  public void nativeCreateConfiguredPlayerSnapshotForTest_returnsConfiguredState() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeCreateConfiguredPlayerSnapshotForTest(context);

    assertThat(summary).contains("state=1");
    assertThat(summary).contains("count=1");
    assertThat(summary).contains("index=0");
    assertThat(summary).contains("playWhenReady=1");
    assertThat(summary).contains("repeat=2");
    assertThat(summary).contains("shuffle=1");
    assertThat(summary).contains("volume=0.25");
    assertThat(summary).contains("speed=1.50");
  }

  @Test
  public void nativeCreatePlaylistSnapshotForTest_returnsPlaylistState() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeCreatePlaylistSnapshotForTest(context);

    assertThat(summary).contains("count=2");
    assertThat(summary).contains("index=1");
    assertThat(summary).contains("positionMs=1234");
    assertThat(summary).contains("playWhenReady=0");
    assertThat(summary).contains("state=1");
  }

  @Test
  public void nativeLifecycleSmokeTest_runsThroughLifecycle() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeLifecycleSmokeTest(context);

    assertThat(summary).isEqualTo("lifecycle-ok");
  }

  @Test
  public void nativePostReleaseCallSafetySmokeTest_doesNotCrashOrHang() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativePostReleaseCallSafetySmokeTest(context);

    assertThat(summary).contains("released=1");
    assertThat(summary).contains("state=1");
    assertThat(summary).contains("count=0");
  }

  @Test
  public void nativeTrackSelectionRoundTripForTest_returnsUpdatedParameters() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeTrackSelectionRoundTripForTest(context);

    assertThat(summary).contains("audio=ja");
    assertThat(summary).contains("text=en");
    assertThat(summary).contains("maxAudioChannelCount=6");
    assertThat(summary).contains("maxAudioBitrate=384000");
    assertThat(summary).contains("width=1280");
    assertThat(summary).contains("height=720");
    assertThat(summary).contains("bitrate=2000000");
    assertThat(summary).contains("textDefault=1");
    assertThat(summary).contains("ignoredTextSelectionFlags=2");
    assertThat(summary).contains("selectUndeterminedTextLanguage=1");
    assertThat(summary).contains("lowest=1");
    assertThat(summary).contains("disableVideo=1");
    assertThat(summary).contains("disableAudio=1");
    assertThat(summary).contains("disableText=0");
    assertThat(summary).contains("disabledTrackTypeCount=2");
    assertThat(summary).contains("audioTrackTypeDisabled=1");
    assertThat(summary).contains("videoTrackTypeDisabled=1");
    assertThat(summary).contains("textTrackTypeDisabled=0");
    assertThat(summary).contains("overrideCount=0");
  }

  @Test
  public void nativeSubtitleSmokeTest_returnsSubtitleSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeSubtitleSmokeTest(context);

    assertThat(summary).contains("mediaId=subtitle-item");
    assertThat(summary).contains("subtitleCount=1");
    assertThat(summary).contains("subtitleLanguages=en");
    assertThat(summary).contains("drmScheme=");
  }

  @Test
  public void nativeDrmSmokeTest_returnsDrmSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeDrmSmokeTest(context);

    assertThat(summary).contains("mediaId=drm-item");
    assertThat(summary).contains("drmScheme=edef8ba9-79d6-4ace-a3c8-27dcd51d21ed");
    assertThat(summary).contains("drmLicenseUri=https://license.example.com");
    assertThat(summary).contains("drmHeaderCount=2");
    assertThat(summary).contains("drmHeader0=Authorization:Bearer test-token");
    assertThat(summary).contains("drmForcedSessionTrackTypes=1|2");
    assertThat(summary).contains("drmKeySetIdLength=4");
    assertThat(summary).contains("playClearWithoutKey=0");
  }

  @Test
  public void nativeClippingSmokeTest_returnsClippingSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeClippingSmokeTest(context);

    assertThat(summary).contains("mediaId=clip-item");
    assertThat(summary).contains("clipStartMs=1000");
    assertThat(summary).contains("clipEndMs=5000");
  }

  @Test
  public void nativeLiveConfigurationSmokeTest_returnsLiveSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeLiveConfigurationSmokeTest(context);

    assertThat(summary).contains("mediaId=live-item");
    assertThat(summary).contains("liveTargetOffsetMs=3000");
    assertThat(summary).contains("liveMinOffsetMs=2000");
    assertThat(summary).contains("liveMaxOffsetMs=5000");
    assertThat(summary).contains("liveMinSpeed=0.97");
    assertThat(summary).contains("liveMaxSpeed=1.03");
  }

  @Test
  public void nativeMultiSubtitleSmokeTest_returnsSubtitleAndPreferenceSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeMultiSubtitleSmokeTest(context);

    assertThat(summary).contains("mediaId=multi-sub-item");
    assertThat(summary).contains("subtitleCount=2");
    assertThat(summary).contains("subtitleLanguages=en|zh");
    assertThat(summary).contains("audio=ja");
    assertThat(summary).contains("text=zh");
    assertThat(summary).contains("textDefault=1");
  }

  @Test
  public void nativePlaylistMutationSmokeTest_returnsUpdatedPlaylistState() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativePlaylistMutationSmokeTestV2(context);

    assertThat(summary).contains("playlistMutationV2=1");
    assertThat(summary).contains("count=3");
    assertThat(summary).contains("currentIndex=0");
    assertThat(summary).contains("firstMediaId=item-5");
    assertThat(summary).contains("item0=item-5");
    assertThat(summary).contains("item1=item-r");
    assertThat(summary).contains("item2=item-2");
    assertThat(summary).contains("moveRangeFirstMediaId=item-4");
    assertThat(summary).contains("singleRemoveRestoredCount=3");
    assertThat(summary).contains("nextIndex=1");
    assertThat(summary).contains("previousIndex=-1");
    assertThat(summary).contains("hasNext=1");
    assertThat(summary).contains("hasPrevious=0");
    assertThat(summary).contains("mediaId=item-5");
  }

  @Test
  public void nativeSeekNavigationSmokeTest_runsNavigationCalls() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeSeekNavigationSmokeTest(context);

    assertThat(summary).contains("count=2");
    assertThat(summary).contains("index=1");
    assertThat(summary).contains("positionMs=0");
  }

  @Test
  public void nativeSeekAliasSmokeTest_runsJavaNameParityAliases() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeSeekAliasSmokeTest(context);

    assertThat(summary).contains("afterNextIndex=1");
    assertThat(summary).contains("afterPreviousIndex=0");
  }

  @Test
  public void nativeSeekParametersSmokeTest_roundTripsSeekParameters() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeSeekParametersSmokeTest(context);

    assertThat(summary).contains("beforeUs=1111");
    assertThat(summary).contains("afterUs=2222");
  }

  @Test
  public void nativeAudioAndQuerySmokeTest_returnsAudioAndStateSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAudioAndQuerySmokeTest(context);

    assertThat(summary).contains("usage=1");
    assertThat(summary).contains("contentType=2");
    assertThat(summary).contains("playbackState=1");
    assertThat(summary).contains("playWhenReady=0");
    assertThat(summary).contains("isPlaying=0");
    assertThat(summary).contains("isLoading=0");
    assertThat(summary).contains("playerErrorCode=0");
    assertThat(summary).contains("currentPositionMs=0");
    assertThat(summary).contains("currentMediaItemIndex=0");
    assertThat(summary).contains("mediaItemCount=1");
    assertThat(summary).contains("repeatMode=0");
    assertThat(summary).contains("shuffleModeEnabled=0");
    assertThat(summary).contains("volume=1.000000");
    assertThat(summary).contains("timelineWindows=1");
    assertThat(summary).contains("timelinePeriods=1");
    assertThat(summary).contains("timelineCurrentIndex=0");
    assertThat(summary).contains("timelineHasNext=0");
    assertThat(summary).contains("timelineHasPrevious=0");
    assertThat(summary).contains("timelineSeekable=0");
    assertThat(summary).contains("timelineWindow0Placeholder=1");
    assertThat(summary).contains("timelineWindow0MediaId=audio-query-item");
    assertThat(summary).contains("timelineWindow0MediaUri=https://example.com/audio-query.mp4");
    assertThat(summary).contains("timelineWindow0TagPresent=0");
    assertThat(summary).contains("timelineWindow0TagString=");
    assertThat(summary).contains("timelineWindow0TagTokenPresent=0");
    assertThat(summary).contains("timelineWindow0UidValuePresent=");
    assertThat(summary).contains("timelineWindow0UidValueType=");
    assertThat(summary).contains("timelineWindow0ManifestValuePresent=0");
    assertThat(summary).contains("timelineWindow0ManifestValueType=0");
    assertThat(summary).contains("timelineWindow0PresentationStartMs=");
    assertThat(summary).contains("timelineWindow0WindowStartMs=");
    assertThat(summary).contains("timelineWindow0ElapsedRealtimeEpochOffsetMs=");
    assertThat(summary).contains("timelineWindow0DefaultPositionUs=");
    assertThat(summary).contains("timelineWindow0PositionInFirstPeriodUs=");
    assertThat(summary).contains("timelinePeriod0DurationUs=");
    assertThat(summary).contains("timelinePeriod0PositionInWindowUs=");
    assertThat(summary).contains("timelinePeriod0IdValuePresent=");
    assertThat(summary).contains("timelinePeriod0UidValuePresent=");
    assertThat(summary).contains("timelinePeriod0AdsIdValuePresent=0");
    assertThat(summary).contains("timelinePeriod0AdsIdValueType=0");
    assertThat(summary).contains("bufferedPercentage=");
    assertThat(summary).contains("contentBufferedPositionMs=");
    assertThat(summary).contains("contentDurationMs=");
    assertThat(summary).contains("contentPositionMs=");
    assertThat(summary).contains("currentLiveOffsetMs=");
    assertThat(summary).contains("currentPeriodIndex=0");
    assertThat(summary).contains("maxSeekToPreviousPositionMs=");
    assertThat(summary).contains("playbackSuppressionReason=0");
    assertThat(summary).contains("seekBackIncrementMs=4321");
    assertThat(summary).contains("seekForwardIncrementMs=8765");
    assertThat(summary).contains("speed=1.250000");
    assertThat(summary).contains("pitch=0.800000");
    assertThat(summary).contains("totalBufferedDurationMs=");
    assertThat(summary).contains("commandPlayPauseAvailable=");
    assertThat(summary).contains("canAdvertiseSession=");
    assertThat(summary).contains("applicationLooperThread=");
    assertThat(summary).contains("applicationLooperThreadId=");
    assertThat(summary).contains("applicationLooperCurrentThread=");
    assertThat(summary).contains("currentAdGroupIndex=");
    assertThat(summary).contains("currentAdIndexInGroup=");
    assertThat(summary).contains("isCurrentMediaItemDynamic=1");
    assertThat(summary).contains("isCurrentMediaItemLive=0");
    assertThat(summary).contains("isCurrentMediaItemSeekable=0");
    assertThat(summary).contains("isPlayingAd=0");
    assertThat(summary).contains("cueCount=2");
    assertThat(summary).contains("cuePresentationTimeUs=456789");
    assertThat(summary).contains("cue0Text=Query Cue 1");
    assertThat(summary).contains("cue0TextTokenPresent=1");
    assertThat(summary).contains("cue0BitmapTokenPresent=1");
    assertThat(summary).contains("cue0TextAlignment=2");
    assertThat(summary).contains("cue0MultiRowAlignment=1");
    assertThat(summary).contains("cue0Line=0.250000");
    assertThat(summary).contains("cue0LineType=0");
    assertThat(summary).contains("cue0LineAnchor=1");
    assertThat(summary).contains("cue0Position=0.500000");
    assertThat(summary).contains("cue0PositionAnchor=2");
    assertThat(summary).contains("cue0Size=0.600000");
    assertThat(summary).contains("cue0BitmapHeight=0.750000");
    assertThat(summary).contains("cue0TextSize=18.000000");
    assertThat(summary).contains("cue0ShearDegrees=12.500000");
    assertThat(summary).contains("cue0ZIndex=4");
    assertThat(summary).contains("cue0WindowColorSet=1");
    assertThat(summary).contains("cue0HasBitmap=1");
    assertThat(summary).contains("cue1Text=Query Cue 2");
    assertThat(summary).contains("cue1TextTokenPresent=1");
    assertThat(summary).contains("cue1BitmapTokenPresent=0");
    assertThat(summary).contains("cue1LineType=1");
    assertThat(summary).contains("cue1PositionAnchor=1");
    assertThat(summary).contains("cue1TextSize=22.000000");
    assertThat(summary).contains("cue1TextSizeType=3");
    assertThat(summary).contains("cue1VerticalType=1");
    assertThat(summary).contains("availableCommandCount=");
    assertThat(summary).contains("commands=");
    assertThat(summary).contains("tracksGroupCount=");
    assertThat(summary).contains("trackGroupVectorCount=");
    assertThat(summary).contains("bridgeTracksGroupCount=");
    assertThat(summary).contains("bridgeTrackGroupVectorCount=");
  }

  @Test
  public void nativeCurrentTracksSmokeTest_returnsTracksSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeCurrentTracksSmokeTest(context);

    assertThat(summary).contains("groupCount=2");
    assertThat(summary).contains("containsAudio=1");
    assertThat(summary).contains("containsVideo=1");
    assertThat(summary).contains("containsText=0");
    assertThat(summary).contains("audioSelected=0");
    assertThat(summary).contains("videoSelected=1");
    assertThat(summary).contains("textSelected=0");
    assertThat(summary).contains("audioSupported=1");
    assertThat(summary).contains("videoSupported=1");
    assertThat(summary).contains("textSupported=0");
    assertThat(summary).contains("audioSupportedAllowingExceeds=0");
    assertThat(summary).contains("videoSupportedAllowingExceeds=1");
    assertThat(summary).contains("textSupportedAllowingExceeds=0");
    assertThat(summary).contains("group0Id=video-group");
    assertThat(summary).contains("group0TokenPresent=1");
    assertThat(summary).contains("group0Type=2");
    assertThat(summary).contains("group0TrackCount=2");
    assertThat(summary).contains("group0Supported=1");
    assertThat(summary).contains("track0Id=video-hd");
    assertThat(summary).contains("track0Language=");
    assertThat(summary).contains("track0Label=Main Video");
    assertThat(summary).contains("track0LabelTokenPresent=1");
    assertThat(summary).contains("track0MimeType=video/avc");
    assertThat(summary).contains("track0ContainerMimeType=video/mp4");
    assertThat(summary).contains("track0Codecs=avc1.640028");
    assertThat(summary).contains("track0Bitrate=2500000");
    assertThat(summary).contains("track0AverageBitrate=2000000");
    assertThat(summary).contains("track0PeakBitrate=2500000");
    assertThat(summary).contains("track0MetadataEntryCount=2");
    assertThat(summary).contains("track0MetadataTokenPresent=1");
    assertThat(summary).contains("track0LabelCount=2");
    assertThat(summary).contains("track0Label0Language=en");
    assertThat(summary).contains("track0Label0Value=Main Video");
    assertThat(summary).contains("track0CustomDataTokenPresent=1");
    assertThat(summary).contains("track0AuxiliaryTrackType=2");
    assertThat(summary).contains("track0MaxInputSize=4096");
    assertThat(summary).contains("track0MaxNumReorderSamples=3");
    assertThat(summary).contains("track0InitializationData=2:7");
    assertThat(summary).contains("track0InitializationDataVectorCount=2");
    assertThat(summary).contains("track0DrmSchemeDataCount=1");
    assertThat(summary).contains("track0DrmSchemeType=cenc");
    assertThat(summary).contains("track0DrmSchemeUuid=edef8ba9-79d6-4ace-a3c8-27dcd51d21ed");
    assertThat(summary).contains("track0DrmSchemeLicenseUrl=https://license.example/video");
    assertThat(summary).contains("track0DrmSchemeMimeType=video/mp4");
    assertThat(summary).contains("track0DrmSchemeDataLength=2");
    assertThat(summary).contains("track0DrmSchemeHasData=1");
    assertThat(summary).contains("track0SubsampleOffsetUs=987654");
    assertThat(summary).contains("track0HasPrerollSamples=1");
    assertThat(summary).contains("track0Width=1920");
    assertThat(summary).contains("track0Height=1080");
    assertThat(summary).contains("track0DecodedSize=1936x1096");
    assertThat(summary).contains("track0FrameRate=30.000000");
    assertThat(summary).contains("track0RotationDegrees=90");
    assertThat(summary).contains("track0PixelRatio=1.250000");
    assertThat(summary).contains("track0ProjectionDataLength=4");
    assertThat(summary).contains("track0ProjectionDataVectorLength=4");
    assertThat(summary).contains("track0StereoMode=2");
    assertThat(summary).contains("track0Color=1:2:3");
    assertThat(summary).contains("track0ColorHdrStaticInfoLength=3");
    assertThat(summary).contains("track0ColorBitdepth=10:10");
    assertThat(summary).contains("track0MaxSubLayers=4");
    assertThat(summary).contains("track0PcmEncoding=-1");
    assertThat(summary).contains("track0EncoderTrim=0:0");
    assertThat(summary).contains("track0AccessibilityChannel=-1");
    assertThat(summary).contains("track0CueReplacementBehavior=1");
    assertThat(summary).contains("track0Tiles=5x6");
    assertThat(summary).contains("track0CryptoType=2");
    assertThat(summary).contains("track0RoleFlags=0");
    assertThat(summary).contains("track0SelectionFlags=0");
    assertThat(summary).contains("track0SupportedWithinCapabilities=1");
    assertThat(summary).contains("track0Selected=1");
    assertThat(summary).contains("track0FormatSupport=1");
    assertThat(summary).contains("group1Id=audio-group");
    assertThat(summary).contains("group1TokenPresent=1");
    assertThat(summary).contains("group1Type=1");
    assertThat(summary).contains("group1TrackCount=1");
    assertThat(summary).contains("group1Supported=1");
    assertThat(summary).contains("group1Track0Id=audio-main");
    assertThat(summary).contains("group1Track0Language=en");
    assertThat(summary).contains("group1Track0Label=Main Audio");
    assertThat(summary).contains("group1Track0LabelTokenPresent=1");
    assertThat(summary).contains("group1Track0MimeType=audio/mp4a-latm");
    assertThat(summary).contains("group1Track0Bitrate=192000");
    assertThat(summary).contains("group1Track0AverageBitrate=160000");
    assertThat(summary).contains("group1Track0PeakBitrate=192000");
    assertThat(summary).contains("group1Track0MetadataEntryCount=1");
    assertThat(summary).contains("group1Track0InitializationData=1:3");
    assertThat(summary).contains("group1Track0PcmEncoding=2");
    assertThat(summary).contains("group1Track0EncoderTrim=12:34");
  }

  @Test
  public void nativeCurrentTimelineSmokeTest_returnsTimelineDetails() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeCurrentTimelineSmokeTest(context);

    assertThat(summary).contains("windowCount=2");
    assertThat(summary).contains("periodCount=2");
    assertThat(summary).contains("empty=0");
    assertThat(summary).contains("currentMediaItemIndex=0");
    assertThat(summary).contains("nextMediaItemIndex=1");
    assertThat(summary).contains("previousMediaItemIndex=-1");
    assertThat(summary).contains("hasNext=1");
    assertThat(summary).contains("hasPrevious=0");
    assertThat(summary).contains("windowSnapshotCount=2");
    assertThat(summary).contains("periodSnapshotCount=2");
    assertThat(summary).contains("window0MediaItemIndex=0");
    assertThat(summary).contains("window0MediaId=timeline-query-item-1");
    assertThat(summary).contains("window0MediaUri=https://example.com/current-timeline-one.m3u8");
    assertThat(summary).contains("window0TagPresent=1");
    assertThat(summary).contains("window0TagString=timeline-query-tag-1");
    assertThat(summary).contains("window0TagTokenPresent=1");
    assertThat(summary).contains("window0Uid=");
    assertThat(summary).contains("window0UidTokenPresent=");
    assertThat(summary).contains("window0UidValuePresent=1");
    assertThat(summary).contains("window0UidValueClass=java.lang.String");
    assertThat(summary).contains("window0UidValueType=1");
    assertThat(summary).contains("window0UidValueString=window-uid-0");
    assertThat(summary).contains("window0LiveConfigurationPresent=");
    assertThat(summary).contains("window0LiveTargetOffsetMs=");
    assertThat(summary).contains("window0LiveMinOffsetMs=");
    assertThat(summary).contains("window0LiveMaxOffsetMs=");
    assertThat(summary).contains("window0LiveMinSpeed=");
    assertThat(summary).contains("window0LiveMaxSpeed=");
    assertThat(summary).contains("window0ManifestPresent=0");
    assertThat(summary).contains("window0ManifestString=");
    assertThat(summary).contains("window0ManifestTokenPresent=0");
    assertThat(summary).contains("window0ManifestValuePresent=0");
    assertThat(summary).contains("window0ManifestValueType=0");
    assertThat(summary).contains("window0FirstPeriodIndex=0");
    assertThat(summary).contains("window0LastPeriodIndex=0");
    assertThat(summary).contains("window0PresentationStartTimeMs=");
    assertThat(summary).contains("window0WindowStartTimeMs=");
    assertThat(summary).contains("window0ElapsedRealtimeEpochOffsetMs=");
    assertThat(summary).contains("window0DurationMs=");
    assertThat(summary).contains("window0DefaultPositionMs=");
    assertThat(summary).contains("window0PositionInFirstPeriodMs=");
    assertThat(summary).contains("window0PositionInFirstPeriodUs=");
    assertThat(summary).contains("window0Seekable=");
    assertThat(summary).contains("window0Dynamic=");
    assertThat(summary).contains("window0Live=");
    assertThat(summary).contains("window0Placeholder=");
    assertThat(summary).contains("window0DefaultPositionUs=");
    assertThat(summary).contains("window0DurationUs=");
    assertThat(summary).contains("window1MediaItemIndex=1");
    assertThat(summary).contains("window1MediaId=timeline-query-item-2");
    assertThat(summary).contains("window1MediaUri=https://example.com/current-timeline-two.mp4");
    assertThat(summary).contains("window1TagPresent=1");
    assertThat(summary).contains("window1TagString=timeline-query-tag-2");
    assertThat(summary).contains("window1TagTokenPresent=1");
    assertThat(summary).contains("window1LiveConfigurationPresent=1");
    assertThat(summary).contains("window1LiveTargetOffsetMs=7100");
    assertThat(summary).contains("window1LiveMinOffsetMs=6400");
    assertThat(summary).contains("window1LiveMaxOffsetMs=8200");
    assertThat(summary).contains("window1LiveMinSpeed=0.930000");
    assertThat(summary).contains("window1LiveMaxSpeed=1.070000");
    assertThat(summary).contains("window1UidValuePresent=1");
    assertThat(summary).contains("window1UidValueClass=java.lang.String");
    assertThat(summary).contains("window1UidValueType=1");
    assertThat(summary).contains("window1UidValueString=window-uid-1");
    assertThat(summary).contains("window1ManifestPresent=1");
    assertThat(summary).contains("window1ManifestString=timeline-query-manifest");
    assertThat(summary).contains("window1ManifestTokenPresent=1");
    assertThat(summary).contains("window1ManifestValuePresent=1");
    assertThat(summary).contains("window1ManifestValueClass=java.lang.String");
    assertThat(summary).contains("window1ManifestValueType=1");
    assertThat(summary).contains("window1ManifestValueString=timeline-query-manifest");
    assertThat(summary).contains("window1FirstPeriodIndex=");
    assertThat(summary).contains("window1LastPeriodIndex=");
    assertThat(summary).contains("window1DurationMs=");
    assertThat(summary).contains("window1DefaultPositionMs=");
    assertThat(summary).contains("window1Seekable=");
    assertThat(summary).contains("window1Uid=");
    assertThat(summary).contains("window1UidTokenPresent=");
    assertThat(summary).contains("window1Dynamic=");
    assertThat(summary).contains("window1Live=");
    assertThat(summary).contains("window1Placeholder=");
    assertThat(summary).contains("window1DefaultPositionUs=");
    assertThat(summary).contains("window1DurationUs=");
    assertThat(summary).contains("period0Id=");
    assertThat(summary).contains("period0IdTokenPresent=1");
    assertThat(summary).contains("period0IdValuePresent=1");
    assertThat(summary).contains("period0IdValueClass=java.lang.String");
    assertThat(summary).contains("period0IdValueType=1");
    assertThat(summary).contains("period0IdValueString=period-0");
    assertThat(summary).contains("period0Uid=");
    assertThat(summary).contains("period0UidTokenPresent=1");
    assertThat(summary).contains("period0UidValuePresent=1");
    assertThat(summary).contains("period0UidValueClass=java.lang.String");
    assertThat(summary).contains("period0UidValueType=1");
    assertThat(summary).contains("period0UidValueString=period-uid-0");
    assertThat(summary).contains("period0AdsId=");
    assertThat(summary).contains("period0AdsIdTokenPresent=");
    assertThat(summary).contains("period0AdsIdValuePresent=0");
    assertThat(summary).contains("period0AdsIdValueType=0");
    assertThat(summary).contains("period0WindowIndex=");
    assertThat(summary).contains("period0AdGroupCount=");
    assertThat(summary).contains("period0DurationMs=");
    assertThat(summary).contains("period0DurationUs=");
    assertThat(summary).contains("period0PositionInWindowMs=");
    assertThat(summary).contains("period0PositionInWindowUs=");
    assertThat(summary).contains("period0Placeholder=");
    assertThat(summary).contains("period1Id=");
    assertThat(summary).contains("period1IdTokenPresent=1");
    assertThat(summary).contains("period1IdValuePresent=1");
    assertThat(summary).contains("period1IdValueClass=java.lang.String");
    assertThat(summary).contains("period1IdValueType=1");
    assertThat(summary).contains("period1IdValueString=period-1");
    assertThat(summary).contains("period1Uid=");
    assertThat(summary).contains("period1UidTokenPresent=");
    assertThat(summary).contains("period1UidValuePresent=1");
    assertThat(summary).contains("period1UidValueClass=java.lang.String");
    assertThat(summary).contains("period1UidValueType=1");
    assertThat(summary).contains("period1UidValueString=period-uid-1");
    assertThat(summary).contains("period1AdsId=period-ads-1");
    assertThat(summary).contains("period1AdsIdTokenPresent=1");
    assertThat(summary).contains("period1AdsIdValuePresent=1");
    assertThat(summary).contains("period1AdsIdValueClass=java.lang.String");
    assertThat(summary).contains("period1AdsIdValueType=1");
    assertThat(summary).contains("period1AdsIdValueString=period-ads-1");
    assertThat(summary).contains("period1WindowIndex=");
    assertThat(summary).contains("period1AdGroupCount=");
    assertThat(summary).contains("period1DurationMs=");
    assertThat(summary).contains("period1DurationUs=");
    assertThat(summary).contains("period1PositionInWindowMs=");
    assertThat(summary).contains("period1PositionInWindowUs=");
    assertThat(summary).contains("period1Placeholder=");
  }

  @Test
  public void nativeAvailableCommandsSmokeTest_returnsContainsStyleSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAvailableCommandsSmokeTest(context);

    assertThat(summary).contains("count=");
    assertThat(summary).contains("containsPlayPause=");
    assertThat(summary).contains("containsGetTimeline=");
    assertThat(summary).contains("containsGetTracks=");
  }

  @Test
  public void nativeSurfaceBridgeSmokeTest_runsSurfaceCalls() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    SurfaceTexture surfaceTexture = new SurfaceTexture(0);
    Surface surface = new Surface(surfaceTexture);

    try {
      String summary =
          CppBridgeNativePlayerTestHelper.nativeSurfaceBridgeSmokeTest(
              context, surface, new SurfaceView(context), new TextureView(context));

      assertThat(summary).isEqualTo("surface-ok");
    } finally {
      surface.release();
      surfaceTexture.release();
    }
  }

  @Test
  public void nativePlayerViewBridgeSmokeTest_bindsAndUnbindsPlayerView() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    AtomicReference<PlayerView> playerViewRef = new AtomicReference<>();
    InstrumentationRegistry.getInstrumentation()
        .runOnMainSync(() -> playerViewRef.set(new PlayerView(context)));

    String summary =
        CppBridgeNativePlayerTestHelper.nativePlayerViewBridgeSmokeTest(
            context, playerViewRef.get());

    assertThat(summary).contains("bound=1");
    assertThat(summary).contains("unbound=1");
  }

  @Test
  public void nativeListenerSmokeTest_reportsExtendedCallbacks() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeListenerSmokeTest(context);

    assertThat(summary).contains("repeatMode=2");
    assertThat(summary).contains("shuffle=1");
    assertThat(summary).contains("text=en");
    assertThat(summary).contains("speed=1.100000");
    assertThat(summary).contains("pitch=0.900000");
    assertThat(summary).contains("repeatCb=1");
    assertThat(summary).contains("shuffleCb=1");
    assertThat(summary).contains("trackCb=1");
    assertThat(summary).contains("playbackParamsCb=1");
    assertThat(summary).contains("timelineCb=1");
    assertThat(summary).contains("timelineWindowCount=2");
    assertThat(summary).contains("timelinePeriodCount=2");
    assertThat(summary).contains("timelineCurrentMediaItemIndex=1");
    assertThat(summary).contains("tracksChangedCb=1");
    assertThat(summary).contains("positionDiscontinuityCb=1");
    assertThat(summary).contains("isLoadingCb=1");
    assertThat(summary).contains("isLoading=1");
    assertThat(summary).contains("timelineWindow0MediaId=listener-item-1");
    assertThat(summary).contains("timelineWindow0TagPresent=1");
    assertThat(summary).contains("timelineWindow0TagString=listener-tag-1");
    assertThat(summary).contains("timelineWindow0TagTokenPresent=1");
    assertThat(summary).contains("timelineWindow1MediaItemIndex=1");
    assertThat(summary).contains("timelineWindow1MediaId=listener-item-2");
    assertThat(summary).contains("timelineWindow1TagString=listener-tag-2");
    assertThat(summary).contains("timelineWindow1TagTokenPresent=1");
    assertThat(summary).contains("timelineWindow1LiveConfigurationPresent=1");
    assertThat(summary).contains("timelineWindow1LiveTargetOffsetMs=6100");
    assertThat(summary).contains("timelineWindow1LiveMinOffsetMs=5200");
    assertThat(summary).contains("timelineWindow1LiveMaxOffsetMs=7800");
    assertThat(summary).contains("timelineWindow1LiveMinSpeed=0.940000");
    assertThat(summary).contains("timelineWindow1LiveMaxSpeed=1.080000");
    assertThat(summary).contains("firstTrackGroupTokenPresent=1");
    assertThat(summary).contains("secondTrackGroupId=");
    assertThat(summary).contains("secondTrackLabel=");
    assertThat(summary).contains("secondTrackLanguage=");
    assertThat(summary).contains("secondTrackMimeType=");
    assertThat(summary).contains("secondTrackAccessibilityChannel=");
    assertThat(summary).contains("secondTrackRoleFlags=");
    assertThat(summary).contains("secondTrackSelectionFlags=");
    assertThat(summary).contains("secondTrackSelected=");
    assertThat(summary).contains("secondTrackSupported=");
    assertThat(summary).contains("secondTrackSupportedWithinCapabilities=");
    assertThat(summary).contains("mediaMetadataCb=1");
    assertThat(summary).contains("mediaMetadataAlbumTitle=Listener Item Album");
    assertThat(summary).contains("mediaMetadataWriter=Listener Item Writer");
    assertThat(summary).contains("mediaMetadataGenre=Listener Item Genre");
    assertThat(summary).contains("mediaMetadataExtrasPresent=1");
    assertThat(summary).contains("mediaMetadataExtrasKeyCount=1");
    assertThat(summary).contains("mediaMetadataExtrasTokenPresent=1");
    assertThat(summary).contains("mediaMetadataArtworkUri=https://example.com/listener-item-artwork.jpg");
    assertThat(summary).contains("mediaMetadataArtworkDataLength=4");
    assertThat(summary).contains("mediaMetadataArtworkDataType=6");
    assertThat(summary).contains("playlistMetadataCb=1");
    assertThat(summary).contains("playlistMetadataAlbumTitle=Listener Playlist Album");
    assertThat(summary).contains("playlistMetadataConductor=Listener Playlist Conductor");
    assertThat(summary).contains("playlistMetadataStation=Listener Playlist Station");
    assertThat(summary).contains("playlistMetadataExtrasPresent=1");
    assertThat(summary).contains("playlistMetadataExtrasKeyCount=1");
    assertThat(summary).contains("playlistMetadataExtrasTokenPresent=1");
    assertThat(summary).contains("playlistMetadataArtworkUri=https://example.com/listener-playlist-artwork.jpg");
    assertThat(summary).contains("playlistMetadataArtworkDataLength=3");
    assertThat(summary).contains("playlistMetadataArtworkDataType=8");
    assertThat(summary).contains("cueCb=1");
    assertThat(summary).contains("cue0Text=Listener Cue 1");
    assertThat(summary).contains("cue0TextAlignment=2");
    assertThat(summary).contains("cue0Line=0.250000");
    assertThat(summary).contains("cue1Text=Listener Cue 2");
    assertThat(summary).contains("cue1LineType=1");
    assertThat(summary).contains("cue1TextSize=22.000000");
    assertThat(summary).contains("cue1VerticalType=1");
    assertThat(summary).contains("oldTagTokenPresent=1");
    assertThat(summary).contains("newPositionMediaItemIndex=1");
    assertThat(summary).contains("newPositionMs=3456");
    assertThat(summary).contains("newMediaId=listener-item-2");
    assertThat(summary).contains("newTagTokenPresent=1");
  }

  @Test
  public void nativeListenerCallbackDetailSmokeTest_reportsSupplementalCallbacks() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeListenerCallbackDetailSmokeTest(context);

    assertThat(summary).contains("seekBackIncrementChangedCb=1");
    assertThat(summary).contains("seekBackIncrementChangedMs=15000");
    assertThat(summary).contains("seekForwardIncrementChangedCb=1");
    assertThat(summary).contains("seekForwardIncrementChangedMs=25000");
    assertThat(summary).contains("maxSeekToPreviousPositionChangedCb=1");
    assertThat(summary).contains("maxSeekToPreviousPositionChangedMs=12000");
    assertThat(summary).contains("playbackSuppressionReason=");
    assertThat(summary).contains("playbackSuppressionCb=0");
    assertThat(summary).contains("availableCommandsCb=1");
    assertThat(summary).contains("availableCommandsCount=");
    assertThat(summary).contains("eventsCb=1");
    assertThat(summary).contains("eventCount=");
  }

  @Test
  public void nativeListenerMetadataCueDetailSmokeTest_reportsSupplementalPayloads() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeListenerMetadataCueDetailSmokeTest(context);

    assertThat(summary).contains("mediaMetadataTitle=Listener Item Title");
    assertThat(summary).contains("mediaMetadataTitleTokenPresent=1");
    assertThat(summary).contains("mediaMetadataArtist=Listener Item Artist");
    assertThat(summary).contains("mediaMetadataArtistTokenPresent=1");
    assertThat(summary).contains("mediaMetadataAlbumArtist=Listener Item Album Artist");
    assertThat(summary).contains("mediaMetadataAlbumArtistTokenPresent=1");
    assertThat(summary).contains("mediaMetadataDisplayTitle=Listener Item Display");
    assertThat(summary).contains("mediaMetadataDisplayTitleTokenPresent=1");
    assertThat(summary).contains("mediaMetadataSubtitle=Listener Item Subtitle");
    assertThat(summary).contains("mediaMetadataSubtitleTokenPresent=1");
    assertThat(summary).contains("mediaMetadataDescription=Listener Item Description");
    assertThat(summary).contains("mediaMetadataDescriptionTokenPresent=1");
    assertThat(summary).contains("mediaMetadataAuthor=Listener Item Author");
    assertThat(summary).contains("mediaMetadataAuthorTokenPresent=1");
    assertThat(summary).contains("mediaMetadataComposer=Listener Item Composer");
    assertThat(summary).contains("mediaMetadataComposerTokenPresent=1");
    assertThat(summary).contains("mediaMetadataConductor=Listener Item Conductor");
    assertThat(summary).contains("mediaMetadataConductorTokenPresent=1");
    assertThat(summary).contains("mediaMetadataCompilation=Listener Item Compilation");
    assertThat(summary).contains("mediaMetadataCompilationTokenPresent=1");
    assertThat(summary).contains("mediaMetadataStation=Listener Item Station");
    assertThat(summary).contains("mediaMetadataStationTokenPresent=1");
    assertThat(summary).contains("mediaMetadataMediaType=5");
    assertThat(summary).contains("playlistMetadataTitle=Listener Playlist");
    assertThat(summary).contains("playlistMetadataArtist=Listener Artist");
    assertThat(summary).contains("playlistMetadataAlbumArtist=Listener Playlist Album Artist");
    assertThat(summary).contains("playlistMetadataDisplayTitle=Listener Playlist Display");
    assertThat(summary).contains("playlistMetadataSubtitle=Listener Playlist Subtitle");
    assertThat(summary).contains("playlistMetadataDescription=Listener Playlist Description");
    assertThat(summary).contains("playlistMetadataWriter=Listener Playlist Writer");
    assertThat(summary).contains("playlistMetadataAuthor=Listener Playlist Author");
    assertThat(summary).contains("playlistMetadataComposer=Listener Playlist Composer");
    assertThat(summary).contains("playlistMetadataCompilation=Listener Playlist Compilation");
    assertThat(summary).contains("playlistMetadataGenre=Listener Playlist Genre");
    assertThat(summary).contains("playlistMetadataMediaType=6");
    assertThat(summary).contains("cueCount=2");
    assertThat(summary).contains("cuePresentationTimeUs=567890");
    assertThat(summary).contains("cue0TextTokenPresent=1");
    assertThat(summary).contains("cue0BitmapTokenPresent=1");
    assertThat(summary).contains("cue0MultiRowAlignment=1");
    assertThat(summary).contains("cue0LineType=0");
    assertThat(summary).contains("cue0PositionAnchor=2");
    assertThat(summary).contains("cue1TextTokenPresent=1");
    assertThat(summary).contains("cue1BitmapTokenPresent=0");
    assertThat(summary).contains("cue1PositionAnchor=1");
    assertThat(summary).contains("cue1TextSizeType=3");
    assertThat(summary).contains("oldPositionMediaItemIndex=");
    assertThat(summary).contains("oldPositionPeriodIndex=");
    assertThat(summary).contains("oldPositionMs=");
    assertThat(summary).contains("oldContentPositionMs=");
    assertThat(summary).contains("oldAdGroupIndex=");
    assertThat(summary).contains("oldAdIndexInAdGroup=");
    assertThat(summary).contains("oldMediaId=");
    assertThat(summary).contains("newPositionPeriodIndex=1");
    assertThat(summary).contains("newContentPositionMs=3456");
    assertThat(summary).contains("newAdGroupIndex=-1");
    assertThat(summary).contains("newAdIndexInAdGroup=-1");
  }

  @Test
  public void nativeListenerDetachSmokeTest_stopsCallbacksAfterRemoval() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeListenerDetachSmokeTest(context);

    assertThat(summary).contains("repeatBeforeDetach=1");
    assertThat(summary).contains("repeatAfterDetach=1");
    assertThat(summary).contains("shuffleAfterDetach=0");
    assertThat(summary).contains("playbackParamsAfterDetach=0");
  }

  @Test
  public void nativeDeviceAndSkipSilenceSmokeTest_returnsDeviceSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeDeviceAndSkipSilenceSmokeTest(context);

    assertThat(summary).contains("deviceType=");
    assertThat(summary).contains("minVol=");
    assertThat(summary).contains("maxVol=");
    assertThat(summary).contains("routingControllerId=");
    assertThat(summary).contains("deviceVol=");
    assertThat(summary).contains("muted=");
    assertThat(summary).contains("skipSilence=");
    assertThat(summary).contains("deviceControlCalls=1");
  }

  @Test
  public void nativeVideoAndMetadataSmokeTest_returnsQuerySummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeVideoAndMetadataSmokeTest(context);

    assertThat(summary).contains("videoWidth=");
    assertThat(summary).contains("videoHeight=");
    assertThat(summary).contains("videoUnappliedRotationDegrees=0");
    assertThat(summary).contains("videoPixelWidthHeightRatio=");
    assertThat(summary).contains("mediaTitle=Video Metadata Title");
    assertThat(summary).contains("mediaTitleTokenPresent=1");
    assertThat(summary).contains("mediaArtist=Video Metadata Artist");
    assertThat(summary).contains("mediaArtistTokenPresent=1");
    assertThat(summary).contains("mediaDisplayTitle=Video Metadata Display");
    assertThat(summary).contains("mediaDisplayTitleTokenPresent=1");
    assertThat(summary).contains("mediaDescription=Video Metadata Description");
    assertThat(summary).contains("mediaAuthor=");
    assertThat(summary).contains("mediaGenre=Video Metadata Genre");
    assertThat(summary).contains("mediaWriter=Video Metadata Writer");
    assertThat(summary).contains("mediaConductor=Video Metadata Conductor");
    assertThat(summary).contains("mediaDurationMs=654321");
    assertThat(summary).contains("mediaTrackNumber=7");
    assertThat(summary).contains("mediaTotalTrackCount=12");
    assertThat(summary).contains("mediaDiscNumber=1");
    assertThat(summary).contains("mediaTotalDiscCount=3");
    assertThat(summary).contains("mediaIsPlayable=1");
    assertThat(summary).contains("mediaFolderType=4");
    assertThat(summary).contains("mediaReleaseYear=2024");
    assertThat(summary).contains("mediaReleaseMonth=6");
    assertThat(summary).contains("mediaReleaseDay=14");
    assertThat(summary).contains("mediaType=1");
    assertThat(summary).contains("mediaStation=Video Metadata Station");
    assertThat(summary).contains("mediaCompilation=Video Metadata Compilation");
    assertThat(summary).contains("mediaExtrasPresent=1");
    assertThat(summary).contains("mediaExtrasKeyCount=1");
    assertThat(summary).contains("mediaExtrasTokenPresent=1");
    assertThat(summary).contains("mediaArtworkUri=https://example.com/video-metadata-artwork.jpg");
    assertThat(summary).contains("mediaArtworkDataLength=4");
    assertThat(summary).contains("mediaArtworkDataType=3");
    assertThat(summary).contains("playlistTitle=Metadata Playlist Title");
    assertThat(summary).contains("playlistTitleTokenPresent=1");
    assertThat(summary).contains("playlistArtist=Metadata Playlist Artist");
    assertThat(summary).contains("playlistArtistTokenPresent=1");
    assertThat(summary).contains("playlistDisplayTitle=Metadata Playlist Display");
    assertThat(summary).contains("playlistDisplayTitleTokenPresent=1");
    assertThat(summary).contains("playlistGenre=Metadata Playlist Genre");
    assertThat(summary).contains("playlistRecordingYear=2023");
    assertThat(summary).contains("playlistRecordingMonth=9");
    assertThat(summary).contains("playlistRecordingDay=18");
    assertThat(summary).contains("playlistReleaseMonth=10");
    assertThat(summary).contains("playlistReleaseDay=4");
    assertThat(summary).contains("playlistAuthor=Metadata Playlist Author");
    assertThat(summary).contains("playlistComposer=Metadata Playlist Composer");
    assertThat(summary).contains("playlistConductor=Metadata Playlist Conductor");
    assertThat(summary).contains("playlistDiscNumber=4");
    assertThat(summary).contains("playlistTotalDiscCount=8");
    assertThat(summary).contains("playlistIsBrowsable=1");
    assertThat(summary).contains("playlistFolderType=2");
    assertThat(summary).contains("playlistMediaType=1");
    assertThat(summary).contains("playlistCompilation=Metadata Playlist Compilation");
    assertThat(summary).contains("playlistExtrasPresent=1");
    assertThat(summary).contains("playlistExtrasKeyCount=1");
    assertThat(summary).contains("playlistExtrasTokenPresent=1");
    assertThat(summary).contains("playlistArtworkUri=https://example.com/metadata-playlist-artwork.jpg");
    assertThat(summary).contains("playlistArtworkDataLength=3");
    assertThat(summary).contains("playlistArtworkDataType=4");
  }

  @Test
  public void nativeAnalyticsSmokeTest_returnsAnalyticsSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAnalyticsSmokeTest(context);

    assertThat(summary).contains("bitrate=");
    assertThat(summary).contains("dropped=");
    assertThat(summary).contains("loadStarted=");
    assertThat(summary).contains("loadCompleted=");
    assertThat(summary).contains("loadDelta=");
    assertThat(summary).contains("audioMime=");
    assertThat(summary).contains("videoMime=");
  }

  @Test
  public void nativeAnalyticsCallbackSmokeTest_reportsListenerDelivery() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAnalyticsCallbackSmokeTest(context);

    assertThat(extractIntMarker(summary, "analyticsCb=")).isAtLeast(2);
    assertThat(summary).contains("bitrate=2222222");
    assertThat(summary).contains("dropped=7");
    assertThat(summary).contains("loadStarted=5");
    assertThat(summary).contains("loadCompleted=3");
    assertThat(summary).contains("loadDelta=2");
    assertThat(summary).contains("audioMime=audio/final");
    assertThat(summary).contains("videoMime=video/final");
  }

  @Test
  public void nativeAnalyticsListenerRegistrationSmokeTest_addsAndRemovesAnalyticsOnlyListener() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsListenerRegistrationSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("bitrate=8765432");
    assertThat(summary).contains("dropped=4");
    assertThat(summary).contains("loadStarted=6");
    assertThat(summary).contains("loadCompleted=4");
    assertThat(summary).contains("loadDelta=2");
    assertThat(summary).contains("audioMime=audio/final");
    assertThat(summary).contains("videoMime=video/final");
  }

  @Test
  public void nativeAnalyticsAudioUnderrunSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsAudioUnderrunSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("bufferSize=4096");
    assertThat(summary).contains("bufferSizeMs=87");
    assertThat(summary).contains("elapsedSinceLastFeedMs=23");
  }

  @Test
  public void nativeAnalyticsDroppedVideoFramesSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsDroppedVideoFramesSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("droppedFrames=8");
    assertThat(summary).contains("elapsedMs=41");
  }

  @Test
  public void nativeAnalyticsBandwidthEstimateSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsBandwidthEstimateSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("elapsedMs=34");
    assertThat(summary).contains("bytesTransferred=67890");
    assertThat(summary).contains("bitrateEstimate=999999");
  }

  @Test
  public void nativeAnalyticsLoadStartedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAnalyticsLoadStartedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("uri=https://example.com/analytics-final.m3u8");
    assertThat(summary).contains("dataType=3");
    assertThat(summary).contains("trackType=1");
    assertThat(summary).contains("retryCount=2");
  }

  @Test
  public void nativeAnalyticsLoadCompletedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsLoadCompletedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("uri=https://example.com/analytics-final-complete.m3u8");
    assertThat(summary).contains("dataType=4");
    assertThat(summary).contains("trackType=1");
  }

  @Test
  public void nativeAnalyticsAudioInputFormatChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsAudioInputFormatChangedSmokeTest(
            context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("sampleMimeType=audio/final");
    assertThat(summary).contains("codecs=ec-3");
    assertThat(summary).contains("channelCount=6");
    assertThat(summary).contains("sampleRate=48000");
  }

  @Test
  public void nativeAnalyticsAudioDecoderInitializedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsAudioDecoderInitializedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("decoderName=c2.android.eac3.decoder");
    assertThat(summary).contains("initializedTimestampMs=222");
    assertThat(summary).contains("initializationDurationMs=19");
  }

  @Test
  public void nativeAnalyticsVideoDecoderInitializedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsVideoDecoderInitializedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("decoderName=c2.android.hevc.decoder");
    assertThat(summary).contains("initializedTimestampMs=444");
    assertThat(summary).contains("initializationDurationMs=29");
  }

  @Test
  public void nativeAnalyticsAudioDecoderReleasedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsAudioDecoderReleasedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("decoderName=c2.android.eac3.decoder");
  }

  @Test
  public void nativeAnalyticsVideoDecoderReleasedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsVideoDecoderReleasedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("decoderName=c2.android.hevc.decoder");
  }

  @Test
  public void nativeAnalyticsRenderedFirstFrameSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsRenderedFirstFrameSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("renderTimeMs=456");
  }

  @Test
  public void nativeAnalyticsVideoSizeChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsVideoSizeChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("width=1920");
    assertThat(summary).contains("height=1080");
    assertThat(summary).contains("pixelWidthHeightRatio=1.250000");
  }

  @Test
  public void nativeAnalyticsAudioPositionAdvancingSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsAudioPositionAdvancingSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("playoutStartSystemTimeMs=2222");
  }

  @Test
  public void nativeAnalyticsVideoFrameProcessingOffsetSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsVideoFrameProcessingOffsetSmokeTest(
            context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("totalProcessingOffsetUs=67890");
    assertThat(summary).contains("frameCount=8");
  }

  @Test
  public void nativeAnalyticsVolumeChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsVolumeChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("volume=0.750000");
  }

  @Test
  public void nativeAnalyticsAudioSessionIdChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsAudioSessionIdChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("audioSessionId=700042");
  }

  @Test
  public void nativeAnalyticsSkipSilenceEnabledChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsSkipSilenceEnabledChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("skipSilenceEnabled=1");
  }

  @Test
  public void nativeAnalyticsDeviceVolumeChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsDeviceVolumeChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("volume=7");
    assertThat(summary).contains("muted=0");
  }

  @Test
  public void nativeAnalyticsPlaybackStateChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsPlaybackStateChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("playbackState=3");
  }

  @Test
  public void nativeAnalyticsIsPlayingChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsIsPlayingChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("isPlaying=1");
  }

  @Test
  public void nativeAnalyticsPlayWhenReadyChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsPlayWhenReadyChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("playWhenReady=1");
    assertThat(summary).contains("reason=2");
  }

  @Test
  public void
      nativeAnalyticsPlaybackSuppressionReasonChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper
            .nativeAnalyticsPlaybackSuppressionReasonChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("playbackSuppressionReason=1");
  }

  @Test
  public void nativeAnalyticsIsLoadingChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsIsLoadingChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("isLoading=1");
  }

  @Test
  public void nativeAnalyticsRepeatModeChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsRepeatModeChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("repeatMode=2");
  }

  @Test
  public void nativeAnalyticsShuffleModeChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsShuffleModeChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("shuffleModeEnabled=1");
  }

  @Test
  public void nativeAnalyticsPlaybackParametersChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsPlaybackParametersChangedSmokeTest(
            context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("speed=1.500000");
    assertThat(summary).contains("pitch=0.750000");
  }

  @Test
  public void nativeAnalyticsAvailableCommandsChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsAvailableCommandsChangedSmokeTest(
            context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("commandCount=3");
    assertThat(summary).contains("firstCommand=3");
    assertThat(summary).contains("contains8=1");
  }

  @Test
  public void nativeAnalyticsEventsSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAnalyticsEventsSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("eventCount=3");
    assertThat(summary).contains("firstEvent=7");
    assertThat(summary).contains("contains9=1");
  }

  @Test
  public void nativeAnalyticsSeekBackIncrementChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsSeekBackIncrementChangedSmokeTest(
            context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("seekBackIncrementMs=15000");
  }

  @Test
  public void nativeAnalyticsSeekForwardIncrementChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsSeekForwardIncrementChangedSmokeTest(
            context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("seekForwardIncrementMs=25000");
  }

  @Test
  public void nativeAnalyticsMaxSeekToPreviousPositionChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper
            .nativeAnalyticsMaxSeekToPreviousPositionChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("maxSeekToPreviousPositionMs=12000");
  }

  @Test
  public void nativeAnalyticsTimelineChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsTimelineChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("reason=2");
  }

  @Test
  public void nativeAnalyticsPositionDiscontinuitySmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsPositionDiscontinuitySmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("reason=5");
  }

  @Test
  public void nativeAnalyticsSeekStartedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAnalyticsSeekStartedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("started=1");
  }

  @Test
  public void nativeAnalyticsPlayerErrorSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAnalyticsPlayerErrorSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("errorCode=2002");
    assertThat(summary).contains("message=analytics-final-error");
  }

  @Test
  public void nativeAnalyticsPlayerErrorChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsPlayerErrorChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("errorCode=4004");
    assertThat(summary).contains("message=analytics-final-changed");
  }

  @Test
  public void nativeAnalyticsTracksChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAnalyticsTracksChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("groupCount=2");
    assertThat(summary).contains("firstGroupType=2");
    assertThat(summary).contains("firstGroupId=video-main");
    assertThat(summary).contains("firstGroupTokenPresent=1");
    assertThat(summary).contains("firstTrackCount=1");
    assertThat(summary).contains("containsAudio=1");
    assertThat(summary).contains("containsVideo=1");
    assertThat(summary).contains("videoSelected=1");
  }

  @Test
  public void nativeAnalyticsMediaItemTransitionSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsMediaItemTransitionSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("mediaId=analytics-transition-final");
    assertThat(summary).contains("sourceType=2");
    assertThat(summary).contains("reason=2");
  }

  @Test
  public void nativeAnalyticsCuesSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAnalyticsCuesSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("cueCount=2");
    assertThat(summary).contains("presentationTimeUs=654321");
    assertThat(summary).contains("text0=Analytics Cue Final");
    assertThat(summary).contains("text0TokenPresent=1");
    assertThat(summary).contains("bitmap0TokenPresent=1");
    assertThat(summary).contains("text1=Analytics Cue Final 2");
    assertThat(summary).contains("text1TokenPresent=1");
    assertThat(summary).contains("bitmap1TokenPresent=0");
  }

  @Test
  public void nativeAnalyticsMetadataSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAnalyticsMetadataSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("entryCount=2");
    assertThat(summary).contains("firstEntryType=MdtaMetadataEntry");
    assertThat(summary).contains("firstEntryText=analytics-metadata-final");
  }

  @Test
  public void nativeAnalyticsLoadErrorSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAnalyticsLoadErrorSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("uri=https://example.com/analytics-error-final.m3u8");
    assertThat(summary).contains("dataType=4");
    assertThat(summary).contains("trackType=2");
    assertThat(summary).contains("message=analytics-load-final");
    assertThat(summary).contains("wasCanceled=0");
  }

  @Test
  public void nativeAnalyticsDeviceInfoChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsDeviceInfoChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("playbackType=1");
    assertThat(summary).contains("minVolume=2");
    assertThat(summary).contains("maxVolume=15");
    assertThat(summary).contains("routingControllerId=route-final");
  }

  @Test
  public void nativeAnalyticsMediaMetadataChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsMediaMetadataChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("title=Analytics Media Final");
    assertThat(summary).contains("artist=Analytics Artist Final");
    assertThat(summary).contains("displayTitle=Analytics Display Final");
  }

  @Test
  public void nativeAnalyticsPlaylistMetadataChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsPlaylistMetadataChangedSmokeTest(context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("title=Analytics Playlist Final");
    assertThat(summary).contains("artist=Analytics Playlist Artist Final");
    assertThat(summary).contains("displayTitle=Analytics Playlist Display Final");
  }

  @Test
  public void nativeAnalyticsVideoInputFormatChangedSmokeTest_reportsConcreteAnalyticsEvent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAnalyticsVideoInputFormatChangedSmokeTest(
            context);

    assertCallbackStoppedAfterRemove(summary);
    assertThat(summary).contains("sampleMimeType=video/final");
    assertThat(summary).contains("codecs=hvc1.1.6.L93.B0");
    assertThat(summary).contains("width=1920");
    assertThat(summary).contains("height=1080");
    assertThat(summary).contains("frameRate=59.939999");
  }

  @Test
  public void nativeImageOutputSmokeTest_returnsListenerSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeImageOutputSmokeTest(context);

    assertThat(summary).contains("imageCount=3");
    assertThat(summary).contains("runtimeDisableDisabledCount=1");
    assertThat(summary).contains("afterRuntimeDisableImageCount=2");
    assertThat(summary).contains("afterRuntimeReenableImageCount=3");
    assertThat(summary).contains("beforeRemoveImageCount=3");
    assertThat(summary).contains("afterRemoveImageCount=3");
    assertThat(summary).contains("callbackStopped=1");
    assertThat(summary).contains("lastPresentationTimeUs=345678");
    assertThat(summary).contains("lastWidth=12");
    assertThat(summary).contains("lastHeight=7");
    assertThat(summary).contains("lastByteCount=336");
    assertThat(summary).contains("lastAllocationByteCount=336");
    assertThat(summary).contains("lastRowBytes=48");
    assertThat(summary).contains("lastHasAlpha=1");
    assertThat(summary).contains("lastIsPremultiplied=1");
    assertThat(summary).contains("lastIsMutable=1");
    assertThat(summary).contains("lastBitmapConfig=ARGB_8888");
    assertThat(summary).contains("disabledCount=2");
    assertThat(summary).contains("reattachImageCount=1");
    assertThat(summary).contains("reattachLastPresentationTimeUs=456789");
    assertThat(summary).contains("reattachLastWidth=14");
    assertThat(summary).contains("reattachLastHeight=8");
    assertThat(summary).contains("reattachLastByteCount=448");
    assertThat(summary).contains("reattachLastAllocationByteCount=448");
    assertThat(summary).contains("reattachLastRowBytes=56");
    assertThat(summary).contains("reattachLastHasAlpha=1");
    assertThat(summary).contains("reattachLastIsPremultiplied=1");
    assertThat(summary).contains("reattachLastIsMutable=1");
    assertThat(summary).contains("reattachLastBitmapConfig=ARGB_8888");
    assertThat(summary).contains("reattachDisabledCount=1");
  }

  @Test
  public void nativeSourceTypeSmokeTest_returnsInferredMimeSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeSourceTypeSmokeTest(context);

    assertThat(summary).contains("mediaId=source-type-item");
    assertThat(summary).contains("sourceType=2");
  }

  @Test
  public void nativeHttpHlsDashPlaybackSmokeTest_preparesLocalStreamsThroughCppApi()
      throws Exception {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    MockWebServer server = createStreamPlaybackServer(context);
    try {
      String summary =
          CppBridgeNativePlayerTestHelper.nativeHttpHlsDashPlaybackSmokeTest(
              context,
              server.url("/http/sample.audio.mp4").toString(),
              server.url("/hls/manifest.m3u8").toString(),
              server.url("/dash/sample.mpd").toString());

      assertThat(summary).contains("httpPrepared=1");
      assertThat(summary).contains("httpAdvanced=1");
      assertThat(summary).contains("httpSourceType=5");
      assertThat(summary).contains("httpMimeType=audio/mp4");
      assertThat(summary).contains("hlsPrepared=1");
      assertThat(summary).contains("hlsAdvanced=1");
      assertThat(summary).contains("hlsSourceType=2");
      assertThat(summary).contains("hlsMimeType=application/x-mpegURL");
      assertThat(summary).contains("dashPrepared=1");
      assertThat(summary).contains("dashAdvanced=1");
      assertThat(summary).contains("dashSourceType=1");
      assertThat(summary).contains("dashMimeType=application/dash+xml");
    } finally {
      server.shutdown();
    }
  }

  @Test
  public void nativeMediaSourceFactoryConfigSmokeTest_returnsConfigSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeMediaSourceFactoryConfigSmokeTest(context);

    assertThat(summary).contains("parseSubtitlesDuringExtraction=0");
    assertThat(summary).contains("loadOnlySelectedTracks=1");
    assertThat(summary).contains("headerCount=2");
    assertThat(summary).contains("header0=X-Test-Header:bridge");
    assertThat(summary).contains("header1=X-Trace-Id:trace-123");
    assertThat(summary).contains("userAgent=cppbridge-agent");
    assertThat(summary).contains("connectTimeoutMs=2345");
    assertThat(summary).contains("readTimeoutMs=6789");
    assertThat(summary).contains("allowCrossProtocolRedirects=1");
    assertThat(summary).contains("liveTargetOffsetMs=3000");
    assertThat(summary).contains("liveMinOffsetMs=2000");
    assertThat(summary).contains("liveMaxOffsetMs=5000");
    assertThat(summary).contains("liveMinSpeed=0.970000");
    assertThat(summary).contains("liveMaxSpeed=1.030000");
  }

  @Test
  public void nativeMediaSourceFactoryInjectionSmokeTest_usesRegisteredFactoryToken() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    String token = "test-injected-media-source-factory";
    CppMediaSourceFactoryRegistry.register(token, new DefaultMediaSourceFactory(context));
    try {
      String summary =
          CppBridgeNativePlayerTestHelper.nativeMediaSourceFactoryInjectionSmokeTest(context);

      assertThat(summary).contains("factoryToken=test-injected-media-source-factory");
      assertThat(summary).contains("injectedFactoryUsed=1");
      assertThat(summary).contains("factoryIdentity=");
    } finally {
      CppMediaSourceFactoryRegistry.unregister(token);
    }
  }

  @Test
  public void nativeMediaSourceFactoryInjectionFallbackSmokeTest_fallsBackWhenTokenIsMissing() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeMediaSourceFactoryInjectionFallbackSmokeTest(
            context);

    assertThat(summary).contains("factoryToken=missing-media-source-factory-token");
    assertThat(summary).contains("injectedFactoryUsed=0");
    assertThat(summary).contains("factoryIdentity=0");
    assertThat(summary).contains("fallbackApplied=1");
    assertThat(summary).contains("userAgent=fallback-agent");
  }

  @Test
  public void nativeMediaSourceFactoryInjectionReplacementSmokeTest_usesLatestRegisteredFactory() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    String token = "replaceable-media-source-factory-token";
    try {
      CppMediaSourceFactoryRegistry.register(token, new DefaultMediaSourceFactory(context));
      String firstSummary =
          CppBridgeNativePlayerTestHelper.nativeMediaSourceFactoryInjectionReplacementSmokeTest(
              context);

      CppMediaSourceFactoryRegistry.register(token, new DefaultMediaSourceFactory(context));
      String secondSummary =
          CppBridgeNativePlayerTestHelper.nativeMediaSourceFactoryInjectionReplacementSmokeTest(
              context);

      assertThat(firstSummary).contains("factoryToken=replaceable-media-source-factory-token");
      assertThat(firstSummary).contains("injectedFactoryUsed=1");
      assertThat(secondSummary).contains("factoryToken=replaceable-media-source-factory-token");
      assertThat(secondSummary).contains("injectedFactoryUsed=1");
      assertThat(extractIntMarker(firstSummary, "factoryIdentity=")).isGreaterThan(0);
      assertThat(extractIntMarker(secondSummary, "factoryIdentity=")).isGreaterThan(0);
      assertThat(extractIntMarker(secondSummary, "factoryIdentity="))
          .isNotEqualTo(extractIntMarker(firstSummary, "factoryIdentity="));
    } finally {
      CppMediaSourceFactoryRegistry.unregister(token);
    }
  }

  @Test
  public void nativeMediaSourceFactoryInjectionMultiTokenSmokeTest_keepsTokensIsolated() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    String firstToken = "multi-token-media-source-factory-a";
    String secondToken = "multi-token-media-source-factory-b";
    try {
      CppMediaSourceFactoryRegistry.register(firstToken, new DefaultMediaSourceFactory(context));
      CppMediaSourceFactoryRegistry.register(secondToken, new DefaultMediaSourceFactory(context));

      String summary =
          CppBridgeNativePlayerTestHelper.nativeMediaSourceFactoryInjectionMultiTokenSmokeTest(
              context);

      assertThat(summary).contains("firstFactoryToken=multi-token-media-source-factory-a");
      assertThat(summary).contains("firstInjectedFactoryUsed=1");
      assertThat(summary).contains("secondFactoryToken=multi-token-media-source-factory-b");
      assertThat(summary).contains("secondInjectedFactoryUsed=1");
      assertThat(summary).contains("tokensIsolated=1");
      assertThat(extractIntMarker(summary, "firstFactoryIdentity=")).isGreaterThan(0);
      assertThat(extractIntMarker(summary, "secondFactoryIdentity=")).isGreaterThan(0);
      assertThat(extractIntMarker(summary, "firstFactoryIdentity="))
          .isNotEqualTo(extractIntMarker(summary, "secondFactoryIdentity="));
    } finally {
      CppMediaSourceFactoryRegistry.unregister(firstToken);
      CppMediaSourceFactoryRegistry.unregister(secondToken);
    }
  }

  @Test
  public void nativeMediaSourceFactoryGeneratedTokenSmokeTest_usesGeneratedRegistryToken() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    String token = CppMediaSourceFactoryRegistry.register(new DefaultMediaSourceFactory(context));
    try {
      String summary =
          CppBridgeNativePlayerTestHelper.nativeMediaSourceFactoryGeneratedTokenSmokeTest(
              context, token);

      assertThat(summary).contains("factoryToken=" + token);
      assertThat(summary).contains("injectedFactoryUsed=1");
      assertThat(summary).contains("generatedTokenPath=1");
      assertThat(extractIntMarker(summary, "factoryIdentity=")).isGreaterThan(0);
    } finally {
      CppMediaSourceFactoryRegistry.unregister(token);
    }
  }

  @Test
  public void nativePlayerConfigFlagsSmokeTest_returnsCreateTimeFlags() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativePlayerConfigFlagsSmokeTest(context);

    assertThat(summary).contains("handleAudioFocus=0");
    assertThat(summary).contains("handleAudioBecomingNoisy=0");
    assertThat(summary).contains("useLazyPreparation=0");
    assertThat(summary).contains("seekBackIncrementMs=1357");
    assertThat(summary).contains("seekForwardIncrementMs=2468");
    assertThat(summary).contains("wakeMode=1");
    assertThat(summary).contains("priority=77");
    assertThat(summary).contains("usePriorityTaskManager=1");
    assertThat(summary).contains("targetPreloadDurationUs=456789");
  }

  @Test
  public void nativeWakeModeRuntimeSmokeTest_updatesWakeMode() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeWakeModeRuntimeSmokeTest(context);

    assertThat(summary).contains("beforeWakeMode=1");
    assertThat(summary).contains("afterWakeMode=0");
    assertThat(summary).contains("runtimeApplied=1");
  }

  @Test
  public void nativeRuntimeControlParitySmokeTest_updatesPhaseOneRuntimeControls() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeRuntimeControlParitySmokeTest(context);

    assertThat(summary).contains("seekBackIncrementMs=4321");
    assertThat(summary).contains("seekForwardIncrementMs=8765");
    assertThat(summary).contains("maxSeekToPreviousPositionMs=9999");
    assertThat(summary).contains("initialPauseAtEnd=0");
    assertThat(summary).contains("afterEnablePauseAtEnd=1");
    assertThat(summary).contains("afterDisablePauseAtEnd=0");
    assertThat(summary).contains("videoScalingMode=2");
    assertThat(summary).contains("videoChangeFrameRateStrategy=-2147483648");
    assertThat(summary).contains("afterDisableNoisyFlag=0");
    assertThat(summary).contains("afterEnableNoisyFlag=1");
    assertThat(summary).contains("afterEnableForegroundFlag=1");
    assertThat(summary).contains("afterDisableForegroundFlag=0");
    assertThat(summary).contains("runtimeApplied=1");
  }

  @Test
  public void nativeAudioAndScrubbingParitySmokeTest_updatesAdvancedRuntimeControls() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeAudioAndScrubbingParitySmokeTest(context);

    assertThat(summary).contains("audioSessionId=1234");
    assertThat(summary).contains("auxEffectAfterSet=0:0.37");
    assertThat(summary).contains("auxEffectAfterClear=0:0.0");
    assertThat(summary).contains("preferredAudioDeviceAfterClear=0");
    assertThat(summary).contains("virtualDeviceId=42");
    assertThat(summary).contains("scrubbingInitially=0");
    assertThat(summary).contains("scrubbingAfterEnable=1");
    assertThat(summary).contains("scrubbingAfterDisable=0");
    assertThat(summary).contains("scrubTracks=2,3");
    assertThat(summary).contains("scrubTolerance=0.125000:0.500000");
    assertThat(summary).contains("scrubFlags=01010");
    assertThat(summary).contains("runtimeApplied=1");
  }

  @Test
  public void nativeCodecParametersParitySmokeTest_setsAudioAndVideoCodecParameters() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeCodecParametersParitySmokeTest(context);

    assertThat(summary)
        .contains(
            "audioCodec=audio-int=int:7;audio-long=long:9876543210;audio-float=float:1.25;"
                + "audio-string=string:music;audio-bytes=bytes:3:012aff;audio-null=null");
    assertThat(summary).contains("videoCodec=video-string=string:video;video-bytes=bytes:2:1020");
    assertThat(summary).contains("runtimeApplied=1");
  }

  @Test
  public void nativeAuxiliaryCallbackParitySmokeTest_reportsCodecVideoAndCameraCallbacks() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeAuxiliaryCallbackParitySmokeTest(context);

    assertThat(summary).contains("audioCodecCb=1");
    assertThat(summary).contains("audioCodec=codec-mode=string:low-latency;codec-rate=int:60");
    assertThat(summary).contains("videoCodecCb=1");
    assertThat(summary).contains("videoCodec=video-profile=string:main");
    assertThat(summary).contains("videoFrameCb=1");
    assertThat(summary).contains("framePresentationUs=123456");
    assertThat(summary).contains("frameReleaseNs=987654321");
    assertThat(summary).contains("frameFormatId=frame-format");
    assertThat(summary).contains("frameMime=video/avc");
    assertThat(summary).contains("frameSize=1920x1080");
    assertThat(summary).contains("frameLabel=Main Camera");
    assertThat(summary).contains("frameLanguage=en");
    assertThat(summary).contains("frameContainerMime=video/mp4");
    assertThat(summary).contains("frameBitrates=333000:222000:333000");
    assertThat(summary).contains("frameRotation=180");
    assertThat(summary).contains("framePixelRatio=1.500000");
    assertThat(summary).contains("frameColor=1:2:3");
    assertThat(summary).contains("frameAudioShape=2:48000");
    assertThat(summary).contains("frameFlags=5:7");
    assertThat(summary).contains("frameMediaFormatPresent=1");
    assertThat(summary).contains("frameMediaFormatMime=video/avc");
    assertThat(summary).contains("frameMediaFormatSize=1920x1080");
    assertThat(summary).contains("frameMediaFormatFrameRate=23.976000");
    assertThat(summary).contains("frameMediaFormatRotation=90");
    assertThat(summary).contains("frameMediaFormatColor=1:2:3");
    assertThat(summary).contains("cameraMotionCb=1");
    assertThat(summary).contains("cameraTimeUs=654321");
    assertThat(summary).contains("cameraRotation=1.000000:2.000000:3.000000");
    assertThat(summary).contains("cameraResetCb=1");
    assertThat(summary).contains("afterRemoveStopped=1");
    assertThat(summary).contains("bridgeCodecRegistrationSafe=1");
    assertThat(summary).contains("callbackApplied=1");
  }

  @Test
  public void nativeVideoFrameMetadataSimulationFallbackSmokeTest_preservesFallbackFields() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeVideoFrameMetadataSimulationFallbackSmokeTest(context);

    assertThat(summary).contains("frameCb=1");
    assertThat(summary).contains("fallbackBitrates=123000:123000:-1");
    assertThat(summary).contains("fallbackColor=1:-1:-1");
    assertThat(summary).contains("fallbackAudioShape=-1:-1");
    assertThat(summary).contains("fallbackApplied=1");
  }

  @Test
  public void nativeCodecParametersMultiListenerParitySmokeTest_routesImmediateCallbacks() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeCodecParametersMultiListenerParitySmokeTest(context);

    assertThat(summary).contains("audioFirstAfterSecondAdd=0");
    assertThat(summary).contains("audioSecondInitial=1");
    assertThat(summary).contains("audioFirst=keyA=int:10;keyB=int:20");
    assertThat(summary).contains("audioSecond=keyB=int:20;keyC=int:30");
    assertThat(summary).contains("audioFirstAfterRemoveDelta=0");
    assertThat(summary).contains("audioSecondAfterRemoveStopped=1");
    assertThat(summary).contains("videoFirstAfterSecondAdd=0");
    assertThat(summary).contains("videoSecondInitial=1");
    assertThat(summary).contains("videoFirst=vKeyA=int:100;vKeyB=int:200");
    assertThat(summary).contains("videoSecond=vKeyB=int:200;vKeyC=int:300");
    assertThat(summary).contains("videoFirstAfterRemoveDelta=0");
    assertThat(summary).contains("videoSecondAfterRemoveStopped=1");
    assertThat(summary).contains("multiListenerApplied=1");
  }

  @Test
  public void nativeRendererAndDeviceStateGetterSmokeTest_readsRendererAndDeviceState() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativePlayerTestHelper.nativeRendererAndDeviceStateGetterSmokeTest(context);

    assertThat(summary).contains("sleepingForOffload=0");
    assertThat(summary).contains("tunnelingEnabled=0");
    assertThat(summary).contains("invalidRendererType=-1");
    assertThat(summary).contains("releasedBefore=0");
    assertThat(summary).contains("getterApplied=1");
  }

  @Test
  public void nativePreloadConfigurationSmokeTest_returnsRuntimeConfig() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativePreloadConfigurationSmokeTest(context);

    assertThat(summary).contains("targetPreloadDurationUs=654321");
    assertThat(summary).contains("afterUnsetTargetPreloadDurationUs=-9223372036854775807");
    assertThat(summary).contains("unsetApplied=1");
  }

  @Test
  public void nativePreloadRoundTripSmokeTest_updatesAndRestoresPreloadTarget() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativePreloadRoundTripSmokeTest(context);

    assertThat(summary).contains("initialTargetPreloadDurationUs=111111");
    assertThat(summary).contains("afterUpdateTargetPreloadDurationUs=222222");
    assertThat(summary).contains("afterUnsetTargetPreloadDurationUs=-9223372036854775807");
    assertThat(summary).contains("afterResetTargetPreloadDurationUs=333333");
    assertThat(summary).contains("updateApplied=1");
    assertThat(summary).contains("unsetApplied=1");
    assertThat(summary).contains("resetApplied=1");
  }

  @Test
  public void nativePreloadBridgeRuntimeSmokeTest_updatesAndMatchesBridgeFlags() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativePreloadBridgeRuntimeSmokeTest(context);

    assertThat(summary).contains("initialFlagTargetPreloadDurationUs=111111");
    assertThat(summary).contains("initialDirectTargetPreloadDurationUs=111111");
    assertThat(summary).contains("afterUpdateFlagTargetPreloadDurationUs=222222");
    assertThat(summary).contains("afterUpdateDirectTargetPreloadDurationUs=222222");
    assertThat(summary)
        .contains("afterUnsetFlagTargetPreloadDurationUs=-9223372036854775807");
    assertThat(summary)
        .contains("afterUnsetDirectTargetPreloadDurationUs=-9223372036854775807");
    assertThat(summary).contains("afterResetFlagTargetPreloadDurationUs=333333");
    assertThat(summary).contains("afterResetDirectTargetPreloadDurationUs=333333");
    assertThat(summary).contains("flagAndDirectMatch=1");
    assertThat(summary).contains("updateApplied=1");
    assertThat(summary).contains("unsetApplied=1");
    assertThat(summary).contains("resetApplied=1");
  }

  @Test
  public void nativePriorityTaskManagerSmokeTest_returnsBridgeState() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativePriorityTaskManagerSmokeTest(context);

    assertThat(summary).contains("initialEnabled=1");
    assertThat(summary).contains("initialAttached=1");
    assertThat(summary).contains("initialRegistered=1");
    assertThat(summary).contains("initialPriority=77");
    assertThat(summary).contains("afterSetPriority=88");
    assertThat(summary).contains("afterSetPriorityAttached=1");
    assertThat(summary).contains("afterSetPriorityRegistered=1");
    assertThat(summary).contains("afterDisableEnabled=0");
    assertThat(summary).contains("afterDisableAttached=0");
    assertThat(summary).contains("afterDisableRegistered=0");
    assertThat(summary).contains("afterEnableEnabled=1");
    assertThat(summary).contains("afterEnableAttached=1");
    assertThat(summary).contains("afterEnableRegistered=1");
    assertThat(summary).contains("afterEnablePriority=88");
  }

  @Test
  public void nativePlayerMessageSmokeTest_returnsDeliverySummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativePlayerMessageSmokeTest(context);

    assertThat(summary).contains("delivered=1");
    assertThat(summary).contains("timedOut=0");
    assertThat(summary).contains("canceled=0");
    assertThat(summary).contains("deliveryCount=1");
    assertThat(summary).contains("type=42");
    assertThat(summary).contains("payload=payload-test");
    assertThat(summary).contains("mediaItemIndex=-1");
    assertThat(summary)
        .contains("positionMs=" + Long.toString(C.TIME_UNSET));
    assertThat(summary).contains("deleteAfterDelivery=1");
    assertThat(summary).contains("thread=");
  }

  @Test
  public void nativeTimedPlayerMessageSmokeTest_returnsRuntimeDeliverySummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeTimedPlayerMessageSmokeTest(context);

    assertThat(summary).contains("delivered=1");
    assertThat(summary).contains("timedOut=0");
    assertThat(summary).contains("canceled=0");
    assertThat(summary).contains("deliveryCount=1");
    assertThat(summary).contains("type=42");
    assertThat(summary).contains("payload=payload-test");
    assertThat(summary).contains("mediaItemIndex=0");
    assertThat(summary).contains("positionMs=");
    assertThat(summary).contains("scheduledPositionMs=");
    assertThat(summary).contains("deleteAfterDelivery=1");
    assertThat(summary).contains("thread=");
    assertThat(summary).contains("playbackAdvanced=1");
    assertThat(summary).contains("runtimePlayerReady=1");
    assertThat(summary).contains("prepared=");
    assertThat(summary).contains("playWhenReady=");
    assertThat(summary).contains("playbackState=");
  }

  @Test
  public void nativeRendererPlayerMessageSmokeTest_returnsRuntimeSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeRendererPlayerMessageSmokeTest(context);

    assertThat(summary).contains("delivered=");
    assertThat(summary).contains("timedOut=");
    assertThat(summary).contains("canceled=");
    assertThat(summary).contains("deliveryCount=");
    assertThat(summary).contains("deleteAfterDelivery=1");
    assertThat(summary).contains("runtimePlayerReady=1");
    assertThat(summary).contains("prepared=");
    assertThat(summary).contains("playWhenReady=");
    assertThat(summary).contains("playbackState=");
  }

  @Test
  public void nativePlayerMessageCancelSmokeTest_returnsCanceledSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativePlayerMessageCancelSmokeTest(context);

    assertThat(summary).contains("delivered=0");
    assertThat(summary).contains("timedOut=0");
    assertThat(summary).contains("canceled=1");
    assertThat(summary).contains("deliveryCount=0");
    assertThat(summary).contains("type=0");
    assertThat(summary).contains("payload=");
  }

  @Test
  public void nativeCurrentMediaItemQuerySmokeTest_returnsStructuredSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeCurrentMediaItemQuerySmokeTest(context);

    assertThat(summary).contains("exists=1");
    assertThat(summary).contains("mediaId=current-item");
    assertThat(summary).contains("uri=https://example.com/current-item.m3u8");
    assertThat(summary).contains("mimeType=application/x-mpegURL");
    assertThat(summary).contains("sourceType=2");
    assertThat(summary).contains("tagPresent=1");
    assertThat(summary).contains("tagString=current-tag");
    assertThat(summary).contains("tagTokenPresent=1");
    assertThat(summary).contains("tagValuePresent=1");
    assertThat(summary).contains("tagValueClass=java.lang.String");
    assertThat(summary).contains("tagValueType=1");
    assertThat(summary).contains("tagValueString=current-tag");
    assertThat(summary).contains("subtitleCount=2");
    assertThat(summary).contains("subtitle0Uri=https://example.com/current.vtt");
    assertThat(summary).contains("subtitle0MimeType=text/vtt");
    assertThat(summary).contains("subtitle0Language=en");
    assertThat(summary).contains("subtitle0Label=English");
    assertThat(summary).contains("subtitle0Id=sub-1");
    assertThat(summary).contains("subtitle0SelectionFlags=5");
    assertThat(summary).contains("subtitle0RoleFlags=11");
    assertThat(summary).contains("subtitle1Uri=https://example.com/current-es.vtt");
    assertThat(summary).contains("subtitle1MimeType=text/vtt");
    assertThat(summary).contains("subtitle1Language=es");
    assertThat(summary).contains("subtitle1Label=Spanish");
    assertThat(summary).contains("subtitle1Id=sub-2");
    assertThat(summary).contains("subtitle1SelectionFlags=3");
    assertThat(summary).contains("subtitle1RoleFlags=13");
    assertThat(summary).contains("hasClipping=1");
    assertThat(summary).contains("clippingStartMs=1000");
    assertThat(summary).contains("clippingEndMs=9000");
    assertThat(summary).contains("clippingRelativeToLiveWindow=1");
    assertThat(summary).contains("clippingRelativeToDefaultPosition=1");
    assertThat(summary).contains("clippingStartsAtKeyFrame=1");
    assertThat(summary).contains("clippingAllowUnseekable=1");
    assertThat(summary).contains("hasLiveConfiguration=1");
    assertThat(summary).contains("liveTargetOffsetMs=3000");
    assertThat(summary).contains("liveMinOffsetMs=2500");
    assertThat(summary).contains("liveMaxOffsetMs=4500");
    assertThat(summary).contains("liveMinSpeed=0.960000");
    assertThat(summary).contains("liveMaxSpeed=1.040000");
    assertThat(summary).contains("hasDrmConfiguration=1");
    assertThat(summary).contains("drmScheme=edef8ba9-79d6-4ace-a3c8-27dcd51d21ed");
    assertThat(summary).contains("drmLicenseUri=https://license.example.com");
    assertThat(summary).contains("drmHeaderCount=2");
    assertThat(summary).contains("drmHeader0=Authorization:Bearer current-token");
    assertThat(summary).contains("drmHeader1=X-Client:cppbridge");
    assertThat(summary).contains("drmForcedSessionTrackTypeCount=1");
    assertThat(summary).contains("drmKeySetIdLength=3");
    assertThat(summary).contains("drmMultiSession=1");
    assertThat(summary).contains("drmForceDefaultLicenseUri=1");
    assertThat(summary).contains("drmPlayClearContentWithoutKey=0");
    assertThat(summary).contains("requestMetadataMediaUri=https://example.com/current-request");
    assertThat(summary).contains("requestMetadataSearchQuery=current search");
    assertThat(summary).contains("requestMetadataExtrasPresent=1");
    assertThat(summary).contains("requestMetadataExtrasKeyCount=5");
    assertThat(summary).contains("requestMetadataExtrasTokenPresent=1");
    assertThat(summary).contains("requestMetadataExtrasValueCount=5");
    assertThat(summary).contains("requestMetadataExtrasValue0Key=enabled");
    assertThat(summary).contains("requestMetadataExtrasValue0Type=4");
    assertThat(summary).contains("requestMetadataExtrasValue0Bool=1");
    assertThat(summary).contains("requestMetadataExtrasValue1Key=episode");
    assertThat(summary).contains("requestMetadataExtrasValue1Type=2");
    assertThat(summary).contains("requestMetadataExtrasValue1Long=42");
    assertThat(summary).contains("requestMetadataExtrasValue2Key=gain");
    assertThat(summary).contains("requestMetadataExtrasValue2Type=3");
    assertThat(summary).contains("requestMetadataExtrasValue2Double=1.500000");
    assertThat(summary).contains("requestMetadataExtrasValue3Key=payload");
    assertThat(summary).contains("requestMetadataExtrasValue3Type=5");
    assertThat(summary).contains("requestMetadataExtrasValue3Bytes=3:6");
    assertThat(summary).contains("requestMetadataExtrasValue4Key=source");
    assertThat(summary).contains("requestMetadataExtrasValue4Type=1");
    assertThat(summary).contains("requestMetadataExtrasValue4String=cppbridge");
    assertThat(summary).contains("adTagUri=https://ads.example.com/tag.xml");
    assertThat(summary).contains("adsId=ads-current");
    assertThat(summary).contains("adsIdTokenPresent=1");
    assertThat(summary).contains("adsIdValuePresent=1");
    assertThat(summary).contains("adsIdValueClass=java.lang.String");
    assertThat(summary).contains("adsIdValueType=1");
    assertThat(summary).contains("adsIdValueString=ads-current");
    assertThat(summary).contains("mediaMetadataTitle=Current Item Title");
    assertThat(summary).contains("mediaMetadataTitleTokenPresent=1");
    assertThat(summary).contains("mediaMetadataArtist=Current Item Artist");
    assertThat(summary).contains("mediaMetadataArtistTokenPresent=1");
    assertThat(summary).contains("mediaMetadataAlbumTitle=Current Album");
    assertThat(summary).contains("mediaMetadataAlbumTitleTokenPresent=1");
    assertThat(summary).contains("mediaMetadataAlbumArtist=Current Album Artist");
    assertThat(summary).contains("mediaMetadataAlbumArtistTokenPresent=1");
    assertThat(summary).contains("mediaMetadataDisplayTitle=Current Item Display");
    assertThat(summary).contains("mediaMetadataDisplayTitleTokenPresent=1");
    assertThat(summary).contains("mediaMetadataSubtitle=Current Item Subtitle");
    assertThat(summary).contains("mediaMetadataSubtitleTokenPresent=1");
    assertThat(summary).contains("mediaMetadataDescription=Current Item Description");
    assertThat(summary).contains("mediaMetadataDescriptionTokenPresent=1");
    assertThat(summary).contains("mediaMetadataAuthor=Current Item Author");
    assertThat(summary).contains("mediaMetadataAuthorTokenPresent=1");
    assertThat(summary).contains("mediaMetadataComposer=Current Item Composer");
    assertThat(summary).contains("mediaMetadataComposerTokenPresent=1");
    assertThat(summary).contains("mediaMetadataConductor=Current Item Conductor");
    assertThat(summary).contains("mediaMetadataConductorTokenPresent=1");
    assertThat(summary).contains("mediaMetadataDurationMs=321000");
    assertThat(summary).contains("mediaMetadataTrackNumber=6");
    assertThat(summary).contains("mediaMetadataTotalTrackCount=14");
    assertThat(summary).contains("mediaMetadataIsBrowsable=0");
    assertThat(summary).contains("mediaMetadataIsPlayable=1");
    assertThat(summary).contains("mediaMetadataFolderType=-1");
    assertThat(summary).contains("mediaMetadataRecordingYear=2023");
    assertThat(summary).contains("mediaMetadataRecordingMonth=8");
    assertThat(summary).contains("mediaMetadataRecordingDay=19");
    assertThat(summary).contains("mediaMetadataReleaseYear=2025");
    assertThat(summary).contains("mediaMetadataReleaseMonth=5");
    assertThat(summary).contains("mediaMetadataReleaseDay=11");
    assertThat(summary).contains("mediaMetadataWriter=Current Item Writer");
    assertThat(summary).contains("mediaMetadataWriterTokenPresent=1");
    assertThat(summary).contains("mediaMetadataDiscNumber=2");
    assertThat(summary).contains("mediaMetadataTotalDiscCount=4");
    assertThat(summary).contains("mediaMetadataGenre=Current Item Genre");
    assertThat(summary).contains("mediaMetadataGenreTokenPresent=1");
    assertThat(summary).contains("mediaMetadataCompilation=Current Item Compilation");
    assertThat(summary).contains("mediaMetadataCompilationTokenPresent=1");
    assertThat(summary).contains("mediaMetadataMediaType=7");
    assertThat(summary).contains("mediaMetadataStation=Current Item Station");
    assertThat(summary).contains("mediaMetadataStationTokenPresent=1");
    assertThat(summary).contains("mediaMetadataExtrasPresent=1");
    assertThat(summary).contains("mediaMetadataExtrasKeyCount=5");
    assertThat(summary).contains("mediaMetadataExtrasTokenPresent=1");
    assertThat(summary).contains("mediaMetadataExtrasValueCount=5");
    assertThat(summary).contains("mediaMetadataExtrasValue0Key=available");
    assertThat(summary).contains("mediaMetadataExtrasValue0Type=4");
    assertThat(summary).contains("mediaMetadataExtrasValue0Bool=1");
    assertThat(summary).contains("mediaMetadataExtrasValue1Key=blob");
    assertThat(summary).contains("mediaMetadataExtrasValue1Type=5");
    assertThat(summary).contains("mediaMetadataExtrasValue1Bytes=3:24");
    assertThat(summary).contains("mediaMetadataExtrasValue2Key=rating");
    assertThat(summary).contains("mediaMetadataExtrasValue2Type=3");
    assertThat(summary).contains("mediaMetadataExtrasValue2Double=4.500000");
    assertThat(summary).contains("mediaMetadataExtrasValue3Key=season");
    assertThat(summary).contains("mediaMetadataExtrasValue3Type=2");
    assertThat(summary).contains("mediaMetadataExtrasValue3Long=2");
    assertThat(summary).contains("mediaMetadataExtrasValue4Key=studio");
    assertThat(summary).contains("mediaMetadataExtrasValue4Type=1");
    assertThat(summary).contains("mediaMetadataExtrasValue4String=Studio");
    assertThat(summary).contains("artworkUri=https://example.com/current-artwork.jpg");
    assertThat(summary).contains("artworkDataLength=4");
    assertThat(summary).contains("artworkDataType=3");
  }

  @Test
  public void nativePlaylistMetadataSmokeTest_roundTripsPlaylistMetadata() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativePlaylistMetadataSmokeTest(context);

    assertThat(summary).contains("title=Playlist Title");
    assertThat(summary).contains("artist=Playlist Artist");
    assertThat(summary).contains("albumTitle=Playlist Album");
    assertThat(summary).contains("albumArtist=Playlist Album Artist");
    assertThat(summary).contains("displayTitle=Playlist Display");
    assertThat(summary).contains("albumTitleTokenPresent=1");
    assertThat(summary).contains("albumArtistTokenPresent=1");
    assertThat(summary).contains("titleTokenPresent=1");
    assertThat(summary).contains("artistTokenPresent=1");
    assertThat(summary).contains("displayTitleTokenPresent=1");
    assertThat(summary).contains("subtitle=Playlist Subtitle");
    assertThat(summary).contains("subtitleTokenPresent=1");
    assertThat(summary).contains("description=Playlist Description");
    assertThat(summary).contains("descriptionTokenPresent=1");
    assertThat(summary).contains("writer=Playlist Writer");
    assertThat(summary).contains("writerTokenPresent=1");
    assertThat(summary).contains("author=Playlist Author");
    assertThat(summary).contains("authorTokenPresent=1");
    assertThat(summary).contains("composer=Playlist Composer");
    assertThat(summary).contains("composerTokenPresent=1");
    assertThat(summary).contains("conductor=Playlist Conductor");
    assertThat(summary).contains("conductorTokenPresent=1");
    assertThat(summary).contains("discNumber=2");
    assertThat(summary).contains("totalDiscCount=5");
    assertThat(summary).contains("genre=Playlist Genre");
    assertThat(summary).contains("genreTokenPresent=1");
    assertThat(summary).contains("compilation=Playlist Compilation");
    assertThat(summary).contains("compilationTokenPresent=1");
    assertThat(summary).contains("artworkUri=https://example.com/playlist-artwork.jpg");
    assertThat(summary).contains("artworkDataLength=3");
    assertThat(summary).contains("artworkDataType=4");
    assertThat(summary).contains("isBrowsable=1");
    assertThat(summary).contains("isPlayable=0");
    assertThat(summary).contains("folderType=2");
    assertThat(summary).contains("mediaType=1");
    assertThat(summary).contains("releaseMonth=4");
    assertThat(summary).contains("releaseDay=22");
    assertThat(summary).contains("station=Playlist Station");
    assertThat(summary).contains("stationTokenPresent=1");
    assertThat(summary).contains("extrasPresent=1");
    assertThat(summary).contains("extrasKeyCount=5");
    assertThat(summary).contains("extrasTokenPresent=1");
    assertThat(summary).contains("extrasValueCount=5");
    assertThat(summary).contains("extrasValue0Key=enabled");
    assertThat(summary).contains("extrasValue0Type=4");
    assertThat(summary).contains("extrasValue0Bool=1");
    assertThat(summary).contains("extrasValue1Key=episode");
    assertThat(summary).contains("extrasValue1Type=2");
    assertThat(summary).contains("extrasValue1Long=12");
    assertThat(summary).contains("extrasValue2Key=gain");
    assertThat(summary).contains("extrasValue2Type=3");
    assertThat(summary).contains("extrasValue2Double=0.750000");
    assertThat(summary).contains("extrasValue3Key=payload");
    assertThat(summary).contains("extrasValue3Type=5");
    assertThat(summary).contains("extrasValue3Bytes=3:15");
    assertThat(summary).contains("extrasValue4Key=source");
    assertThat(summary).contains("extrasValue4Type=1");
    assertThat(summary).contains("extrasValue4String=playlist-decoded");
  }

  @Test
  public void nativePlaylistMetadataOpaqueTokenSmokeTest_resolvesRegisteredObjects() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String titleToken = CppOpaqueObjectRegistry.register(new SpannableString("registered-title"));
    String artistToken = CppOpaqueObjectRegistry.register(new SpannableString("registered-artist"));
    String albumTitleToken =
        CppOpaqueObjectRegistry.register(new SpannableString("registered-album-title"));
    String albumArtistToken =
        CppOpaqueObjectRegistry.register(new SpannableString("registered-album-artist"));
    String displayTitleToken =
        CppOpaqueObjectRegistry.register(new SpannableString("registered-display"));
    String subtitleToken =
        CppOpaqueObjectRegistry.register(new SpannableString("registered-subtitle"));
    String descriptionToken =
        CppOpaqueObjectRegistry.register(new SpannableString("registered-description"));
    String writerToken = CppOpaqueObjectRegistry.register(new SpannableString("registered-writer"));
    String authorToken = CppOpaqueObjectRegistry.register(new SpannableString("registered-author"));
    String composerToken =
        CppOpaqueObjectRegistry.register(new SpannableString("registered-composer"));
    String conductorToken =
        CppOpaqueObjectRegistry.register(new SpannableString("registered-conductor"));
    String genreToken = CppOpaqueObjectRegistry.register(new SpannableString("registered-genre"));
    String compilationToken =
        CppOpaqueObjectRegistry.register(new SpannableString("registered-compilation"));
    String stationToken = CppOpaqueObjectRegistry.register(new SpannableString("registered-station"));
    Bundle extras = new Bundle();
    extras.putString("registered-key", "registered-value");
    String extrasToken = CppOpaqueObjectRegistry.register(extras);

    String summary =
        CppBridgeNativePlayerTestHelper.nativePlaylistMetadataOpaqueTokenSmokeTest(
            context,
            titleToken,
            artistToken,
            albumTitleToken,
            albumArtistToken,
            displayTitleToken,
            subtitleToken,
            descriptionToken,
            writerToken,
            authorToken,
            composerToken,
            conductorToken,
            genreToken,
            compilationToken,
            stationToken,
            extrasToken);

    assertThat(summary).contains("title=registered-title");
    assertThat(summary).contains("artist=registered-artist");
    assertThat(summary).contains("albumTitle=registered-album-title");
    assertThat(summary).contains("albumArtist=registered-album-artist");
    assertThat(summary).contains("displayTitle=registered-display");
    assertThat(summary).contains("subtitle=registered-subtitle");
    assertThat(summary).contains("description=registered-description");
    assertThat(summary).contains("writer=registered-writer");
    assertThat(summary).contains("author=registered-author");
    assertThat(summary).contains("composer=registered-composer");
    assertThat(summary).contains("conductor=registered-conductor");
    assertThat(summary).contains("genre=registered-genre");
    assertThat(summary).contains("compilation=registered-compilation");
    assertThat(summary).contains("station=registered-station");
    assertThat(summary).contains("titleTokenPresent=1");
    assertThat(summary).contains("artistTokenPresent=1");
    assertThat(summary).contains("albumTitleTokenPresent=1");
    assertThat(summary).contains("albumArtistTokenPresent=1");
    assertThat(summary).contains("displayTitleTokenPresent=1");
    assertThat(summary).contains("subtitleTokenPresent=1");
    assertThat(summary).contains("descriptionTokenPresent=1");
    assertThat(summary).contains("writerTokenPresent=1");
    assertThat(summary).contains("authorTokenPresent=1");
    assertThat(summary).contains("composerTokenPresent=1");
    assertThat(summary).contains("conductorTokenPresent=1");
    assertThat(summary).contains("genreTokenPresent=1");
    assertThat(summary).contains("compilationTokenPresent=1");
    assertThat(summary).contains("stationTokenPresent=1");
    assertThat(summary).contains("extrasPresent=1");
    assertThat(summary).contains("extrasKeyCount=1");
    assertThat(summary).contains("extrasTokenPresent=1");
  }

  @Test
  public void nativeMediaItemAtSmokeTest_returnsSnapshotAndHandlesOutOfBounds() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeMediaItemAtSmokeTest(context);

    assertThat(summary).contains("exists=1");
    assertThat(summary).contains("mediaId=at-2");
    assertThat(summary).contains("uri=https://example.com/at-two.m3u8");
    assertThat(summary).contains("mimeType=application/x-mpegURL");
    assertThat(summary).contains("sourceType=2");
    assertThat(summary).contains("tagPresent=1");
    assertThat(summary).contains("tagString=at-two-tag");
    assertThat(summary).contains("tagTokenPresent=1");
    assertThat(summary).contains("tagValuePresent=1");
    assertThat(summary).contains("tagValueClass=java.lang.String");
    assertThat(summary).contains("tagValueType=1");
    assertThat(summary).contains("tagValueString=at-two-tag");
    assertThat(summary).contains("subtitleCount=2");
    assertThat(summary).contains("subtitle0Uri=https://example.com/at-two.vtt");
    assertThat(summary).contains("subtitle0Id=sub-at-2");
    assertThat(summary).contains("subtitle0SelectionFlags=7");
    assertThat(summary).contains("subtitle0RoleFlags=9");
    assertThat(summary).contains("subtitle1Uri=https://example.com/at-two-es.vtt");
    assertThat(summary).contains("subtitle1MimeType=text/vtt");
    assertThat(summary).contains("subtitle1Language=es");
    assertThat(summary).contains("subtitle1Label=Spanish");
    assertThat(summary).contains("subtitle1Id=sub-at-2b");
    assertThat(summary).contains("subtitle1SelectionFlags=1");
    assertThat(summary).contains("subtitle1RoleFlags=5");
    assertThat(summary).contains("clippingStartMs=2222");
    assertThat(summary).contains("clippingEndMs=7777");
    assertThat(summary).contains("clippingAllowUnseekable=1");
    assertThat(summary).contains("liveTargetOffsetMs=4444");
    assertThat(summary).contains("liveMinOffsetMs=3333");
    assertThat(summary).contains("liveMaxOffsetMs=6666");
    assertThat(summary).contains("liveMinSpeed=0.950000");
    assertThat(summary).contains("liveMaxSpeed=1.050000");
    assertThat(summary).contains("drmScheme=edef8ba9-79d6-4ace-a3c8-27dcd51d21ed");
    assertThat(summary).contains("drmLicenseUri=https://license.example.com/at-two");
    assertThat(summary).contains("drmHeaderCount=2");
    assertThat(summary).contains("drmHeader0=Authorization:Bearer at-two");
    assertThat(summary).contains("drmHeader1=X-Env:staging");
    assertThat(summary).contains("drmForceDefaultLicenseUri=1");
    assertThat(summary).contains("drmPlayClearContentWithoutKey=0");
    assertThat(summary).contains("requestMetadataMediaUri=https://example.com/at-two-request");
    assertThat(summary).contains("requestMetadataSearchQuery=at two search");
    assertThat(summary).contains("requestMetadataExtrasPresent=1");
    assertThat(summary).contains("requestMetadataExtrasKeyCount=1");
    assertThat(summary).contains("requestMetadataExtrasTokenPresent=1");
    assertThat(summary).contains("adTagUri=https://ads.example.com/at-two.xml");
    assertThat(summary).contains("adsId=ads-at-2");
    assertThat(summary).contains("adsIdTokenPresent=1");
    assertThat(summary).contains("adsIdValuePresent=1");
    assertThat(summary).contains("adsIdValueClass=java.lang.String");
    assertThat(summary).contains("adsIdValueType=1");
    assertThat(summary).contains("adsIdValueString=ads-at-2");
    assertThat(summary).contains("mediaMetadataTitle=At Two Title");
    assertThat(summary).contains("mediaMetadataTitleTokenPresent=1");
    assertThat(summary).contains("mediaMetadataArtist=At Two Artist");
    assertThat(summary).contains("mediaMetadataArtistTokenPresent=1");
    assertThat(summary).contains("mediaMetadataAlbumTitle=At Two Album");
    assertThat(summary).contains("mediaMetadataAlbumTitleTokenPresent=1");
    assertThat(summary).contains("mediaMetadataAlbumArtist=At Two Album Artist");
    assertThat(summary).contains("mediaMetadataAlbumArtistTokenPresent=1");
    assertThat(summary).contains("mediaMetadataDisplayTitle=At Two Display");
    assertThat(summary).contains("mediaMetadataDisplayTitleTokenPresent=1");
    assertThat(summary).contains("mediaMetadataSubtitle=At Two Subtitle");
    assertThat(summary).contains("mediaMetadataSubtitleTokenPresent=1");
    assertThat(summary).contains("mediaMetadataDescription=At Two Description");
    assertThat(summary).contains("mediaMetadataDescriptionTokenPresent=1");
    assertThat(summary).contains("mediaMetadataAuthor=At Two Author");
    assertThat(summary).contains("mediaMetadataAuthorTokenPresent=1");
    assertThat(summary).contains("mediaMetadataComposer=At Two Composer");
    assertThat(summary).contains("mediaMetadataComposerTokenPresent=1");
    assertThat(summary).contains("mediaMetadataConductor=At Two Conductor");
    assertThat(summary).contains("mediaMetadataConductorTokenPresent=1");
    assertThat(summary).contains("mediaMetadataWriter=At Two Writer");
    assertThat(summary).contains("mediaMetadataWriterTokenPresent=1");
    assertThat(summary).contains("mediaMetadataGenre=At Two Genre");
    assertThat(summary).contains("mediaMetadataGenreTokenPresent=1");
    assertThat(summary).contains("mediaMetadataCompilation=At Two Compilation");
    assertThat(summary).contains("mediaMetadataCompilationTokenPresent=1");
    assertThat(summary).contains("mediaMetadataMediaType=4");
    assertThat(summary).contains("mediaMetadataStation=At Two Station");
    assertThat(summary).contains("mediaMetadataStationTokenPresent=1");
    assertThat(summary).contains("mediaMetadataExtrasPresent=1");
    assertThat(summary).contains("mediaMetadataExtrasKeyCount=1");
    assertThat(summary).contains("mediaMetadataExtrasTokenPresent=1");
    assertThat(summary).contains("artworkUri=https://example.com/at-two-artwork.jpg");
    assertThat(summary).contains("artworkDataLength=4");
    assertThat(summary).contains("artworkDataType=5");
    assertThat(summary).contains("missingExists=0");
  }

  @Test
  public void nativeMediaItemOpaqueTokenSmokeTest_resolvesRegisteredObjects() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    String tagToken =
        CppOpaqueObjectRegistry.register(
            new Object() {
              @Override
              public String toString() {
                return "registered-tag-object";
              }
            });
    String adsIdToken =
        CppOpaqueObjectRegistry.register(
            new Object() {
              @Override
              public String toString() {
                return "registered-ads-object";
              }
            });
    Bundle extras = new Bundle();
    extras.putString("opaque-key", "opaque-value");
    String extrasToken = CppOpaqueObjectRegistry.register(extras);

    String summary =
        CppBridgeNativePlayerTestHelper.nativeMediaItemOpaqueTokenSmokeTest(
            context, tagToken, adsIdToken, extrasToken);

    assertThat(summary).contains("tagString=registered-tag-object");
    assertThat(summary).contains("tagTokenPresent=1");
    assertThat(summary).contains("tagValuePresent=1");
    assertThat(summary).contains("tagValueType=5");
    assertThat(summary).contains("tagValueString=registered-tag-object");
    assertThat(summary).contains("adsId=registered-ads-object");
    assertThat(summary).contains("adsIdTokenPresent=1");
    assertThat(summary).contains("adsIdValuePresent=1");
    assertThat(summary).contains("adsIdValueType=5");
    assertThat(summary).contains("adsIdValueString=registered-ads-object");
    assertThat(summary).contains("requestMetadataExtrasPresent=1");
    assertThat(summary).contains("requestMetadataExtrasKeyCount=1");
    assertThat(summary).contains("requestMetadataExtrasTokenPresent=1");
  }

  @Test
  public void nativeMediaSetOverloadsSmokeTest_returnsUpdatedPlaylistSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativePlayerTestHelper.nativeMediaSetOverloadsSmokeTest(context);

    assertThat(summary).contains("count=3");
    assertThat(summary).contains("index=1");
    assertThat(summary).contains("positionMs=3456");
    assertThat(summary).contains("mediaId=overload-2");
  }
}
