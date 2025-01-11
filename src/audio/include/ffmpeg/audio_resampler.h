#ifndef FFMPEG_AUDIO_RESAMPLER
#define FFMPEG_AUDIO_RESAMPLER

#include "frame.h"
#include "maybe.h"
#include "pimpl.h"

namespace ffmpeg {

class audio_resampler_private;

struct audio_buffer {
  audio_buffer(const uint8_t *const *data_planes, int _nb_samples)
      : data(data_planes), nb_samples(_nb_samples) {}

  const std::uint8_t *const *data;
  int nb_samples;
};

class audio_resampler {
public:
  audio_resampler();
  audio_resampler(const audio_resampler &) = delete;
  audio_resampler(audio_resampler &&) = delete;
  audio_resampler &operator=(const audio_resampler &) = delete;
  audio_resampler &operator=(audio_resampler &&) = delete;
  ~audio_resampler();

  ffmpeg::maybe<audio_buffer> convert(const frame &frame,
                                      const audio_format &out_format);

private:
  ffmpeg::pimpl<audio_resampler_private> d;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_AUDIO_RESAMPLER */
