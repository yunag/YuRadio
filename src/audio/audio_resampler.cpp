#include "ffmpeg/audio_resampler.h"
#include "ffmpeg/error.h"

#include "ffmpeg/audio_format_p.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include <cassert>

namespace ffmpeg {

class audio_resampler_private {
public:
  SwrContext *ctx = nullptr;

  uint8_t *audio_data[32] = {};
  uint8_t *audio_buf = nullptr;
  uint32_t audio_buf_size = 0;

  /* We need to store these variables to know when to switch the resampler context */
  AVChannelLayout out_ch_layout;
  AVSampleFormat out_sample_format = AV_SAMPLE_FMT_NONE;
  int out_sample_rate = -1;

  AVChannelLayout in_ch_layout;
  AVSampleFormat in_sample_format = AV_SAMPLE_FMT_NONE;
  int in_sample_rate = -1;
};

audio_resampler::audio_resampler() : d(make_pimpl<audio_resampler_private>()) {}

audio_resampler::~audio_resampler() {
  swr_free(&d->ctx);
  av_freep(reinterpret_cast<void *>(&d->audio_buf));
}

ffmpeg::maybe<audio_buffer>
audio_resampler::convert(const frame &frame, const audio_format &out_format) {
  const AVFrame *avframe = frame.avframe();

  const AVSampleFormat out_sample_format =
    get_av_sample_format(out_format.sample_format);

  const int out_sample_rate = out_format.sample_rate;

  AVChannelLayout out_ch_layout;
  av_channel_layout_default(&out_ch_layout, out_format.channel_count);

  /* Sanity check if number of channels less than num of data pointers */
  assert(out_ch_layout.nb_channels < AV_NUM_DATA_POINTERS);

  const AVChannelLayout ch_layout = avframe->ch_layout;

  const bool need_convert =
    avframe->format != out_sample_format ||
    avframe->sample_rate != out_sample_rate ||
    av_channel_layout_compare(&ch_layout, &out_ch_layout);

  if (need_convert) {
    const bool need_switch_ctx =
      d->out_sample_format != out_sample_format ||
      d->out_sample_rate != out_sample_rate ||
      av_channel_layout_compare(&d->out_ch_layout, &out_ch_layout) ||
      d->in_sample_format != avframe->format ||
      d->in_sample_rate != avframe->sample_rate ||
      av_channel_layout_compare(&d->in_ch_layout, &avframe->ch_layout);

    if (need_switch_ctx || !d->ctx) {
      int ret = swr_alloc_set_opts2(
        &d->ctx, &out_ch_layout, out_sample_format, out_sample_rate, &ch_layout,
        static_cast<AVSampleFormat>(avframe->format), avframe->sample_rate, 0,
        nullptr);
      if (ret < 0) {
        return from_av_error_code(ret);
      }

      ret = swr_init(d->ctx);
      if (ret < 0) {
        return from_av_error_code(ret);
      }

      d->in_ch_layout = avframe->ch_layout;
      d->in_sample_format = static_cast<AVSampleFormat>(avframe->format);
      d->in_sample_rate = avframe->sample_rate;

      d->out_ch_layout = out_ch_layout;
      d->out_sample_format = out_sample_format;
      d->out_sample_rate = out_sample_rate;

      std::fill(std::begin(d->audio_data), std::end(d->audio_data), nullptr);
    }

    const int64_t delay = swr_get_delay(d->ctx, out_sample_rate);
    const int out_nb_samples = static_cast<int>(
      av_rescale_rnd(delay + avframe->nb_samples, out_sample_rate,
                     avframe->sample_rate, AV_ROUND_UP));

    const int out_size = av_samples_get_buffer_size(
      nullptr, out_format.channel_count, out_nb_samples, out_sample_format, 0);
    if (out_size < 0) {
      return errc::einval;
    }

    av_fast_malloc(reinterpret_cast<void *>(&d->audio_buf), &d->audio_buf_size,
                   static_cast<std::size_t>(out_size));
    assert(d->audio_buf != nullptr);

    int ret = av_samples_fill_arrays(d->audio_data, nullptr, d->audio_buf,
                                     out_ch_layout.nb_channels, out_nb_samples,
                                     out_sample_format, 0);
    if (ret < 0) {
      return errc::einval;
    }

    const int samples =
      swr_convert(d->ctx, d->audio_data, out_nb_samples,
                  const_cast<const uint8_t **>(avframe->extended_data),
                  avframe->nb_samples);
    if (samples < 0) {
      /* The documentation doesn't tell us if a valid error code is returned */
      return errc::invaliddata;
    }

    return audio_buffer(d->audio_data, samples);
  }

  return audio_buffer(avframe->extended_data, avframe->nb_samples);
}

}  // namespace ffmpeg
