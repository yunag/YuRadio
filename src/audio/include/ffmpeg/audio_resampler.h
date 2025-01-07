#ifndef FFMPEG_AUDIO_RESAMPLER
#define FFMPEG_AUDIO_RESAMPLER

#include "frame.h"
#include "maybe.h"
#include "pimpl.h"

namespace ffmpeg {

class audio_resampler_private;

struct audio_buffer {
  audio_buffer(const uint8_t *buf, int nb_samples)
      : m_buf(buf), m_nb_samples(nb_samples) {}

  const std::uint8_t *data() const { return m_buf; }

  int nb_samples() const { return m_nb_samples; }

private:
  const std::uint8_t *m_buf;
  int m_nb_samples;
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
