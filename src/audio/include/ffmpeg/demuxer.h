#ifndef FFMPEG_DEMUXER_H
#define FFMPEG_DEMUXER_H

#include <optional>
#include <system_error>

#include "ffmpeg/frame.h"
#include "ffmpeg/packet.h"
#include "ffmpeg/pimpl.h"
#include "ffmpeg/types.h"

struct AVStream;

namespace ffmpeg {

class demuxer_private;

class demuxer {
public:
  demuxer();
  ~demuxer();
  demuxer(const demuxer &) = delete;
  demuxer(demuxer &&) = delete;
  demuxer &operator=(const demuxer &) = delete;
  demuxer &operator=(demuxer &&) = delete;

  std::error_code read(ffmpeg::packet &packet);
  std::error_code open(const char *url);
  std::error_code decode(const ffmpeg::packet &packet, ffmpeg::frame &frame);

  ffmpeg::metadata_map metadata() const;
  std::optional<std::string> metadata(const std::string &key) const;

  audio_format output_format() const;

  void close();
  void abort();

private:
  ffmpeg::pimpl<demuxer_private> d;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_DEMUXER_H */
