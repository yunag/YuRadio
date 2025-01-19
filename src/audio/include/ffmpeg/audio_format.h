#ifndef FFMPEG_AUDIO_FORMAT_H
#define FFMPEG_AUDIO_FORMAT_H

#include <cstdint>

namespace ffmpeg {

enum class sample_format : std::uint8_t {
  unknown,

  u8,   ///< unsigned 8 bits
  s16,  ///< signed 16 bits
  s32,  ///< signed 32 bits
  f32,  ///< float
  f64,  ///< double

  u8p,   ///< unsigned 8 bits, planar
  s16p,  ///< signed 16 bits, planar
  s32p,  ///< signed 32 bits, planar
  f32p,  ///< float, planar
  f64p,  ///< double, planar
  s64,   ///< signed 64 bits
  s64p,  ///< signed 64 bits, planar
};

class audio_format {
public:
  audio_format();
  audio_format(const audio_format &) = default;
  audio_format(audio_format &&) = default;
  audio_format &operator=(const audio_format &) = default;
  audio_format &operator=(audio_format &&) = default;
  ~audio_format() = default;

  bool operator==(const audio_format &rhs) const;

  bool operator!=(const audio_format &rhs) const { return !operator==(rhs); }

  int bytes_per_sample() const;
  bool valid() const;
  bool planar() const;

public:
  ffmpeg::sample_format sample_format = sample_format::unknown;
  int sample_rate = 0;
  int channel_count = 0;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_AUDIO_FORMAT_H */
