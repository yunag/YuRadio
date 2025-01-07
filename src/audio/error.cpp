#include "ffmpeg/error.h"

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
