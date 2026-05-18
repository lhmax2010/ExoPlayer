package androidx.media3.exoplayer.cppbridge;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.text.Layout;
import androidx.media3.common.C;
import androidx.media3.common.ColorInfo;
import androidx.media3.common.DrmInitData;
import androidx.media3.common.Effect;
import androidx.media3.common.Label;
import androidx.media3.common.MediaItem;
import androidx.media3.common.MediaMetadata;
import androidx.media3.common.Metadata;
import androidx.media3.common.TrackGroup;
import androidx.media3.common.TrackSelectionOverride;
import androidx.media3.common.TrackSelectionParameters;
import androidx.media3.common.Tracks;
import androidx.media3.common.Player;
import androidx.media3.common.Format;
import androidx.media3.common.text.Cue;
import androidx.media3.effect.Presentation;
import androidx.media3.effect.RgbAdjustment;
import androidx.media3.effect.ScaleAndRotateTransformation;
import androidx.media3.exoplayer.source.DefaultMediaSourceFactory;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.core.app.ApplicationProvider;
import java.util.Arrays;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public final class CppBridgeConvertersTest {

  @Test
  public void toMediaItem_mapsAdvancedFields() {
    CppMediaItem item =
        new CppMediaItem(
            "https://example.com/video.mpd",
            "media-id",
            "application/dash+xml",
            1,
            false,
            null,
            null,
            null,
            null,
            null,
            new CppSubtitleConfiguration[] {
              new CppSubtitleConfiguration(
                  "https://example.com/sub.vtt", "text/vtt", "en", "English", "sub-en", 1, 2)
            },
            new CppClippingConfiguration(1000, 5000, false, true, true, false),
            new CppLiveConfiguration(3000, 2000, 5000, 0.97f, 1.03f),
            new CppDrmConfiguration(
                "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed",
                "https://license.example.com",
                new String[] {"Authorization", "X-Test"},
                new String[] {"Bearer abc", "1"},
                new int[] {C.TRACK_TYPE_AUDIO, C.TRACK_TYPE_VIDEO},
                new byte[] {1, 2, 3},
                true,
                true,
                false));

    MediaItem mediaItem = CppBridgeConverters.toMediaItem(item);

    assertThat(mediaItem.mediaId).isEqualTo("media-id");
    assertThat(mediaItem.localConfiguration).isNotNull();
    assertThat(mediaItem.localConfiguration.mimeType).isEqualTo("application/dash+xml");
    assertThat(mediaItem.localConfiguration.subtitleConfigurations).hasSize(1);
    assertThat(mediaItem.localConfiguration.subtitleConfigurations.get(0).language).isEqualTo("en");
    assertThat(mediaItem.clippingConfiguration.startPositionMs).isEqualTo(1000);
    assertThat(mediaItem.clippingConfiguration.endPositionMs).isEqualTo(5000);
    assertThat(mediaItem.liveConfiguration.targetOffsetMs).isEqualTo(3000);
    assertThat(mediaItem.localConfiguration.drmConfiguration).isNotNull();
    assertThat(mediaItem.localConfiguration.drmConfiguration.multiSession).isTrue();
    assertThat(mediaItem.localConfiguration.drmConfiguration.licenseRequestHeaders)
        .containsEntry("Authorization", "Bearer abc");
    assertThat(mediaItem.localConfiguration.drmConfiguration.forcedSessionTrackTypes)
        .containsExactly(C.TRACK_TYPE_AUDIO, C.TRACK_TYPE_VIDEO)
        .inOrder();
    assertThat(
            Arrays.equals(
                mediaItem.localConfiguration.drmConfiguration.getKeySetId(),
                new byte[] {1, 2, 3}))
        .isTrue();
    assertThat(mediaItem.localConfiguration.drmConfiguration.playClearContentWithoutKey).isFalse();
  }

  @Test
  public void toMediaItem_infersMimeTypeFromSourceTypeHint() {
    CppMediaItem item =
        new CppMediaItem(
            "https://example.com/live.m3u8",
            "hls-id",
            null,
            2,
            false,
            null,
            null,
            null,
            null,
            null,
            new CppSubtitleConfiguration[0],
            null,
            null,
            null);

    MediaItem mediaItem = CppBridgeConverters.toMediaItem(item);

    assertThat(mediaItem.localConfiguration).isNotNull();
    assertThat(mediaItem.localConfiguration.mimeType).isEqualTo("application/x-mpegURL");
  }

  @Test
  public void toMediaItem_normalizesHlsMimeTypeAliases() {
    CppMediaItem item =
        new CppMediaItem(
            "https://example.com/live",
            "hls-id",
            "application/vnd.apple.mpegurl",
            0,
            false,
            null,
            null,
            null,
            null,
            null,
            new CppSubtitleConfiguration[0],
            null,
            null,
            null);

    MediaItem mediaItem = CppBridgeConverters.toMediaItem(item);

    assertThat(mediaItem.localConfiguration).isNotNull();
    assertThat(mediaItem.localConfiguration.mimeType).isEqualTo("application/x-mpegURL");
  }

  @Test
  public void toMediaItem_withNullUriFallsBackToEmptyUri() {
    CppMediaItem item =
        new CppMediaItem(
            null,
            "no-uri-id",
            null,
            0,
            false,
            null,
            null,
            null,
            null,
            null,
            new CppSubtitleConfiguration[0],
            null,
            null,
            null);

    MediaItem mediaItem = CppBridgeConverters.toMediaItem(item);

    assertThat(mediaItem.localConfiguration).isNotNull();
    assertThat(mediaItem.localConfiguration.uri).isEqualTo(Uri.EMPTY);
  }

  @Test
  public void toMediaItem_mapsAdsConfiguration() {
    CppMediaItem item =
        new CppMediaItem(
            "https://example.com/content.m3u8",
            "ads-id",
            null,
            2,
            false,
            null,
            null,
            null,
            null,
            new CppAdsConfiguration(
                "https://example.com/ads-tag.vmap", "ads-object", "ads-token-1"),
            new CppSubtitleConfiguration[0],
            null,
            null,
            null);
    CppOpaqueObjectRegistry.register("ads-token-1", "resolved-ads-object");

    try {
      MediaItem mediaItem = CppBridgeConverters.toMediaItem(item);

      assertThat(mediaItem.localConfiguration).isNotNull();
      assertThat(mediaItem.localConfiguration.adsConfiguration).isNotNull();
      assertThat(mediaItem.localConfiguration.adsConfiguration.adTagUri.toString())
          .isEqualTo("https://example.com/ads-tag.vmap");
      assertThat(mediaItem.localConfiguration.adsConfiguration.adsId)
          .isEqualTo("resolved-ads-object");
    } finally {
      CppOpaqueObjectRegistry.unregister("ads-token-1");
    }
  }

  @Test
  public void toMediaItem_buildsRequestMetadataExtrasFromDecodedValues() {
    CppMediaItem item =
        new CppMediaItem(
            "https://example.com/content.m3u8",
            "request-extras-id",
            null,
            2,
            false,
            null,
            null,
            null,
            new CppRequestMetadata(
                "https://example.com/request",
                "search terms",
                true,
                5,
                null,
                new CppBundleValue[] {
                  new CppBundleValue(
                      "enabled", CppBundleValue.TYPE_BOOLEAN, null, 0L, 0.0, true, null),
                  new CppBundleValue(
                      "episode", CppBundleValue.TYPE_LONG, null, 42L, 0.0, false, null),
                  new CppBundleValue(
                      "gain", CppBundleValue.TYPE_DOUBLE, null, 0L, 1.5, false, null),
                  new CppBundleValue(
                      "payload",
                      CppBundleValue.TYPE_BYTE_ARRAY,
                      null,
                      0L,
                      0.0,
                      false,
                      new byte[] {1, 2, 3}),
                  new CppBundleValue(
                      "source",
                      CppBundleValue.TYPE_STRING,
                      "cppbridge",
                      0L,
                      0.0,
                      false,
                      null)
                }),
            null,
            new CppSubtitleConfiguration[0],
            null,
            null,
            null);

    MediaItem mediaItem = CppBridgeConverters.toMediaItem(item);

    assertThat(mediaItem.requestMetadata.mediaUri.toString())
        .isEqualTo("https://example.com/request");
    assertThat(mediaItem.requestMetadata.searchQuery).isEqualTo("search terms");
    assertThat(mediaItem.requestMetadata.extras.getBoolean("enabled")).isTrue();
    assertThat(mediaItem.requestMetadata.extras.getLong("episode")).isEqualTo(42L);
    assertThat(mediaItem.requestMetadata.extras.getDouble("gain")).isEqualTo(1.5);
    assertThat(mediaItem.requestMetadata.extras.getByteArray("payload"))
        .isEqualTo(new byte[] {1, 2, 3});
    assertThat(mediaItem.requestMetadata.extras.getString("source")).isEqualTo("cppbridge");
  }

  @Test
  public void toMediaItem_buildsRequestMetadataExtrasWhenPresenceFlagIsFalse() {
    CppMediaItem item =
        new CppMediaItem(
            "https://example.com/content.mpd",
            "request-extras-id",
            null,
            1,
            false,
            null,
            null,
            null,
            new CppRequestMetadata(
                null,
                null,
                false,
                0,
                null,
                new CppBundleValue[] {
                  new CppBundleValue(
                      "source", CppBundleValue.TYPE_STRING, "decoded", 0L, 0.0, false, null)
                }),
            null,
            new CppSubtitleConfiguration[0],
            null,
            null,
            null);

    MediaItem mediaItem = CppBridgeConverters.toMediaItem(item);

    assertThat(mediaItem.requestMetadata.extras.getString("source")).isEqualTo("decoded");
  }

  @Test
  public void trackSelectionRoundTrip_preservesPreferences() {
    CppTrackSelectionParameters cppParameters =
        new CppTrackSelectionParameters(
            "de",
            "en",
            new String[] {"de", "fr"},
            new String[] {"en", "es"},
            4,
            8,
            6,
            384000,
            1280,
            720,
            2_000_000,
            1920,
            1080,
            false,
            true,
            C.SELECTION_FLAG_FORCED,
            true,
            true,
            false,
            false,
            true,
            false,
            new int[] {C.TRACK_TYPE_AUDIO, C.TRACK_TYPE_IMAGE},
            new CppTrackSelectionOverride[] {
              new CppTrackSelectionOverride("audio-group", C.TRACK_TYPE_AUDIO, new int[] {0})
            });

    TrackGroup audioTrackGroup =
        new TrackGroup(
            "audio-group",
            new Format.Builder()
                .setId("audio-1")
                .setSampleMimeType("audio/mp4a-latm")
                .setLanguage("de")
                .build());
    Tracks currentTracks =
        new Tracks(
            Arrays.asList(
                new Tracks.Group(
                    audioTrackGroup,
                    false,
                    new int[] {C.FORMAT_HANDLED},
                    new boolean[] {true})));

    TrackSelectionParameters platformParameters =
        CppBridgeConverters.toTrackSelectionParameters(
            TrackSelectionParameters.DEFAULT, currentTracks, cppParameters);
    CppTrackSelectionParameters roundTrip =
        CppBridgeConverters.fromTrackSelectionParameters(platformParameters);

    assertThat(roundTrip.preferredAudioLanguage).isEqualTo("de");
    assertThat(roundTrip.preferredTextLanguage).isEqualTo("en");
    assertThat(roundTrip.preferredAudioLanguages).asList().containsExactly("de", "fr").inOrder();
    assertThat(roundTrip.preferredTextLanguages).asList().containsExactly("en", "es").inOrder();
    assertThat(roundTrip.maxAudioChannelCount).isEqualTo(6);
    assertThat(roundTrip.maxAudioBitrate).isEqualTo(384000);
    assertThat(roundTrip.maxVideoWidth).isEqualTo(1280);
    assertThat(roundTrip.maxVideoHeight).isEqualTo(720);
    assertThat(roundTrip.maxVideoBitrate).isEqualTo(2_000_000);
    assertThat(roundTrip.viewportWidth).isEqualTo(1920);
    assertThat(roundTrip.viewportHeight).isEqualTo(1080);
    assertThat(roundTrip.viewportOrientationMayChange).isFalse();
    assertThat(roundTrip.selectTextByDefault).isTrue();
    assertThat(roundTrip.ignoredTextSelectionFlags).isEqualTo(C.SELECTION_FLAG_FORCED);
    assertThat(roundTrip.selectUndeterminedTextLanguage).isTrue();
    assertThat(roundTrip.forceLowestBitrate).isTrue();
    assertThat(roundTrip.disableAudio).isTrue();
    assertThat(roundTrip.disabledTrackTypes)
        .asList()
        .containsExactly(C.TRACK_TYPE_AUDIO, C.TRACK_TYPE_IMAGE);
    assertThat(roundTrip.overrides).hasLength(1);
    assertThat(roundTrip.overrides[0].trackGroupId).isEqualTo("audio-group");
    assertThat(roundTrip.overrides[0].trackType).isEqualTo(C.TRACK_TYPE_AUDIO);
    assertThat(roundTrip.overrides[0].trackIndices).asList().containsExactly(0);
    assertThat(platformParameters.overrides.values())
        .containsExactly(new TrackSelectionOverride(audioTrackGroup, 0));
    assertThat(platformParameters.disabledTrackTypes)
        .containsAtLeast(C.TRACK_TYPE_AUDIO, C.TRACK_TYPE_IMAGE);
    assertThat(platformParameters.maxAudioChannelCount).isEqualTo(6);
    assertThat(platformParameters.maxAudioBitrate).isEqualTo(384000);
    assertThat(platformParameters.ignoredTextSelectionFlags).isEqualTo(C.SELECTION_FLAG_FORCED);
    assertThat(platformParameters.selectUndeterminedTextLanguage).isTrue();
  }

  @Test
  public void toCppTrackGroups_mapsSelectionAndSupport() {
    TrackGroup trackGroup =
        new TrackGroup(
            "group-1",
            new Format.Builder()
                .setId("video-1")
                .setLabel("Main Video")
                .setLabels(
                    Arrays.asList(new Label("en", "Main Video"), new Label("es", "Video principal")))
                .setSampleMimeType("video/avc")
                .setContainerMimeType("video/mp4")
                .setCodecs("avc1.640028")
                .setMetadata(new Metadata(new Metadata.Entry() {}, new Metadata.Entry() {}))
                .setCustomData("custom-format-payload")
                .setMaxInputSize(4096)
                .setMaxNumReorderSamples(3)
                .setInitializationData(Arrays.asList(new byte[] {1, 2, 3}, new byte[] {4, 5, 6, 7}))
                .setDrmInitData(
                    new DrmInitData(
                        "cenc",
                        new DrmInitData.SchemeData(
                            C.WIDEVINE_UUID,
                            "https://license.example/video",
                            "video/mp4",
                            new byte[] {8, 9})))
                .setSubsampleOffsetUs(987_654)
                .setHasPrerollSamples(true)
                .setWidth(1920)
                .setHeight(1080)
                .setDecodedWidth(1936)
                .setDecodedHeight(1096)
                .setFrameRate(23.976f)
                .setAverageBitrate(4_000_000)
                .setPeakBitrate(5_000_000)
                .setRotationDegrees(90)
                .setPixelWidthHeightRatio(1.25f)
                .setProjectionData(new byte[] {9, 8, 7, 6})
                .setStereoMode(C.STEREO_MODE_LEFT_RIGHT)
                .setColorInfo(
                    new ColorInfo.Builder()
                        .setColorSpace(C.COLOR_SPACE_BT709)
                        .setColorRange(C.COLOR_RANGE_LIMITED)
                        .setColorTransfer(C.COLOR_TRANSFER_SDR)
                        .setHdrStaticInfo(new byte[] {10, 11, 12})
                        .setLumaBitdepth(10)
                        .setChromaBitdepth(10)
                        .build())
                .setMaxSubLayers(4)
                .setPcmEncoding(C.ENCODING_PCM_16BIT)
                .setEncoderDelay(12)
                .setEncoderPadding(34)
                .setCueReplacementBehavior(Format.CUE_REPLACEMENT_BEHAVIOR_REPLACE)
                .setTileCountHorizontal(5)
                .setTileCountVertical(6)
                .setCryptoType(C.CRYPTO_TYPE_FRAMEWORK)
                .setRoleFlags(C.ROLE_FLAG_MAIN | C.ROLE_FLAG_AUXILIARY)
                .setAuxiliaryTrackType(C.AUXILIARY_TRACK_TYPE_DEPTH_LINEAR)
                .setSelectionFlags(C.SELECTION_FLAG_DEFAULT)
                .build(),
            new Format.Builder()
                .setId("video-2")
                .setSampleMimeType("video/avc")
                .setCodecs("avc1.4d401f")
                .setWidth(1280)
                .setHeight(720)
                .setAverageBitrate(2_000_000)
                .build());
    TrackGroup exceedsOnlyTrackGroup =
        new TrackGroup(
            "group-2",
            new Format.Builder()
                .setId("video-3")
                .setSampleMimeType("video/avc")
                .setCodecs("avc1.640033")
                .setWidth(3840)
                .setHeight(2160)
                .setAverageBitrate(12_000_000)
                .build());
    TrackGroup textTrackGroup =
        new TrackGroup(
            "group-3",
            new Format.Builder()
                .setId("text-1")
                .setSampleMimeType("text/vtt")
                .setContainerMimeType("application/x-subrip")
                .setLanguage("en")
                .setAccessibilityChannel(2)
                .build());
    Tracks tracks =
        new Tracks(
            Arrays.asList(
                new Tracks.Group(
                    trackGroup,
                    true,
                    new int[] {C.FORMAT_HANDLED, C.FORMAT_EXCEEDS_CAPABILITIES},
                    new boolean[] {true, false}),
                new Tracks.Group(
                    exceedsOnlyTrackGroup,
                    false,
                    new int[] {C.FORMAT_EXCEEDS_CAPABILITIES},
                    new boolean[] {false}),
                new Tracks.Group(
                    textTrackGroup,
                    false,
                    new int[] {C.FORMAT_HANDLED},
                    new boolean[] {false})));

    CppTrackGroup[] groups = CppBridgeConverters.toCppTrackGroups(tracks);

    assertThat(groups).hasLength(3);
    assertThat(groups[0].id).isEqualTo("group-1");
    assertThat(groups[0].adaptiveSupported).isTrue();
    assertThat(groups[0].selected).isTrue();
    assertThat(groups[0].supported).isTrue();
    assertThat(groups[0].supportedAllowingExceedsCapabilities).isTrue();
    assertThat(groups[0].tracks).hasLength(2);
    assertThat(groups[0].tracks[0].id).isEqualTo("video-1");
    assertThat(groups[0].tracks[0].containerMimeType).isEqualTo("video/mp4");
    assertThat(groups[0].tracks[0].codecs).isEqualTo("avc1.640028");
    assertThat(groups[0].tracks[0].bitrate).isEqualTo(5_000_000);
    assertThat(groups[0].tracks[0].averageBitrate).isEqualTo(4_000_000);
    assertThat(groups[0].tracks[0].peakBitrate).isEqualTo(5_000_000);
    assertThat(groups[0].tracks[0].metadataEntryCount).isEqualTo(2);
    assertThat(groups[0].tracks[0].metadataToken).isNotNull();
    assertThat(groups[0].tracks[0].labelLanguages).asList().containsExactly("en", "es");
    assertThat(groups[0].tracks[0].labelValues)
        .asList()
        .containsExactly("Main Video", "Video principal");
    assertThat(groups[0].tracks[0].customDataToken).isNotNull();
    assertThat(groups[0].tracks[0].maxInputSize).isEqualTo(4096);
    assertThat(groups[0].tracks[0].maxNumReorderSamples).isEqualTo(3);
    assertThat(groups[0].tracks[0].initializationDataCount).isEqualTo(2);
    assertThat(groups[0].tracks[0].initializationDataTotalBytes).isEqualTo(7);
    assertThat(groups[0].tracks[0].initializationData).hasLength(2);
    assertThat(groups[0].tracks[0].initializationData[0]).isEqualTo(new byte[] {1, 2, 3});
    assertThat(groups[0].tracks[0].initializationData[1]).isEqualTo(new byte[] {4, 5, 6, 7});
    assertThat(groups[0].tracks[0].drmSchemeType).isEqualTo("cenc");
    assertThat(groups[0].tracks[0].drmSchemeDataCount).isEqualTo(1);
    assertThat(groups[0].tracks[0].drmSchemeUuids)
        .asList()
        .containsExactly(C.WIDEVINE_UUID.toString());
    assertThat(groups[0].tracks[0].drmSchemeLicenseServerUrls)
        .asList()
        .containsExactly("https://license.example/video");
    assertThat(groups[0].tracks[0].drmSchemeMimeTypes).asList().containsExactly("video/mp4");
    assertThat(groups[0].tracks[0].drmSchemeData).hasLength(1);
    assertThat(groups[0].tracks[0].drmSchemeData[0]).isEqualTo(new byte[] {8, 9});
    assertThat(groups[0].tracks[0].drmSchemeDataHasData).asList().containsExactly(1);
    assertThat(groups[0].tracks[0].subsampleOffsetUs).isEqualTo(987_654);
    assertThat(groups[0].tracks[0].hasPrerollSamples).isTrue();
    assertThat(groups[0].tracks[0].decodedWidth).isEqualTo(1936);
    assertThat(groups[0].tracks[0].decodedHeight).isEqualTo(1096);
    assertThat(groups[0].tracks[0].frameRate).isEqualTo(23.976f);
    assertThat(groups[0].tracks[0].rotationDegrees).isEqualTo(90);
    assertThat(groups[0].tracks[0].pixelWidthHeightRatio).isEqualTo(1.25f);
    assertThat(groups[0].tracks[0].projectionDataLength).isEqualTo(4);
    assertThat(groups[0].tracks[0].projectionData).isEqualTo(new byte[] {9, 8, 7, 6});
    assertThat(groups[0].tracks[0].stereoMode).isEqualTo(C.STEREO_MODE_LEFT_RIGHT);
    assertThat(groups[0].tracks[0].colorStandard).isEqualTo(C.COLOR_SPACE_BT709);
    assertThat(groups[0].tracks[0].colorRange).isEqualTo(C.COLOR_RANGE_LIMITED);
    assertThat(groups[0].tracks[0].colorTransfer).isEqualTo(C.COLOR_TRANSFER_SDR);
    assertThat(groups[0].tracks[0].colorHdrStaticInfo).isEqualTo(new byte[] {10, 11, 12});
    assertThat(groups[0].tracks[0].colorLumaBitdepth).isEqualTo(10);
    assertThat(groups[0].tracks[0].colorChromaBitdepth).isEqualTo(10);
    assertThat(groups[0].tracks[0].maxSubLayers).isEqualTo(4);
    assertThat(groups[0].tracks[0].pcmEncoding).isEqualTo(C.ENCODING_PCM_16BIT);
    assertThat(groups[0].tracks[0].encoderDelay).isEqualTo(12);
    assertThat(groups[0].tracks[0].encoderPadding).isEqualTo(34);
    assertThat(groups[0].tracks[0].cueReplacementBehavior)
        .isEqualTo(Format.CUE_REPLACEMENT_BEHAVIOR_REPLACE);
    assertThat(groups[0].tracks[0].tileCountHorizontal).isEqualTo(5);
    assertThat(groups[0].tracks[0].tileCountVertical).isEqualTo(6);
    assertThat(groups[0].tracks[0].cryptoType).isEqualTo(C.CRYPTO_TYPE_FRAMEWORK);
    assertThat(groups[0].tracks[0].roleFlags).isEqualTo(C.ROLE_FLAG_MAIN | C.ROLE_FLAG_AUXILIARY);
    assertThat(groups[0].tracks[0].auxiliaryTrackType)
        .isEqualTo(C.AUXILIARY_TRACK_TYPE_DEPTH_LINEAR);
    assertThat(groups[0].tracks[0].selectionFlags).isEqualTo(C.SELECTION_FLAG_DEFAULT);
    assertThat(groups[0].tracks[0].formatSupport).isEqualTo(C.FORMAT_HANDLED);
    assertThat(groups[0].tracks[0].selected).isTrue();
    assertThat(groups[0].tracks[1].supported).isTrue();
    assertThat(groups[0].tracks[1].averageBitrate).isEqualTo(2_000_000);
    assertThat(groups[0].tracks[1].peakBitrate).isEqualTo(Format.NO_VALUE);
    assertThat(groups[0].tracks[1].metadataEntryCount).isEqualTo(0);
    assertThat(groups[0].tracks[1].initializationDataCount).isEqualTo(0);
    assertThat(groups[0].tracks[1].initializationDataTotalBytes).isEqualTo(0);
    assertThat(groups[0].tracks[1].drmSchemeDataCount).isEqualTo(0);
    assertThat(groups[0].tracks[1].subsampleOffsetUs).isEqualTo(Format.OFFSET_SAMPLE_RELATIVE);
    assertThat(groups[0].tracks[1].hasPrerollSamples).isFalse();
    assertThat(groups[0].tracks[1].colorStandard).isEqualTo(Format.NO_VALUE);
    assertThat(groups[0].tracks[1].colorRange).isEqualTo(Format.NO_VALUE);
    assertThat(groups[0].tracks[1].colorTransfer).isEqualTo(Format.NO_VALUE);
    assertThat(groups[0].tracks[1].formatSupport).isEqualTo(C.FORMAT_EXCEEDS_CAPABILITIES);
    assertThat(groups[0].tracks[0].supportedWithinCapabilities).isTrue();
    assertThat(groups[0].tracks[1].supportedWithinCapabilities).isFalse();
    assertThat(groups[1].id).isEqualTo("group-2");
    assertThat(groups[1].supported).isFalse();
    assertThat(groups[1].supportedAllowingExceedsCapabilities).isTrue();
    assertThat(groups[2].id).isEqualTo("group-3");
    assertThat(groups[2].tracks[0].containerMimeType).isEqualTo("application/x-subrip");
    assertThat(groups[2].tracks[0].accessibilityChannel).isEqualTo(2);
  }

  @Test
  public void toCppTracks_mapsTypeLevelSummary() {
    TrackGroup audioTrackGroup =
        new TrackGroup(
            "audio-group",
            new Format.Builder()
                .setId("audio-1")
                .setSampleMimeType("audio/mp4a-latm")
                .setLanguage("en")
                .build());
    TrackGroup videoTrackGroup =
        new TrackGroup(
            "video-group",
            new Format.Builder()
                .setId("video-1")
                .setSampleMimeType("video/avc")
                .setWidth(1280)
                .setHeight(720)
                .build());
    Tracks tracks =
        new Tracks(
            Arrays.asList(
                new Tracks.Group(
                    audioTrackGroup,
                    false,
                    new int[] {C.FORMAT_HANDLED},
                    new boolean[] {false}),
                new Tracks.Group(
                    videoTrackGroup,
                    false,
                    new int[] {C.FORMAT_EXCEEDS_CAPABILITIES},
                    new boolean[] {true})));

    CppTracks converted = CppBridgeConverters.toCppTracks(tracks);

    assertThat(converted.groups).hasLength(2);
    assertThat(converted.containsAudio).isTrue();
    assertThat(converted.containsVideo).isTrue();
    assertThat(converted.containsText).isFalse();
    assertThat(converted.audioSelected).isFalse();
    assertThat(converted.videoSelected).isTrue();
    assertThat(converted.audioSupported).isTrue();
    assertThat(converted.videoSupported).isFalse();
    assertThat(converted.videoSupportedAllowingExceedsCapabilities).isTrue();
  }

  @Test
  public void fromMediaItem_mapsSupportedFieldsBackToCppDescriptor() {
    MediaItem mediaItem =
        new MediaItem.Builder()
            .setUri("https://example.com/playlist.m3u8")
            .setMediaId("roundtrip-id")
            .setMimeType("application/x-mpegURL")
            .setSubtitleConfigurations(
                Arrays.asList(
                    new MediaItem.SubtitleConfiguration.Builder(
                            android.net.Uri.parse("https://example.com/sub.vtt"))
                        .setMimeType("text/vtt")
                        .setLanguage("en")
                        .setLabel("English")
                        .setId("sub-1")
                        .setSelectionFlags(1)
                        .setRoleFlags(2)
                        .build()))
            .setClippingConfiguration(
                new MediaItem.ClippingConfiguration.Builder()
                    .setStartPositionMs(1000)
                    .setEndPositionMs(5000)
                    .setRelativeToDefaultPosition(true)
                    .build())
            .setLiveConfiguration(
                new MediaItem.LiveConfiguration.Builder()
                    .setTargetOffsetMs(3000)
                    .setMinOffsetMs(2000)
                    .setMaxOffsetMs(5000)
                    .setMinPlaybackSpeed(0.97f)
                    .setMaxPlaybackSpeed(1.03f)
                    .build())
            .build();

    CppMediaItem cppItem = CppBridgeConverters.fromMediaItem(mediaItem);

    assertThat(cppItem.uri).isEqualTo("https://example.com/playlist.m3u8");
    assertThat(cppItem.mediaId).isEqualTo("roundtrip-id");
    assertThat(cppItem.mimeType).isEqualTo("application/x-mpegURL");
    assertThat(cppItem.sourceType).isEqualTo(2);
    assertThat(cppItem.subtitleConfigurations).hasLength(1);
    assertThat(cppItem.subtitleConfigurations[0].language).isEqualTo("en");
    assertThat(cppItem.clippingConfiguration).isNotNull();
    assertThat(cppItem.clippingConfiguration.startPositionMs).isEqualTo(1000);
    assertThat(cppItem.liveConfiguration).isNotNull();
    assertThat(cppItem.liveConfiguration.targetOffsetMs).isEqualTo(3000);
  }

  @Test
  public void fromMediaItem_infersSourceTypeFromUriExtensions() {
    CppMediaItem dashItem =
        CppBridgeConverters.fromMediaItem(
            new MediaItem.Builder().setUri("https://example.com/a.mpd").build());
    CppMediaItem hlsItem =
        CppBridgeConverters.fromMediaItem(
            new MediaItem.Builder().setUri("https://example.com/b.m3u8").build());
    CppMediaItem ssItem =
        CppBridgeConverters.fromMediaItem(
            new MediaItem.Builder().setUri("https://example.com/live.ism/manifest").build());
    CppMediaItem progressiveItem =
        CppBridgeConverters.fromMediaItem(
            new MediaItem.Builder().setUri("https://example.com/file.mp4").build());
    CppMediaItem genericManifestItem =
        CppBridgeConverters.fromMediaItem(
            new MediaItem.Builder().setUri("https://example.com/api/manifest").build());
    CppMediaItem emptyItem = CppBridgeConverters.fromMediaItem(new MediaItem.Builder().build());

    assertThat(dashItem.sourceType).isEqualTo(1);
    assertThat(hlsItem.sourceType).isEqualTo(2);
    assertThat(ssItem.sourceType).isEqualTo(3);
    assertThat(progressiveItem.sourceType).isEqualTo(5);
    assertThat(genericManifestItem.sourceType).isEqualTo(5);
    assertThat(emptyItem.sourceType).isEqualTo(0);
  }

  @Test
  public void fromMediaItem_infersSourceTypeFromMimeTypeAliases() {
    CppMediaItem hlsItem =
        CppBridgeConverters.fromMediaItem(
            new MediaItem.Builder()
                .setUri("https://example.com/live")
                .setMimeType("application/vnd.apple.mpegurl")
                .build());
    CppMediaItem lowerCaseHlsItem =
        CppBridgeConverters.fromMediaItem(
            new MediaItem.Builder()
                .setUri("https://example.com/live")
                .setMimeType("application/x-mpegurl")
                .build());
    CppMediaItem progressiveItem =
        CppBridgeConverters.fromMediaItem(
            new MediaItem.Builder()
                .setUri("https://example.com/file")
                .setMimeType("audio/mp4")
                .build());

    assertThat(hlsItem.sourceType).isEqualTo(2);
    assertThat(lowerCaseHlsItem.sourceType).isEqualTo(2);
    assertThat(progressiveItem.sourceType).isEqualTo(5);
  }

  @Test
  public void fromMediaMetadata_mapsExpandedTextFields() {
    Bundle extras = new Bundle();
    extras.putBoolean("available", true);
    extras.putByteArray("blob", new byte[] {9, 8, 7});
    extras.putDouble("rating", 4.5);
    extras.putLong("season", 2L);
    extras.putString("studio", "Studio");
    MediaMetadata metadata =
        new MediaMetadata.Builder()
            .setTitle("Title")
            .setArtist("Artist")
            .setAlbumTitle("Album")
            .setAlbumArtist("Album Artist")
            .setDisplayTitle("Display")
            .setSubtitle("Subtitle")
            .setDescription("Description")
            .setArtworkUri(Uri.parse("https://example.com/artwork.jpg"))
            .setDurationMs(123456L)
            .setTrackNumber(3)
            .setTotalTrackCount(12)
            .setIsBrowsable(true)
            .setIsPlayable(false)
            .setFolderType(MediaMetadata.FOLDER_TYPE_ARTISTS)
            .setRecordingYear(2024)
            .setRecordingMonth(6)
            .setRecordingDay(18)
            .setReleaseYear(2025)
            .setReleaseMonth(1)
            .setReleaseDay(9)
            .setWriter("Writer")
            .setAuthor("Author")
            .setComposer("Composer")
            .setConductor("Conductor")
            .setDiscNumber(1)
            .setTotalDiscCount(2)
            .setGenre("Genre")
            .setCompilation("Compilation")
            .setMediaType(MediaMetadata.MEDIA_TYPE_MUSIC)
            .setStation("Station")
            .setExtras(extras)
            .build();

    CppMediaMetadata converted = CppBridgeConverters.fromMediaMetadata(metadata);

    assertThat(converted.title).isEqualTo("Title");
    assertThat(converted.artist).isEqualTo("Artist");
    assertThat(converted.albumTitle).isEqualTo("Album");
    assertThat(converted.albumArtist).isEqualTo("Album Artist");
    assertThat(converted.displayTitle).isEqualTo("Display");
    assertThat(converted.subtitle).isEqualTo("Subtitle");
    assertThat(converted.description).isEqualTo("Description");
    assertThat(converted.artworkUri).isEqualTo("https://example.com/artwork.jpg");
    assertThat(converted.durationMs).isEqualTo(123456L);
    assertThat(converted.trackNumber).isEqualTo(3);
    assertThat(converted.totalTrackCount).isEqualTo(12);
    assertThat(converted.isBrowsable).isEqualTo(1);
    assertThat(converted.isPlayable).isEqualTo(0);
    assertThat(converted.folderType).isEqualTo(MediaMetadata.FOLDER_TYPE_ARTISTS);
    assertThat(converted.recordingYear).isEqualTo(2024);
    assertThat(converted.recordingMonth).isEqualTo(6);
    assertThat(converted.recordingDay).isEqualTo(18);
    assertThat(converted.releaseYear).isEqualTo(2025);
    assertThat(converted.releaseMonth).isEqualTo(1);
    assertThat(converted.releaseDay).isEqualTo(9);
    assertThat(converted.writer).isEqualTo("Writer");
    assertThat(converted.author).isEqualTo("Author");
    assertThat(converted.composer).isEqualTo("Composer");
    assertThat(converted.conductor).isEqualTo("Conductor");
    assertThat(converted.discNumber).isEqualTo(1);
    assertThat(converted.totalDiscCount).isEqualTo(2);
    assertThat(converted.genre).isEqualTo("Genre");
    assertThat(converted.compilation).isEqualTo("Compilation");
    assertThat(converted.mediaType).isEqualTo(MediaMetadata.MEDIA_TYPE_MUSIC);
    assertThat(converted.station).isEqualTo("Station");
    assertThat(converted.extrasPresent).isTrue();
    assertThat(converted.extrasKeyCount).isEqualTo(5);
    assertThat(converted.extrasToken).isNotNull();
    assertThat(converted.extrasValues).hasLength(5);
    assertThat(converted.extrasValues[0].key).isEqualTo("available");
    assertThat(converted.extrasValues[0].valueType).isEqualTo(CppBundleValue.TYPE_BOOLEAN);
    assertThat(converted.extrasValues[0].booleanValue).isTrue();
    assertThat(converted.extrasValues[1].key).isEqualTo("blob");
    assertThat(converted.extrasValues[1].valueType).isEqualTo(CppBundleValue.TYPE_BYTE_ARRAY);
    assertThat(converted.extrasValues[1].byteArrayValue).isEqualTo(new byte[] {9, 8, 7});
    assertThat(converted.extrasValues[2].key).isEqualTo("rating");
    assertThat(converted.extrasValues[2].valueType).isEqualTo(CppBundleValue.TYPE_DOUBLE);
    assertThat(converted.extrasValues[2].doubleValue).isEqualTo(4.5);
    assertThat(converted.extrasValues[3].key).isEqualTo("season");
    assertThat(converted.extrasValues[3].valueType).isEqualTo(CppBundleValue.TYPE_LONG);
    assertThat(converted.extrasValues[3].longValue).isEqualTo(2L);
    assertThat(converted.extrasValues[4].key).isEqualTo("studio");
    assertThat(converted.extrasValues[4].valueType).isEqualTo(CppBundleValue.TYPE_STRING);
    assertThat(converted.extrasValues[4].stringValue).isEqualTo("Studio");
  }

  @Test
  public void toMediaMetadata_buildsExtrasFromDecodedValues() {
    CppMediaMetadata metadata =
        new CppMediaMetadata(
            "Title",
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            -1,
            -1L,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            -1,
            -1,
            null,
            null,
            null,
            null,
            -1,
            null,
            null,
            true,
            2,
            null,
            new CppBundleValue[] {
              new CppBundleValue(
                  "episode", CppBundleValue.TYPE_LONG, null, 17L, 0.0, false, null),
              new CppBundleValue(
                  "title", CppBundleValue.TYPE_STRING, "Decoded", 0L, 0.0, false, null)
            });

    MediaMetadata converted = CppBridgeConverters.toMediaMetadata(metadata);

    assertThat(converted.title.toString()).isEqualTo("Title");
    assertThat(converted.extras.getLong("episode")).isEqualTo(17L);
    assertThat(converted.extras.getString("title")).isEqualTo("Decoded");
  }

  @Test
  public void toMediaMetadata_buildsExtrasWhenPresenceFlagIsFalse() {
    CppMediaMetadata metadata =
        new CppMediaMetadata(
            "Title",
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            -1,
            -1L,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            -1,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            -1,
            -1,
            null,
            null,
            null,
            null,
            -1,
            null,
            null,
            false,
            0,
            null,
            new CppBundleValue[] {
              new CppBundleValue(
                  "title", CppBundleValue.TYPE_STRING, "Decoded", 0L, 0.0, false, null)
            });

    MediaMetadata converted = CppBridgeConverters.toMediaMetadata(metadata);

    assertThat(converted.extras.getString("title")).isEqualTo("Decoded");
  }

  @Test
  public void fromPositionInfo_mapsDiscontinuityFields() {
    MediaItem mediaItem =
        new MediaItem.Builder()
            .setUri("https://example.com/position.m3u8")
            .setMediaId("position-id")
            .setMimeType("application/x-mpegURL")
            .build();

    Player.PositionInfo positionInfo =
        new Player.PositionInfo(
            /* windowUid= */ null,
            2,
            mediaItem,
            /* periodUid= */ null,
            4,
            3456L,
            3000L,
            C.INDEX_UNSET,
            C.INDEX_UNSET);

    CppPositionInfo converted = CppBridgeConverters.fromPositionInfo(positionInfo);

    assertThat(converted.mediaItemIndex).isEqualTo(2);
    assertThat(converted.mediaItem).isNotNull();
    assertThat(converted.mediaItem.mediaId).isEqualTo("position-id");
    assertThat(converted.periodIndex).isEqualTo(4);
    assertThat(converted.positionMs).isEqualTo(3456L);
    assertThat(converted.contentPositionMs).isEqualTo(3000L);
    assertThat(converted.adGroupIndex).isEqualTo(C.INDEX_UNSET);
    assertThat(converted.adIndexInAdGroup).isEqualTo(C.INDEX_UNSET);
  }

  @Test
  public void fromCue_mapsLayoutFields() {
    Cue cue =
        new Cue.Builder()
            .setText("Hello")
            .setTextAlignment(Layout.Alignment.ALIGN_CENTER)
            .setMultiRowAlignment(Layout.Alignment.ALIGN_NORMAL)
            .setLine(0.25f, Cue.LINE_TYPE_FRACTION)
            .setLineAnchor(Cue.ANCHOR_TYPE_END)
            .setPosition(0.6f)
            .setPositionAnchor(Cue.ANCHOR_TYPE_MIDDLE)
            .setSize(0.8f)
            .setBitmapHeight(0.4f)
            .setTextSize(0.05f, Cue.TEXT_SIZE_TYPE_FRACTIONAL)
            .setVerticalType(Cue.VERTICAL_TYPE_RL)
            .setShearDegrees(12.5f)
            .setZIndex(7)
            .setWindowColor(0x11223344)
            .build();

    CppCue converted = CppBridgeConverters.fromCue(cue);

    assertThat(converted.text).isEqualTo("Hello");
    assertThat(converted.textAlignment).isEqualTo(2);
    assertThat(converted.multiRowAlignment).isEqualTo(1);
    assertThat(converted.line).isEqualTo(0.25f);
    assertThat(converted.lineType).isEqualTo(Cue.LINE_TYPE_FRACTION);
    assertThat(converted.lineAnchor).isEqualTo(Cue.ANCHOR_TYPE_END);
    assertThat(converted.position).isEqualTo(0.6f);
    assertThat(converted.positionAnchor).isEqualTo(Cue.ANCHOR_TYPE_MIDDLE);
    assertThat(converted.size).isEqualTo(0.8f);
    assertThat(converted.bitmapHeight).isEqualTo(0.4f);
    assertThat(converted.textSize).isEqualTo(0.05f);
    assertThat(converted.textSizeType).isEqualTo(Cue.TEXT_SIZE_TYPE_FRACTIONAL);
    assertThat(converted.verticalType).isEqualTo(Cue.VERTICAL_TYPE_RL);
    assertThat(converted.shearDegrees).isEqualTo(12.5f);
    assertThat(converted.zIndex).isEqualTo(7);
    assertThat(converted.windowColorSet).isTrue();
    assertThat(converted.windowColor).isEqualTo(0x11223344);
    assertThat(converted.hasBitmap).isFalse();
  }

  @Test
  public void toVideoEffects_mapsSupportedDescriptors() {
    CppVideoEffect[] effects =
        new CppVideoEffect[] {
          new CppVideoEffect(
              CppVideoEffect.TYPE_SCALE_AND_ROTATE, 1.25f, 0.75f, 45f, 1f, 1f, 1f, 0, 0, 0),
          new CppVideoEffect(
              CppVideoEffect.TYPE_RGB_ADJUSTMENT, 1f, 1f, 0f, 1.1f, 0.9f, 0.8f, 0, 0, 0),
          new CppVideoEffect(
              CppVideoEffect.TYPE_PRESENTATION,
              1f,
              1f,
              0f,
              1f,
              1f,
              1f,
              1920,
              1080,
              Presentation.LAYOUT_SCALE_TO_FIT)
        };

    List<Effect> converted = CppBridgeConverters.toVideoEffects(effects);

    assertThat(converted).hasSize(3);
    assertThat(converted.get(0)).isInstanceOf(ScaleAndRotateTransformation.class);
    assertThat(converted.get(1)).isInstanceOf(RgbAdjustment.class);
    assertThat(converted.get(2)).isInstanceOf(Presentation.class);
  }

  @Test
  public void opaqueObjectRegistry_reusesTokenForSameObjectIdentity() {
    Object object = new Object();

    String tokenOne = CppOpaqueObjectRegistry.register(object);
    String tokenTwo = CppOpaqueObjectRegistry.register(object);

    assertThat(tokenTwo).isEqualTo(tokenOne);
    assertThat(CppOpaqueObjectRegistry.resolve(tokenOne)).isSameInstanceAs(object);
    CppOpaqueObjectRegistry.unregister(tokenOne);
  }

  @Test
  public void opaqueObjectRegistry_unregisterIsIdempotentAndClearsResolution() {
    Object object = new Object();
    String token = CppOpaqueObjectRegistry.register(object);

    assertThat(CppOpaqueObjectRegistry.resolve(token)).isSameInstanceAs(object);

    CppOpaqueObjectRegistry.unregister(token);
    CppOpaqueObjectRegistry.unregister(token);

    assertThat(CppOpaqueObjectRegistry.resolve(token)).isNull();
  }

  @Test
  public void opaqueObjectRegistry_registerWithExistingToken_replacesReverseMapping() {
    Object first = new Object();
    Object second = new Object();

    CppOpaqueObjectRegistry.register("opaque-replace-token", first);
    CppOpaqueObjectRegistry.register("opaque-replace-token", second);

    assertThat(CppOpaqueObjectRegistry.resolve("opaque-replace-token")).isSameInstanceAs(second);
    assertThat(CppOpaqueObjectRegistry.register(first)).isNotEqualTo("opaque-replace-token");

    CppOpaqueObjectRegistry.unregister("opaque-replace-token");
  }

  @Test
  public void mediaSourceFactoryRegistry_registerWithExistingToken_replacesReverseMapping() {
    Context context = ApplicationProvider.getApplicationContext();
    DefaultMediaSourceFactory first =
        new DefaultMediaSourceFactory(context);
    DefaultMediaSourceFactory second =
        new DefaultMediaSourceFactory(context);

    CppMediaSourceFactoryRegistry.register("factory-replace-token", first);
    CppMediaSourceFactoryRegistry.register("factory-replace-token", second);

    assertThat(CppMediaSourceFactoryRegistry.resolve("factory-replace-token"))
        .isSameInstanceAs(second);
    assertThat(CppMediaSourceFactoryRegistry.register(first))
        .isNotEqualTo("factory-replace-token");

    CppMediaSourceFactoryRegistry.unregister("factory-replace-token");
  }

  @Test
  public void opaqueObjectRegistry_registeringSameObjectWithNewTokenReplacesOldToken() {
    Object object = new Object();

    CppOpaqueObjectRegistry.register("opaque-old-token", object);
    CppOpaqueObjectRegistry.register("opaque-new-token", object);

    assertThat(CppOpaqueObjectRegistry.resolve("opaque-old-token")).isNull();
    assertThat(CppOpaqueObjectRegistry.resolve("opaque-new-token")).isSameInstanceAs(object);

    CppOpaqueObjectRegistry.unregister("opaque-old-token");
    assertThat(CppOpaqueObjectRegistry.register(object)).isEqualTo("opaque-new-token");

    CppOpaqueObjectRegistry.unregister("opaque-new-token");
  }

  @Test
  public void mediaSourceFactoryRegistry_registeringSameFactoryWithNewTokenReplacesOldToken() {
    Context context = ApplicationProvider.getApplicationContext();
    DefaultMediaSourceFactory factory =
        new DefaultMediaSourceFactory(context);

    CppMediaSourceFactoryRegistry.register("factory-old-token", factory);
    CppMediaSourceFactoryRegistry.register("factory-new-token", factory);

    assertThat(CppMediaSourceFactoryRegistry.resolve("factory-old-token")).isNull();
    assertThat(CppMediaSourceFactoryRegistry.resolve("factory-new-token"))
        .isSameInstanceAs(factory);

    CppMediaSourceFactoryRegistry.unregister("factory-old-token");
    assertThat(CppMediaSourceFactoryRegistry.register(factory)).isEqualTo("factory-new-token");

    CppMediaSourceFactoryRegistry.unregister("factory-new-token");
  }
}
