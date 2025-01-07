#ifndef FFMPEGRADIOCONTROLLER_H
#define FFMPEGRADIOCONTROLLER_H

#include "ffmpeg/pimpl.h"
#include "platformradiocontroller.h"

class FFmpegRadioControllerPrivate;
class PlayerListener;

class FFmpegRadioController : public PlatformRadioController {
  Q_OBJECT

public:
  explicit FFmpegRadioController(QObject *parent = nullptr);
  ~FFmpegRadioController() override;

  void play() override;
  void stop() override;
  void pause() override;

  void setVolume(qreal volume) override;
  void setMediaItem(const MediaItem &mediaItem) override;

private:
  friend PlayerListener;
  ffmpeg::pimpl<FFmpegRadioControllerPrivate> d;
};

#endif /* !FFMPEGRADIOCONTROLLER_H */
