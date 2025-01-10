#ifndef FFMPEG_PLAYER_H
#define FFMPEG_PLAYER_H

#include "ffmpeg/audio_device.h"
#include "ffmpeg/frame.h"
#include "ffmpeg/pimpl.h"
#include "ffmpeg/types.h"

#include <cstdint>
#include <string>

namespace ffmpeg {

class player_private;
class player_listener;

class player {
public:
  enum class state : std::uint8_t {
    playing = 0,
    stopped,
    paused,
  };

  enum class media_status : std::uint8_t {
    nomedia = 0,
    invalid,
    loading,
    loaded,
    end_of_file,
  };

  enum class error : std::uint8_t {
    noerror = 0,
    resource,
    network,
  };

  player();
  player(const player &) = delete;
  player(player &&) = delete;
  player &operator=(const player &) = delete;
  player &operator=(player &&) = delete;
  ~player();

  void play();
  void stop();
  void pause();

  void set_source(const std::string &url);
  std::string source() const;

  void set_listener(std::shared_ptr<player_listener> listener);
  void set_audio_device(const ffmpeg::audio_device &device);

  media_status get_media_status() const;
  state get_state() const;
  error get_error() const;
  std::string error_string() const;

  ffmpeg::metadata_map metadata() const;

  void set_volume(double volume);
  double volume() const;

  ffmpeg::pimpl<player_private> d;
};

class player_listener {
public:
  virtual ~player_listener() = default;

  /**
   * @brief Callback for metadata
   *
   * @param metadata Metadata (will contain ONLY CHANGED metadata)
   */
  virtual void on_metadata_changed(const ffmpeg::metadata_map &metadata) {
    (void)metadata;
  }

  virtual void on_frame_captured(const ffmpeg::frame &frame) { (void)frame; }

  virtual void on_error_changed(ffmpeg::player::error error,
                                const std::string &error_string) {
    (void)error;
    (void)error_string;
  }

  virtual void on_media_status_changed(ffmpeg::player::media_status status) {
    (void)status;
  }

  virtual void on_state_changed(ffmpeg::player::state state) { (void)state; }
};

}  // namespace ffmpeg

#endif /* !FFMPEG_PLAYER_H */
