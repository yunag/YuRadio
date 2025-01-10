#ifndef FFMPEG_MUXER_H
#define FFMPEG_MUXER_H

#include <system_error>

#include "ffmpeg/frame.h"
#include "ffmpeg/pimpl.h"
#include "ffmpeg/types.h"

namespace ffmpeg {

class muxer_private;

class muxer {
public:
  muxer();
  ~muxer();
  muxer(const muxer &) = delete;
  muxer(muxer &&) = delete;
  muxer &operator=(const muxer &) = delete;
  muxer &operator=(muxer &&) = delete;

  /**
   * @brief Set output bitrate
   *
   * @param bit_rate bitrate in bit/s
   */
  void set_output_bitrate(int bit_rate);
  void set_preferred_output_sample_rate(int sample_rate);
  void set_output_channel_count(int channel_count);

  void set_input_format(const audio_format &input_format);
  audio_format input_format() const;

  std::error_code open(const char *filename);
  std::error_code write(const ffmpeg::frame &frame);
  std::error_code write_header(const ffmpeg::metadata_map &metadata = {});
  std::error_code write_trailer();

  const char *filename() const;

  bool opened() const;
  void close();

private:
  ffmpeg::pimpl<muxer_private> d;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_MUXER_H */
