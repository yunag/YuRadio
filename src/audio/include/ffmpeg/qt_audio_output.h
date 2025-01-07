#ifndef FFMPEG_AUDIO_OUTPUT_H
#define FFMPEG_AUDIO_OUTPUT_H

#include "ffmpeg/audio_device.h"
#include "ffmpeg/frame.h"
#include "ffmpeg/pimpl.h"

#include <system_error>

namespace ffmpeg {

class audio_output_private;

class audio_output {
public:
  enum class state : std::uint8_t {
    playing = 0,
    paused,
    stopped,
  };

  audio_output();
  audio_output(const audio_output &) = delete;
  audio_output(audio_output &&) = delete;
  audio_output &operator=(const audio_output &) = delete;
  audio_output &operator=(audio_output &&) = delete;
  ~audio_output();

  void set_volume(double volume);
  double volume() const;

  void set_input_format(ffmpeg::audio_format format);
  ffmpeg::audio_format output_format() const;

  void set_buffer_size(std::size_t size);
  std::size_t buffer_size() const;

  std::size_t bytes_in_queue() const;

  void set_audio_device(const ffmpeg::audio_device &device);

  std::error_code push_frame(const ffmpeg::frame &frame);
  void start();
  void stop();
  void pause();

private:
  ffmpeg::pimpl<audio_output_private> d;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_AUDIO_OUTPUT_H */
