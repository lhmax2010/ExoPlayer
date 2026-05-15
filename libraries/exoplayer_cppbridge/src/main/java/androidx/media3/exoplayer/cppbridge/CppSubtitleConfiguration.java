package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Subtitle descriptor used by the native bridge. */
public final class CppSubtitleConfiguration {

  public final String uri;
  @Nullable public final String mimeType;
  @Nullable public final String language;
  @Nullable public final String label;
  @Nullable public final String id;
  public final int selectionFlags;
  public final int roleFlags;

  public CppSubtitleConfiguration(
      String uri,
      @Nullable String mimeType,
      @Nullable String language,
      @Nullable String label,
      @Nullable String id,
      int selectionFlags,
      int roleFlags) {
    this.uri = uri;
    this.mimeType = mimeType;
    this.language = language;
    this.label = label;
    this.id = id;
    this.selectionFlags = selectionFlags;
    this.roleFlags = roleFlags;
  }
}
