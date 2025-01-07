#ifndef FFMPEG_AUDIO_DEVICE_H
#define FFMPEG_AUDIO_DEVICE_H

#include "ffmpeg/pimpl.h"

namespace ffmpeg {

class audio_device_private;

class audio_device {
public:
  audio_device();
  audio_device(const audio_device &) = delete;
  audio_device(audio_device &&) = delete;
  audio_device &operator=(const audio_device &) = delete;
  audio_device &operator=(audio_device &&) = delete;
  ~audio_device();

  bool is_null() const;

private:
  ffmpeg::pimpl<audio_device_private> d;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_AUDIO_DEVICE_H */
