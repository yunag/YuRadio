#ifndef RADIOPLAYER_H
#define RADIOPLAYER_H

#include <QtQml>

#include "audiostreamrecorder.h"

class PlatformRadioController;

struct MediaItem {
  Q_GADGET
  Q_PROPERTY(QUrl source MEMBER source)
  Q_PROPERTY(QString author MEMBER author)
  Q_PROPERTY(QUrl artworkUri MEMBER artworkUri)
  QML_VALUE_TYPE(mediaitem)

public:
  MediaItem() = default;

  bool operator==(const MediaItem &other) const {
    return std::tie(source, author, artworkUri) ==
           std::tie(other.source, other.author, other.artworkUri);
  }

  bool operator!=(const MediaItem &other) const { return !(other == *this); }

  QUrl source;
  QString author;
  QUrl artworkUri;
};

class RadioPlayer : public QObject {
  Q_OBJECT
  Q_PROPERTY(
    QString streamTitle READ streamTitle NOTIFY streamTitleChanged FINAL)
  Q_PROPERTY(MediaItem mediaItem READ mediaItem WRITE setMediaItem NOTIFY
               mediaItemChanged FINAL)
  Q_PROPERTY(
    qreal volume READ volume WRITE setVolume NOTIFY volumeChanged FINAL)
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged FINAL)
  Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
  Q_PROPERTY(
    PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
  Q_PROPERTY(MediaStatus mediaStatus READ mediaStatus NOTIFY mediaStatusChanged)
  Q_PROPERTY(Error error READ error NOTIFY errorChanged)
  Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
  Q_PROPERTY(AudioStreamRecorder *audioStreamRecorder READ audioStreamRecorder
               WRITE setAudioStreamRecorder NOTIFY audioStreamRecorderChanged)
  Q_PROPERTY(bool stopOnPause READ stopOnPause WRITE setStopOnPause NOTIFY
               stopOnPauseChanged)
  QML_ELEMENT

public:
  explicit RadioPlayer(QObject *parent = nullptr);

  enum PlaybackState {
    PlayingState = 0,
    PausedState,
    StoppedState,
  };
  Q_ENUM(PlaybackState)

  enum Error {
    NoError = 0,
    ResourceError,
    FormatError,
    NetworkError,
    AccessDeniedError,
  };
  Q_ENUM(Error)

  enum MediaStatus {
    NoMedia = 0,
    LoadingMedia,
    LoadedMedia,
    InvalidMedia,
    EndOfFile,
  };
  Q_ENUM(MediaStatus)

public slots:
  void play();
  void pause();
  void stop();

  /* Useful when user wants to stop player with `MediaPlayerService` */
  void setStopOnPause(bool stop);
  bool stopOnPause() const;

  static MediaItem constructMediaItem();

  MediaItem mediaItem() const;
  void setMediaItem(const MediaItem &mediaItem);

  QString streamTitle() const;
  bool isLoading() const;

  PlaybackState playbackState() const;
  MediaStatus mediaStatus() const;

  Error error() const;
  QString errorString() const;
  bool isPlaying() const;

  qreal volume() const;
  void setVolume(qreal newVolume);

  AudioStreamRecorder *audioStreamRecorder() const;
  void setAudioStreamRecorder(AudioStreamRecorder *recorder);

signals:
  void audioStreamRecorderChanged();
  void streamTitleChanged();
  void icecastHintChanged();
  void loadingChanged();
  void playbackStateChanged();
  void errorChanged();
  void playingChanged();
  void audioOutputChanged();
  void volumeChanged();
  void mediaItemChanged();
  void mediaStatusChanged();
  void stopOnPauseChanged();

private:
  PlatformRadioController *m_controller;
  bool m_stopOnPause;
};

#endif /* !RADIOPLAYER_H */
