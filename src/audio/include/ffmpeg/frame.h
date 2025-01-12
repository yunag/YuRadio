#ifndef FFMPEG_FRAME_H
#define FFMPEG_FRAME_H

#include "ffmpeg/audio_format.h"

struct AVFrame;

namespace ffmpeg {

class frame {
public:
  frame();
  frame(const frame &);
  frame(frame &&) noexcept;
  frame &operator=(const frame &);
  frame &operator=(frame &&) noexcept;
  ~frame();

  AVFrame *avframe();
  const AVFrame *avframe() const;

  void unref();
  ffmpeg::audio_format audio_format() const;

protected:
  void swap(frame &other) noexcept;

private:
  AVFrame *m_frame;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_FRAME_H */
