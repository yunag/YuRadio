#include <QLoggingCategory>
Q_LOGGING_CATEGORY(radioPlayerLog, "YuRadio.RadioPlayer")

#include "radioplayer.h"

#include "ffmpegradiocontroller.h"

#include "platformradiocontroller.h"

RadioPlayer::RadioPlayer(QObject *parent)
    : QObject(parent), m_controller(new FFmpegRadioController(this)) {

#ifdef QT_DEBUG
  connect(m_controller, &PlatformRadioController::mediaStatusChanged, this,
          [this]() {
    qCDebug(radioPlayerLog) << "MediaStatus:" << m_controller->mediaStatus();
  });
  connect(m_controller, &PlatformRadioController::errorChanged, this, [this]() {
    qCDebug(radioPlayerLog).nospace() << "Error: " << m_controller->error()
                                      << ":" << m_controller->errorString();
  });
  connect(m_controller, &PlatformRadioController::playbackStateChanged, this,
          [this]() {
    qCDebug(radioPlayerLog)
      << "PlaybackState:" << m_controller->playbackState();
  });
#endif

  connect(m_controller, &PlatformRadioController::audioStreamRecorderChanged,
          this, &RadioPlayer::audioStreamRecorderChanged);
  connect(m_controller, &PlatformRadioController::volumeChanged, this,
          &RadioPlayer::volumeChanged);
  connect(m_controller, &PlatformRadioController::mediaItemChanged, this,
          &RadioPlayer::mediaItemChanged);
  connect(m_controller, &PlatformRadioController::mediaStatusChanged, this,
          &RadioPlayer::mediaStatusChanged);
  connect(m_controller, &PlatformRadioController::playbackStateChanged, this,
          &RadioPlayer::playbackStateChanged);
  connect(m_controller, &PlatformRadioController::isPlayingChanged, this,
          &RadioPlayer::playingChanged);
  connect(m_controller, &PlatformRadioController::isLoadingChanged, this,
          &RadioPlayer::loadingChanged);
  connect(m_controller, &PlatformRadioController::errorChanged, this,
          &RadioPlayer::errorChanged);
  connect(m_controller, &PlatformRadioController::streamTitleChanged, this,
          &RadioPlayer::streamTitleChanged);
}

void RadioPlayer::play() {
  if (m_controller->canPlay()) {
    m_controller->play();
  }
}

void RadioPlayer::pause() {
  if (m_stopOnPause) {
    m_controller->stop();
  } else {
    m_controller->pause();
  }
}

void RadioPlayer::stop() {
  m_controller->stop();
}

RadioPlayer::PlaybackState RadioPlayer::playbackState() const {
  return m_controller->playbackState();
}

RadioPlayer::MediaStatus RadioPlayer::mediaStatus() const {
  return m_controller->mediaStatus();
}

RadioPlayer::Error RadioPlayer::error() const {
  return m_controller->error();
}

QString RadioPlayer::errorString() const {
  return m_controller->errorString();
}

bool RadioPlayer::isPlaying() const {
  return m_controller->isPlaying();
}

QString RadioPlayer::streamTitle() const {
  return m_controller->streamTitle();
}

bool RadioPlayer::isLoading() const {
  return m_controller->isLoading();
}

qreal RadioPlayer::volume() const {
  return m_controller->volume();
}

void RadioPlayer::setVolume(qreal volume) {
  m_controller->setVolume(volume);
}

MediaItem RadioPlayer::mediaItem() const {
  return m_controller->mediaItem();
}

void RadioPlayer::setMediaItem(const MediaItem &mediaItem) {
  m_controller->setMediaItem(mediaItem);
}

MediaItem RadioPlayer::constructMediaItem() {
  return {};
}

AudioStreamRecorder *RadioPlayer::audioStreamRecorder() const {
  return m_controller->audioStreamRecorder();
}

void RadioPlayer::setAudioStreamRecorder(AudioStreamRecorder *recorder) {
  m_controller->setAudioStreamRecorder(recorder);
}

void RadioPlayer::setStopOnPause(bool stop) {
  if (m_stopOnPause != stop) {
    m_stopOnPause = stop;
    emit stopOnPauseChanged();
  }
}

bool RadioPlayer::stopOnPause() const {
  return m_stopOnPause;
}
