package androidx.media3.demo.cppbridge;

import android.content.Intent;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.SeekBar;
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

  private static final int MENU_HTTP = 1;
  private static final int MENU_DASH = 2;
  private static final int MENU_HLS = 3;
  private static final int MENU_FILE = 4;
  private static final int MENU_PLAYLIST = 5;
  private static final int MENU_AUDIO = 6;
  private static final int MENU_TEXT = 7;
  private static final int MENU_TEXT_EN = 8;
  private static final int MENU_SPEED_HALF = 9;
  private static final int MENU_SPEED_NORMAL = 10;
  private static final int MENU_SPEED_ONE_HALF = 11;
  private static final int MENU_SPEED_TWO = 12;
  private static final int MENU_SEEK_BACK = 13;
  private static final int MENU_SEEK_FORWARD = 14;
  private static final int MENU_STOP = 15;
  private static final int MENU_SUBTITLE_FILE = 16;

  private static final long TIME_UNSET = -9223372036854775807L;
  private static final long SEEK_STEP_MS = 10_000L;
  private static final long PROGRESS_UPDATE_MS = 500L;

  private static final String DEFAULT_HTTP_URL =
      "https://storage.googleapis.com/exoplayer-test-media-0/BigBuckBunny_320x180.mp4";
  private static final String DEFAULT_DASH_URL =
      "https://storage.googleapis.com/shaka-demo-assets/sintel/dash.mpd";
  private static final String DEFAULT_HLS_URL =
      "https://devstreaming-cdn.apple.com/videos/streaming/examples/img_bipbop_adv_example_fmp4/"
          + "master.m3u8";
  private static final String DEFAULT_HTTP_MIME = "video/mp4";
  private static final String DEFAULT_DASH_MIME = "application/dash+xml";
  private static final String DEFAULT_HLS_MIME = "application/x-mpegURL";
  private static final String[] DEMO_SUBTITLE_URLS = {
    "asset:///subtitles/demo_en.vtt", "asset:///subtitles/demo_es.vtt"
  };
  private static final String[] DEMO_SUBTITLE_MIME_TYPES = {"text/vtt", "text/vtt"};
  private static final String[] DEMO_SUBTITLE_LANGUAGES = {"en", "es"};
  private static final String[] DEMO_SUBTITLE_LABELS = {"English", "Spanish"};

  static {
    System.loadLibrary("exoplayer_cppbridge_jni");
    try {
      System.loadLibrary("exoplayer_cppbridge_jni_testhooks");
    } catch (UnsatisfiedLinkError ignored) {
      // Test hooks are optional for production-only demo builds.
    }
  }

  private final Handler progressHandler = new Handler(Looper.getMainLooper());
  private final Runnable progressRunnable =
      new Runnable() {
        @Override
        public void run() {
          updatePlaybackProgress();
          progressHandler.postDelayed(this, PROGRESS_UPDATE_MS);
        }
      };

  private long nativePlayerHandle;
  private PlayerView playerView;
  private TextView currentSourceLabel;
  private TextView playbackTimeText;
  private SeekBar playbackSeekBar;
  private Button playPauseButton;
  private PopupWindow playerMenuWindow;
  private ActivityResultLauncher<String[]> openDocumentLauncher;
  private ActivityResultLauncher<String[]> openSubtitleDocumentLauncher;
  private boolean isUserSeeking;
  private float currentSpeed = 1.0f;
  private String currentSourceName = "HTTP";
  private String currentMediaUrl;
  private int currentSourceType = SOURCE_PROGRESSIVE;
  private String currentMimeType = DEFAULT_HTTP_MIME;
  private String selectedSubtitleUrl;
  private String selectedSubtitleMimeType;
  private String selectedSubtitleLanguage;
  private String selectedSubtitleLabel;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    enterImmersiveMode();
    setContentView(R.layout.activity_main);

    playerView = findViewById(R.id.player_view);
    currentSourceLabel = findViewById(R.id.current_source_label);
    playbackTimeText = findViewById(R.id.playback_time_text);
    playbackSeekBar = findViewById(R.id.playback_seek_bar);
    playPauseButton = findViewById(R.id.play_pause_button);
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
    openSubtitleDocumentLauncher =
        registerForActivityResult(
            new ActivityResultContracts.OpenDocument(),
            uri -> {
              if (uri == null) {
                showTransientMessage("Subtitle selection canceled");
                return;
              }
              if (TextUtils.isEmpty(currentMediaUrl)) {
                showTransientMessage(getString(R.string.load_media_first));
                return;
              }
              grantPersistableReadPermission(uri);
              selectedSubtitleUrl = uri.toString();
              selectedSubtitleMimeType = resolveSubtitleMimeType(uri);
              selectedSubtitleLanguage = "und";
              selectedSubtitleLabel = getString(R.string.external_subtitle_label);
              reloadCurrentMedia(/* preservePlayback= */ true);
              showTransientMessage(getString(R.string.subtitle_attached));
            });

    wireCompactControls();
    wireSeekBar();

    if (nativePlayerHandle == 0L) {
      currentSourceLabel.setText(R.string.player_create_failed);
      showTransientMessage(getString(R.string.player_create_failed));
    } else if (!applyLaunchIntent(getIntent())) {
      loadAndPlay("HTTP", DEFAULT_HTTP_URL, SOURCE_PROGRESSIVE, DEFAULT_HTTP_MIME);
    }
    progressHandler.post(progressRunnable);
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      enterImmersiveMode();
    }
  }

  @Override
  protected void onDestroy() {
    progressHandler.removeCallbacks(progressRunnable);
    if (playerMenuWindow != null) {
      playerMenuWindow.dismiss();
      playerMenuWindow = null;
    }
    if (nativePlayerHandle != 0L) {
      nativeRelease(nativePlayerHandle, playerView);
      nativePlayerHandle = 0L;
    }
    super.onDestroy();
  }

  private void wireCompactControls() {
    playPauseButton.setOnClickListener(
        view -> {
          if (!ensurePlayerReady()) {
            return;
          }
          if (nativeIsPlaying(nativePlayerHandle)) {
            nativePause(nativePlayerHandle);
          } else {
            nativePlay(nativePlayerHandle);
          }
          updatePlaybackProgress();
        });
    findViewById(R.id.player_menu_button).setOnClickListener(view -> showPlayerMenu());
  }

  private void wireSeekBar() {
    playbackSeekBar.setOnSeekBarChangeListener(
        new SeekBar.OnSeekBarChangeListener() {
          @Override
          public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            if (fromUser) {
              playbackTimeText.setText(
                  formatTime(progress) + " / " + formatTime(nativeGetDuration(nativePlayerHandle)));
            }
          }

          @Override
          public void onStartTrackingTouch(SeekBar seekBar) {
            isUserSeeking = true;
          }

          @Override
          public void onStopTrackingTouch(SeekBar seekBar) {
            isUserSeeking = false;
            if (ensurePlayerReady()) {
              nativeSeekTo(nativePlayerHandle, seekBar.getProgress());
            }
          }
        });
  }

  private void showPlayerMenu() {
    if (playerMenuWindow != null && playerMenuWindow.isShowing()) {
      playerMenuWindow.dismiss();
      return;
    }

    LinearLayout panel = new LinearLayout(this);
    panel.setOrientation(LinearLayout.VERTICAL);
    panel.setPadding(dp(6), dp(6), dp(6), dp(6));
    panel.setBackgroundColor(Color.rgb(38, 38, 38));

    addMenuRow(
        panel,
        new int[] {MENU_HTTP, MENU_DASH, MENU_HLS},
        new int[] {R.string.http_sample, R.string.dash_sample, R.string.hls_sample});
    addMenuRow(
        panel,
        new int[] {MENU_FILE, MENU_PLAYLIST, MENU_STOP},
        new int[] {R.string.pick_usb_file, R.string.load_demo_playlist, R.string.stop});
    addMenuRow(
        panel,
        new int[] {MENU_SPEED_HALF, MENU_SPEED_NORMAL, MENU_SPEED_ONE_HALF, MENU_SPEED_TWO},
        new int[] {
          R.string.speed_half, R.string.speed_normal, R.string.speed_one_half, R.string.speed_two
        });
    addMenuRow(
        panel,
        new int[] {MENU_SEEK_BACK, MENU_SEEK_FORWARD, MENU_AUDIO},
        new int[] {R.string.seek_back_10s, R.string.seek_forward_10s, R.string.cycle_audio});
    addMenuRow(
        panel,
        new int[] {MENU_TEXT, MENU_TEXT_EN, MENU_SUBTITLE_FILE},
        new int[] {R.string.cycle_text, R.string.prefer_text, R.string.pick_subtitle_file});

    playerMenuWindow =
        new PopupWindow(panel, dp(300), ViewGroup.LayoutParams.WRAP_CONTENT, /* focusable= */ true);
    playerMenuWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
    playerMenuWindow.setOutsideTouchable(true);
    playerMenuWindow.setElevation(dp(8));
    playerMenuWindow.showAtLocation(
        playerView, Gravity.BOTTOM | Gravity.END, dp(16), dp(88));
  }

  private void addMenuRow(LinearLayout panel, int[] actionIds, int[] textIds) {
    LinearLayout row = new LinearLayout(this);
    row.setOrientation(LinearLayout.HORIZONTAL);
    panel.addView(row, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(34)));
    for (int i = 0; i < actionIds.length; i++) {
      Button button = new Button(this);
      button.setAllCaps(false);
      button.setMinWidth(0);
      button.setMinHeight(0);
      button.setPadding(dp(4), 0, dp(4), 0);
      button.setText(textIds[i]);
      button.setTextColor(Color.WHITE);
      button.setTextSize(12);
      button.setBackgroundTintList(ColorStateList.valueOf(Color.rgb(27, 39, 51)));
      int actionId = actionIds[i];
      button.setOnClickListener(
          view -> {
            if (playerMenuWindow != null) {
              playerMenuWindow.dismiss();
            }
            handleMenuAction(actionId);
          });
      LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0, dp(30), 1f);
      params.setMargins(dp(2), dp(2), dp(2), dp(2));
      row.addView(button, params);
    }
  }

  private boolean handleMenuAction(int actionId) {
    switch (actionId) {
      case MENU_HTTP:
        loadAndPlay("HTTP", DEFAULT_HTTP_URL, SOURCE_PROGRESSIVE, DEFAULT_HTTP_MIME);
        return true;
      case MENU_DASH:
        loadAndPlay("DASH", DEFAULT_DASH_URL, SOURCE_DASH, DEFAULT_DASH_MIME);
        return true;
      case MENU_HLS:
        loadAndPlay("HLS", DEFAULT_HLS_URL, SOURCE_HLS, DEFAULT_HLS_MIME);
        return true;
      case MENU_FILE:
        openDocumentLauncher.launch(new String[] {"*/*"});
        return true;
      case MENU_PLAYLIST:
        loadPlaylist();
        return true;
      case MENU_SPEED_HALF:
        applyPlaybackSpeed(0.5f);
        return true;
      case MENU_SPEED_NORMAL:
        applyPlaybackSpeed(1.0f);
        return true;
      case MENU_SPEED_ONE_HALF:
        applyPlaybackSpeed(1.5f);
        return true;
      case MENU_SPEED_TWO:
        applyPlaybackSpeed(2.0f);
        return true;
      case MENU_SEEK_BACK:
        if (ensurePlayerReady()) {
          seekRelative(-SEEK_STEP_MS);
        }
        return true;
      case MENU_SEEK_FORWARD:
        if (ensurePlayerReady()) {
          seekRelative(SEEK_STEP_MS);
        }
        return true;
      case MENU_AUDIO:
        if (ensurePlayerReady()) {
          showTrackMessage(nativeCycleAudioTrack(nativePlayerHandle), R.string.audio_changed);
        }
        return true;
      case MENU_TEXT:
        if (ensurePlayerReady()) {
          showTrackMessage(nativeCycleTextTrack(nativePlayerHandle), R.string.text_changed);
        }
        return true;
      case MENU_TEXT_EN:
        if (ensurePlayerReady()) {
          nativePreferTextLanguage(nativePlayerHandle, "en");
          showTransientMessage(getString(R.string.prefer_text_applied));
        }
        return true;
      case MENU_SUBTITLE_FILE:
        if (TextUtils.isEmpty(currentMediaUrl)) {
          showTransientMessage(getString(R.string.load_media_first));
        } else {
          openSubtitleDocumentLauncher.launch(
              new String[] {
                "text/*", "application/ttml+xml", "application/x-subrip", "application/octet-stream"
              });
        }
        return true;
      case MENU_STOP:
        if (ensurePlayerReady()) {
          nativeStop(nativePlayerHandle);
          updatePlaybackProgress();
        }
        return true;
      default:
        return false;
    }
  }

  private void loadPlaylist() {
    if (!ensurePlayerReady()) {
      return;
    }
    nativeLoadDemoPlaylist(nativePlayerHandle);
    nativePlay(nativePlayerHandle);
    currentSourceName = "Playlist";
    currentMediaUrl = null;
    clearSelectedSubtitle();
    updateCurrentSourceLabel();
    updatePlaybackProgress();
  }

  private void loadAndPlay(String label, String mediaUrl, int sourceType, String mimeType) {
    if (!ensurePlayerReady()) {
      return;
    }
    currentSourceName = label;
    currentMediaUrl = mediaUrl;
    currentSourceType = sourceType;
    currentMimeType = TextUtils.isEmpty(mimeType) ? inferMimeType(sourceType) : mimeType;
    clearSelectedSubtitle();
    nativeLoadMediaWithSubtitles(
        nativePlayerHandle,
        currentMediaUrl,
        currentSourceType,
        currentMimeType,
        buildSubtitleUrls(),
        buildSubtitleMimeTypes(),
        buildSubtitleLanguages(),
        buildSubtitleLabels());
    nativePlay(nativePlayerHandle);
    updateCurrentSourceLabel();
    updatePlaybackProgress();
  }

  private void reloadCurrentMedia(boolean preservePlayback) {
    if (!ensurePlayerReady() || TextUtils.isEmpty(currentMediaUrl)) {
      return;
    }
    long resumePositionMs =
        preservePlayback ? Math.max(0L, nativeGetCurrentPosition(nativePlayerHandle)) : 0L;
    boolean resumePlaying = preservePlayback && nativeIsPlaying(nativePlayerHandle);
    nativeLoadMediaWithSubtitles(
        nativePlayerHandle,
        currentMediaUrl,
        currentSourceType,
        currentMimeType,
        buildSubtitleUrls(),
        buildSubtitleMimeTypes(),
        buildSubtitleLanguages(),
        buildSubtitleLabels());
    if (resumePositionMs > 0L) {
      nativeSeekTo(nativePlayerHandle, resumePositionMs);
    }
    if (resumePlaying || !preservePlayback) {
      nativePlay(nativePlayerHandle);
    } else {
      nativePause(nativePlayerHandle);
    }
    updateCurrentSourceLabel();
    updatePlaybackProgress();
  }

  private String[] buildSubtitleUrls() {
    if (TextUtils.isEmpty(selectedSubtitleUrl)) {
      return DEMO_SUBTITLE_URLS;
    }
    return append(DEMO_SUBTITLE_URLS, selectedSubtitleUrl);
  }

  private String[] buildSubtitleMimeTypes() {
    if (TextUtils.isEmpty(selectedSubtitleUrl)) {
      return DEMO_SUBTITLE_MIME_TYPES;
    }
    return append(DEMO_SUBTITLE_MIME_TYPES, selectedSubtitleMimeType);
  }

  private String[] buildSubtitleLanguages() {
    if (TextUtils.isEmpty(selectedSubtitleUrl)) {
      return DEMO_SUBTITLE_LANGUAGES;
    }
    return append(DEMO_SUBTITLE_LANGUAGES, selectedSubtitleLanguage);
  }

  private String[] buildSubtitleLabels() {
    if (TextUtils.isEmpty(selectedSubtitleUrl)) {
      return DEMO_SUBTITLE_LABELS;
    }
    return append(DEMO_SUBTITLE_LABELS, selectedSubtitleLabel);
  }

  private String[] append(String[] values, String value) {
    String[] result = new String[values.length + 1];
    System.arraycopy(values, 0, result, 0, values.length);
    result[values.length] = value;
    return result;
  }

  private void clearSelectedSubtitle() {
    selectedSubtitleUrl = null;
    selectedSubtitleMimeType = null;
    selectedSubtitleLanguage = null;
    selectedSubtitleLabel = null;
  }

  private void seekRelative(long offsetMs) {
    long durationMs = nativeGetDuration(nativePlayerHandle);
    long positionMs = nativeGetCurrentPosition(nativePlayerHandle);
    long targetMs = Math.max(0L, positionMs + offsetMs);
    if (isDurationKnown(durationMs)) {
      targetMs = Math.min(durationMs, targetMs);
    }
    nativeSeekTo(nativePlayerHandle, targetMs);
    updatePlaybackProgress();
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
        currentMediaUrl = null;
        clearSelectedSubtitle();
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

    currentMediaUrl = mediaUrl;
    currentSourceType = sourceType;
    currentMimeType = resolvedMimeType;
    if (TextUtils.isEmpty(subtitleUrl)) {
      clearSelectedSubtitle();
    } else {
      selectedSubtitleUrl = subtitleUrl;
      selectedSubtitleMimeType = "text/vtt";
      selectedSubtitleLanguage = "en";
      selectedSubtitleLabel = getString(R.string.external_subtitle_label);
    }
    nativeLoadMediaWithSubtitles(
        nativePlayerHandle,
        currentMediaUrl,
        currentSourceType,
        currentMimeType,
        buildSubtitleUrls(),
        buildSubtitleMimeTypes(),
        buildSubtitleLanguages(),
        buildSubtitleLabels());
    if (autoPlay) {
      nativePlay(nativePlayerHandle);
    }
    currentSourceName = sourceTypeToLabel(sourceType);
    updateCurrentSourceLabel();
    updatePlaybackProgress();
    return true;
  }

  private void updatePlaybackProgress() {
    if (nativePlayerHandle == 0L) {
      playbackTimeText.setText(R.string.time_unset);
      playPauseButton.setText(R.string.play);
      return;
    }

    long positionMs = Math.max(0L, nativeGetCurrentPosition(nativePlayerHandle));
    long durationMs = nativeGetDuration(nativePlayerHandle);
    boolean durationKnown = isDurationKnown(durationMs);
    playbackSeekBar.setEnabled(durationKnown);
    if (durationKnown) {
      int max = (int) Math.min(durationMs, Integer.MAX_VALUE);
      if (playbackSeekBar.getMax() != max) {
        playbackSeekBar.setMax(max);
      }
      if (!isUserSeeking) {
        playbackSeekBar.setProgress((int) Math.min(positionMs, max));
      }
      playbackTimeText.setText(formatTime(positionMs) + " / " + formatTime(durationMs));
    } else {
      if (!isUserSeeking) {
        playbackSeekBar.setProgress(0);
      }
      playbackTimeText.setText(formatTime(positionMs) + " / --:--");
    }
    playPauseButton.setText(nativeIsPlaying(nativePlayerHandle) ? R.string.pause : R.string.play);
  }

  private boolean isDurationKnown(long durationMs) {
    return durationMs > 0L && durationMs != TIME_UNSET && durationMs <= Integer.MAX_VALUE;
  }

  private String formatTime(long timeMs) {
    if (timeMs <= 0L || timeMs == TIME_UNSET) {
      return "0:00";
    }
    long totalSeconds = timeMs / 1000L;
    long seconds = totalSeconds % 60L;
    long minutes = (totalSeconds / 60L) % 60L;
    long hours = totalSeconds / 3600L;
    if (hours > 0L) {
      return String.format(Locale.US, "%d:%02d:%02d", hours, minutes, seconds);
    }
    return String.format(Locale.US, "%d:%02d", minutes, seconds);
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

  private String resolveSubtitleMimeType(Uri uri) {
    if ("content".equalsIgnoreCase(uri.getScheme())) {
      String contentMimeType = getContentResolver().getType(uri);
      if (!TextUtils.isEmpty(contentMimeType)
          && !TextUtils.equals(contentMimeType, "application/octet-stream")) {
        return contentMimeType;
      }
    }
    String path = String.valueOf(uri.getLastPathSegment()).toLowerCase(Locale.US);
    if (path.endsWith(".srt")) {
      return "application/x-subrip";
    }
    if (path.endsWith(".ttml") || path.endsWith(".dfxp") || path.endsWith(".xml")) {
      return "application/ttml+xml";
    }
    if (path.endsWith(".ass") || path.endsWith(".ssa")) {
      return "text/x-ssa";
    }
    return "text/vtt";
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

  private void showTrackMessage(String nativeSummary, int fallbackTextId) {
    if (TextUtils.isEmpty(nativeSummary)) {
      showTransientMessage(getString(fallbackTextId));
    } else {
      showTransientMessage(nativeSummary);
    }
  }

  private int dp(int value) {
    return (int) (value * getResources().getDisplayMetrics().density + 0.5f);
  }

  private void enterImmersiveMode() {
    getWindow()
        .getDecorView()
        .setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
  }

  private native long nativeCreatePlayer(android.content.Context context, PlayerView playerView);

  private native String nativeLoadMedia(
      long nativeHandle, String url, int sourceType, String mimeType);

  private native String nativeLoadMediaWithSubtitle(
      long nativeHandle, String mediaUrl, int sourceType, String mimeType, String subtitleUrl);

  private native String nativeLoadMediaWithSubtitles(
      long nativeHandle,
      String mediaUrl,
      int sourceType,
      String mimeType,
      String[] subtitleUrls,
      String[] subtitleMimeTypes,
      String[] subtitleLanguages,
      String[] subtitleLabels);

  private native String nativeLoadDemoPlaylist(long nativeHandle);

  private native void nativePlay(long nativeHandle);

  private native void nativePause(long nativeHandle);

  private native void nativeStop(long nativeHandle);

  private native void nativeSeekTo(long nativeHandle, long positionMs);

  private native void nativeSetPlaybackSpeed(long nativeHandle, float speed);

  private native void nativePreferTextLanguage(long nativeHandle, String language);

  private native String nativeCycleAudioTrack(long nativeHandle);

  private native String nativeCycleTextTrack(long nativeHandle);

  private native long nativeGetCurrentPosition(long nativeHandle);

  private native long nativeGetDuration(long nativeHandle);

  private native boolean nativeIsPlaying(long nativeHandle);

  private native void nativeRelease(long nativeHandle, PlayerView playerView);
}
