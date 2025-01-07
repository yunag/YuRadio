#ifndef FFMPEG_AUDIO_OUTPUT_DEVICE_H
#define FFMPEG_AUDIO_OUTPUT_DEVICE_H

#include "ffmpeg/frame.h"
#include "ffmpeg/pimpl.h"

#include <QIODevice>

#include <system_error>

class AudioOutputDevicePrivate;

class AudioOutputDevice : public QIODevice {
public:
  explicit AudioOutputDevice(QObject *parent = nullptr);
  ~AudioOutputDevice() override;

  bool isSequential() const override { return true; }

  qint64 bytesAvailable() const override;
  qint64 size() const override;

  qint64 readData(char *data, qint64 maxlen) override;
  qint64 writeData(const char *data, qint64 maxlen) override;

  std::size_t samples_in_queue() const;

  void clear();

  void start();
  void stop();
  std::error_code push_frame(const ffmpeg::frame &frame,
                             const ffmpeg::audio_format &out_format);

private:
  ffmpeg::pimpl<AudioOutputDevicePrivate> d;
};

#endif /* !FFMPEG_AUDIO_OUTPUT_DEVICE_H */
