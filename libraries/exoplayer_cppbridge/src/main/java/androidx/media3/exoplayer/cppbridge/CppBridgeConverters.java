package androidx.media3.exoplayer.cppbridge;

import android.net.Uri;
import android.os.Bundle;
import android.text.Layout;
import androidx.annotation.Nullable;
import androidx.media3.common.C;
import androidx.media3.common.DrmInitData;
import androidx.media3.common.Effect;
import androidx.media3.common.Format;
import androidx.media3.common.Label;
import androidx.media3.common.MediaItem;
import androidx.media3.common.MediaMetadata;
import androidx.media3.common.MimeTypes;
import androidx.media3.common.Player;
import androidx.media3.common.TrackSelectionOverride;
import androidx.media3.common.TrackSelectionParameters;
import androidx.media3.common.Tracks;
import androidx.media3.common.text.Cue;
import androidx.media3.common.util.Util;
import androidx.media3.effect.Presentation;
import androidx.media3.effect.RgbAdjustment;
import androidx.media3.effect.ScaleAndRotateTransformation;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

final class CppBridgeConverters {

  private static final int ALIGNMENT_UNSET = 0;
  private static final int ALIGNMENT_NORMAL = 1;
  private static final int ALIGNMENT_CENTER = 2;
  private static final int ALIGNMENT_OPPOSITE = 3;

  private CppBridgeConverters() {}

  private static int toCppAlignment(@androidx.annotation.Nullable Layout.Alignment alignment) {
    if (alignment == null) {
      return ALIGNMENT_UNSET;
    }
    if (alignment == Layout.Alignment.ALIGN_NORMAL) {
      return ALIGNMENT_NORMAL;
    }
    if (alignment == Layout.Alignment.ALIGN_CENTER) {
      return ALIGNMENT_CENTER;
    }
    if (alignment == Layout.Alignment.ALIGN_OPPOSITE) {
      return ALIGNMENT_OPPOSITE;
    }
    return ALIGNMENT_UNSET;
  }

  private static @androidx.annotation.Nullable String inferMimeType(CppMediaItem mediaItem) {
    if (mediaItem.mimeType != null && !mediaItem.mimeType.isEmpty()) {
      return normalizeMimeType(mediaItem.mimeType);
    }
    switch (mediaItem.sourceType) {
      case 1:
        return MimeTypes.APPLICATION_MPD;
      case 2:
        return MimeTypes.APPLICATION_M3U8;
      case 3:
        return MimeTypes.APPLICATION_SS;
      case 4:
        return MimeTypes.APPLICATION_RTSP;
      default:
        return mediaItem.mimeType;
    }
  }

  private static @androidx.annotation.Nullable String normalizeMimeType(
      @androidx.annotation.Nullable String mimeType) {
    if (mimeType == null) {
      return null;
    }
    String trimmedMimeType = mimeType.trim();
    if (MimeTypes.APPLICATION_MPD.equalsIgnoreCase(trimmedMimeType)) {
      return MimeTypes.APPLICATION_MPD;
    }
    if (MimeTypes.APPLICATION_M3U8.equalsIgnoreCase(trimmedMimeType)
        || "application/x-mpegURL".equalsIgnoreCase(trimmedMimeType)
        || "application/vnd.apple.mpegurl".equalsIgnoreCase(trimmedMimeType)) {
      return MimeTypes.APPLICATION_M3U8;
    }
    if (MimeTypes.APPLICATION_SS.equalsIgnoreCase(trimmedMimeType)) {
      return MimeTypes.APPLICATION_SS;
    }
    if (MimeTypes.APPLICATION_RTSP.equalsIgnoreCase(trimmedMimeType)
        || "application/x-rtsp".equalsIgnoreCase(trimmedMimeType)) {
      return MimeTypes.APPLICATION_RTSP;
    }
    return trimmedMimeType;
  }

  private static int toCppSourceType(@C.ContentType int contentType) {
    switch (contentType) {
      case C.CONTENT_TYPE_DASH:
        return 1;
      case C.CONTENT_TYPE_HLS:
        return 2;
      case C.CONTENT_TYPE_SS:
        return 3;
      case C.CONTENT_TYPE_RTSP:
        return 4;
      case C.CONTENT_TYPE_OTHER:
        return 5;
      default:
        return 0;
    }
  }

  private static int inferSourceType(
      @androidx.annotation.Nullable String uri, @androidx.annotation.Nullable String mimeType) {
    String normalizedUri = uri != null ? uri : "";
    String normalizedMimeType = normalizeMimeType(mimeType);
    if (normalizedUri.isEmpty()
        && (normalizedMimeType == null || normalizedMimeType.isEmpty())) {
      return 0;
    }
    return toCppSourceType(
        Util.inferContentTypeForUriAndMimeType(Uri.parse(normalizedUri), normalizedMimeType));
  }

  private static CppBundleValue[] toCppBundleValues(@Nullable Bundle bundle) {
    if (bundle == null) {
      return new CppBundleValue[0];
    }
    ArrayList<String> keys = new ArrayList<>(bundle.keySet());
    Collections.sort(keys);
    ArrayList<CppBundleValue> values = new ArrayList<>(keys.size());
    for (String key : keys) {
      Object value = bundle.get(key);
      if (value instanceof CharSequence) {
        values.add(
            new CppBundleValue(
                key, CppBundleValue.TYPE_STRING, value.toString(), 0L, 0.0, false, null));
      } else if (value instanceof Byte
          || value instanceof Short
          || value instanceof Integer
          || value instanceof Long) {
        values.add(
            new CppBundleValue(
                key,
                CppBundleValue.TYPE_LONG,
                null,
                ((Number) value).longValue(),
                0.0,
                false,
                null));
      } else if (value instanceof Float || value instanceof Double) {
        values.add(
            new CppBundleValue(
                key,
                CppBundleValue.TYPE_DOUBLE,
                null,
                0L,
                ((Number) value).doubleValue(),
                false,
                null));
      } else if (value instanceof Boolean) {
        values.add(
            new CppBundleValue(
                key, CppBundleValue.TYPE_BOOLEAN, null, 0L, 0.0, (Boolean) value, null));
      } else if (value instanceof byte[]) {
        values.add(
            new CppBundleValue(
                key,
                CppBundleValue.TYPE_BYTE_ARRAY,
                null,
                0L,
                0.0,
                false,
                ((byte[]) value).clone()));
      }
    }
    return values.toArray(new CppBundleValue[0]);
  }

  private static Bundle toBundle(CppBundleValue[] values) {
    Bundle bundle = new Bundle();
    if (values == null) {
      return bundle;
    }
    for (CppBundleValue value : values) {
      if (value == null || value.key == null) {
        continue;
      }
      switch (value.valueType) {
        case CppBundleValue.TYPE_STRING:
          bundle.putString(value.key, value.stringValue != null ? value.stringValue : "");
          break;
        case CppBundleValue.TYPE_LONG:
          bundle.putLong(value.key, value.longValue);
          break;
        case CppBundleValue.TYPE_DOUBLE:
          bundle.putDouble(value.key, value.doubleValue);
          break;
        case CppBundleValue.TYPE_BOOLEAN:
          bundle.putBoolean(value.key, value.booleanValue);
          break;
        case CppBundleValue.TYPE_BYTE_ARRAY:
          bundle.putByteArray(value.key, value.byteArrayValue.clone());
          break;
        default:
          break;
      }
    }
    return bundle;
  }

  private static CppObjectValue toCppObjectValue(@Nullable Object value) {
    if (value == null) {
      return CppObjectValue.nullValue();
    }
    if (value instanceof CharSequence) {
      return new CppObjectValue(
          true,
          value.getClass().getName(),
          CppObjectValue.TYPE_STRING,
          value.toString(),
          0L,
          0.0,
          false);
    }
    if (value instanceof Byte
        || value instanceof Short
        || value instanceof Integer
        || value instanceof Long) {
      return new CppObjectValue(
          true,
          value.getClass().getName(),
          CppObjectValue.TYPE_LONG,
          null,
          ((Number) value).longValue(),
          0.0,
          false);
    }
    if (value instanceof Float || value instanceof Double) {
      return new CppObjectValue(
          true,
          value.getClass().getName(),
          CppObjectValue.TYPE_DOUBLE,
          null,
          0L,
          ((Number) value).doubleValue(),
          false);
    }
    if (value instanceof Boolean) {
      return new CppObjectValue(
          true,
          value.getClass().getName(),
          CppObjectValue.TYPE_BOOLEAN,
          null,
          0L,
          0.0,
          (Boolean) value);
    }
    return new CppObjectValue(
        true,
        value.getClass().getName(),
        CppObjectValue.TYPE_OTHER,
        String.valueOf(value),
        0L,
        0.0,
        false);
  }

  private static Object toJavaObjectValue(
      @Nullable CppObjectValue value, @Nullable String fallbackString) {
    if (value == null || !value.present) {
      return fallbackString != null ? fallbackString : "";
    }
    switch (value.valueType) {
      case CppObjectValue.TYPE_STRING:
      case CppObjectValue.TYPE_OTHER:
        return value.stringValue != null
            ? value.stringValue
            : fallbackString != null ? fallbackString : "";
      case CppObjectValue.TYPE_LONG:
        return value.longValue;
      case CppObjectValue.TYPE_DOUBLE:
        return value.doubleValue;
      case CppObjectValue.TYPE_BOOLEAN:
        return value.booleanValue;
      default:
        return fallbackString != null ? fallbackString : "";
    }
  }

  private static @Nullable CharSequence toJavaCharSequenceValue(
      @Nullable CppObjectValue value, @Nullable String fallbackString) {
    if (value == null || !value.present) {
      return fallbackString;
    }
    switch (value.valueType) {
      case CppObjectValue.TYPE_STRING:
      case CppObjectValue.TYPE_OTHER:
        return value.stringValue != null ? value.stringValue : fallbackString;
      case CppObjectValue.TYPE_LONG:
        return Long.toString(value.longValue);
      case CppObjectValue.TYPE_DOUBLE:
        return Double.toString(value.doubleValue);
      case CppObjectValue.TYPE_BOOLEAN:
        return Boolean.toString(value.booleanValue);
      default:
        return fallbackString;
    }
  }

  private static @Nullable CharSequence resolveMetadataText(
      @Nullable String token, @Nullable CppObjectValue value, @Nullable String fallbackString) {
    Object resolved = CppOpaqueObjectRegistry.resolve(token);
    return resolved instanceof CharSequence
        ? (CharSequence) resolved
        : (resolved != null ? resolved.toString() : toJavaCharSequenceValue(value, fallbackString));
  }

  static MediaItem toMediaItem(CppMediaItem mediaItem) {
    MediaItem.Builder builder = new MediaItem.Builder();
    if (mediaItem.uri != null && !mediaItem.uri.isEmpty()) {
      builder.setUri(mediaItem.uri);
    } else {
      builder.setUri(Uri.EMPTY);
    }
    if (mediaItem.mediaId != null) {
      builder.setMediaId(mediaItem.mediaId);
    }
    String inferredMimeType = inferMimeType(mediaItem);
    if (inferredMimeType != null) {
      builder.setMimeType(inferredMimeType);
    }
    if (mediaItem.tagPresent) {
      Object resolvedTag = CppOpaqueObjectRegistry.resolve(mediaItem.tagToken);
      builder.setTag(
          resolvedTag != null
              ? resolvedTag
              : toJavaObjectValue(mediaItem.tagValue, mediaItem.tagString));
    }
    if (mediaItem.mediaMetadata != null) {
      builder.setMediaMetadata(toMediaMetadata(mediaItem.mediaMetadata));
    }
    if (mediaItem.requestMetadata != null) {
      MediaItem.RequestMetadata.Builder requestMetadataBuilder = new MediaItem.RequestMetadata.Builder();
      if (mediaItem.requestMetadata.mediaUri != null && !mediaItem.requestMetadata.mediaUri.isEmpty()) {
        requestMetadataBuilder.setMediaUri(Uri.parse(mediaItem.requestMetadata.mediaUri));
      }
      if (mediaItem.requestMetadata.searchQuery != null) {
        requestMetadataBuilder.setSearchQuery(mediaItem.requestMetadata.searchQuery);
      }
      if (mediaItem.requestMetadata.extrasPresent
          || mediaItem.requestMetadata.extrasValues.length > 0) {
        Object resolvedExtras = CppOpaqueObjectRegistry.resolve(mediaItem.requestMetadata.extrasToken);
        requestMetadataBuilder.setExtras(
            resolvedExtras instanceof Bundle
                ? (Bundle) resolvedExtras
                : toBundle(mediaItem.requestMetadata.extrasValues));
      }
      builder.setRequestMetadata(requestMetadataBuilder.build());
    }
    if (mediaItem.adsConfiguration != null && mediaItem.adsConfiguration.adTagUri != null) {
      MediaItem.AdsConfiguration.Builder adsBuilder =
          new MediaItem.AdsConfiguration.Builder(Uri.parse(mediaItem.adsConfiguration.adTagUri));
      if (mediaItem.adsConfiguration.adsId != null
          || mediaItem.adsConfiguration.adsIdValue.present) {
        Object resolvedAdsId = CppOpaqueObjectRegistry.resolve(mediaItem.adsConfiguration.adsIdToken);
        adsBuilder.setAdsId(
            resolvedAdsId != null
                ? resolvedAdsId
                : toJavaObjectValue(
                    mediaItem.adsConfiguration.adsIdValue, mediaItem.adsConfiguration.adsId));
      }
      builder.setAdsConfiguration(adsBuilder.build());
    }
    if (mediaItem.subtitleConfigurations.length > 0) {
      List<MediaItem.SubtitleConfiguration> subtitles =
          new ArrayList<>(mediaItem.subtitleConfigurations.length);
      for (CppSubtitleConfiguration subtitleConfiguration : mediaItem.subtitleConfigurations) {
        MediaItem.SubtitleConfiguration.Builder subtitleBuilder =
            new MediaItem.SubtitleConfiguration.Builder(Uri.parse(subtitleConfiguration.uri));
        if (subtitleConfiguration.mimeType != null) {
          subtitleBuilder.setMimeType(subtitleConfiguration.mimeType);
        }
        if (subtitleConfiguration.language != null) {
          subtitleBuilder.setLanguage(subtitleConfiguration.language);
        }
        if (subtitleConfiguration.label != null) {
          subtitleBuilder.setLabel(subtitleConfiguration.label);
        }
        if (subtitleConfiguration.id != null) {
          subtitleBuilder.setId(subtitleConfiguration.id);
        }
        subtitleBuilder.setSelectionFlags(subtitleConfiguration.selectionFlags);
        subtitleBuilder.setRoleFlags(subtitleConfiguration.roleFlags);
        subtitles.add(subtitleBuilder.build());
      }
      builder.setSubtitleConfigurations(subtitles);
    }
    if (mediaItem.clippingConfiguration != null) {
      builder.setClippingConfiguration(
          new MediaItem.ClippingConfiguration.Builder()
              .setStartPositionMs(mediaItem.clippingConfiguration.startPositionMs)
              .setEndPositionMs(mediaItem.clippingConfiguration.endPositionMs)
              .setRelativeToLiveWindow(mediaItem.clippingConfiguration.relativeToLiveWindow)
              .setRelativeToDefaultPosition(
                  mediaItem.clippingConfiguration.relativeToDefaultPosition)
              .setStartsAtKeyFrame(mediaItem.clippingConfiguration.startsAtKeyFrame)
              .setAllowUnseekableMedia(mediaItem.clippingConfiguration.allowUnseekableMedia)
              .build());
    }
    if (mediaItem.liveConfiguration != null) {
      MediaItem.LiveConfiguration.Builder liveBuilder = new MediaItem.LiveConfiguration.Builder();
      if (mediaItem.liveConfiguration.targetOffsetMs != C.TIME_UNSET) {
        liveBuilder.setTargetOffsetMs(mediaItem.liveConfiguration.targetOffsetMs);
      }
      if (mediaItem.liveConfiguration.minOffsetMs != C.TIME_UNSET) {
        liveBuilder.setMinOffsetMs(mediaItem.liveConfiguration.minOffsetMs);
      }
      if (mediaItem.liveConfiguration.maxOffsetMs != C.TIME_UNSET) {
        liveBuilder.setMaxOffsetMs(mediaItem.liveConfiguration.maxOffsetMs);
      }
      if (mediaItem.liveConfiguration.minPlaybackSpeed != C.RATE_UNSET) {
        liveBuilder.setMinPlaybackSpeed(mediaItem.liveConfiguration.minPlaybackSpeed);
      }
      if (mediaItem.liveConfiguration.maxPlaybackSpeed != C.RATE_UNSET) {
        liveBuilder.setMaxPlaybackSpeed(mediaItem.liveConfiguration.maxPlaybackSpeed);
      }
      builder.setLiveConfiguration(liveBuilder.build());
    }
    if (mediaItem.drmConfiguration != null
        && mediaItem.drmConfiguration.schemeUuid != null
        && !mediaItem.drmConfiguration.schemeUuid.isEmpty()) {
      MediaItem.DrmConfiguration.Builder drmBuilder =
          new MediaItem.DrmConfiguration.Builder(
              UUID.fromString(mediaItem.drmConfiguration.schemeUuid));
      if (mediaItem.drmConfiguration.licenseUri != null) {
        drmBuilder.setLicenseUri(mediaItem.drmConfiguration.licenseUri);
      }
      if (mediaItem.drmConfiguration.licenseRequestHeaderNames.length > 0) {
        Map<String, String> licenseRequestHeaders = new LinkedHashMap<>();
        int pairCount =
            Math.min(
                mediaItem.drmConfiguration.licenseRequestHeaderNames.length,
                mediaItem.drmConfiguration.licenseRequestHeaderValues.length);
        for (int i = 0; i < pairCount; i++) {
          String headerName = mediaItem.drmConfiguration.licenseRequestHeaderNames[i];
          String headerValue = mediaItem.drmConfiguration.licenseRequestHeaderValues[i];
          if (headerName != null && !headerName.isEmpty() && headerValue != null) {
            licenseRequestHeaders.put(headerName, headerValue);
          }
        }
        if (!licenseRequestHeaders.isEmpty()) {
          drmBuilder.setLicenseRequestHeaders(licenseRequestHeaders);
        }
      }
      if (mediaItem.drmConfiguration.forcedSessionTrackTypes.length > 0) {
        List<Integer> forcedSessionTrackTypes =
            new ArrayList<>(mediaItem.drmConfiguration.forcedSessionTrackTypes.length);
        for (int trackType : mediaItem.drmConfiguration.forcedSessionTrackTypes) {
          forcedSessionTrackTypes.add(trackType);
        }
        drmBuilder.setForcedSessionTrackTypes(forcedSessionTrackTypes);
      }
      if (mediaItem.drmConfiguration.keySetId.length > 0) {
        drmBuilder.setKeySetId(mediaItem.drmConfiguration.keySetId);
      }
      drmBuilder
          .setMultiSession(mediaItem.drmConfiguration.multiSession)
          .setForceDefaultLicenseUri(mediaItem.drmConfiguration.forceDefaultLicenseUri)
          .setPlayClearContentWithoutKey(mediaItem.drmConfiguration.playClearContentWithoutKey);
      builder.setDrmConfiguration(drmBuilder.build());
    }
    return builder.build();
  }

  static CppMediaItem fromMediaItem(MediaItem mediaItem) {
    @Nullable MediaItem.LocalConfiguration localConfiguration = mediaItem.localConfiguration;
    String uri =
        localConfiguration != null && localConfiguration.uri != null
            ? localConfiguration.uri.toString()
            : "";
    @Nullable String mimeType = localConfiguration != null ? localConfiguration.mimeType : null;
    boolean tagPresent = localConfiguration != null && localConfiguration.tag != null;
    @Nullable String tagString =
        localConfiguration != null && localConfiguration.tag != null
            ? localConfiguration.tag.toString()
            : null;
    @Nullable String tagToken =
        localConfiguration != null && localConfiguration.tag != null
            ? CppOpaqueObjectRegistry.register(localConfiguration.tag)
            : null;

    CppSubtitleConfiguration[] subtitleConfigurations = new CppSubtitleConfiguration[0];
    if (localConfiguration != null && !localConfiguration.subtitleConfigurations.isEmpty()) {
      subtitleConfigurations =
          new CppSubtitleConfiguration[localConfiguration.subtitleConfigurations.size()];
      for (int i = 0; i < localConfiguration.subtitleConfigurations.size(); i++) {
        MediaItem.SubtitleConfiguration subtitleConfiguration =
            localConfiguration.subtitleConfigurations.get(i);
        subtitleConfigurations[i] =
            new CppSubtitleConfiguration(
                subtitleConfiguration.uri.toString(),
                subtitleConfiguration.mimeType,
                subtitleConfiguration.language,
                subtitleConfiguration.label,
                subtitleConfiguration.id,
                subtitleConfiguration.selectionFlags,
                subtitleConfiguration.roleFlags);
      }
    }

    CppClippingConfiguration clippingConfiguration = null;
    if (mediaItem.clippingConfiguration.startPositionMs != 0
        || mediaItem.clippingConfiguration.endPositionMs != C.TIME_END_OF_SOURCE
        || mediaItem.clippingConfiguration.relativeToLiveWindow
        || mediaItem.clippingConfiguration.relativeToDefaultPosition
        || mediaItem.clippingConfiguration.startsAtKeyFrame
        || mediaItem.clippingConfiguration.allowUnseekableMedia) {
      clippingConfiguration =
          new CppClippingConfiguration(
              mediaItem.clippingConfiguration.startPositionMs,
              mediaItem.clippingConfiguration.endPositionMs,
              mediaItem.clippingConfiguration.relativeToLiveWindow,
              mediaItem.clippingConfiguration.relativeToDefaultPosition,
              mediaItem.clippingConfiguration.startsAtKeyFrame,
              mediaItem.clippingConfiguration.allowUnseekableMedia);
    }

    CppLiveConfiguration liveConfiguration = null;
    if (mediaItem.liveConfiguration.targetOffsetMs != C.TIME_UNSET
        || mediaItem.liveConfiguration.minOffsetMs != C.TIME_UNSET
        || mediaItem.liveConfiguration.maxOffsetMs != C.TIME_UNSET
        || mediaItem.liveConfiguration.minPlaybackSpeed != C.RATE_UNSET
        || mediaItem.liveConfiguration.maxPlaybackSpeed != C.RATE_UNSET) {
      liveConfiguration =
          new CppLiveConfiguration(
              mediaItem.liveConfiguration.targetOffsetMs,
              mediaItem.liveConfiguration.minOffsetMs,
              mediaItem.liveConfiguration.maxOffsetMs,
              mediaItem.liveConfiguration.minPlaybackSpeed,
              mediaItem.liveConfiguration.maxPlaybackSpeed);
    }

    CppDrmConfiguration drmConfiguration = null;
    if (localConfiguration != null && localConfiguration.drmConfiguration != null) {
      MediaItem.DrmConfiguration drm = localConfiguration.drmConfiguration;
      String[] headerNames = new String[drm.licenseRequestHeaders.size()];
      String[] headerValues = new String[drm.licenseRequestHeaders.size()];
      int headerIndex = 0;
      for (Map.Entry<String, String> entry : drm.licenseRequestHeaders.entrySet()) {
        headerNames[headerIndex] = entry.getKey();
        headerValues[headerIndex] = entry.getValue();
        headerIndex++;
      }
      int[] forcedSessionTrackTypes = new int[drm.forcedSessionTrackTypes.size()];
      for (int i = 0; i < drm.forcedSessionTrackTypes.size(); i++) {
        forcedSessionTrackTypes[i] = drm.forcedSessionTrackTypes.get(i);
      }
      drmConfiguration =
          new CppDrmConfiguration(
              drm.scheme != null ? drm.scheme.toString() : null,
              drm.licenseUri != null ? drm.licenseUri.toString() : null,
              headerNames,
              headerValues,
              forcedSessionTrackTypes,
              drm.getKeySetId(),
              drm.multiSession,
              drm.forceDefaultLicenseUri,
              drm.playClearContentWithoutKey);
    }

    CppAdsConfiguration adsConfiguration = null;
    if (localConfiguration != null && localConfiguration.adsConfiguration != null) {
      MediaItem.AdsConfiguration ads = localConfiguration.adsConfiguration;
      adsConfiguration =
          new CppAdsConfiguration(
              ads.adTagUri.toString(),
              ads.adsId != null ? ads.adsId.toString() : null,
              ads.adsId != null ? CppOpaqueObjectRegistry.register(ads.adsId) : null,
              toCppObjectValue(ads.adsId));
    }

    CppRequestMetadata requestMetadata = null;
    if (mediaItem.requestMetadata.mediaUri != null
        || mediaItem.requestMetadata.searchQuery != null
        || mediaItem.requestMetadata.extras != null) {
      requestMetadata =
          new CppRequestMetadata(
              mediaItem.requestMetadata.mediaUri != null
                  ? mediaItem.requestMetadata.mediaUri.toString()
                  : null,
              mediaItem.requestMetadata.searchQuery,
              mediaItem.requestMetadata.extras != null,
              mediaItem.requestMetadata.extras != null ? mediaItem.requestMetadata.extras.size() : 0,
              mediaItem.requestMetadata.extras != null
                  ? CppOpaqueObjectRegistry.register(mediaItem.requestMetadata.extras)
                  : null,
              toCppBundleValues(mediaItem.requestMetadata.extras));
    }

    return new CppMediaItem(
        uri,
        mediaItem.mediaId,
        mimeType,
        inferSourceType(uri, mimeType),
        tagPresent,
        tagString,
        tagToken,
        toCppObjectValue(localConfiguration != null ? localConfiguration.tag : null),
        fromMediaMetadata(mediaItem.mediaMetadata),
        requestMetadata,
        adsConfiguration,
        subtitleConfigurations,
        clippingConfiguration,
        liveConfiguration,
        drmConfiguration);
  }

  static MediaMetadata toMediaMetadata(CppMediaMetadata metadata) {
    MediaMetadata.Builder builder = new MediaMetadata.Builder();
    builder.setTitle(
        resolveMetadataText(metadata.titleToken, metadata.titleValue, metadata.title));
    builder.setArtist(
        resolveMetadataText(metadata.artistToken, metadata.artistValue, metadata.artist));
    builder.setAlbumTitle(
        resolveMetadataText(
            metadata.albumTitleToken, metadata.albumTitleValue, metadata.albumTitle));
    builder.setAlbumArtist(
        resolveMetadataText(
            metadata.albumArtistToken, metadata.albumArtistValue, metadata.albumArtist));
    builder.setDisplayTitle(
        resolveMetadataText(
            metadata.displayTitleToken, metadata.displayTitleValue, metadata.displayTitle));
    builder.setSubtitle(
        resolveMetadataText(metadata.subtitleToken, metadata.subtitleValue, metadata.subtitle));
    builder.setDescription(
        resolveMetadataText(
            metadata.descriptionToken, metadata.descriptionValue, metadata.description));
    if (metadata.artworkUri != null) {
      builder.setArtworkUri(Uri.parse(metadata.artworkUri));
    }
    if (metadata.artworkData != null) {
      builder.setArtworkData(
          metadata.artworkData,
          metadata.artworkDataType >= 0 ? metadata.artworkDataType : null);
    }
    if (metadata.durationMs >= 0) {
      builder.setDurationMs(metadata.durationMs);
    }
    if (metadata.trackNumber >= 0) {
      builder.setTrackNumber(metadata.trackNumber);
    }
    if (metadata.totalTrackCount >= 0) {
      builder.setTotalTrackCount(metadata.totalTrackCount);
    }
    if (metadata.isBrowsable >= 0) {
      builder.setIsBrowsable(metadata.isBrowsable != 0);
    }
    if (metadata.isPlayable >= 0) {
      builder.setIsPlayable(metadata.isPlayable != 0);
    }
    if (metadata.folderType >= 0) {
      builder.setFolderType(metadata.folderType);
    }
    if (metadata.recordingYear >= 0) {
      builder.setRecordingYear(metadata.recordingYear);
    }
    if (metadata.recordingMonth >= 0) {
      builder.setRecordingMonth(metadata.recordingMonth);
    }
    if (metadata.recordingDay >= 0) {
      builder.setRecordingDay(metadata.recordingDay);
    }
    if (metadata.releaseYear >= 0) {
      builder.setReleaseYear(metadata.releaseYear);
    }
    if (metadata.releaseMonth >= 0) {
      builder.setReleaseMonth(metadata.releaseMonth);
    }
    if (metadata.releaseDay >= 0) {
      builder.setReleaseDay(metadata.releaseDay);
    }
    builder.setWriter(
        resolveMetadataText(metadata.writerToken, metadata.writerValue, metadata.writer));
    builder.setAuthor(
        resolveMetadataText(metadata.authorToken, metadata.authorValue, metadata.author));
    builder.setComposer(
        resolveMetadataText(metadata.composerToken, metadata.composerValue, metadata.composer));
    builder.setConductor(
        resolveMetadataText(
            metadata.conductorToken, metadata.conductorValue, metadata.conductor));
    if (metadata.discNumber >= 0) {
      builder.setDiscNumber(metadata.discNumber);
    }
    if (metadata.totalDiscCount >= 0) {
      builder.setTotalDiscCount(metadata.totalDiscCount);
    }
    builder.setGenre(
        resolveMetadataText(metadata.genreToken, metadata.genreValue, metadata.genre));
    builder.setCompilation(
        resolveMetadataText(
            metadata.compilationToken, metadata.compilationValue, metadata.compilation));
    if (metadata.mediaType >= 0) {
      builder.setMediaType(metadata.mediaType);
    }
    builder.setStation(
        resolveMetadataText(metadata.stationToken, metadata.stationValue, metadata.station));
    if (metadata.extrasPresent || metadata.extrasValues.length > 0) {
      Object resolvedExtras = CppOpaqueObjectRegistry.resolve(metadata.extrasToken);
      builder.setExtras(
          resolvedExtras instanceof Bundle ? (Bundle) resolvedExtras : toBundle(metadata.extrasValues));
    }
    return builder.build();
  }

  static List<Effect> toVideoEffects(CppVideoEffect[] videoEffects) {
    if (videoEffects == null) {
      return new ArrayList<>();
    }
    List<Effect> converted = new ArrayList<>(videoEffects.length);
    for (CppVideoEffect videoEffect : videoEffects) {
      if (videoEffect == null) {
        continue;
      }
      switch (videoEffect.type) {
        case CppVideoEffect.TYPE_SCALE_AND_ROTATE:
          converted.add(
              new ScaleAndRotateTransformation.Builder()
                  .setScale(videoEffect.scaleX, videoEffect.scaleY)
                  .setRotationDegrees(videoEffect.rotationDegrees)
                  .build());
          break;
        case CppVideoEffect.TYPE_RGB_ADJUSTMENT:
          converted.add(
              new RgbAdjustment.Builder()
                  .setRedScale(videoEffect.redScale)
                  .setGreenScale(videoEffect.greenScale)
                  .setBlueScale(videoEffect.blueScale)
                  .build());
          break;
        case CppVideoEffect.TYPE_PRESENTATION:
          converted.add(
              Presentation.createForWidthAndHeight(
                  videoEffect.presentationWidth,
                  videoEffect.presentationHeight,
                  videoEffect.presentationLayout));
          break;
        default:
          break;
      }
    }
    return converted;
  }

  static CppPositionInfo fromPositionInfo(Player.PositionInfo positionInfo) {
    return new CppPositionInfo(
        positionInfo.mediaItemIndex,
        positionInfo.mediaItem != null ? fromMediaItem(positionInfo.mediaItem) : null,
        positionInfo.periodIndex,
        positionInfo.positionMs,
        positionInfo.contentPositionMs,
        positionInfo.adGroupIndex,
        positionInfo.adIndexInAdGroup);
  }

  static CppMediaMetadata fromMediaMetadata(MediaMetadata metadata) {
    return new CppMediaMetadata(
        metadata.title != null ? metadata.title.toString() : null,
        metadata.title != null ? CppOpaqueObjectRegistry.register(metadata.title) : null,
        metadata.artist != null ? metadata.artist.toString() : null,
        metadata.artist != null ? CppOpaqueObjectRegistry.register(metadata.artist) : null,
        metadata.albumTitle != null ? metadata.albumTitle.toString() : null,
        metadata.albumTitle != null ? CppOpaqueObjectRegistry.register(metadata.albumTitle) : null,
        metadata.albumArtist != null ? metadata.albumArtist.toString() : null,
        metadata.albumArtist != null
            ? CppOpaqueObjectRegistry.register(metadata.albumArtist)
            : null,
        metadata.displayTitle != null ? metadata.displayTitle.toString() : null,
        metadata.displayTitle != null ? CppOpaqueObjectRegistry.register(metadata.displayTitle) : null,
        metadata.subtitle != null ? metadata.subtitle.toString() : null,
        metadata.subtitle != null ? CppOpaqueObjectRegistry.register(metadata.subtitle) : null,
        metadata.description != null ? metadata.description.toString() : null,
        metadata.description != null ? CppOpaqueObjectRegistry.register(metadata.description) : null,
        metadata.artworkUri != null ? metadata.artworkUri.toString() : null,
        metadata.artworkData != null ? metadata.artworkData.clone() : null,
        metadata.artworkDataType != null ? metadata.artworkDataType : -1,
        metadata.durationMs != null ? metadata.durationMs : -1L,
        metadata.trackNumber != null ? metadata.trackNumber : -1,
        metadata.totalTrackCount != null ? metadata.totalTrackCount : -1,
        metadata.isBrowsable != null ? (metadata.isBrowsable ? 1 : 0) : -1,
        metadata.isPlayable != null ? (metadata.isPlayable ? 1 : 0) : -1,
        metadata.folderType != null ? metadata.folderType : -1,
        metadata.recordingYear != null ? metadata.recordingYear : -1,
        metadata.recordingMonth != null ? metadata.recordingMonth : -1,
        metadata.recordingDay != null ? metadata.recordingDay : -1,
        metadata.releaseYear != null ? metadata.releaseYear : -1,
        metadata.releaseMonth != null ? metadata.releaseMonth : -1,
        metadata.releaseDay != null ? metadata.releaseDay : -1,
        metadata.writer != null ? metadata.writer.toString() : null,
        metadata.writer != null ? CppOpaqueObjectRegistry.register(metadata.writer) : null,
        metadata.author != null ? metadata.author.toString() : null,
        metadata.author != null ? CppOpaqueObjectRegistry.register(metadata.author) : null,
        metadata.composer != null ? metadata.composer.toString() : null,
        metadata.composer != null ? CppOpaqueObjectRegistry.register(metadata.composer) : null,
        metadata.conductor != null ? metadata.conductor.toString() : null,
        metadata.conductor != null ? CppOpaqueObjectRegistry.register(metadata.conductor) : null,
        metadata.discNumber != null ? metadata.discNumber : -1,
        metadata.totalDiscCount != null ? metadata.totalDiscCount : -1,
        metadata.genre != null ? metadata.genre.toString() : null,
        metadata.genre != null ? CppOpaqueObjectRegistry.register(metadata.genre) : null,
        metadata.compilation != null ? metadata.compilation.toString() : null,
        metadata.compilation != null ? CppOpaqueObjectRegistry.register(metadata.compilation) : null,
        metadata.mediaType != null ? metadata.mediaType : -1,
        metadata.station != null ? metadata.station.toString() : null,
        metadata.station != null ? CppOpaqueObjectRegistry.register(metadata.station) : null,
        metadata.extras != null,
        metadata.extras != null ? metadata.extras.size() : 0,
        metadata.extras != null ? CppOpaqueObjectRegistry.register(metadata.extras) : null,
        toCppBundleValues(metadata.extras),
        toCppObjectValue(metadata.title),
        toCppObjectValue(metadata.artist),
        toCppObjectValue(metadata.albumTitle),
        toCppObjectValue(metadata.albumArtist),
        toCppObjectValue(metadata.displayTitle),
        toCppObjectValue(metadata.subtitle),
        toCppObjectValue(metadata.description),
        toCppObjectValue(metadata.writer),
        toCppObjectValue(metadata.author),
        toCppObjectValue(metadata.composer),
        toCppObjectValue(metadata.conductor),
        toCppObjectValue(metadata.genre),
        toCppObjectValue(metadata.compilation),
        toCppObjectValue(metadata.station));
  }

  static CppCue fromCue(Cue cue) {
    return new CppCue(
        cue.text != null ? cue.text.toString() : null,
        cue.text != null ? CppOpaqueObjectRegistry.register(cue.text) : null,
        cue.bitmap != null ? CppOpaqueObjectRegistry.register(cue.bitmap) : null,
        toCppAlignment(cue.textAlignment),
        toCppAlignment(cue.multiRowAlignment),
        cue.line,
        cue.lineType,
        cue.lineAnchor,
        cue.position,
        cue.positionAnchor,
        cue.size,
        cue.bitmapHeight,
        cue.textSize,
        cue.textSizeType,
        cue.verticalType,
        cue.shearDegrees,
        cue.zIndex,
        cue.windowColorSet,
        cue.windowColor,
        cue.bitmap != null);
  }

  static TrackSelectionParameters toTrackSelectionParameters(
      TrackSelectionParameters baseParameters,
      Tracks currentTracks,
      CppTrackSelectionParameters parameters) {
    TrackSelectionParameters.Builder builder = baseParameters.buildUpon();
    if (parameters.preferredAudioLanguages.length > 0) {
      builder.setPreferredAudioLanguages(parameters.preferredAudioLanguages);
    } else {
      builder.setPreferredAudioLanguage(parameters.preferredAudioLanguage);
    }
    if (parameters.preferredTextLanguages.length > 0) {
      builder.setPreferredTextLanguages(parameters.preferredTextLanguages);
    } else {
      builder.setPreferredTextLanguage(parameters.preferredTextLanguage);
    }
    builder.setPreferredAudioRoleFlags(parameters.preferredAudioRoleFlags);
    builder.setPreferredTextRoleFlags(parameters.preferredTextRoleFlags);
    builder.setMaxAudioChannelCount(parameters.maxAudioChannelCount);
    builder.setMaxAudioBitrate(parameters.maxAudioBitrate);
    builder.setMaxVideoSize(parameters.maxVideoWidth, parameters.maxVideoHeight);
    builder.setMaxVideoBitrate(parameters.maxVideoBitrate);
    builder.setViewportSize(
        parameters.viewportWidth,
        parameters.viewportHeight,
        parameters.viewportOrientationMayChange);
    builder.setSelectTextByDefault(parameters.selectTextByDefault);
    builder.setIgnoredTextSelectionFlags(parameters.ignoredTextSelectionFlags);
    builder.setSelectUndeterminedTextLanguage(parameters.selectUndeterminedTextLanguage);
    builder.setForceLowestBitrate(parameters.forceLowestBitrate);
    builder.setForceHighestSupportedBitrate(parameters.forceHighestSupportedBitrate);
    builder.setTrackTypeDisabled(C.TRACK_TYPE_VIDEO, parameters.disableVideo);
    builder.setTrackTypeDisabled(C.TRACK_TYPE_AUDIO, parameters.disableAudio);
    builder.setTrackTypeDisabled(C.TRACK_TYPE_TEXT, parameters.disableText);
    for (int disabledTrackType : parameters.disabledTrackTypes) {
      builder.setTrackTypeDisabled(disabledTrackType, true);
    }
    builder.clearOverrides();
    for (CppTrackSelectionOverride override : parameters.overrides) {
      boolean matchedGroup = false;
      for (Tracks.Group group : currentTracks.getGroups()) {
        if (group.getType() != override.trackType) {
          continue;
        }
        if (!group.getMediaTrackGroup().id.equals(override.trackGroupId)) {
          continue;
        }
        ArrayList<Integer> trackIndices = new ArrayList<>(override.trackIndices.length);
        for (int trackIndex : override.trackIndices) {
          if (trackIndex >= 0 && trackIndex < group.length) {
            trackIndices.add(trackIndex);
          }
        }
        builder.addOverride(new TrackSelectionOverride(group.getMediaTrackGroup(), trackIndices));
        matchedGroup = true;
        break;
      }
      // Best-effort mapping: stale override ids from an older media item are ignored because
      // TrackSelectionOverride requires a currently attached MediaTrackGroup instance.
      if (!matchedGroup) {
        continue;
      }
    }
    return builder.build();
  }

  static CppTrackSelectionParameters fromTrackSelectionParameters(
      TrackSelectionParameters parameters) {
    String[] preferredAudioLanguages =
        parameters.preferredAudioLanguages.toArray(new String[0]);
    String[] preferredTextLanguages =
        parameters.preferredTextLanguages.toArray(new String[0]);
    CppTrackSelectionOverride[] overrides =
        new CppTrackSelectionOverride[parameters.overrides.size()];
    int overrideIndex = 0;
    for (TrackSelectionOverride override : parameters.overrides.values()) {
      int[] trackIndices = new int[override.trackIndices.size()];
      for (int i = 0; i < override.trackIndices.size(); i++) {
        trackIndices[i] = override.trackIndices.get(i);
      }
      overrides[overrideIndex++] =
          new CppTrackSelectionOverride(
              override.mediaTrackGroup.id, override.getType(), trackIndices);
    }
    int[] disabledTrackTypes = new int[parameters.disabledTrackTypes.size()];
    int disabledTrackTypeIndex = 0;
    for (int disabledTrackType : parameters.disabledTrackTypes) {
      disabledTrackTypes[disabledTrackTypeIndex++] = disabledTrackType;
    }
    return new CppTrackSelectionParameters(
        parameters.preferredAudioLanguages.isEmpty()
            ? null
            : parameters.preferredAudioLanguages.get(0),
        parameters.preferredTextLanguages.isEmpty()
            ? null
            : parameters.preferredTextLanguages.get(0),
        preferredAudioLanguages,
        preferredTextLanguages,
        parameters.preferredAudioRoleFlags,
        parameters.preferredTextRoleFlags,
        parameters.maxAudioChannelCount,
        parameters.maxAudioBitrate,
        parameters.maxVideoWidth,
        parameters.maxVideoHeight,
        parameters.maxVideoBitrate,
        parameters.viewportWidth,
        parameters.viewportHeight,
        parameters.viewportOrientationMayChange,
        parameters.selectTextByDefault,
        parameters.ignoredTextSelectionFlags,
        parameters.selectUndeterminedTextLanguage,
        parameters.forceLowestBitrate,
        parameters.forceHighestSupportedBitrate,
        parameters.disabledTrackTypes.contains(C.TRACK_TYPE_VIDEO),
        parameters.disabledTrackTypes.contains(C.TRACK_TYPE_AUDIO),
        parameters.disabledTrackTypes.contains(C.TRACK_TYPE_TEXT),
        disabledTrackTypes,
        overrides);
  }

  static CppTrackGroup[] toCppTrackGroups(Tracks tracks) {
    List<Tracks.Group> groups = tracks.getGroups();
    CppTrackGroup[] result = new CppTrackGroup[groups.size()];
    for (int i = 0; i < groups.size(); i++) {
      Tracks.Group group = groups.get(i);
      CppTrackInfo[] trackInfos = new CppTrackInfo[group.length];
      for (int j = 0; j < group.length; j++) {
        androidx.media3.common.Format format = group.getTrackFormat(j);
        @Nullable DrmInitData drmInitData = format.drmInitData;
        trackInfos[j] =
            new CppTrackInfo(
                format.id,
                format.language,
                format.label,
                format.label != null ? CppOpaqueObjectRegistry.register(format.label) : null,
                format.sampleMimeType,
                format.containerMimeType,
                format.codecs,
                format.bitrate,
                format.averageBitrate,
                format.peakBitrate,
                format.metadata != null ? format.metadata.length() : 0,
                format.maxInputSize,
                format.maxNumReorderSamples,
                format.initializationData.size(),
                getInitializationDataTotalBytes(format.initializationData),
                drmInitData != null ? drmInitData.schemeDataCount : 0,
                format.subsampleOffsetUs,
                format.hasPrerollSamples,
                format.width,
                format.height,
                format.decodedWidth,
                format.decodedHeight,
                format.frameRate,
                format.rotationDegrees,
                format.pixelWidthHeightRatio,
                format.projectionData != null ? format.projectionData.length : 0,
                format.stereoMode,
                format.colorInfo != null ? format.colorInfo.colorSpace : Format.NO_VALUE,
                format.colorInfo != null ? format.colorInfo.colorRange : Format.NO_VALUE,
                format.colorInfo != null ? format.colorInfo.colorTransfer : Format.NO_VALUE,
                format.maxSubLayers,
                format.sampleRate,
                format.channelCount,
                format.pcmEncoding,
                format.encoderDelay,
                format.encoderPadding,
                format.accessibilityChannel,
                format.cueReplacementBehavior,
                format.tileCountHorizontal,
                format.tileCountVertical,
                format.cryptoType,
                format.roleFlags,
                format.selectionFlags,
                group.getTrackSupport(j),
                group.isTrackSelected(j),
                group.isTrackSupported(j, true),
                group.isTrackSupported(j),
                format.metadata != null ? CppOpaqueObjectRegistry.register(format.metadata) : null,
                getLabelLanguages(format.labels),
                getLabelValues(format.labels),
                format.customData != null ? CppOpaqueObjectRegistry.register(format.customData) : null,
                copyByteArrays(format.initializationData),
                drmInitData != null ? drmInitData.schemeType : null,
                getDrmSchemeUuids(drmInitData),
                getDrmSchemeLicenseServerUrls(drmInitData),
                getDrmSchemeMimeTypes(drmInitData),
                getDrmSchemeData(drmInitData),
                format.colorInfo != null && format.colorInfo.hdrStaticInfo != null
                    ? format.colorInfo.hdrStaticInfo.clone()
                    : null,
                format.colorInfo != null ? format.colorInfo.lumaBitdepth : Format.NO_VALUE,
                format.colorInfo != null ? format.colorInfo.chromaBitdepth : Format.NO_VALUE,
                format.projectionData != null ? format.projectionData.clone() : null,
                format.auxiliaryTrackType,
                getDrmSchemeDataHasData(drmInitData));
      }
      result[i] =
          new CppTrackGroup(
              group.getMediaTrackGroup().id,
              CppOpaqueObjectRegistry.register(group.getMediaTrackGroup()),
              group.getType(),
              group.isAdaptiveSupported(),
              group.isSelected(),
              group.isSupported(),
              group.isSupported(/* allowExceedsCapabilities= */ true),
              trackInfos);
    }
    return result;
  }

  private static int getInitializationDataTotalBytes(List<byte[]> initializationData) {
    int totalBytes = 0;
    for (byte[] data : initializationData) {
      totalBytes += data.length;
    }
    return totalBytes;
  }

  private static String[] getLabelLanguages(List<Label> labels) {
    String[] languages = new String[labels.size()];
    for (int i = 0; i < labels.size(); i++) {
      languages[i] = labels.get(i).language != null ? labels.get(i).language : "";
    }
    return languages;
  }

  private static String[] getLabelValues(List<Label> labels) {
    String[] values = new String[labels.size()];
    for (int i = 0; i < labels.size(); i++) {
      values[i] = labels.get(i).value;
    }
    return values;
  }

  private static byte[][] copyByteArrays(List<byte[]> values) {
    byte[][] result = new byte[values.size()][];
    for (int i = 0; i < values.size(); i++) {
      result[i] = values.get(i).clone();
    }
    return result;
  }

  private static String[] getDrmSchemeUuids(@Nullable DrmInitData drmInitData) {
    if (drmInitData == null) {
      return new String[0];
    }
    String[] uuids = new String[drmInitData.schemeDataCount];
    for (int i = 0; i < drmInitData.schemeDataCount; i++) {
      uuids[i] = drmInitData.get(i).uuid.toString();
    }
    return uuids;
  }

  private static String[] getDrmSchemeLicenseServerUrls(@Nullable DrmInitData drmInitData) {
    if (drmInitData == null) {
      return new String[0];
    }
    String[] licenseServerUrls = new String[drmInitData.schemeDataCount];
    for (int i = 0; i < drmInitData.schemeDataCount; i++) {
      @Nullable String licenseServerUrl = drmInitData.get(i).licenseServerUrl;
      licenseServerUrls[i] = licenseServerUrl != null ? licenseServerUrl : "";
    }
    return licenseServerUrls;
  }

  private static String[] getDrmSchemeMimeTypes(@Nullable DrmInitData drmInitData) {
    if (drmInitData == null) {
      return new String[0];
    }
    String[] mimeTypes = new String[drmInitData.schemeDataCount];
    for (int i = 0; i < drmInitData.schemeDataCount; i++) {
      mimeTypes[i] = drmInitData.get(i).mimeType;
    }
    return mimeTypes;
  }

  private static byte[][] getDrmSchemeData(@Nullable DrmInitData drmInitData) {
    if (drmInitData == null) {
      return new byte[0][];
    }
    byte[][] data = new byte[drmInitData.schemeDataCount][];
    for (int i = 0; i < drmInitData.schemeDataCount; i++) {
      @Nullable byte[] schemeData = drmInitData.get(i).data;
      data[i] = schemeData != null ? schemeData.clone() : new byte[0];
    }
    return data;
  }

  private static int[] getDrmSchemeDataHasData(@Nullable DrmInitData drmInitData) {
    if (drmInitData == null) {
      return new int[0];
    }
    int[] hasData = new int[drmInitData.schemeDataCount];
    for (int i = 0; i < drmInitData.schemeDataCount; i++) {
      hasData[i] = drmInitData.get(i).hasData() ? 1 : 0;
    }
    return hasData;
  }

  static CppTracks toCppTracks(Tracks tracks) {
    return new CppTracks(
        toCppTrackGroups(tracks),
        tracks.containsType(C.TRACK_TYPE_AUDIO),
        tracks.containsType(C.TRACK_TYPE_VIDEO),
        tracks.containsType(C.TRACK_TYPE_TEXT),
        tracks.containsType(C.TRACK_TYPE_IMAGE),
        tracks.isTypeSelected(C.TRACK_TYPE_AUDIO),
        tracks.isTypeSelected(C.TRACK_TYPE_VIDEO),
        tracks.isTypeSelected(C.TRACK_TYPE_TEXT),
        tracks.isTypeSelected(C.TRACK_TYPE_IMAGE),
        tracks.isTypeSupported(C.TRACK_TYPE_AUDIO),
        tracks.isTypeSupported(C.TRACK_TYPE_VIDEO),
        tracks.isTypeSupported(C.TRACK_TYPE_TEXT),
        tracks.isTypeSupported(C.TRACK_TYPE_IMAGE),
        tracks.isTypeSupported(C.TRACK_TYPE_AUDIO, true),
        tracks.isTypeSupported(C.TRACK_TYPE_VIDEO, true),
        tracks.isTypeSupported(C.TRACK_TYPE_TEXT, true),
        tracks.isTypeSupported(C.TRACK_TYPE_IMAGE, true));
  }
}
