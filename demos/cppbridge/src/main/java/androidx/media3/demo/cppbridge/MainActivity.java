package androidx.media3.demo.cppbridge;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import androidx.media3.ui.PlayerView;
import java.util.Locale;

/** Demo activity that presents the native ExoPlayer bridge as a simple player. */
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
  private static final int SOURCE_PROGRESSIVE = 5;

  private static final String DEFAULT_HTTP_URL =
      "https://storage.googleapis.com/exoplayer-test-media-0/BigBuckBunny_320x180.mp4";
  private static final String DEFAULT_DASH_URL =
      "https://storage.googleapis.com/wvmedia/clear/h264/tears/tears.mpd";
  private static final String DEFAULT_HLS_URL =
      "https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_4x3/"
          + "bipbop_4x3_variant.m3u8";
  private static final String DEFAULT_HTTP_MIME = "video/mp4";
  private static final String DEFAULT_DASH_MIME = "application/dash+xml";
  private static final String DEFAULT_HLS_MIME = "application/x-mpegURL";

  static {
    System.loadLibrary("exoplayer_cppbridge_jni");
    try {
      System.loadLibrary("exoplayer_cppbridge_jni_testhooks");
    } catch (UnsatisfiedLinkError ignored) {
      // Test hooks are optional for production-only demo builds.
    }
  }

  private long nativePlayerHandle;
  private PlayerView playerView;
  private TextView currentSourceLabel;
  private ActivityResultLauncher<String[]> openDocumentLauncher;
  private float currentSpeed = 1.0f;
  private String currentSourceName = "HTTP";

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    playerView = findViewById(R.id.player_view);
    currentSourceLabel = findViewById(R.id.current_source_label);
    nativePlayerHandle = nativeCreatePlayer(this, playerView);

    openDocumentLauncher =
        registerForActivityResult(
            new ActivityResultContracts.OpenDocument(),
            uri -> {
              if (uri == null) {
                showTransientMessage("File selection canceled");
                return;
              }
              grantPersistableReadPermission(uri);
              loadAndPlay("File", uri.toString(), SOURCE_PROGRESSIVE, resolveMimeType(uri, ""));
            });

    wireSourceButtons();
    wireTransportButtons();
    wireSeekButtons();
    wireSpeedButtons();
    wireTrackButtons();

    if (nativePlayerHandle == 0L) {
      currentSourceLabel.setText(R.string.player_create_failed);
      showTransientMessage(getString(R.string.player_create_failed));
    } else if (!applyLaunchIntent(getIntent())) {
      loadAndPlay("HTTP", DEFAULT_HTTP_URL, SOURCE_PROGRESSIVE, DEFAULT_HTTP_MIME);
    }
  }

  @Override
  protected void onDestroy() {
    if (nativePlayerHandle != 0L) {
      nativeRelease(nativePlayerHandle, playerView);
      nativePlayerHandle = 0L;
    }
    super.onDestroy();
  }

  private void wireSourceButtons() {
    Button httpSampleButton = findViewById(R.id.http_sample_button);
    Button dashSampleButton = findViewById(R.id.dash_sample_button);
    Button hlsSampleButton = findViewById(R.id.hls_sample_button);
    Button pickUsbButton = findViewById(R.id.pick_usb_button);
    Button playlistButton = findViewById(R.id.playlist_button);

    httpSampleButton.setOnClickListener(
        view -> loadAndPlay("HTTP", DEFAULT_HTTP_URL, SOURCE_PROGRESSIVE, DEFAULT_HTTP_MIME));
    dashSampleButton.setOnClickListener(
        view -> loadAndPlay("DASH", DEFAULT_DASH_URL, SOURCE_DASH, DEFAULT_DASH_MIME));
    hlsSampleButton.setOnClickListener(
        view -> loadAndPlay("HLS", DEFAULT_HLS_URL, SOURCE_HLS, DEFAULT_HLS_MIME));
    pickUsbButton.setOnClickListener(view -> openDocumentLauncher.launch(new String[] {"*/*"}));
    playlistButton.setOnClickListener(
        view -> {
          if (!ensurePlayerReady()) {
            return;
          }
          nativeLoadDemoPlaylist(nativePlayerHandle);
          nativePlay(nativePlayerHandle);
          currentSourceName = "Playlist";
          updateCurrentSourceLabel();
        });
  }

  private void wireTransportButtons() {
    Button resumeButton = findViewById(R.id.play_button);
    Button pauseButton = findViewById(R.id.pause_button);
    Button stopButton = findViewById(R.id.stop_button);

    resumeButton.setOnClickListener(
        view -> {
          if (ensurePlayerReady()) {
            nativePlay(nativePlayerHandle);
          }
        });
    pauseButton.setOnClickListener(
        view -> {
          if (ensurePlayerReady()) {
            nativePause(nativePlayerHandle);
          }
        });
    stopButton.setOnClickListener(
        view -> {
          if (ensurePlayerReady()) {
            nativeStop(nativePlayerHandle);
          }
        });
  }

  private void wireSeekButtons() {
    Button startButton = findViewById(R.id.seek_to_button);
    Button seekBackButton = findViewById(R.id.seek_back_button);
    Button seekForwardButton = findViewById(R.id.seek_forward_button);
    Button previousButton = findViewById(R.id.previous_button);
    Button nextButton = findViewById(R.id.next_button);

    startButton.setOnClickListener(
        view -> {
          if (ensurePlayerReady()) {
            nativeSeekTo(nativePlayerHandle, 0L);
          }
        });
    seekBackButton.setOnClickListener(
        view -> {
          if (ensurePlayerReady()) {
            nativeSeekBack(nativePlayerHandle);
          }
        });
    seekForwardButton.setOnClickListener(
        view -> {
          if (ensurePlayerReady()) {
            nativeSeekForward(nativePlayerHandle);
          }
        });
    previousButton.setOnClickListener(
        view -> {
          if (ensurePlayerReady()) {
            nativeSeekToPrevious(nativePlayerHandle);
          }
        });
    nextButton.setOnClickListener(
        view -> {
          if (ensurePlayerReady()) {
            nativeSeekToNext(nativePlayerHandle);
          }
        });
  }

  private void wireSpeedButtons() {
    findViewById(R.id.speed_half_button).setOnClickListener(view -> applyPlaybackSpeed(0.5f));
    findViewById(R.id.speed_normal_button).setOnClickListener(view -> applyPlaybackSpeed(1.0f));
    findViewById(R.id.speed_one_half_button).setOnClickListener(view -> applyPlaybackSpeed(1.5f));
    findViewById(R.id.speed_two_button).setOnClickListener(view -> applyPlaybackSpeed(2.0f));
  }

  private void wireTrackButtons() {
    Button audioButton = findViewById(R.id.audio_button);
    Button textButton = findViewById(R.id.text_button);
    Button textPrefButton = findViewById(R.id.text_pref_button);

    audioButton.setOnClickListener(
        view -> {
          if (ensurePlayerReady()) {
            nativeCycleAudioTrack(nativePlayerHandle);
          }
        });
    textButton.setOnClickListener(
        view -> {
          if (ensurePlayerReady()) {
            nativeCycleTextTrack(nativePlayerHandle);
          }
        });
    textPrefButton.setOnClickListener(
        view -> {
          if (ensurePlayerReady()) {
            nativePreferTextLanguage(nativePlayerHandle, "en");
          }
        });
  }

  private void loadAndPlay(String label, String mediaUrl, int sourceType, String mimeType) {
    if (!ensurePlayerReady()) {
      return;
    }
    nativeLoadMedia(nativePlayerHandle, mediaUrl, sourceType, mimeType);
    nativePlay(nativePlayerHandle);
    currentSourceName = label;
    updateCurrentSourceLabel();
  }

  private void applyPlaybackSpeed(float speed) {
    if (!ensurePlayerReady()) {
      return;
    }
    currentSpeed = speed;
    nativeSetPlaybackSpeed(nativePlayerHandle, speed);
    updateCurrentSourceLabel();
  }

  private boolean ensurePlayerReady() {
    if (nativePlayerHandle != 0L) {
      return true;
    }
    showTransientMessage(getString(R.string.player_create_failed));
    return false;
  }

  private boolean applyLaunchIntent(Intent intent) {
    if (intent == null) {
      return false;
    }

    boolean skipDefaultLoad = intent.getBooleanExtra(EXTRA_SKIP_DEFAULT_LOAD, false);
    String mediaUrl = intent.getStringExtra(EXTRA_MEDIA_URL);
    if (TextUtils.isEmpty(mediaUrl)) {
      if (skipDefaultLoad) {
        currentSourceName = "Select source";
        updateCurrentSourceLabel();
        return true;
      }
      return false;
    }

    int sourceType = intent.getIntExtra(EXTRA_SOURCE_TYPE, SOURCE_AUTO);
    String mimeType = intent.getStringExtra(EXTRA_MIME_TYPE);
    String subtitleUrl = intent.getStringExtra(EXTRA_SUBTITLE_URL);
    boolean autoPlay = intent.getBooleanExtra(EXTRA_AUTO_PLAY, true);
    String resolvedMimeType =
        TextUtils.isEmpty(mimeType)
            ? resolveMimeType(Uri.parse(mediaUrl), inferMimeType(sourceType))
            : mimeType;

    if (!TextUtils.isEmpty(subtitleUrl)) {
      nativeLoadMediaWithSubtitle(
          nativePlayerHandle, mediaUrl, sourceType, resolvedMimeType, subtitleUrl);
    } else {
      nativeLoadMedia(nativePlayerHandle, mediaUrl, sourceType, resolvedMimeType);
    }
    if (autoPlay) {
      nativePlay(nativePlayerHandle);
    }
    currentSourceName = sourceTypeToLabel(sourceType);
    updateCurrentSourceLabel();
    return true;
  }

  private String inferMimeType(int sourceType) {
    if (sourceType == SOURCE_DASH) {
      return DEFAULT_DASH_MIME;
    }
    if (sourceType == SOURCE_HLS) {
      return DEFAULT_HLS_MIME;
    }
    if (sourceType == SOURCE_PROGRESSIVE) {
      return DEFAULT_HTTP_MIME;
    }
    return "";
  }

  private String sourceTypeToLabel(int sourceType) {
    if (sourceType == SOURCE_DASH) {
      return "DASH";
    }
    if (sourceType == SOURCE_HLS) {
      return "HLS";
    }
    if (sourceType == SOURCE_PROGRESSIVE) {
      return "HTTP";
    }
    return "Media";
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

  private void grantPersistableReadPermission(Uri uri) {
    try {
      getContentResolver()
          .takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
    } catch (SecurityException ignored) {
      // Some providers only grant transient permissions, which are enough for immediate playback.
    }
  }

  private void updateCurrentSourceLabel() {
    currentSourceLabel.setText(
        String.format(Locale.US, "%s  |  %.1fx", currentSourceName, currentSpeed));
  }

  private void showTransientMessage(String message) {
    Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
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

  private native void nativeRelease(long nativeHandle, PlayerView playerView);
}
