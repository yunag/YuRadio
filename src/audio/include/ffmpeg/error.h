#ifndef FFMPEG_ERROR_H
#define FFMPEG_ERROR_H

#include <system_error>

extern "C" {
#include <libavutil/error.h>
}

namespace ffmpeg {

enum class errc {
  ok = 0,

  eagain = AVERROR(EAGAIN),
  einval = AVERROR(EINVAL),
  enomem = AVERROR(ENOMEM),

  bsf_not_found = AVERROR_BSF_NOT_FOUND,
  bug = AVERROR_BUG,
  buffer_too_small = AVERROR_BUFFER_TOO_SMALL,
  decoder_not_found = AVERROR_DECODER_NOT_FOUND,
  demuxer_not_found = AVERROR_DEMUXER_NOT_FOUND,
  encoder_not_found = AVERROR_ENCODER_NOT_FOUND,
  eof = AVERROR_EOF,
  exit = AVERROR_EXIT,
  external = AVERROR_EXTERNAL,
  filter_not_found = AVERROR_FILTER_NOT_FOUND,
  invaliddata = AVERROR_INVALIDDATA,
  muxer_not_found = AVERROR_MUXER_NOT_FOUND,
  option_not_found = AVERROR_OPTION_NOT_FOUND,
  patchwelcome = AVERROR_PATCHWELCOME,
  protocol_not_found = AVERROR_PROTOCOL_NOT_FOUND,
  stream_not_found = AVERROR_STREAM_NOT_FOUND,

  bug2 = AVERROR_BUG2,
  unknown = AVERROR_UNKNOWN,
  experimental = AVERROR_EXPERIMENTAL,
  input_changed = AVERROR_INPUT_CHANGED,
  output_changed = AVERROR_OUTPUT_CHANGED,

  http_bad_request = AVERROR_HTTP_BAD_REQUEST,
  http_unauthorized = AVERROR_HTTP_UNAUTHORIZED,
  http_forbidden = AVERROR_HTTP_FORBIDDEN,
  http_not_found = AVERROR_HTTP_NOT_FOUND,
  http_too_many_requests = AVERROR_HTTP_TOO_MANY_REQUESTS,
  http_other_4xx = AVERROR_HTTP_OTHER_4XX,
  http_server_error = AVERROR_HTTP_SERVER_ERROR
};

std::error_code make_error_code(errc e);

}  // namespace ffmpeg

namespace std {
template <>
struct is_error_code_enum<ffmpeg::errc> : public true_type {};
}  // namespace std

#endif /* !FFMPEG_ERROR_H */
