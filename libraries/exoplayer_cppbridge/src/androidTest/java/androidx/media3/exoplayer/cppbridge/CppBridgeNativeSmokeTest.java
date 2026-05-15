package androidx.media3.exoplayer.cppbridge;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;
import androidx.media3.exoplayer.source.DefaultMediaSourceFactory;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public final class CppBridgeNativeSmokeTest {

  private static int extractIntMarker(String summary, String marker) {
    int start = summary.indexOf(marker);
    assertThat(start).isAtLeast(0);
    start += marker.length();
    int end = summary.indexOf(',', start);
    String value = end >= 0 ? summary.substring(start, end) : summary.substring(start);
    return Integer.parseInt(value);
  }

  @Test
  public void nativeBuildTrackSummaryForTest_returnsExpectedSummary() {
    String summary = CppBridgeNativeSmokeTestHelper.nativeBuildTrackSummaryForTest();

    assertThat(summary).contains("video group [selected]");
    assertThat(summary).contains("English [selected]");
    assertThat(summary).contains("audio group");
  }

  @Test
  public void nativeTracksSnapshotConversionSmokeTest_returnsStructuredSummary() {
    String summary = CppBridgeNativeSmokeTestHelper.nativeTracksSnapshotConversionSmokeTest();

    assertThat(summary).contains("groupCount=2");
    assertThat(summary).contains("containsAudio=1");
    assertThat(summary).contains("containsVideo=1");
    assertThat(summary).contains("containsText=0");
    assertThat(summary).contains("audioSelected=0");
    assertThat(summary).contains("videoSelected=1");
    assertThat(summary).contains("audioSupported=1");
    assertThat(summary).contains("videoSupported=1");
    assertThat(summary).contains("audioSupportedAllowingExceeds=0");
    assertThat(summary).contains("videoSupportedAllowingExceeds=1");
    assertThat(summary).contains("textSupportedAllowingExceeds=0");
    assertThat(summary).contains("group0Id=video-group");
    assertThat(summary).contains("group0TokenPresent=1");
    assertThat(summary).contains("group0Type=2");
    assertThat(summary).contains("group0Adaptive=1");
    assertThat(summary).contains("group0Selected=1");
    assertThat(summary).contains("group0Supported=1");
    assertThat(summary).contains("group0SupportedAllowingExceeds=1");
    assertThat(summary).contains("group0TrackCount=2");
    assertThat(summary).contains("track0Id=video-hd");
    assertThat(summary).contains("track0LabelTokenPresent=1");
    assertThat(summary).contains("track0Label=Main Video");
    assertThat(summary).contains("track0Language=");
    assertThat(summary).contains("track0MimeType=video/avc");
    assertThat(summary).contains("track0ContainerMimeType=video/mp4");
    assertThat(summary).contains("track0Codecs=avc1.640028");
    assertThat(summary).contains("track0Bitrate=2500000");
    assertThat(summary).contains("track0Width=1920");
    assertThat(summary).contains("track0Height=1080");
    assertThat(summary).contains("track0FrameRate=29.970000");
    assertThat(summary).contains("track0AccessibilityChannel=-1");
    assertThat(summary).contains("track0RoleFlags=0");
    assertThat(summary).contains("track0SelectionFlags=0");
    assertThat(summary).contains("track0Selected=1");
    assertThat(summary).contains("track0Supported=1");
    assertThat(summary).contains("track0SupportedWithinCapabilities=1");
    assertThat(summary).contains("track1Id=video-sd");
    assertThat(summary).contains("track1Selected=0");
    assertThat(summary).contains("track1Supported=1");
    assertThat(summary).contains("track1SupportedWithinCapabilities=0");
    assertThat(summary).contains("group1Id=audio-group");
    assertThat(summary).contains("group1TokenPresent=1");
    assertThat(summary).contains("group1Type=1");
    assertThat(summary).contains("group1Selected=1");
    assertThat(summary).contains("group1Supported=1");
    assertThat(summary).contains("group1SupportedAllowingExceeds=0");
    assertThat(summary).contains("group1TrackCount=1");
    assertThat(summary).contains("group1Track0Label=Main Audio");
    assertThat(summary).contains("group1Track0LabelTokenPresent=1");
    assertThat(summary).contains("group1Track0Language=en");
    assertThat(summary).contains("group1Track0MimeType=audio/mp4a-latm");
    assertThat(summary).contains("group1Track0ChannelCount=2");
    assertThat(summary).contains("group1Track0SampleRate=48000");
    assertThat(summary).contains("group1Track0RoleFlags=0");
    assertThat(summary).contains("group1Track0SelectionFlags=0");
  }

  @Test
  public void nativeCueSnapshotConversionSmokeTest_returnsStructuredSummary() {
    String summary = CppBridgeNativeSmokeTestHelper.nativeCueSnapshotConversionSmokeTest();

    assertThat(summary).contains("cueCount=2");
    assertThat(summary).contains("presentationTimeUs=987654");
    assertThat(summary).contains("textsCount=2");
    assertThat(summary).contains("text0=Hello Cue");
    assertThat(summary).contains("text0TokenPresent=1");
    assertThat(summary).contains("bitmap0TokenPresent=1");
    assertThat(summary).contains("text1=Second Cue");
    assertThat(summary).contains("text1TokenPresent=1");
    assertThat(summary).contains("cue0Text=Hello Cue");
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
    assertThat(summary).contains("cue0WindowColor=65280");
    assertThat(summary).contains("cue0HasBitmap=1");
    assertThat(summary).contains("cue1Text=Second Cue");
    assertThat(summary).contains("cue1TextTokenPresent=1");
    assertThat(summary).contains("cue1BitmapTokenPresent=0");
    assertThat(summary).contains("cue1LineType=1");
    assertThat(summary).contains("cue1PositionAnchor=1");
    assertThat(summary).contains("cue1TextSize=22.000000");
    assertThat(summary).contains("cue1TextSizeType=3");
    assertThat(summary).contains("cue1VerticalType=1");
  }

  @Test
  public void nativeListenerPayloadCaptureSmokeTest_returnsStructuredSummary() {
    String summary = CppBridgeNativeSmokeTestHelper.nativeListenerPayloadCaptureSmokeTest();

    assertThat(summary).contains("timelineCb=1");
    assertThat(summary).contains("timelineWindowCount=2");
    assertThat(summary).contains("timelinePeriodCount=3");
    assertThat(summary).contains("timelineCurrentMediaItemIndex=1");
    assertThat(summary).contains("timelineReason=9");
    assertThat(summary).contains("timelineWindow0Dynamic=1");
    assertThat(summary).contains("timelineWindow0TagPresent=1");
    assertThat(summary).contains("timelineWindow0TagString=payload-window-tag");
    assertThat(summary).contains("timelineWindow0TagTokenPresent=1");
    assertThat(summary).contains("timelineWindow0Uid=payload-window-uid");
    assertThat(summary).contains("timelineWindow0UidTokenPresent=1");
    assertThat(summary).contains("timelineWindow0LiveConfigurationPresent=1");
    assertThat(summary).contains("timelineWindow0LiveTargetOffsetMs=3333");
    assertThat(summary).contains("timelineWindow0LiveMinOffsetMs=2222");
    assertThat(summary).contains("timelineWindow0LiveMaxOffsetMs=5555");
    assertThat(summary).contains("timelineWindow0LiveMinSpeed=0.950000");
    assertThat(summary).contains("timelineWindow0LiveMaxSpeed=1.050000");
    assertThat(summary).contains("timelineWindow0ManifestPresent=1");
    assertThat(summary).contains("timelineWindow0ManifestString=payload-window-manifest");
    assertThat(summary).contains("timelineWindow0ManifestTokenPresent=1");
    assertThat(summary).contains("timelineWindow1MediaId=payload-window-1");
    assertThat(summary).contains("timelineWindow1MediaUri=https://example.com/payload-window-1.mp4");
    assertThat(summary).contains("timelineWindow1TagPresent=1");
    assertThat(summary).contains("timelineWindow1TagString=payload-window-tag-2");
    assertThat(summary).contains("timelineWindow1TagTokenPresent=1");
    assertThat(summary).contains("timelineWindow1Dynamic=0");
    assertThat(summary).contains("timelineWindow0PresentationStartTimeMs=111000");
    assertThat(summary).contains("timelinePeriod0Id=payload-period-id");
    assertThat(summary).contains("timelinePeriod0IdTokenPresent=1");
    assertThat(summary).contains("timelinePeriod0Uid=payload-period-0");
    assertThat(summary).contains("timelinePeriod0UidTokenPresent=1");
    assertThat(summary).contains("timelinePeriod0AdsId=payload-period-ads-id");
    assertThat(summary).contains("timelinePeriod0AdsIdTokenPresent=1");
    assertThat(summary).contains("timelinePeriod0AdGroupCount=2");
    assertThat(summary).contains("timelinePeriod0DurationMs=777");
    assertThat(summary).contains("timelinePeriod0DurationUs=777000");
    assertThat(summary).contains("timelinePeriod1Id=payload-period-id-2");
    assertThat(summary).contains("timelinePeriod1IdTokenPresent=1");
    assertThat(summary).contains("timelinePeriod1Uid=payload-period-1");
    assertThat(summary).contains("timelinePeriod1UidTokenPresent=1");
    assertThat(summary).contains("timelinePeriod1AdsId=payload-period-ads-id-2");
    assertThat(summary).contains("timelinePeriod1AdsIdTokenPresent=1");
    assertThat(summary).contains("timelinePeriod1DurationUs=888000");
    assertThat(summary).contains("tracksChangedCb=1");
    assertThat(summary).contains("trackGroupCount=2");
    assertThat(summary).contains("firstTrackGroupType=2");
    assertThat(summary).contains("firstTrackGroupId=payload-video");
    assertThat(summary).contains("firstTrackGroupTokenPresent=1");
    assertThat(summary).contains("firstTrackCount=1");
    assertThat(summary).contains("secondTrackGroupId=payload-audio");
    assertThat(summary).contains("secondTrackGroupTokenPresent=1");
    assertThat(summary).contains("secondTrackCount=1");
    assertThat(summary).contains("secondTrackLabel=Payload Audio");
    assertThat(summary).contains("secondTrackLabelTokenPresent=1");
    assertThat(summary).contains("firstTrackLabelTokenPresent=1");
    assertThat(summary).contains("firstTrackSelected=1");
    assertThat(summary).contains("firstTrackSupported=1");
    assertThat(summary).contains("firstTrackSupportedWithinCapabilities=1");
    assertThat(summary).contains("containsAudio=1");
    assertThat(summary).contains("containsVideo=1");
    assertThat(summary).contains("containsText=0");
    assertThat(summary).contains("audioSelected=1");
    assertThat(summary).contains("videoSelected=1");
    assertThat(summary).contains("videoSupported=1");
    assertThat(summary).contains("videoSupportedAllowingExceeds=1");
    assertThat(summary).contains("positionDiscontinuityCb=1");
    assertThat(summary).contains("positionDiscontinuityReason=12");
    assertThat(summary).contains("oldMediaId=payload-old");
    assertThat(summary).contains("oldTagTokenPresent=1");
    assertThat(summary).contains("newMediaId=payload-new");
    assertThat(summary).contains("newTagTokenPresent=1");
    assertThat(summary).contains("availableCommandsCb=1");
    assertThat(summary).contains("availableCommandsCount=3");
    assertThat(summary).contains("eventsCb=1");
    assertThat(summary).contains("eventCount=2");
  }

  @Test
  public void nativeBuilderConfigSmokeTest_returnsConfiguredSnapshot() {
    String summary = CppBridgeNativeSmokeTestHelper.nativeBuilderConfigSmokeTest();

    assertThat(summary).contains("handleAudioFocus=0");
    assertThat(summary).contains("handleAudioBecomingNoisy=0");
    assertThat(summary).contains("useLazyPreparation=0");
    assertThat(summary).contains("seekBackIncrementMs=1111");
    assertThat(summary).contains("seekForwardIncrementMs=2222");
    assertThat(summary).contains("wakeMode=1");
    assertThat(summary).contains("priority=77");
    assertThat(summary).contains("usePriorityTaskManager=1");
    assertThat(summary).contains("targetPreloadDurationUs=987654");
    assertThat(summary).contains("parseSubtitlesDuringExtraction=0");
    assertThat(summary).contains("loadOnlySelectedTracks=1");
    assertThat(summary).contains("userAgent=Builder UA");
    assertThat(summary).contains("headerCount=1");
    assertThat(summary).contains("header0=X-Test:1");
    assertThat(summary).contains("connectTimeoutMs=3333");
    assertThat(summary).contains("readTimeoutMs=4444");
    assertThat(summary).contains("allowCrossProtocolRedirects=1");
    assertThat(summary).contains("liveTargetOffsetMs=5555");
    assertThat(summary).contains("liveMinOffsetMs=4444");
    assertThat(summary).contains("liveMaxOffsetMs=6666");
    assertThat(summary).contains("liveMinSpeed=0.950000");
    assertThat(summary).contains("liveMaxSpeed=1.050000");
  }

  @Test
  public void nativeBuilderBuildSmokeTest_buildsConfiguredPlayer() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativeSmokeTestHelper.nativeBuilderBuildSmokeTest(context);

    assertThat(summary).contains("builderBuild=1");
    assertThat(summary).contains("targetPreloadDurationUs=321654");
    assertThat(summary).contains("seekBackIncrementMs=1357");
    assertThat(summary).contains("seekForwardIncrementMs=2468");
    assertThat(summary).contains("parseSubtitlesDuringExtraction=0");
    assertThat(summary).contains("loadOnlySelectedTracks=1");
    assertThat(summary).contains("headerCount=2");
    assertThat(summary).contains("header0=X-Build:yes");
    assertThat(summary).contains("header1=X-Build-Trace:trace-build");
    assertThat(summary).contains("userAgent=Builder Build UA");
    assertThat(summary).contains("connectTimeoutMs=5555");
    assertThat(summary).contains("readTimeoutMs=6666");
    assertThat(summary).contains("allowCrossProtocolRedirects=1");
    assertThat(summary).contains("liveTargetOffsetMs=7777");
    assertThat(summary).contains("liveMinOffsetMs=7000");
    assertThat(summary).contains("liveMaxOffsetMs=8000");
    assertThat(summary).contains("liveMinSpeed=0.980000");
    assertThat(summary).contains("liveMaxSpeed=1.020000");
  }

  @Test
  public void nativePlayerDoubleReleaseSmokeTest_isIdempotent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativeSmokeTestHelper.nativePlayerDoubleReleaseSmokeTest(context);

    assertThat(summary).contains("playerBuild=1");
    assertThat(summary).contains("doubleReleaseSafe=1");
  }

  @Test
  public void nativeListenerLifecycleNegativeSmokeTest_handlesRepeatedAndNullRemoval() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativeSmokeTestHelper.nativeListenerLifecycleNegativeSmokeTest(context);

    assertThat(summary).contains("playerBuild=1");
    assertThat(summary).contains("listenerMutationSequenceSafe=1");
    assertThat(summary).contains("analyticsNullClearSafe=1");
    assertThat(summary).contains("releaseAfterMutationSafe=1");
  }

  @Test
  public void nativeOpaqueTokenReleaseSmokeTest_releasesRegisteredTokensThroughSdk() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    Object object = new Object();
    String token = CppOpaqueObjectRegistry.register(object);

    try {
      assertThat(CppOpaqueObjectRegistry.resolve(token)).isSameInstanceAs(object);

      String summary =
          CppBridgeNativeSmokeTestHelper.nativeOpaqueTokenReleaseSmokeTest(
              context, new String[] {token, token, "", null});

      assertThat(summary).contains("playerBuild=1");
      assertThat(summary).contains("releaseCallSafe=1");
      assertThat(summary).contains("tokenCount=3");
      assertThat(CppOpaqueObjectRegistry.resolve(token)).isNull();
    } finally {
      CppOpaqueObjectRegistry.unregister(token);
    }
  }

  @Test
  public void nativeBuilderPreloadRoundTripSmokeTest_updatesAndRestoresPreloadTarget() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativeSmokeTestHelper.nativeBuilderPreloadRoundTripSmokeTest(context);

    assertThat(summary).contains("builderPreloadRoundTrip=1");
    assertThat(summary).contains("initialTargetPreloadDurationUs=111111");
    assertThat(summary).contains("afterUpdateTargetPreloadDurationUs=222222");
    assertThat(summary).contains("afterUnsetTargetPreloadDurationUs=-9223372036854775807");
    assertThat(summary).contains("afterResetTargetPreloadDurationUs=333333");
    assertThat(summary).contains("updateApplied=1");
    assertThat(summary).contains("unsetApplied=1");
    assertThat(summary).contains("resetApplied=1");
  }

  @Test
  public void nativeBuilderMediaSourceFactoryInjectionSmokeTest_buildsWithRegisteredFactoryToken() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    String token = "test-injected-media-source-factory";
    CppMediaSourceFactoryRegistry.register(token, new DefaultMediaSourceFactory(context));
    try {
      String summary =
          CppBridgeNativeSmokeTestHelper.nativeBuilderMediaSourceFactoryInjectionSmokeTest(context);

      assertThat(summary).contains("builderInjectedFactoryBuild=1");
      assertThat(summary).contains("configFactoryToken=test-injected-media-source-factory");
      assertThat(summary).contains("resolvedFactoryToken=test-injected-media-source-factory");
      assertThat(summary).contains("resolvedInjectedFactoryUsed=1");
      assertThat(summary).contains("resolvedFactoryIdentity=");
      assertThat(summary).contains("userAgent=Builder Injected Factory UA");
    } finally {
      CppMediaSourceFactoryRegistry.unregister(token);
    }
  }

  @Test
  public void nativeBuilderMediaSourceFactoryFallbackSmokeTest_buildsWithDefaultFactoryWhenTokenIsMissing() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativeSmokeTestHelper.nativeBuilderMediaSourceFactoryFallbackSmokeTest(context);

    assertThat(summary).contains("builderFallbackFactoryBuild=1");
    assertThat(summary).contains("configFactoryToken=missing-media-source-factory-token");
    assertThat(summary).contains("resolvedFactoryToken=missing-media-source-factory-token");
    assertThat(summary).contains("resolvedInjectedFactoryUsed=0");
    assertThat(summary).contains("resolvedFactoryIdentity=0");
    assertThat(summary).contains("fallbackApplied=1");
    assertThat(summary).contains("userAgent=Builder Fallback UA");
  }

  @Test
  public void nativeBuilderMediaSourceFactoryInjectionReplacementSmokeTest_usesLatestRegisteredFactory() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    String token = "replaceable-media-source-factory-token";
    try {
      CppMediaSourceFactoryRegistry.register(token, new DefaultMediaSourceFactory(context));
      String firstSummary =
          CppBridgeNativeSmokeTestHelper.nativeBuilderMediaSourceFactoryInjectionReplacementSmokeTest(
              context);

      CppMediaSourceFactoryRegistry.register(token, new DefaultMediaSourceFactory(context));
      String secondSummary =
          CppBridgeNativeSmokeTestHelper.nativeBuilderMediaSourceFactoryInjectionReplacementSmokeTest(
              context);

      assertThat(firstSummary).contains("builderReplacementFactoryBuild=1");
      assertThat(firstSummary).contains("configFactoryToken=replaceable-media-source-factory-token");
      assertThat(firstSummary).contains("resolvedFactoryToken=replaceable-media-source-factory-token");
      assertThat(firstSummary).contains("resolvedInjectedFactoryUsed=1");
      assertThat(secondSummary).contains("builderReplacementFactoryBuild=1");
      assertThat(secondSummary).contains("resolvedInjectedFactoryUsed=1");
      assertThat(extractIntMarker(firstSummary, "resolvedFactoryIdentity=")).isGreaterThan(0);
      assertThat(extractIntMarker(secondSummary, "resolvedFactoryIdentity=")).isGreaterThan(0);
      assertThat(extractIntMarker(secondSummary, "resolvedFactoryIdentity="))
          .isNotEqualTo(extractIntMarker(firstSummary, "resolvedFactoryIdentity="));
    } finally {
      CppMediaSourceFactoryRegistry.unregister(token);
    }
  }

  @Test
  public void nativeBuilderMediaSourceFactoryInjectionMultiTokenSmokeTest_keepsTokensIsolated() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    String firstToken = "multi-token-media-source-factory-a";
    String secondToken = "multi-token-media-source-factory-b";
    try {
      CppMediaSourceFactoryRegistry.register(firstToken, new DefaultMediaSourceFactory(context));
      CppMediaSourceFactoryRegistry.register(secondToken, new DefaultMediaSourceFactory(context));

      String summary =
          CppBridgeNativeSmokeTestHelper.nativeBuilderMediaSourceFactoryInjectionMultiTokenSmokeTest(
              context);

      assertThat(summary).contains("builderMultiTokenBuild=1");
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
  public void nativeBuilderMediaSourceFactoryGeneratedTokenSmokeTest_buildsWithGeneratedRegistryToken() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    String token = CppMediaSourceFactoryRegistry.register(new DefaultMediaSourceFactory(context));
    try {
      String summary =
          CppBridgeNativeSmokeTestHelper.nativeBuilderMediaSourceFactoryGeneratedTokenSmokeTest(
              context, token);

      assertThat(summary).contains("builderGeneratedTokenBuild=1");
      assertThat(summary).contains("resolvedFactoryToken=" + token);
      assertThat(summary).contains("resolvedInjectedFactoryUsed=1");
      assertThat(summary).contains("generatedTokenPath=1");
      assertThat(summary).contains("userAgent=Builder Generated Token UA");
      assertThat(extractIntMarker(summary, "resolvedFactoryIdentity=")).isGreaterThan(0);
    } finally {
      CppMediaSourceFactoryRegistry.unregister(token);
    }
  }

  @Test
  public void nativePriorityTaskManagerWrapperSmokeTest_returnsStructuredSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary =
        CppBridgeNativeSmokeTestHelper.nativePriorityTaskManagerWrapperSmokeTest(context);

    assertThat(summary).contains("proceed77=1");
    assertThat(summary).contains("proceed76=0");
    assertThat(summary).contains("attachedEnabled=1");
    assertThat(summary).contains("attachedState=1");
    assertThat(summary).contains("attachedRegistered=1");
    assertThat(summary).contains("attachedPriority=");
    assertThat(summary).contains("clearedEnabled=0");
    assertThat(summary).contains("clearedState=0");
    assertThat(summary).contains("clearedRegistered=0");
    assertThat(summary).contains("clearedPriority=");
    assertThat(summary).contains("proceedAfterRemove=0");
  }

  @Test
  public void nativeVideoEffectsConversionSmokeTest_returnsStructuredSummary() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    String summary = CppBridgeNativeSmokeTestHelper.nativeVideoEffectsConversionSmokeTest(context);

    assertThat(summary).contains("effectCount=2");
    assertThat(summary).contains("effect0=scaleAndRotate:scaleX=1.5:scaleY=0.75:rotationDegrees=45.0");
    assertThat(summary)
        .contains("effect1=rgbAdjustment:redScale=1.2:greenScale=0.8:blueScale=1.1");
    assertThat(summary).contains("afterClear=effectCount=0");
    assertThat(summary)
        .contains(
            "afterReapply=effectCount=7,effect0=rgbAdjustment:redScale=0.9:greenScale=1.05:blueScale=1.15");
    assertThat(summary)
        .contains("effect1=scaleAndRotate:scaleX=2.0:scaleY=0.5:rotationDegrees=90.0");
    assertThat(summary)
        .contains("effect2=scaleAndRotate:scaleX=1.0:scaleY=1.0:rotationDegrees=180.0");
    assertThat(summary)
        .contains("effect3=rgbAdjustment:redScale=1.0:greenScale=1.0:blueScale=1.0");
    assertThat(summary)
        .contains("effect4=scaleAndRotate:scaleX=1.0:scaleY=1.0:rotationDegrees=0.0");
    assertThat(summary).contains("effect5=presentation:width=640:height=360:layout=2");
    assertThat(summary).contains("effect6=presentation:width=320:height=240:layout=1");
  }

  @Test
  public void nativeBuildPlaylistIdsForTest_assignsStablePlaylistIds() {
    String playlistIds =
        CppBridgeNativeSmokeTestHelper.nativeBuildPlaylistIdsForTest(
            new String[] {"https://example.com/a.mp4", "https://example.com/b.mp4"});

    assertThat(playlistIds).isEqualTo("playlist-item-0,playlist-item-1");
  }
}
