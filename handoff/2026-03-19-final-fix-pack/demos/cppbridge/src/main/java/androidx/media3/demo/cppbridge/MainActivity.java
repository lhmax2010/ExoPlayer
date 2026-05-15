package androidx.media3.demo.cppbridge;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import androidx.media3.ui.PlayerView;

/** Demo activity that exercises the native ExoPlayer bridge from C++. */
public final class MainActivity extends AppCompatActivity {

  private static final String EXTRA_MEDIA_URL = "media_url";
  private static final String EXTRA_SOURCE_TYPE = "source_type";
  private static final String EXTRA_MIME_TYPE = "mime_type";
  private static final String EXTRA_SUBTITLE_URL = "subtitle_url";
  private static final String EXTRA_AUTO_PLAY = "auto_play";
  private static final String EXTRA_SKIP_DEFAULT_LOAD = "skip_default_load";

  private static final int SOURCE_AUTO = 0;
  private static final int SOURCE_DASH = 1;
  private static final int SOURCE_HLS = 2;
  private static final int SOURCE_SMOOTH_STREAMING = 3;
  private static final int SOURCE_RTSP = 4;
  private static final int SOURCE_PROGRESSIVE = 5;

  private static final String DEFAULT_HTTP_URL =
      "https://storage.googleapis.com/exoplayer-test-media-0/BigBuckBunny_320x180.mp4";
  private static final String DEFAULT_DASH_URL =
      "https://storage.googleapis.com/wvmedia/clear/h264/tears/tears.mpd";
  private static final String DEFAULT_HLS_URL =
      "https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_4x3/"
          + "bipbop_4x3_variant.m3u8";
  private static final String DEFAULT_SUBTITLE_URL =
      "https://bitdash-a.akamaihd.net/content/sintel/subtitles/subtitles_en.vtt";
  private static final String DEFAULT_HTTP_MIME = "video/mp4";
  private static final String DEFAULT_DASH_MIME = "application/dash+xml";
  private static final String DEFAULT_HLS_MIME = "application/x-mpegURL";

  private static final String[] SOURCE_LABELS = {
    "Auto",
    "DASH",
    "HLS",
    "HTTP / USB Progressive",
    "SmoothStreaming",
    "RTSP"
  };

  private static final int[] SOURCE_VALUES = {
    SOURCE_AUTO,
    SOURCE_DASH,
    SOURCE_HLS,
    SOURCE_PROGRESSIVE,
    SOURCE_SMOOTH_STREAMING,
    SOURCE_RTSP
  };

  static {
    System.loadLibrary("exoplayer_cppbridge_jni");
    System.loadLibrary("exoplayer_cppbridge_jni_testhooks");
  }

  private long nativePlayerHandle;
  private PlayerView playerView;
  private EditText urlInput;
  private EditText subtitleUrlInput;
  private EditText seekInput;
  private Spinner sourceTypeSpinner;
  private TextView statusText;
  private ActivityResultLauncher<String[]> openDocumentLauncher;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    playerView = findViewById(R.id.player_view);
    urlInput = findViewById(R.id.url_input);
    subtitleUrlInput = findViewById(R.id.subtitle_url_input);
    seekInput = findViewById(R.id.seek_input);
    sourceTypeSpinner = findViewById(R.id.source_type_spinner);
    statusText = findViewById(R.id.status_text);

    ArrayAdapter<String> sourceAdapter =
        new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, SOURCE_LABELS);
    sourceAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
    sourceTypeSpinner.setAdapter(sourceAdapter);

    openDocumentLauncher =
        registerForActivityResult(
            new ActivityResultContracts.OpenDocument(),
            uri -> {
              if (uri == null) {
                showStatus("USB/local file selection was canceled.");
                return;
              }
              grantPersistableReadPermission(uri);
              urlInput.setText(uri.toString());
              setSelectedSourceType(SOURCE_PROGRESSIVE);
              loadCurrentSource(
                  "USB/local file selected.\n"
                      + nativeLoadMedia(
                          nativePlayerHandle,
                          uri.toString(),
                          SOURCE_PROGRESSIVE,
                          resolveMimeType(uri, "")));
            });

    Button loadButton = findViewById(R.id.load_button);
    Button loadSubtitleButton = findViewById(R.id.load_subtitle_button);
    Button pickUsbButton = findViewById(R.id.pick_usb_button);
    Button httpSampleButton = findViewById(R.id.http_sample_button);
    Button dashSampleButton = findViewById(R.id.dash_sample_button);
    Button hlsSampleButton = findViewById(R.id.hls_sample_button);
    Button playlistButton = findViewById(R.id.playlist_button);
    Button playButton = findViewById(R.id.play_button);
    Button pauseButton = findViewById(R.id.pause_button);
    Button stopButton = findViewById(R.id.stop_button);
    Button seekToButton = findViewById(R.id.seek_to_button);
    Button seekBackButton = findViewById(R.id.seek_back_button);
    Button seekForwardButton = findViewById(R.id.seek_forward_button);
    Button previousButton = findViewById(R.id.previous_button);
    Button nextButton = findViewById(R.id.next_button);
    Button speedHalfButton = findViewById(R.id.speed_half_button);
    Button speedNormalButton = findViewById(R.id.speed_normal_button);
    Button speedOneHalfButton = findViewById(R.id.speed_one_half_button);
    Button speedTwoButton = findViewById(R.id.speed_two_button);
    Button audioButton = findViewById(R.id.audio_button);
    Button textButton = findViewById(R.id.text_button);
    Button textPrefButton = findViewById(R.id.text_pref_button);
    Button playbackInfoButton = findViewById(R.id.playback_info_button);
    Button tracksButton = findViewById(R.id.tracks_button);
    Button currentItemButton = findViewById(R.id.current_item_button);
    Button timelineButton = findViewById(R.id.timeline_button);
    Button metadataButton = findViewById(R.id.metadata_button);
    Button cuesButton = findViewById(R.id.cues_button);

    urlInput.setText(DEFAULT_HTTP_URL);
    subtitleUrlInput.setText(DEFAULT_SUBTITLE_URL);
    seekInput.setText("30");
    setSelectedSourceType(SOURCE_PROGRESSIVE);

    nativePlayerHandle = nativeCreatePlayer(this, playerView);
    if (!applyLaunchIntent(getIntent())) {
      loadCurrentSource(
          "Default HTTP progressive sample loaded.\n"
              + nativeLoadMedia(
                  nativePlayerHandle,
                  DEFAULT_HTTP_URL,
                  SOURCE_PROGRESSIVE,
                  DEFAULT_HTTP_MIME));
    }

    loadButton.setOnClickListener(
        view -> loadCurrentSource("Loaded current URL via C++ API.\n" + loadSelectedSource()));
    loadSubtitleButton.setOnClickListener(
        view -> {
          String mediaUrl = urlInput.getText().toString().trim();
          String subtitleUrl = subtitleUrlInput.getText().toString().trim();
          if (TextUtils.isEmpty(mediaUrl) || TextUtils.isEmpty(subtitleUrl)) {
            showStatus("Please provide both a media URL and a subtitle URL.");
            return;
          }
          showStatus(
              "Loaded media with external subtitle via C++ API.\n"
                  + nativeLoadMediaWithSubtitle(
                      nativePlayerHandle,
                      mediaUrl,
                      getSelectedSourceType(),
                      resolveMimeType(Uri.parse(mediaUrl), inferMimeTypeForCurrentSelection()),
                      subtitleUrl));
        });
    pickUsbButton.setOnClickListener(view -> openDocumentLauncher.launch(new String[] {"*/*"}));

    httpSampleButton.setOnClickListener(
        view -> {
          urlInput.setText(DEFAULT_HTTP_URL);
          setSelectedSourceType(SOURCE_PROGRESSIVE);
          loadCurrentSource(
              "HTTP progressive sample.\n"
                  + nativeLoadMedia(
                      nativePlayerHandle,
                      DEFAULT_HTTP_URL,
                      SOURCE_PROGRESSIVE,
                      DEFAULT_HTTP_MIME));
        });
    dashSampleButton.setOnClickListener(
        view -> {
          urlInput.setText(DEFAULT_DASH_URL);
          setSelectedSourceType(SOURCE_DASH);
          loadCurrentSource(
              "DASH sample.\n"
                  + nativeLoadMedia(
                      nativePlayerHandle,
                      DEFAULT_DASH_URL,
                      SOURCE_DASH,
                      DEFAULT_DASH_MIME));
        });
    hlsSampleButton.setOnClickListener(
        view -> {
          urlInput.setText(DEFAULT_HLS_URL);
          setSelectedSourceType(SOURCE_HLS);
          loadCurrentSource(
              "HLS sample.\n"
                  + nativeLoadMedia(
                      nativePlayerHandle,
                      DEFAULT_HLS_URL,
                      SOURCE_HLS,
                      DEFAULT_HLS_MIME));
        });
    playlistButton.setOnClickListener(
        view -> showStatus("Mixed playlist sample.\n" + nativeLoadDemoPlaylist(nativePlayerHandle)));

    playButton.setOnClickListener(
        view -> {
          nativePlay(nativePlayerHandle);
          showPlaybackState("Play");
        });
    pauseButton.setOnClickListener(
        view -> {
          nativePause(nativePlayerHandle);
          showPlaybackState("Pause");
        });
    stopButton.setOnClickListener(
        view -> {
          nativeStop(nativePlayerHandle);
          showPlaybackState("Stop");
        });

    seekToButton.setOnClickListener(
        view -> {
          long seekMs = parseSeekPositionMs();
          nativeSeekTo(nativePlayerHandle, seekMs);
          showPlaybackState("Seek to " + seekMs + " ms");
        });
    seekBackButton.setOnClickListener(
        view -> {
          nativeSeekBack(nativePlayerHandle);
          showPlaybackState("Seek back");
        });
    seekForwardButton.setOnClickListener(
        view -> {
          nativeSeekForward(nativePlayerHandle);
          showPlaybackState("Seek forward");
        });
    previousButton.setOnClickListener(
        view -> {
          nativeSeekToPrevious(nativePlayerHandle);
          showPlaybackState("Previous item");
        });
    nextButton.setOnClickListener(
        view -> {
          nativeSeekToNext(nativePlayerHandle);
          showPlaybackState("Next item");
        });

    speedHalfButton.setOnClickListener(view -> applyPlaybackSpeed(0.5f));
    speedNormalButton.setOnClickListener(view -> applyPlaybackSpeed(1.0f));
    speedOneHalfButton.setOnClickListener(view -> applyPlaybackSpeed(1.5f));
    speedTwoButton.setOnClickListener(view -> applyPlaybackSpeed(2.0f));

    audioButton.setOnClickListener(
        view -> showStatus(nativeCycleAudioTrack(nativePlayerHandle)));
    textButton.setOnClickListener(
        view -> showStatus(nativeCycleTextTrack(nativePlayerHandle)));
    textPrefButton.setOnClickListener(
        view -> {
          nativePreferTextLanguage(nativePlayerHandle, "en");
          showPlaybackState("Preferred text language set to en");
        });

    playbackInfoButton.setOnClickListener(
        view -> showStatus(nativeGetPlaybackSummary(nativePlayerHandle)));
    tracksButton.setOnClickListener(
        view -> showStatus(nativeGetTrackSummary(nativePlayerHandle)));
    currentItemButton.setOnClickListener(
        view -> showStatus(nativeGetCurrentItemSummary(nativePlayerHandle)));
    timelineButton.setOnClickListener(
        view -> showStatus(nativeGetTimelineSummary(nativePlayerHandle)));
    metadataButton.setOnClickListener(
        view -> showStatus(nativeGetCurrentMetadataSummary(nativePlayerHandle)));
    cuesButton.setOnClickListener(
        view -> showStatus(nativeGetCurrentCuesSummary(nativePlayerHandle)));
  }

  @Override
  protected void onDestroy() {
    if (nativePlayerHandle != 0L) {
      nativeRelease(nativePlayerHandle, playerView);
      nativePlayerHandle = 0L;
    }
    super.onDestroy();
  }

  private void applyPlaybackSpeed(float speed) {
    nativeSetPlaybackSpeed(nativePlayerHandle, speed);
    showPlaybackState("Trick speed " + speed + "x");
  }

  private void grantPersistableReadPermission(Uri uri) {
    try {
      getContentResolver()
          .takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
    } catch (SecurityException ignored) {
      // Some providers only grant a transient permission, which is still enough for immediate
      // playback. The native playback path remains the same either way.
    }
  }

  private int getSelectedSourceType() {
    int index = sourceTypeSpinner.getSelectedItemPosition();
    if (index < 0 || index >= SOURCE_VALUES.length) {
      return SOURCE_AUTO;
    }
    return SOURCE_VALUES[index];
  }

  private boolean applyLaunchIntent(Intent intent) {
    if (intent == null) {
      return false;
    }

    boolean skipDefaultLoad = intent.getBooleanExtra(EXTRA_SKIP_DEFAULT_LOAD, false);
    String mediaUrl = intent.getStringExtra(EXTRA_MEDIA_URL);
    if (TextUtils.isEmpty(mediaUrl)) {
      if (skipDefaultLoad) {
        showStatus("Demo launched without the default remote sample. Load a source to begin.");
        return true;
      }
      return false;
    }

    int sourceType = intent.getIntExtra(EXTRA_SOURCE_TYPE, SOURCE_AUTO);
    String mimeType = intent.getStringExtra(EXTRA_MIME_TYPE);
    String subtitleUrl = intent.getStringExtra(EXTRA_SUBTITLE_URL);
    boolean autoPlay = intent.getBooleanExtra(EXTRA_AUTO_PLAY, false);

    urlInput.setText(mediaUrl);
    setSelectedSourceType(sourceType);

    if (!TextUtils.isEmpty(subtitleUrl)) {
      subtitleUrlInput.setText(subtitleUrl);
    }

    Uri mediaUri = Uri.parse(mediaUrl);
    String resolvedMimeType =
        TextUtils.isEmpty(mimeType)
            ? resolveMimeType(mediaUri, inferMimeTypeForCurrentSelection())
            : mimeType;

    String summary;
    if (!TextUtils.isEmpty(subtitleUrl)) {
      summary =
          "Intent-loaded media + subtitle via C++ API.\n"
              + nativeLoadMediaWithSubtitle(
                  nativePlayerHandle, mediaUrl, sourceType, resolvedMimeType, subtitleUrl);
    } else {
      summary =
          "Intent-loaded media via C++ API.\n"
              + nativeLoadMedia(nativePlayerHandle, mediaUrl, sourceType, resolvedMimeType);
    }

    if (autoPlay) {
      nativePlay(nativePlayerHandle);
      showStatus(summary + "\n\nAutoplay requested.\n" + nativeGetPlaybackSummary(nativePlayerHandle));
    } else {
      loadCurrentSource(summary);
    }
    return true;
  }

  private String inferMimeTypeForCurrentSelection() {
    int sourceType = getSelectedSourceType();
    if (sourceType == SOURCE_DASH) {
      return DEFAULT_DASH_MIME;
    }
    if (sourceType == SOURCE_HLS) {
      return DEFAULT_HLS_MIME;
    }
    if (sourceType == SOURCE_RTSP) {
      return "application/x-rtsp";
    }
    return "";
  }

  private void loadCurrentSource(String summary) {
    showStatus(summary + "\n\n" + nativeGetPlaybackSummary(nativePlayerHandle));
  }

  private String loadSelectedSource() {
    String mediaUrl = urlInput.getText().toString().trim();
    if (TextUtils.isEmpty(mediaUrl)) {
      return "Please provide a media URL.";
    }
    return nativeLoadMedia(
        nativePlayerHandle,
        mediaUrl,
        getSelectedSourceType(),
        resolveMimeType(Uri.parse(mediaUrl), inferMimeTypeForCurrentSelection()));
  }

  private long parseSeekPositionMs() {
    String rawValue = seekInput.getText().toString().trim();
    if (TextUtils.isEmpty(rawValue)) {
      return 30_000L;
    }
    try {
      return Math.max(0L, (long) (Double.parseDouble(rawValue) * 1000d));
    } catch (NumberFormatException e) {
      return 30_000L;
    }
  }

  private String resolveMimeType(Uri uri, String fallbackMimeType) {
    if ("content".equalsIgnoreCase(uri.getScheme())) {
      String contentMimeType = getContentResolver().getType(uri);
      if (!TextUtils.isEmpty(contentMimeType)) {
        return contentMimeType;
      }
    }
    return fallbackMimeType;
  }

  private void setSelectedSourceType(int sourceType) {
    for (int i = 0; i < SOURCE_VALUES.length; i++) {
      if (SOURCE_VALUES[i] == sourceType) {
        sourceTypeSpinner.setSelection(i);
        return;
      }
    }
    sourceTypeSpinner.setSelection(0);
  }

  private void showPlaybackState(String action) {
    showStatus(action + "\n" + nativeGetPlaybackSummary(nativePlayerHandle));
  }

  private void showStatus(String message) {
    statusText.setText(message);
  }

  private native long nativeCreatePlayer(android.content.Context context, PlayerView playerView);

  private native String nativeLoadMedia(
      long nativeHandle, String url, int sourceType, String mimeType);

  private native String nativeLoadMediaWithSubtitle(
      long nativeHandle, String mediaUrl, int sourceType, String mimeType, String subtitleUrl);

  private native String nativeLoadDemoPlaylist(long nativeHandle);

  private native void nativePlay(long nativeHandle);

  private native void nativePause(long nativeHandle);

  private native void nativeStop(long nativeHandle);

  private native void nativeSeekTo(long nativeHandle, long positionMs);

  private native void nativeSeekBack(long nativeHandle);

  private native void nativeSeekForward(long nativeHandle);

  private native void nativeSeekToNext(long nativeHandle);

  private native void nativeSeekToPrevious(long nativeHandle);

  private native void nativeSetPlaybackSpeed(long nativeHandle, float speed);

  private native void nativePreferTextLanguage(long nativeHandle, String language);

  private native String nativeCycleAudioTrack(long nativeHandle);

  private native String nativeCycleTextTrack(long nativeHandle);

  private native String nativeGetPlaybackSummary(long nativeHandle);

  private native String nativeGetTrackSummary(long nativeHandle);

  private native String nativeGetCurrentItemSummary(long nativeHandle);

  private native String nativeGetTimelineSummary(long nativeHandle);

  private native String nativeGetCurrentMetadataSummary(long nativeHandle);

  private native String nativeGetCurrentCuesSummary(long nativeHandle);

  private native void nativeRelease(long nativeHandle, PlayerView playerView);
}
