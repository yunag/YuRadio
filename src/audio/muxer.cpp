#include "ffmpeg/muxer.h"
#include "ffmpeg/audio_format_p.h"
#include "ffmpeg/error.h"
#include "ffmpeg/packet.h"
#include "ffmpeg/scope_guard.h"

#include <algorithm>
#include <cassert>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>
}

namespace {

constexpr int default_output_bit_rate = 320000; /* Highest bitrate for MP3 */
constexpr int default_output_channel_count = 2;
constexpr int default_output_sample_rate = 44100; /* CD quality */

namespace details {

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(61, 30, 0)
template <typename T>
std::vector<T> supported_configs(AVCodecContext *codec_context,
                                 AVCodecConfig config) {
  const T *out_configs;
  int num_configs = 0;
  int ret = avcodec_get_supported_config(
    codec_context, nullptr, config, 0,
    reinterpret_cast<const void **>(&out_configs), &num_configs);
  if (ret < 0) {
    return {};
  }

  return std::vector(out_configs, out_configs + num_configs);
}
#endif

}  // namespace details

auto supported_sample_rates(AVCodecContext *codec_context) {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(61, 30, 0)
  return details::supported_configs<int>(codec_context,
                                         AV_CODEC_CONFIG_SAMPLE_RATE);
#else
  std::vector<int> sample_rates;

  const int *sample_rate = codec_context->codec->supported_samplerates;
  while (sample_rate && *sample_rate != 0) {
    sample_rates.push_back(*sample_rate);
  }
  return sample_rates;
#endif
}

auto supported_sample_formats(AVCodecContext *codec_context) {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(61, 30, 0)
  return details::supported_configs<AVSampleFormat>(
    codec_context, AV_CODEC_CONFIG_SAMPLE_FORMAT);
#else

  std::vector<AVSampleFormat> sample_formats;

  const AVSampleFormat *sample_format = codec_context->codec->sample_fmts;
  while (sample_format && *sample_format != -1) {
    sample_formats.push_back(*sample_format);
  }
  return sample_formats;
#endif
}

namespace algorithm {

template <typename T>
bool contains(const std::vector<T> &container, const T &value) {
  return std::ranges::any_of(container, [&value](auto &&container_value) {
    return value == container_value;
  });
}

}  // namespace algorithm

}  // namespace

namespace ffmpeg {

class muxer_private {
public:
  AVFormatContext *ctx = nullptr;
  AVCodecContext *codec_ctx = nullptr;

  AVAudioFifo *fifo = nullptr;
  SwrContext *resampler_ctx = nullptr;
  audio_format input_format;

  int output_bit_rate = default_output_bit_rate;
  int preferred_output_sample_rate = default_output_sample_rate;
  int output_channel_count = default_output_channel_count;

  std::int64_t pts = 0;

  std::error_code read_encode_and_write() {
    ffmpeg::frame temp_output_frame;
    AVFrame *output_frame = temp_output_frame.avframe();

    const int frame_size =
      std::min(av_audio_fifo_size(fifo), codec_ctx->frame_size);
    std::error_code ec = output_frame_init(temp_output_frame, frame_size);
    if (ec) {
      return ec;
    }

    int ret = av_audio_fifo_read(
      fifo, reinterpret_cast<void **>(output_frame->data), frame_size);
    if (ret < 0) {
      return from_av_error_code(ret);
    }
    if (ret < frame_size) {
      return errc::einval;
    }

    ec = encode_and_write_frame(output_frame);
    if (ec && ec != errc::eagain) {
      return ec;
    }

    return errc::ok;
  }

  std::error_code audio_resampler_init() {
    AVChannelLayout in_ch_layout;
    av_channel_layout_default(&in_ch_layout, input_format.channel_count);

    int ret = swr_alloc_set_opts2(
      &resampler_ctx, &codec_ctx->ch_layout, codec_ctx->sample_fmt,
      codec_ctx->sample_rate, &in_ch_layout,
      get_av_sample_format(input_format.sample_format),
      input_format.sample_rate, 0, nullptr);
    if (ret < 0) {
      return from_av_error_code(ret);
    }

    ret = swr_init(resampler_ctx);
    if (ret < 0) {
      return from_av_error_code(ret);
    }

    return errc::ok;
  }

  std::error_code encode_and_write_frame(AVFrame *output_frame) {
    ffmpeg::packet temp_packet;
    AVPacket *output_packet = temp_packet.avpacket();

    if (output_frame) {
      output_frame->pts = pts;
      pts += output_frame->nb_samples;
    }

    int ret = avcodec_send_frame(codec_ctx, output_frame);

    if (ret < 0 && ret != AVERROR_EOF) {
      return from_av_error_code(ret);
    }

    ret = avcodec_receive_packet(codec_ctx, output_packet);
    if (ret < 0) {
      return from_av_error_code(ret);
    }

    ret = av_write_frame(ctx, output_packet);
    if (ret < 0) {
      return from_av_error_code(ret);
    }

    return errc::ok;
  }

  std::error_code output_frame_init(ffmpeg::frame &frame,
                                    int frame_size) const {
    AVFrame *avframe = frame.avframe();

    avframe->nb_samples = frame_size;
    av_channel_layout_copy(&avframe->ch_layout, &codec_ctx->ch_layout);
    avframe->format = codec_ctx->sample_fmt;
    avframe->sample_rate = codec_ctx->sample_rate;

    int ret = av_frame_get_buffer(avframe, 0);
    return from_av_error_code(ret);
  }

  std::error_code open_impl(const char *filename) {
    assert(input_format.valid());

    ctx = avformat_alloc_context();
    if (!ctx) {
      return errc::enomem;
    }

    AVIOContext *io_ctx = nullptr;

    int ret = avio_open(&io_ctx, filename, AVIO_FLAG_WRITE);
    if (ret < 0) {
      return from_av_error_code(ret);
    }

    ctx->pb = io_ctx;
    ctx->oformat = av_guess_format(nullptr, filename, nullptr);

    if (!ctx->oformat) {
      return errc::invaliddata;
    }

    /* NOTE: will be freed by `avformat_free_context` */
    ctx->url = av_strdup(filename);

    /* TODO: We can also use AAC codec. So make it customizable */
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (!codec) {
      return errc::encoder_not_found;
    }
    assert(
      avformat_query_codec(ctx->oformat, codec->id, FF_COMPLIANCE_STRICT) == 1);

    AVStream *stream = avformat_new_stream(ctx, codec);
    if (!stream) {
      return errc::enomem;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
      return errc::enomem;
    }

    codec_ctx->codec = codec;

    av_channel_layout_default(&codec_ctx->ch_layout, output_channel_count);

    auto available_sample_rates = supported_sample_rates(codec_ctx);
    if (algorithm::contains(available_sample_rates,
                            preferred_output_sample_rate)) {
      codec_ctx->sample_rate = preferred_output_sample_rate;
    } else {
      codec_ctx->sample_rate = default_output_sample_rate;
      assert(algorithm::contains(available_sample_rates,
                                 default_output_sample_rate));
    }

    std::vector<AVSampleFormat> available_sample_formats =
      supported_sample_formats(codec_ctx);
    AVSampleFormat input_sample_format =
      get_av_sample_format(input_format.sample_format);

    if (algorithm::contains(available_sample_formats, input_sample_format)) {
      /* Prefer input sample format to avoid resampling */
      codec_ctx->sample_fmt = input_sample_format;
    } else {
      codec_ctx->sample_fmt = available_sample_formats.front();
    }
    codec_ctx->bit_rate = output_bit_rate;

    /* Set the sample rate for the container. */
    stream->time_base.den = codec_ctx->sample_rate;
    stream->time_base.num = 1;

    /* Some container formats (like MP4) require global headers to be present.
     * Mark the encoder so that it behaves accordingly. */
    if (ctx->oformat->flags & AVFMT_GLOBALHEADER) {
      codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
      return from_av_error_code(ret);
    }

    ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
    if (ret < 0) {
      return from_av_error_code(ret);
    }

    fifo = av_audio_fifo_alloc(codec_ctx->sample_fmt,
                               codec_ctx->ch_layout.nb_channels, 1);
    if (!fifo) {
      return errc::enomem;
    }

    std::error_code ec = audio_resampler_init();
    if (ec) {
      return ec;
    }

    return errc::ok;
  }

  std::error_code convert_frame(const ffmpeg::frame &frame) const {
    const AVFrame *input_frame = frame.avframe();
    uint8_t **converted_input_sampels = nullptr;

    /* TODO: reduce memory footprint */
    auto cleanup = make_scope_guard([&converted_input_sampels]() {
      if (converted_input_sampels) {
        av_freep(reinterpret_cast<void *>(&converted_input_sampels[0]));
      }
      av_freep(reinterpret_cast<void *>(&converted_input_sampels));
    });

    int64_t delay = swr_get_delay(resampler_ctx, codec_ctx->sample_rate);

    int out_nb_samples = static_cast<int>(
      av_rescale_rnd(delay + input_frame->nb_samples, codec_ctx->sample_rate,
                     input_frame->sample_rate, AV_ROUND_UP));

    int ret = av_samples_alloc_array_and_samples(
      &converted_input_sampels, nullptr, codec_ctx->ch_layout.nb_channels,
      out_nb_samples, codec_ctx->sample_fmt, 0);
    if (ret < 0) {
      return from_av_error_code(ret);
    }

    int nb_samples =
      swr_convert(resampler_ctx, converted_input_sampels, out_nb_samples,
                  input_frame->extended_data, input_frame->nb_samples);
    if (nb_samples < 0) {
      return errc::invaliddata;
    }

    ret = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + nb_samples);
    if (ret < 0) {
      return from_av_error_code(ret);
    }

    ret = av_audio_fifo_write(
      fifo, reinterpret_cast<void **>(converted_input_sampels), nb_samples);
    if (ret < nb_samples) {
      return errc::enomem;
    }

    return errc::ok;
  }
};

muxer::muxer() : d(ffmpeg::make_pimpl<muxer_private>()) {}

muxer::~muxer() {
  close();
}

std::error_code muxer::open(const char *filename) {
  if (opened()) {
    close();
  }

  std::error_code ec = d->open_impl(filename);
  if (ec) {
    close();
  }

  return ec;
}

void muxer::close() {
  if (d->codec_ctx) {
    avcodec_free_context(&d->codec_ctx);
  }
  if (d->ctx) {
    if (d->ctx->pb) {
      avio_closep(&d->ctx->pb);
    }
    avformat_free_context(d->ctx);
    d->ctx = nullptr;
  }
  if (d->resampler_ctx) {
    swr_free(&d->resampler_ctx);
  }
  if (d->fifo) {
    av_audio_fifo_free(d->fifo);
    d->fifo = nullptr;
  }
  d->pts = 0;
}

bool muxer::opened() const {
  return d->ctx;
}

std::error_code muxer::write_header(const metadata_map &metadata) {
  assert(opened());

  for (const auto &[key, value] : metadata) {
    av_dict_set(&d->ctx->metadata, key.c_str(), value.c_str(), 0);
  }

  const int ret = avformat_write_header(d->ctx, nullptr);
  return from_av_error_code(ret);
}

std::error_code muxer::write_trailer() {
  assert(opened());

  while (av_audio_fifo_size(d->fifo) > 0) {
    std::error_code ec = d->read_encode_and_write();
    if (ec) {
      return ec;
    }
  }

  /* Flush encoder */
  while (!d->encode_and_write_frame(nullptr)) {}

  const int ret = av_write_trailer(d->ctx);

  close();

  return from_av_error_code(ret);
}

std::error_code muxer::write(const ffmpeg::frame &frame) {
  assert(opened());
  assert(frame.audio_format() == d->input_format);

  std::error_code ec = d->convert_frame(frame);
  if (ec) {
    return ec;
  }

  while (av_audio_fifo_size(d->fifo) >= d->codec_ctx->frame_size) {
    ec = d->read_encode_and_write();
    if (ec) {
      return ec;
    }
  }

  return errc::ok;
}

void muxer::set_input_format(const audio_format &input_format) {
  assert(!opened());
  assert(input_format.valid());

  d->input_format = input_format;
}

void muxer::set_output_bitrate(int bit_rate) {
  assert(!opened());

  d->output_bit_rate = bit_rate;
}

void muxer::set_preferred_output_sample_rate(int sample_rate) {
  assert(!opened());

  d->preferred_output_sample_rate = sample_rate;
}

void muxer::set_output_channel_count(int channel_count) {
  assert(!opened());

  d->output_channel_count = channel_count;
}

const char *muxer::filename() const {
  assert(opened());

  return d->ctx->url;
}

audio_format muxer::input_format() const {
  return d->input_format;
}

}  // namespace ffmpeg
