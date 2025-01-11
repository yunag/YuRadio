/**
 * @file
 * @brief FFmpeg error codes
 *
 * @warning Casting from int to error code is wrong! Use `from_av_error_code`
 */

#ifndef FFMPEG_ERROR_H
#define FFMPEG_ERROR_H

#include <system_error>

namespace ffmpeg {

enum class errc {
  ok = 0,

  eagain,
  einval,
  enomem,

  bsf_not_found,
  bug,
  buffer_too_small,
  decoder_not_found,
  demuxer_not_found,
  encoder_not_found,
  eof,
  exit,
  external,
  filter_not_found,
  invaliddata,
  muxer_not_found,
  option_not_found,
  patchwelcome,
  protocol_not_found,
  stream_not_found,

  bug2,
  unknown,
  experimental,
  input_changed,
  output_changed,

  http_bad_request,
  http_unauthorized,
  http_forbidden,
  http_not_found,
  http_too_many_requests,
  http_other_4xx,
  http_server_error
};

std::error_code make_error_code(ffmpeg::errc e);
ffmpeg::errc from_av_error_code(int averror_code);

}  // namespace ffmpeg

namespace std {
template <>
struct is_error_code_enum<ffmpeg::errc> : public true_type {};
}  // namespace std

#endif /* !FFMPEG_ERROR_H */
