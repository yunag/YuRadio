#include "ffmpeg/demuxer.h"
#include "ffmpeg/audio_format_p.h"
#include "ffmpeg/decoder.h"
#include "ffmpeg/error.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <atomic>
#include <cassert>
#include <mutex>

#define FFMPEG_PLAYER_DEBUG

namespace ffmpeg {

class demuxer_private {
public:
  AVFormatContext *ctx = nullptr;
  ffmpeg::decoder decoder;
  int audio_stream_index;

  mutable std::mutex ctx_mutex;
  mutable std::mutex decoder_mutex;
  std::atomic_bool abort = false;
};

demuxer::demuxer() : d(make_pimpl<demuxer_private>()) {}

demuxer::~demuxer() {
  close();
};

void demuxer::close() {
  std::unique_lock locker(d->ctx_mutex);
  avformat_close_input(&d->ctx);
  locker.unlock();

  d->abort = false;
}

void demuxer::abort() {
  d->abort = true;
}

std::error_code demuxer::open(const char *url) {
  std::unique_lock locker(d->ctx_mutex);
  /* Context must be closed */
  assert(d->ctx == nullptr);

  d->ctx = avformat_alloc_context();
  d->ctx->interrupt_callback.opaque = d.get();
  d->ctx->interrupt_callback.callback = [](void *opaque) -> int {
    const auto *demuxer = reinterpret_cast<const demuxer_private *>(opaque);
    return demuxer ? static_cast<int>(demuxer->abort) : 0;
  };

  int ret = avformat_open_input(&d->ctx, url, nullptr, nullptr);
  if (ret < 0) {
    return from_av_error_code(ret);
  }

  ret = avformat_find_stream_info(d->ctx, nullptr);
  if (ret < 0) {
    return from_av_error_code(ret);
  }

  d->audio_stream_index =
    av_find_best_stream(d->ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
  if (d->audio_stream_index < 0) {
    /* Could not find an audio stream */
    return errc::stream_not_found;
  }

  AVStream *stream = d->ctx->streams[d->audio_stream_index];

  /* Discard non audio streams */
  /* TODO: remove if video will be implemented */
  for (decltype(d->ctx->nb_streams) i = 0; i < d->ctx->nb_streams; ++i) {
    d->ctx->streams[i]->discard = AVDISCARD_ALL;
  }
  stream->discard = AVDISCARD_DEFAULT;

#ifdef FFMPEG_PLAYER_DEBUG
  /* Dump useful information about loaded media */
  av_dump_format(d->ctx, -1, url, 0);
#endif /* FFMPEG_PLAYER_DEBUG */

  locker.unlock();

  const std::lock_guard decoder_locker(d->decoder_mutex);
  return d->decoder.open(stream);
}

std::error_code demuxer::read(ffmpeg::packet &packet) {
  AVPacket *avpacket = packet.avpacket();

  const std::lock_guard locker(d->ctx_mutex);

  const int ret = av_read_frame(d->ctx, avpacket);
  if (ret < 0) {
    if (ret == AVERROR_EOF || avio_feof(d->ctx->pb)) {
      return errc::eof;
    }

    return from_av_error_code(ret);
  }

  AVStream *stream = d->ctx->streams[d->audio_stream_index];
  avpacket->time_base = stream->time_base;

  return errc::ok;
}

std::error_code demuxer::decode(const ffmpeg::packet &packet,
                                ffmpeg::frame &frame) {
  const std::lock_guard locker(d->decoder_mutex);

  std::error_code ec;
  for (;;) {
    ec = d->decoder.receive(frame);
    if (!ec || ec == errc::eof) {
      return ec;
    }

    if (ec == errc::eagain) {
      break;
    }
  }

  ec = d->decoder.send(packet);
  assert(ec != errc::eagain);

  if (!ec) {
    /* Error code to tell that caller need to send new packet */
    return errc::eagain;
  }

  return ec;
}

ffmpeg::metadata_map demuxer::metadata() const {
  const std::lock_guard locker(d->ctx_mutex);

  assert(d->ctx != nullptr);

  metadata_map meta;

  const AVDictionaryEntry *entry = nullptr;
  while ((entry = av_dict_iterate(d->ctx->metadata, entry))) {
    meta[entry->key] = entry->value;
  }

  return meta;
}

std::optional<std::string> demuxer::metadata(const std::string &key) const {
  const std::lock_guard locker(d->ctx_mutex);

  assert(d->ctx != nullptr);

  const AVDictionaryEntry *entry =
    av_dict_get(d->ctx->metadata, key.c_str(), nullptr, AV_DICT_IGNORE_SUFFIX);
  if (!entry) {
    return {};
  }

  return entry->value;
}

audio_format demuxer::output_format() const {
  const std::lock_guard locker(d->ctx_mutex);

  const AVCodecContext *codec_ctx = d->decoder.av_decoder();
  assert(codec_ctx != nullptr);

  audio_format format;
  format.sample_rate = codec_ctx->sample_rate;
  format.channel_count = codec_ctx->ch_layout.nb_channels;
  format.sample_format = get_sample_format(codec_ctx->sample_fmt);

  return format;
}

}  // namespace ffmpeg
