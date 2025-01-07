#ifndef FFMPEG_DECODER_H
#define FFMPEG_DECODER_H

#include <system_error>

#include "ffmpeg/frame.h"
#include "ffmpeg/packet.h"
#include "ffmpeg/pimpl.h"

struct AVStream;
struct AVCodecContext;

namespace ffmpeg {

class decoder_private;

class decoder {
public:
  decoder();
  decoder(const decoder &) = delete;
  decoder(decoder &&) = delete;
  decoder &operator=(const decoder &) = delete;
  decoder &operator=(decoder &&) = delete;
  ~decoder();

  std::error_code send(const ffmpeg::packet &packet);
  std::error_code open(const AVStream *stream);
  std::error_code receive(ffmpeg::frame &frame);

  AVCodecContext *av_decoder();
  const AVCodecContext *av_decoder() const;

  void close();
  bool opened() const;

private:
  ffmpeg::pimpl<decoder_private> d;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_DECODER_H */
