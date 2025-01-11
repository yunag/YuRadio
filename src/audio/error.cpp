#include "ffmpeg/error.h"

extern "C" {
#include <libavutil/error.h>
}

#ifndef AVERROR_HTTP_TOO_MANY_REQUESTS
#define AVERROR_HTTP_TOO_MANY_REQUESTS FFERRTAG(0xF8, '4', '2', '9')
#endif /* !AVERROR_HTTP_TOO_MANY_REQUESTS */

#include <cstring>

std::error_code ffmpeg::make_error_code(errc e) {
  static const struct : public std::error_category {
    const char *name() const noexcept override { return "ffmpeg"; }

    std::string message(int ev) const override {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      const char *errbuf_ptr = errbuf;

      if (av_strerror(ev, errbuf, sizeof(errbuf)) < 0) {
        /* Error string cannot be found so it's a standard error */
        errbuf_ptr = strerror(AVUNERROR(ev));
      }

      return errbuf_ptr;
    };
  } category;

  return {static_cast<int>(e), category};
}

ffmpeg::errc ffmpeg::from_av_error_code(int averror_code) {
  if (averror_code >= 0) {
    return errc::ok;
  }

  switch (averror_code) {
    case AVERROR(EAGAIN):
      return errc::eagain;
    case AVERROR(EINVAL):
      return errc::einval;
    case AVERROR(ENOMEM):
      return errc::enomem;
    case AVERROR_BSF_NOT_FOUND:
      return errc::bsf_not_found;
    case AVERROR_BUG:
      return errc::bug;
    case AVERROR_BUFFER_TOO_SMALL:
      return errc::buffer_too_small;
    case AVERROR_DECODER_NOT_FOUND:
      return errc::decoder_not_found;
    case AVERROR_DEMUXER_NOT_FOUND:
      return errc::demuxer_not_found;
    case AVERROR_ENCODER_NOT_FOUND:
      return errc::encoder_not_found;
    case AVERROR_EOF:
      return errc::eof;
    case AVERROR_EXIT:
      return errc::exit;
    case AVERROR_EXTERNAL:
      return errc::external;
    case AVERROR_FILTER_NOT_FOUND:
      return errc::filter_not_found;
    case AVERROR_INVALIDDATA:
      return errc::invaliddata;
    case AVERROR_MUXER_NOT_FOUND:
      return errc::muxer_not_found;
    case AVERROR_OPTION_NOT_FOUND:
      return errc::option_not_found;
    case AVERROR_PATCHWELCOME:
      return errc::patchwelcome;
    case AVERROR_PROTOCOL_NOT_FOUND:
      return errc::protocol_not_found;
    case AVERROR_STREAM_NOT_FOUND:
      return errc::stream_not_found;
    case AVERROR_BUG2:
      return errc::bug2;
    case AVERROR_EXPERIMENTAL:
      return errc::experimental;
    case AVERROR_INPUT_CHANGED:
      return errc::input_changed;
    case AVERROR_OUTPUT_CHANGED:
      return errc::output_changed;
    case AVERROR_HTTP_BAD_REQUEST:
      return errc::http_bad_request;
    case AVERROR_HTTP_UNAUTHORIZED:
      return errc::http_unauthorized;
    case AVERROR_HTTP_FORBIDDEN:
      return errc::http_forbidden;
    case AVERROR_HTTP_NOT_FOUND:
      return errc::http_not_found;
    case AVERROR_HTTP_TOO_MANY_REQUESTS:
      return errc::http_too_many_requests;
    case AVERROR_HTTP_OTHER_4XX:
      return errc::http_other_4xx;
    case AVERROR_HTTP_SERVER_ERROR:
      return errc::http_server_error;
    case AVERROR_UNKNOWN:
    default:
      return errc::unknown;
  }
}
