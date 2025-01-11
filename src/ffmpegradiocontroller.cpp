#include <QLoggingCategory>
Q_LOGGING_CATEGORY(ffmpegRadioControllerLog, "YuRadio.FFmpegRadioControllerLog")

#include "ffmpegradiocontroller.h"

#include "ffmpeg/player.h"

namespace {

RadioPlayer::PlaybackState
getRadioPlayerPlaybackState(ffmpeg::player::state state) {
  switch (state) {
    case ffmpeg::player::state::playing:
      return RadioPlayer::PlayingState;
    case ffmpeg::player::state::stopped:
      return RadioPlayer::StoppedState;
    case ffmpeg::player::state::paused:
      return RadioPlayer::PausedState;
  }

  Q_UNREACHABLE();
  return RadioPlayer::StoppedState;
}

RadioPlayer::MediaStatus
getRadioPlayerMediaStatus(ffmpeg::player::media_status status) {
  switch (status) {
    case ffmpeg::player::media_status::nomedia:
      return RadioPlayer::NoMedia;
    case ffmpeg::player::media_status::invalid:
      return RadioPlayer::InvalidMedia;
    case ffmpeg::player::media_status::loading:
      return RadioPlayer::LoadingMedia;
    case ffmpeg::player::media_status::loaded:
      return RadioPlayer::LoadedMedia;
    case ffmpeg::player::media_status::end_of_file:
      return RadioPlayer::EndOfFile;
  }

  Q_UNREACHABLE();
  return RadioPlayer::NoMedia;
}

RadioPlayer::Error getRadioPlayerError(ffmpeg::player::error error) {
  switch (error) {
    case ffmpeg::player::error::noerror:
      return RadioPlayer::NoError;
    case ffmpeg::player::error::resource:
      return RadioPlayer::ResourceError;
    case ffmpeg::player::error::network:
      return RadioPlayer::NetworkError;
  }

  Q_UNREACHABLE();
  return RadioPlayer::NoError;
}

}  // namespace

class FFmpegRadioControllerPrivate {
public:
  std::shared_ptr<PlayerListener> listener;
  ffmpeg::player player;
};

class PlayerListener : public ffmpeg::player_listener {
public:
  PlayerListener(FFmpegRadioController *controller)
      : m_controller(controller) {}

  void on_metadata_changed(const ffmpeg::metadata_map &metadata) override {
    if (metadata.contains("StreamTitle")) {
      QMetaObject::invokeMethod(m_controller, [this, m = metadata]() {
        m_controller->setStreamTitle(
          QString::fromStdString(m.at("StreamTitle")));
      }, Qt::QueuedConnection);
    }
  }

  void on_state_changed(ffmpeg::player::state state) override {
    QMetaObject::invokeMethod(m_controller, [this, state]() {
      m_controller->setPlaybackState(getRadioPlayerPlaybackState(state));
    }, Qt::QueuedConnection);
  }

  void on_media_status_changed(ffmpeg::player::media_status status) override {
    QMetaObject::invokeMethod(m_controller, [this, status]() {
      if (status == ffmpeg::player::media_status::end_of_file) {
        m_controller->stop();
      }

      m_controller->setMediaStatus(getRadioPlayerMediaStatus(status));
    }, Qt::QueuedConnection);
  }

  void on_error_changed(ffmpeg::player::error error,
                        const std::string &error_string) override {
    QMetaObject::invokeMethod(m_controller, [this, error, error_string]() {
      m_controller->setError(getRadioPlayerError(error),
                             QString::fromStdString(error_string));
      if (error != ffmpeg::player::error::noerror) {
        m_controller->stop();
      }
    }, Qt::QueuedConnection);
  }

  void on_frame_captured(const ffmpeg::frame &frame) override {
    QMetaObject::invokeMethod(m_controller, [this, f = frame]() {
      AudioStreamRecorder *recorder = m_controller->audioStreamRecorder();

      if (recorder->recording()) {
        RecorderSink *sink = recorder->recorderSink();
        sink->send(f, m_controller->streamTitle());
      }
    }, Qt::QueuedConnection);
  }

private:
  FFmpegRadioController *m_controller = nullptr;
};

FFmpegRadioController::FFmpegRadioController(QObject *parent)
    : PlatformRadioController(parent),
      d(ffmpeg::make_pimpl<FFmpegRadioControllerPrivate>()) {
  d->listener = std::make_shared<PlayerListener>(this);
  d->player.set_listener(d->listener);
}

FFmpegRadioController::~FFmpegRadioController() = default;

void FFmpegRadioController::play() {
  d->player.play();
}

void FFmpegRadioController::stop() {
  d->player.stop();
}

void FFmpegRadioController::pause() {
  d->player.pause();
}

void FFmpegRadioController::setVolume(qreal volume) {
  d->player.set_volume(volume);
  PlatformRadioController::setVolume(volume);
}

void FFmpegRadioController::setMediaItem(const MediaItem &mediaItem) {
  std::string source = mediaItem.source.toString().toStdString();

  d->player.set_source(source);
  PlatformRadioController::setMediaItem(mediaItem);
}
