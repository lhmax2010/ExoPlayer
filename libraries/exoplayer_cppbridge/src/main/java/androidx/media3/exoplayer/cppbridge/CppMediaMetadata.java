package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;

/** Reduced media metadata descriptor used by the native bridge. */
public final class CppMediaMetadata {

  @Nullable public final String title;
  @Nullable public final String titleToken;
  public final CppObjectValue titleValue;
  @Nullable public final String artist;
  @Nullable public final String artistToken;
  public final CppObjectValue artistValue;
  @Nullable public final String albumTitle;
  @Nullable public final String albumTitleToken;
  public final CppObjectValue albumTitleValue;
  @Nullable public final String albumArtist;
  @Nullable public final String albumArtistToken;
  public final CppObjectValue albumArtistValue;
  @Nullable public final String displayTitle;
  @Nullable public final String displayTitleToken;
  public final CppObjectValue displayTitleValue;
  @Nullable public final String subtitle;
  @Nullable public final String subtitleToken;
  public final CppObjectValue subtitleValue;
  @Nullable public final String description;
  @Nullable public final String descriptionToken;
  public final CppObjectValue descriptionValue;
  @Nullable public final String artworkUri;
  @Nullable public final byte[] artworkData;
  public final int artworkDataType;
  public final long durationMs;
  public final int trackNumber;
  public final int totalTrackCount;
  public final int isBrowsable;
  public final int isPlayable;
  public final int folderType;
  public final int recordingYear;
  public final int recordingMonth;
  public final int recordingDay;
  public final int releaseYear;
  public final int releaseMonth;
  public final int releaseDay;
  @Nullable public final String writer;
  @Nullable public final String writerToken;
  public final CppObjectValue writerValue;
  @Nullable public final String author;
  @Nullable public final String authorToken;
  public final CppObjectValue authorValue;
  @Nullable public final String composer;
  @Nullable public final String composerToken;
  public final CppObjectValue composerValue;
  @Nullable public final String conductor;
  @Nullable public final String conductorToken;
  public final CppObjectValue conductorValue;
  public final int discNumber;
  public final int totalDiscCount;
  @Nullable public final String genre;
  @Nullable public final String genreToken;
  public final CppObjectValue genreValue;
  @Nullable public final String compilation;
  @Nullable public final String compilationToken;
  public final CppObjectValue compilationValue;
  public final int mediaType;
  @Nullable public final String station;
  @Nullable public final String stationToken;
  public final CppObjectValue stationValue;
  public final boolean extrasPresent;
  public final int extrasKeyCount;
  @Nullable public final String extrasToken;
  public final CppBundleValue[] extrasValues;

  public CppMediaMetadata(
      @Nullable String title,
      @Nullable String titleToken,
      @Nullable String artist,
      @Nullable String artistToken,
      @Nullable String albumTitle,
      @Nullable String albumTitleToken,
      @Nullable String albumArtist,
      @Nullable String albumArtistToken,
      @Nullable String displayTitle,
      @Nullable String displayTitleToken,
      @Nullable String subtitle,
      @Nullable String subtitleToken,
      @Nullable String description,
      @Nullable String descriptionToken,
      @Nullable String artworkUri,
      @Nullable byte[] artworkData,
      int artworkDataType,
      long durationMs,
      int trackNumber,
      int totalTrackCount,
      int isBrowsable,
      int isPlayable,
      int folderType,
      int recordingYear,
      int recordingMonth,
      int recordingDay,
      int releaseYear,
      int releaseMonth,
      int releaseDay,
      @Nullable String writer,
      @Nullable String writerToken,
      @Nullable String author,
      @Nullable String authorToken,
      @Nullable String composer,
      @Nullable String composerToken,
      @Nullable String conductor,
      @Nullable String conductorToken,
      int discNumber,
      int totalDiscCount,
      @Nullable String genre,
      @Nullable String genreToken,
      @Nullable String compilation,
      @Nullable String compilationToken,
      int mediaType,
      @Nullable String station,
      @Nullable String stationToken,
      boolean extrasPresent,
      int extrasKeyCount,
      @Nullable String extrasToken) {
    this(
        title,
        titleToken,
        artist,
        artistToken,
        albumTitle,
        albumTitleToken,
        albumArtist,
        albumArtistToken,
        displayTitle,
        displayTitleToken,
        subtitle,
        subtitleToken,
        description,
        descriptionToken,
        artworkUri,
        artworkData,
        artworkDataType,
        durationMs,
        trackNumber,
        totalTrackCount,
        isBrowsable,
        isPlayable,
        folderType,
        recordingYear,
        recordingMonth,
        recordingDay,
        releaseYear,
        releaseMonth,
        releaseDay,
        writer,
        writerToken,
        author,
        authorToken,
        composer,
        composerToken,
        conductor,
        conductorToken,
        discNumber,
        totalDiscCount,
        genre,
        genreToken,
        compilation,
        compilationToken,
        mediaType,
        station,
        stationToken,
        extrasPresent,
        extrasKeyCount,
        extrasToken,
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
        null);
  }

  public CppMediaMetadata(
      @Nullable String title,
      @Nullable String titleToken,
      @Nullable String artist,
      @Nullable String artistToken,
      @Nullable String albumTitle,
      @Nullable String albumTitleToken,
      @Nullable String albumArtist,
      @Nullable String albumArtistToken,
      @Nullable String displayTitle,
      @Nullable String displayTitleToken,
      @Nullable String subtitle,
      @Nullable String subtitleToken,
      @Nullable String description,
      @Nullable String descriptionToken,
      @Nullable String artworkUri,
      @Nullable byte[] artworkData,
      int artworkDataType,
      long durationMs,
      int trackNumber,
      int totalTrackCount,
      int isBrowsable,
      int isPlayable,
      int folderType,
      int recordingYear,
      int recordingMonth,
      int recordingDay,
      int releaseYear,
      int releaseMonth,
      int releaseDay,
      @Nullable String writer,
      @Nullable String writerToken,
      @Nullable String author,
      @Nullable String authorToken,
      @Nullable String composer,
      @Nullable String composerToken,
      @Nullable String conductor,
      @Nullable String conductorToken,
      int discNumber,
      int totalDiscCount,
      @Nullable String genre,
      @Nullable String genreToken,
      @Nullable String compilation,
      @Nullable String compilationToken,
      int mediaType,
      @Nullable String station,
      @Nullable String stationToken,
      boolean extrasPresent,
      int extrasKeyCount,
      @Nullable String extrasToken,
      @Nullable CppBundleValue[] extrasValues) {
    this(
        title,
        titleToken,
        artist,
        artistToken,
        albumTitle,
        albumTitleToken,
        albumArtist,
        albumArtistToken,
        displayTitle,
        displayTitleToken,
        subtitle,
        subtitleToken,
        description,
        descriptionToken,
        artworkUri,
        artworkData,
        artworkDataType,
        durationMs,
        trackNumber,
        totalTrackCount,
        isBrowsable,
        isPlayable,
        folderType,
        recordingYear,
        recordingMonth,
        recordingDay,
        releaseYear,
        releaseMonth,
        releaseDay,
        writer,
        writerToken,
        author,
        authorToken,
        composer,
        composerToken,
        conductor,
        conductorToken,
        discNumber,
        totalDiscCount,
        genre,
        genreToken,
        compilation,
        compilationToken,
        mediaType,
        station,
        stationToken,
        extrasPresent,
        extrasKeyCount,
        extrasToken,
        extrasValues,
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
        null);
  }

  public CppMediaMetadata(
      @Nullable String title,
      @Nullable String titleToken,
      @Nullable String artist,
      @Nullable String artistToken,
      @Nullable String albumTitle,
      @Nullable String albumTitleToken,
      @Nullable String albumArtist,
      @Nullable String albumArtistToken,
      @Nullable String displayTitle,
      @Nullable String displayTitleToken,
      @Nullable String subtitle,
      @Nullable String subtitleToken,
      @Nullable String description,
      @Nullable String descriptionToken,
      @Nullable String artworkUri,
      @Nullable byte[] artworkData,
      int artworkDataType,
      long durationMs,
      int trackNumber,
      int totalTrackCount,
      int isBrowsable,
      int isPlayable,
      int folderType,
      int recordingYear,
      int recordingMonth,
      int recordingDay,
      int releaseYear,
      int releaseMonth,
      int releaseDay,
      @Nullable String writer,
      @Nullable String writerToken,
      @Nullable String author,
      @Nullable String authorToken,
      @Nullable String composer,
      @Nullable String composerToken,
      @Nullable String conductor,
      @Nullable String conductorToken,
      int discNumber,
      int totalDiscCount,
      @Nullable String genre,
      @Nullable String genreToken,
      @Nullable String compilation,
      @Nullable String compilationToken,
      int mediaType,
      @Nullable String station,
      @Nullable String stationToken,
      boolean extrasPresent,
      int extrasKeyCount,
      @Nullable String extrasToken,
      @Nullable CppBundleValue[] extrasValues,
      @Nullable CppObjectValue titleValue,
      @Nullable CppObjectValue artistValue,
      @Nullable CppObjectValue albumTitleValue,
      @Nullable CppObjectValue albumArtistValue,
      @Nullable CppObjectValue displayTitleValue,
      @Nullable CppObjectValue subtitleValue,
      @Nullable CppObjectValue descriptionValue,
      @Nullable CppObjectValue writerValue,
      @Nullable CppObjectValue authorValue,
      @Nullable CppObjectValue composerValue,
      @Nullable CppObjectValue conductorValue,
      @Nullable CppObjectValue genreValue,
      @Nullable CppObjectValue compilationValue,
      @Nullable CppObjectValue stationValue) {
    this.title = title;
    this.titleToken = titleToken;
    this.titleValue = titleValue != null ? titleValue : CppObjectValue.nullValue();
    this.artist = artist;
    this.artistToken = artistToken;
    this.artistValue = artistValue != null ? artistValue : CppObjectValue.nullValue();
    this.albumTitle = albumTitle;
    this.albumTitleToken = albumTitleToken;
    this.albumTitleValue =
        albumTitleValue != null ? albumTitleValue : CppObjectValue.nullValue();
    this.albumArtist = albumArtist;
    this.albumArtistToken = albumArtistToken;
    this.albumArtistValue =
        albumArtistValue != null ? albumArtistValue : CppObjectValue.nullValue();
    this.displayTitle = displayTitle;
    this.displayTitleToken = displayTitleToken;
    this.displayTitleValue =
        displayTitleValue != null ? displayTitleValue : CppObjectValue.nullValue();
    this.subtitle = subtitle;
    this.subtitleToken = subtitleToken;
    this.subtitleValue = subtitleValue != null ? subtitleValue : CppObjectValue.nullValue();
    this.description = description;
    this.descriptionToken = descriptionToken;
    this.descriptionValue =
        descriptionValue != null ? descriptionValue : CppObjectValue.nullValue();
    this.artworkUri = artworkUri;
    this.artworkData = artworkData;
    this.artworkDataType = artworkDataType;
    this.durationMs = durationMs;
    this.trackNumber = trackNumber;
    this.totalTrackCount = totalTrackCount;
    this.isBrowsable = isBrowsable;
    this.isPlayable = isPlayable;
    this.folderType = folderType;
    this.recordingYear = recordingYear;
    this.recordingMonth = recordingMonth;
    this.recordingDay = recordingDay;
    this.releaseYear = releaseYear;
    this.releaseMonth = releaseMonth;
    this.releaseDay = releaseDay;
    this.writer = writer;
    this.writerToken = writerToken;
    this.writerValue = writerValue != null ? writerValue : CppObjectValue.nullValue();
    this.author = author;
    this.authorToken = authorToken;
    this.authorValue = authorValue != null ? authorValue : CppObjectValue.nullValue();
    this.composer = composer;
    this.composerToken = composerToken;
    this.composerValue = composerValue != null ? composerValue : CppObjectValue.nullValue();
    this.conductor = conductor;
    this.conductorToken = conductorToken;
    this.conductorValue =
        conductorValue != null ? conductorValue : CppObjectValue.nullValue();
    this.discNumber = discNumber;
    this.totalDiscCount = totalDiscCount;
    this.genre = genre;
    this.genreToken = genreToken;
    this.genreValue = genreValue != null ? genreValue : CppObjectValue.nullValue();
    this.compilation = compilation;
    this.compilationToken = compilationToken;
    this.compilationValue =
        compilationValue != null ? compilationValue : CppObjectValue.nullValue();
    this.mediaType = mediaType;
    this.station = station;
    this.stationToken = stationToken;
    this.stationValue = stationValue != null ? stationValue : CppObjectValue.nullValue();
    this.extrasPresent = extrasPresent;
    this.extrasKeyCount = extrasKeyCount;
    this.extrasToken = extrasToken;
    this.extrasValues = extrasValues != null ? extrasValues : new CppBundleValue[0];
  }
}
