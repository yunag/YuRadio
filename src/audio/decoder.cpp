#include <QDebug>

#include "ffmpeg/decoder.h"
#include "ffmpeg/error.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace ffmpeg {

class decoder_private {
public:
  AVCodecContext *codec_ctx = nullptr;
};

decoder::decoder() : d(make_pimpl<decoder_private>()) {};

decoder::~decoder() {
  close();
}

std::error_code decoder::open(const AVStream *stream) {
  if (opened()) {
    close();
  }

  d->codec_ctx = avcodec_alloc_context3(nullptr);

  if (!stream) {
    return errc::stream_not_found;
  }

  const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);

  if (!codec) {
    return errc::decoder_not_found;
  }

  int ret = avcodec_parameters_to_context(d->codec_ctx, stream->codecpar);
  if (ret < 0) {
    return static_cast<errc>(ret);
  }

  ret = avcodec_open2(d->codec_ctx, codec, nullptr);
  if (ret < 0) {
    return static_cast<errc>(ret);
  }

  d->codec_ctx->codec_id = codec->id;
  d->codec_ctx->codec = codec;

  return errc::ok;
}

void decoder::close() {
  if (d->codec_ctx) {
    avcodec_free_context(&d->codec_ctx);
  }
}

std::error_code decoder::send(const ffmpeg::packet &packet) {
  assert(opened());

  const int ret = avcodec_send_packet(d->codec_ctx, packet.avpacket());
  return static_cast<errc>(ret);
}

std::error_code decoder::receive(ffmpeg::frame &frame) {
  assert(opened());

  const int ret = avcodec_receive_frame(d->codec_ctx, frame.avframe());
  return static_cast<errc>(ret);
}

bool decoder::opened() const {
  return d->codec_ctx;
}

AVCodecContext *decoder::av_decoder() {
  return d->codec_ctx;
}

const AVCodecContext *decoder::av_decoder() const {
  return d->codec_ctx;
}

}  // namespace ffmpeg
