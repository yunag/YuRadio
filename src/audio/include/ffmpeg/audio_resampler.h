#ifndef FFMPEG_AUDIO_RESAMPLER_H
#define FFMPEG_AUDIO_RESAMPLER_H

#include "frame.h"
#include "pimpl.h"

#include <system_error>
#include <vector>

namespace ffmpeg {

class audio_resampler_private;

class audio_buffer {
public:
  audio_buffer() = default;

  audio_buffer(audio_format format) { reset(format); }

  void reserve(std::size_t nbytes) { m_buf.reserve(nbytes); }

  void reset(audio_format format) {
    m_buf = {};
    m_audio_data = {};
    m_format = format;
    m_samples_count = 0;

    if (format.planar()) {
      m_audio_data.resize(static_cast<std::size_t>(format.channel_count));
    } else {
      m_audio_data.resize(1);
    }
  }

  uint8_t *data() { return m_buf.data(); }

  const uint8_t *data() const { return m_buf.data(); }

  const uint8_t *const *data_pointers() const { return m_audio_data.data(); }

  uint8_t **data_pointers() { return m_audio_data.data(); }

  void set_samples_count(int nsamples) { m_samples_count = nsamples; }

  int samples_count() const { return m_samples_count; }

  audio_format format() const { return m_format; }

private:
  std::vector<uint8_t> m_buf;
  std::vector<uint8_t *> m_audio_data;
  audio_format m_format;

  int m_samples_count = 0;
};

class audio_resampler {
public:
  audio_resampler();
  audio_resampler(const audio_resampler &) = delete;
  audio_resampler(audio_resampler &&) = delete;
  audio_resampler &operator=(const audio_resampler &) = delete;
  audio_resampler &operator=(audio_resampler &&) = delete;
  ~audio_resampler();

  /**
   * @brief Convert frame to data to desired format
   *
   * @param frame Frame
   * @param buffer Audio buffer to convert to
   */
  std::error_code convert(const frame &frame, audio_buffer &buffer);

private:
  ffmpeg::pimpl<audio_resampler_private> d;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_AUDIO_RESAMPLER_H */
