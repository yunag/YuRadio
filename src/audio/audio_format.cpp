#include "ffmpeg/audio_format.h"
#include "ffmpeg/audio_format_p.h"

namespace ffmpeg {

audio_format::audio_format() = default;

bool audio_format::valid() const {
  return sample_format != sample_format::unknown && sample_rate > 0 &&
         channel_count > 0;
}

bool audio_format::operator==(const audio_format &rhs) const {
  return sample_format == rhs.sample_format && sample_rate == rhs.sample_rate &&
         channel_count == rhs.channel_count;
}

int audio_format::bytes_per_sample() const {
  return av_get_bytes_per_sample(get_av_sample_format(sample_format));
}

}  // namespace ffmpeg
