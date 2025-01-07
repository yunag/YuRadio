#include <QLoggingCategory>
Q_LOGGING_CATEGORY(ffmpegLog, "ffmpeg")

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/log.h>
}

#include "ffmpeg/demuxer.h"
#include "ffmpeg/error.h"
#include "ffmpeg/miniaudio_audio_output.h"
#include "ffmpeg/packet_queue.h"
#include "ffmpeg/player.h"
#include "ffmpeg/utils.h"

#include <QDebug>

#include <condition_variable>
#include <future>
#include <shared_mutex>
#include <utility>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

namespace ffmpeg {

class player_private {
public:
  player_private(player *q_ptr) : q(q_ptr) {}

  ffmpeg::demuxer demuxer;
  miniaudio::audio_output audio_output;

  std::shared_ptr<ffmpeg::player_listener> listener;
  std::shared_future<std::error_code> load_future;
  std::future<void> read_future;
  std::future<void> play_future;

  mutable std::shared_mutex state_mutex;

  ffmpeg::player::error error = player::error::noerror;
  std::string error_string;

  mutable std::mutex wait_mutex;
  std::condition_variable wait_cond;

  /* Keep track of updated StreamTitle */
  std::string stream_title;

  ffmpeg::player::state state = player::state::stopped;
  ffmpeg::player::media_status media_status = player::media_status::nomedia;
  ffmpeg::packet_queue packets;

  ffmpeg::player *q = nullptr;
  std::string url;

  std::atomic_bool quit = false;
  std::atomic_bool eof = false;

  void set_error(player::error newerror, const std::string &newerror_string) {
    std::unique_lock locker(state_mutex);

    if (newerror != error || newerror_string != error_string) {
      error = newerror;
      error_string = newerror_string;

      if (listener) {
        listener->on_error_changed(newerror, newerror_string);
      }
    }
  }

  void set_media_status(player::media_status newstatus) {
    std::unique_lock locker(state_mutex);

    if (newstatus != media_status) {
      media_status = newstatus;

      if (listener) {
        listener->on_media_status_changed(newstatus);
      }
    }
  };

  void set_state(player::state newstate) {
    std::unique_lock locker(state_mutex);

    if (newstate != state) {
      state = newstate;

      if (listener) {
        listener->on_state_changed(newstate);
      }
    }
  };

  void notify_updated_metadata(const metadata_map &metadata) {
    std::unique_lock locker(state_mutex);

    if (listener) {
      listener->on_metadata_changed(metadata);
    }
  };

  void update_stream_title(std::optional<std::string> maybe_stream_title) {
    if (maybe_stream_title && *maybe_stream_title != stream_title) {
      stream_title = *maybe_stream_title;

      metadata_map updated_metadata;
      updated_metadata["StreamTitle"] = stream_title;

      notify_updated_metadata(updated_metadata);
    }
  };

  void read_packets() {
    while (!quit) {
      ffmpeg::packet packet;

      const std::error_code ec = demuxer.read(packet);
      if (ec) {
        if (ec == errc::eof) {
          eof = true;
        }
        return;
      }

      update_stream_title(demuxer.metadata("StreamTitle"));

      packets.enqueue(std::move(packet));
    }
  }

  void load_media() {
    set_error(player::error::noerror, {});
    set_media_status(player::media_status::loading);

    load_future = std::async(std::launch::async, [this]() {
      std::error_code ec = demuxer.open(url.c_str());

      if (ec == errc::exit) {
        return ec;
      }

      if (ec) {
        set_media_status(player::media_status::invalid);

        if (ec == errc::http_forbidden || ec == errc::http_not_found ||
            ec == errc::http_other_4xx || ec == errc::http_bad_request ||
            ec == errc::http_server_error || ec == errc::http_unauthorized ||
            ec == errc::http_too_many_requests) {
          set_error(player::error::network, ec.message());
        } else {
          set_error(player::error::resource, ec.message());
        }

        qWarning().noquote() << "Failed to load media:" << url << ec.message();
      } else {
        set_media_status(player::media_status::loaded);

        notify_updated_metadata(demuxer.metadata());
      }

      return ec;
    });

    read_future = std::async(std::launch::async, [this]() {
      assert(load_future.valid());

      std::error_code ec = load_future.get();
      if (!ec) {
        read_packets();
      }
    });
  }

  void play_audio() {
    const audio_format output_format = demuxer.output_format();
    audio_output.set_input_format(output_format);

    const audio_format audio_output_format = audio_output.output_format();

    /* One second worth of samples 
     *
     * TODO: Make it configurable
     */
    const std::size_t minimum_audio_queue_size =
      static_cast<std::size_t>(audio_output_format.sample_rate);
    const std::size_t maximum_audio_queue_size =
      minimum_audio_queue_size +
      static_cast<std::size_t>(audio_output_format.sample_rate);

    const std::size_t preferred_audio_queue_size =
      (minimum_audio_queue_size + maximum_audio_queue_size) / 2;
    const std::size_t samples_wait =
      maximum_audio_queue_size - preferred_audio_queue_size;

    const std::chrono::duration<double> wait_duration(
      static_cast<double>(samples_wait) / audio_output_format.sample_rate);

    qCDebug(ffmpegLog).noquote()
      << "Audio buffer parameters:\n"
      << "\tMinimum size:" << minimum_audio_queue_size << "\n"
      << "\tMaximum size:" << maximum_audio_queue_size << "\n"
      << "\tPreferred size:" << preferred_audio_queue_size << "\n"
      << "\tWait duration:" << wait_duration;

    std::error_code ec;
    ffmpeg::frame frame;
    ffmpeg::packet packet = packets.dequeue();

    while (!quit) {
      if (q->get_state() == player::state::paused) {
        audio_output.pause();

        std::unique_lock locker(wait_mutex);
        wait_cond.wait(locker, [this]() {
          return q->get_state() != player::state::paused || quit;
        });

        qCDebug(ffmpegLog) << "Resumed after pause";

        if (quit) {
          break;
        }
      }

      if (audio_output.samples_in_queue() > minimum_audio_queue_size) {
        audio_output.start();
      }

      if (audio_output.samples_in_queue() > maximum_audio_queue_size) {
        std::unique_lock locker(wait_mutex);
        wait_cond.wait_for(locker, wait_duration);
        continue;
      }

      ec = demuxer.decode(packet, frame);
      if (ec == errc::eof) {
        set_media_status(player::media_status::end_of_file);
        return;
      }

      if (ec) {
        if (eof && packets.empty()) {
          qCDebug(ffmpegLog) << "Flush decoder";

          /* Null packet to flush decoder's internal buffers */
          packet = ffmpeg::packet(nullptr);
        } else {
          packet = packets.dequeue();
        }

        continue;
      }

      ec = audio_output.push_frame(frame);
      if (ec) {
        qWarning() << "Failed to push frame to audio output:" << ec.message();
        return;
      }
    }
  }
};

static void av_log_callback(void * /*ptr*/, int level, const char *fmt,
                            va_list vl) {
  /* Filter logs above the chosen level and AV_LOG_QUIET (negative level) */
  if (level < 0 || level > av_log_get_level()) {
    return;
  }

  static std::mutex mutex;

  const std::lock_guard locker(mutex);
  static QString message;

  message += QString::vasprintf(fmt, vl);

  /* I expect messages to end with a newline */
  if (message.endsWith('\n'_L1)) {
    message.removeLast();

    if (level == AV_LOG_DEBUG || level == AV_LOG_TRACE) {
      qCDebug(ffmpegLog).noquote() << message;
    } else if (level == AV_LOG_VERBOSE || level == AV_LOG_INFO) {
      qCInfo(ffmpegLog).noquote() << message;
    } else if (level == AV_LOG_WARNING) {
      qCWarning(ffmpegLog).noquote() << message;
    } else if (level == AV_LOG_ERROR || level == AV_LOG_FATAL ||
               level == AV_LOG_PANIC) {
      qCCritical(ffmpegLog).noquote() << message;
    }

    message.clear();
  }
}

player::player() : d(make_pimpl<player_private>(this)) {
  static std::once_flag ffmpeg_initialized;

  std::call_once(ffmpeg_initialized, []() {
    av_log_set_level(AV_LOG_DEBUG);
    av_log_set_callback(av_log_callback);
  });

  qInfo() << "FFmpeg version:" << av_version_info() << avutil_license();
}

player::~player() {
  stop();
};

void player::set_source(const std::string &url) {
  if (!url_is_valid(url) || d->url == url) {
    return;
  }

  stop();

  d->url = url;
  d->load_media();
}

void player::play() {
  if (get_state() == state::paused) {
    d->set_state(state::playing);
    d->wait_cond.notify_all();
    return;
  }

  if (get_state() == state::playing || d->url.empty() ||
      d->play_future.valid()) {
    return;
  }

  if (get_media_status() == media_status::nomedia) {
    d->load_media();
  }

  d->play_future = std::async(std::launch::async, [this]() {
    assert(d->load_future.valid());

    std::error_code ec = d->load_future.get();
    if (!ec) {
      d->set_state(state::playing);
      d->play_audio();
      d->set_state(state::stopped);
    }
  });
}

void player::stop() {
  if (get_state() != state::stopped ||
      get_media_status() != media_status::nomedia) {
    d->quit = true;
    d->wait_cond.notify_all();

    d->packets.abort();
    d->demuxer.abort();
    d->audio_output.stop();

    if (d->play_future.valid() && !future_ready(d->play_future, 3s)) {
      qWarning() << "Play future failed to quit!";
    }
    if (d->read_future.valid() && !future_ready(d->read_future, 3s)) {
      qWarning() << "Read future failed to quit!";
    }
    if (d->load_future.valid() && !future_ready(d->load_future, 3s)) {
      qWarning() << "Load future failed to quit!";
    }

    d->read_future = {};
    d->play_future = {};
    d->load_future = {};
    d->stream_title = {};

    d->demuxer.close();
    d->packets.reset();
    d->quit = false;
    d->eof = false;

    d->set_state(state::stopped);
    d->set_media_status(media_status::nomedia);
  }
}

void player::pause() {
  if (get_state() != state::playing) {
    return;
  }

  d->set_state(state::paused);
  d->wait_cond.notify_all();
}

player::media_status player::get_media_status() const {
  const std::shared_lock locker(d->state_mutex);
  return d->media_status;
}

player::state player::get_state() const {
  const std::shared_lock locker(d->state_mutex);
  return d->state;
}

std::string player::source() const {
  return d->url;
}

player::error player::get_error() const {
  const std::shared_lock locker(d->state_mutex);
  return d->error;
}

std::string player::error_string() const {
  const std::shared_lock locker(d->state_mutex);
  return d->error_string;
}

void player::set_listener(std::shared_ptr<player_listener> listener) {
  const std::unique_lock locker(d->state_mutex);
  d->listener = std::move(listener);
}

void player::set_audio_device(const ffmpeg::audio_device &device) {
  d->audio_output.set_audio_device(device);
}

void player::set_volume(double volume) {
  d->audio_output.set_volume(volume);
}

double player::volume() const {
  return d->audio_output.volume();
}

metadata_map player::metadata() const {
  return d->demuxer.metadata();
}

}  // namespace ffmpeg
