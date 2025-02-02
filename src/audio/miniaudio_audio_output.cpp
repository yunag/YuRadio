#include "ffmpeg/miniaudio_audio_output.h"
#include "ffmpeg/audio_format_p.h"
#include "ffmpeg/audio_resampler.h"

/* Disable components that we don't need */
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION
extern "C" {
#include "miniaudio.h"
}

extern "C" {
#include <libavutil/audio_fifo.h>
}

#include <QDebug>
#include <mutex>

using namespace std::chrono_literals;

namespace {

ma_format get_miniaudio_sample_format(ffmpeg::sample_format format) {
  switch (format) {
    case ffmpeg::sample_format::u8:
      return ma_format_u8;
    case ffmpeg::sample_format::s16:
      return ma_format_s16;
    case ffmpeg::sample_format::s32:
      return ma_format_s32;
    case ffmpeg::sample_format::f32:
      return ma_format_f32;
    case ffmpeg::sample_format::f64:
    case ffmpeg::sample_format::u8p:
    case ffmpeg::sample_format::s16p:
    case ffmpeg::sample_format::s32p:
    case ffmpeg::sample_format::f32p:
    case ffmpeg::sample_format::f64p:
    case ffmpeg::sample_format::s64:
    case ffmpeg::sample_format::s64p:
    case ffmpeg::sample_format::unknown:
    default:
      return ma_format_unknown;
  }
}

}  // namespace

namespace miniaudio {

class audio_output_private {
public:
  audio_output_private(audio_output *qptr) : q(qptr) {}

  AVAudioFifo *audio_fifo = nullptr;
  audio_output *q = nullptr;

  ffmpeg::audio_buffer audio_buffer;
  ffmpeg::audio_resampler resampler;
  ffmpeg::audio_format output_format;
  ma_device miniaudio_audio_device;

  mutable std::mutex mutex;

  static void miniaudio_data_callback(ma_device *device, void *output,
                                      const void * /*input*/,
                                      ma_uint32 frame_count) {
    auto *d =
      reinterpret_cast<miniaudio::audio_output_private *>(device->pUserData);
    assert(d != nullptr);

    const std::unique_lock locker(d->mutex);
    av_audio_fifo_read(d->audio_fifo, &output, static_cast<int>(frame_count));
  }

  void reset(ffmpeg::audio_format format) {
    if (q->output_format() == format) {
      return;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback);

    config.playback.format = get_miniaudio_sample_format(format.sample_format);
    config.playback.channels = static_cast<ma_uint32>(format.channel_count);
    config.sampleRate = static_cast<ma_uint32>(format.sample_rate);
    config.dataCallback = miniaudio_data_callback;
    config.pUserData = this;

    float volume;
    ma_device_get_master_volume(&miniaudio_audio_device, &volume);

    {
      const std::unique_lock locker(mutex);

      output_format = format;
      audio_buffer.reset(format);

      ma_device_uninit(&miniaudio_audio_device);
      av_audio_fifo_free(audio_fifo);
      audio_fifo = nullptr;

      if (ma_device_init(nullptr, &config, &miniaudio_audio_device) !=
          MA_SUCCESS) {
        return;
      }

      audio_fifo =
        av_audio_fifo_alloc(get_av_sample_format(output_format.sample_format),
                            output_format.channel_count, 1);
    }

    ma_device_set_master_volume(&miniaudio_audio_device, volume);
  }
};

audio_output::audio_output()
    : d(ffmpeg::make_pimpl<audio_output_private>(this)) {}

audio_output::~audio_output() {
  {
    /* Data callback may be still spinning. Acquire lock */
    const std::unique_lock locker(d->mutex);
    ma_device_uninit(&d->miniaudio_audio_device);
    av_audio_fifo_free(d->audio_fifo);
  }
}

void audio_output::start() {
  ma_device_start(&d->miniaudio_audio_device);
}

void audio_output::stop() {
  {
    const std::unique_lock locker(d->mutex);
    if (d->audio_fifo) {
      av_audio_fifo_drain(d->audio_fifo, av_audio_fifo_size(d->audio_fifo));
    }
  }

  ma_device_stop(&d->miniaudio_audio_device);
}

void audio_output::pause() {
  ma_device_stop(&d->miniaudio_audio_device);
}

void audio_output::set_volume(double volume) {
  assert(volume >= 0);

  const ma_result res = ma_device_set_master_volume(&d->miniaudio_audio_device,
                                                    static_cast<float>(volume));
  assert(res == MA_SUCCESS);
}

double audio_output::volume() const {
  float miniaudio_volume;
  const ma_result res = ma_device_get_master_volume(
    const_cast<ma_device *>(&d->miniaudio_audio_device), &miniaudio_volume);

  assert(res == MA_SUCCESS);
  return static_cast<double>(miniaudio_volume);
}

std::error_code audio_output::push_frame(const ffmpeg::frame &frame) {
  std::error_code ec = d->resampler.convert(frame, d->audio_buffer);
  if (ec) {
    return ec;
  }

  const std::unique_lock locker(d->mutex);

  av_audio_fifo_write(
    d->audio_fifo,
    reinterpret_cast<void *const *>(
      const_cast<uint8_t *const *>(d->audio_buffer.data_pointers())),
    d->audio_buffer.samples_count());

  return {};
}

int audio_output::samples_in_queue() const {
  const std::unique_lock locker(d->mutex);
  return d->audio_fifo ? av_audio_fifo_size(d->audio_fifo) : 0;
}

void audio_output::set_audio_device(const ffmpeg::audio_device & /* device */) {
  /* TODO: implement */
}

void audio_output::set_input_format(ffmpeg::audio_format format) {
  const ma_format miniaudio_sample_format =
    get_miniaudio_sample_format(format.sample_format);
  if (miniaudio_sample_format == ma_format_unknown) {
    /* Use something that works */
    format.sample_format = ffmpeg::sample_format::s32;
  }

  d->reset(format);
}

ffmpeg::audio_format audio_output::output_format() const {
  const std::unique_lock locker(d->mutex);
  return d->output_format;
}

}  // namespace miniaudio
