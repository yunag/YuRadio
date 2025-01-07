#include "ffmpeg/qt_audio_output.h"
#include "ffmpeg/qt_audio_output_device.h"

#include <QAudioSink>
#include <QDebug>
#include <QMediaDevices>
#include <QPointer>
#include <QThread>

#include <cstddef>
#include <shared_mutex>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

namespace {

constexpr qint64 default_audio_buffer_size =
  static_cast<const qint64>(1024) * 46;

QAudioFormat qt_audio_format(const ffmpeg::audio_format &format) {
  QAudioFormat qt_format;

  qt_format.setSampleRate(format.sample_rate);
  qt_format.setChannelCount(format.channel_count);

  switch (format.sample_format) {
    case ffmpeg::sample_format::u8:
      qt_format.setSampleFormat(QAudioFormat::UInt8);
      break;
    case ffmpeg::sample_format::s16:
      qt_format.setSampleFormat(QAudioFormat::Int16);
      break;
    case ffmpeg::sample_format::s32:
      qt_format.setSampleFormat(QAudioFormat::Int32);
      break;
    case ffmpeg::sample_format::f32:
      qt_format.setSampleFormat(QAudioFormat::Float);
      break;
    case ffmpeg::sample_format::unknown:
    default:
      qt_format.setSampleFormat(QAudioFormat::Unknown);
      break;
  }

  return qt_format;
}

}  // namespace

namespace ffmpeg {

class audio_output_private : public QObject {
public:
  std::size_t buffer_size = default_audio_buffer_size;

  /* NOTE: Sink must be modified only from the audio thread */
  QPointer<QAudioSink> sink;
  audio_format input_format;
  audio_format output_format;

  AudioOutputDevice device;
  QThread audio_thread;
  mutable std::shared_mutex mutex;

  audio_output::state state = audio_output::state::stopped;

  QAudioDevice audio_device = QMediaDevices::defaultAudioOutput();
  double volume = 1.0;

  void reset() {
    device.stop();

    if (QThread::currentThread() != &audio_thread) {
      QMetaObject::invokeMethod(this, &audio_output_private::reset);
      return;
    }

    std::unique_lock locker(mutex);

    if (sink) {
      sink->stop();
      sink->deleteLater();
      sink = nullptr;
    }

    QAudioFormat qt_format = qt_audio_format(output_format);
    assert(qt_format.isValid());
    assert(qt_format.sampleFormat() != QAudioFormat::Unknown);

    sink = new QAudioSink(audio_device, qt_format, this);
    QObject::connect(&audio_thread, &QThread::finished, sink, [o = sink]() {
      o->stop();
      o->deleteLater();
    });

    if (buffer_size > 0) {
      sink->setBufferSize(static_cast<qsizetype>(buffer_size));
    }
    sink->setVolume(volume);

    if (state == audio_output::state::playing) {
      device.start();

      locker.unlock();
      sink->start(&device);
    }
  }
};

audio_output::audio_output() : d(make_pimpl<audio_output_private>()) {
  d->moveToThread(&d->audio_thread);
  d->device.open(QIODevice::ReadOnly);
  d->audio_thread.setObjectName("Audio"_L1);
  d->audio_thread.start(QThread::Priority::TimeCriticalPriority);
}

audio_output::~audio_output() {
  stop();

  d->audio_thread.quit();
  if (!d->audio_thread.wait(QDeadlineTimer(3s))) {
    qWarning() << "Failed to quit!" << &d->audio_thread;
  }
}

void audio_output::start() {
  const std::unique_lock locker(d->mutex);

  if (d->sink && d->state != state::playing) {
    d->state = state::playing;
    d->device.start();

    QMetaObject::invokeMethod(d.get(), [this]() {
      if (d->sink->state() == QtAudio::SuspendedState) {
        d->sink->resume();
      } else {
        d->sink->start(&d->device);
      }
    });
  }
}

void audio_output::stop() {
  const std::unique_lock locker(d->mutex);

  if (d->sink && d->state != state::stopped) {
    d->state = state::stopped;

    QMetaObject::invokeMethod(d.get(), [this]() {
      d->sink->stop();
    });

    d->device.stop();
    d->device.clear();
  }
}

void audio_output::pause() {
  const std::unique_lock locker(d->mutex);

  if (d->sink && d->state != state::paused) {
    d->state = state::paused;

    d->device.stop();
    QMetaObject::invokeMethod(d.get(), [this]() {
      d->sink->suspend();
    });
  }
}

void audio_output::set_volume(double volume) {
  const std::unique_lock locker(d->mutex);
  d->volume = volume;

  if (d->sink) {
    QMetaObject::invokeMethod(d.get(), [this, volume]() {
      d->sink->setVolume(volume);
    });
  }
}

double audio_output::volume() const {
  const std::shared_lock locker(d->mutex);
  return d->volume;
}

void audio_output::set_buffer_size(std::size_t size) {
  const std::shared_lock locker(d->mutex);
  d->buffer_size = size;
}

std::size_t audio_output::buffer_size() const {
  const std::shared_lock locker(d->mutex);
  return d->buffer_size;
}

std::error_code audio_output::push_frame(const ffmpeg::frame &frame) {
  audio_format frame_format = frame.audio_format();
  assert(frame_format.valid());

  return d->device.push_frame(frame, output_format());
}

std::size_t audio_output::bytes_in_queue() const {
  return d->device.bytes_in_queue();
}

void audio_output::set_audio_device(const ffmpeg::audio_device &device) {
  const std::unique_lock locker(d->mutex);

  if (!device.is_null()) {
    d->audio_device = device.qt_audio_device();
    d->reset();
  }
}

void audio_output::set_input_format(ffmpeg::audio_format format) {
  const std::unique_lock locker(d->mutex);

  if (format != d->input_format) {
    d->input_format = format;
    d->output_format = d->input_format;

    const QAudioFormat qt_format = qt_audio_format(d->input_format);
    if (qt_format.sampleFormat() == QAudioFormat::Unknown) {
      d->output_format.sample_format = sample_format::s32;
    }

    d->reset();
  }
}

ffmpeg::audio_format audio_output::output_format() const {
  const std::shared_lock locker(d->mutex);
  return d->output_format;
}

}  // namespace ffmpeg
