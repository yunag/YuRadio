#include "ffmpeg/qt_audio_output_device.h"
#include "ffmpeg/audio_resampler.h"

#include <condition_variable>
#include <deque>
#include <mutex>

class AudioOutputDevicePrivate {
public:
  std::deque<std::vector<uint8_t>> buffer;
  ffmpeg::audio_resampler resampler;

  mutable std::mutex mutex;
  std::condition_variable cond;

  std::size_t total_samples = 0;
  std::size_t offset = 0;
  bool quit = false;
};

AudioOutputDevice::AudioOutputDevice(QObject *parent)
    : QIODevice(parent), d(ffmpeg::make_pimpl<AudioOutputDevicePrivate>()) {}

AudioOutputDevice::~AudioOutputDevice() {
  stop();
};

qint64 AudioOutputDevice::bytesAvailable() const {
  return std::numeric_limits<qint64>::max();
}

qint64 AudioOutputDevice::size() const {
  const std::lock_guard locker(d->mutex);
  return static_cast<qint64>(d->buffer.size());
}

std::size_t AudioOutputDevice::samples_in_queue() const {
  const std::lock_guard locker(d->mutex);
  return d->total_samples;
}

qint64 AudioOutputDevice::readData(char *data, qint64 len) {
  if (!len) {
    return 0;
  }

  qint64 bytes_to_read = len;
  std::unique_lock locker(d->mutex);
  while (len > 0 && !d->quit) {
    if (d->buffer.empty()) {
      d->cond.wait(locker, [this]() {
        return d->quit || !d->buffer.empty();
      });
      if (d->quit) {
        break;
      }
    }

    const auto &frame = d->buffer.front();

    const std::size_t bytes_write =
      std::min(frame.size() - d->offset, static_cast<std::size_t>(len));
    std::memcpy(data, frame.data() + d->offset, bytes_write);

    data += bytes_write;
    len -= static_cast<qint64>(bytes_write);
    d->offset += bytes_write;

    assert(len >= 0);
    assert(d->offset <= frame.size());

    if (d->offset == frame.size()) {
      d->total_samples -= frame.size();
      d->offset = 0;
      d->buffer.pop_front();
    }
  }

  if (d->quit) {
    std::memset(data, 0, static_cast<std::size_t>(len));
  }

  return bytes_to_read;
}

qint64 AudioOutputDevice::writeData(const char * /*data*/, qint64 /*maxlen*/) {
  return 0;
}

void AudioOutputDevice::clear() {
  const std::lock_guard locker(d->mutex);

  d->offset = 0;
  d->total_samples = 0;
  d->buffer.clear();
}

void AudioOutputDevice::start() {
  {
    const std::lock_guard locker(d->mutex);
    d->quit = false;
  }

  d->cond.notify_all();
}

void AudioOutputDevice::stop() {
  {
    const std::lock_guard locker(d->mutex);
    d->quit = true;
  }

  d->cond.notify_all();
}

std::error_code
AudioOutputDevice::push_frame(const ffmpeg::frame &frame,
                              const ffmpeg::audio_format &out_format) {
  {
    const std::lock_guard locker(d->mutex);
    auto maybe_data = d->resampler.convert(frame, out_format);
    if (!maybe_data) {
      return maybe_data.error();
    }

    ffmpeg::audio_buffer data = *maybe_data;
    const auto size =
      static_cast<int>(data.nb_samples() * out_format.bytes_per_sample() *
                       out_format.channel_count);
    d->total_samples += static_cast<std::size_t>(data.nb_samples());
    d->buffer.emplace_back(data.data(), data.data() + size);
  }

  d->cond.notify_all();

  return {};
}
