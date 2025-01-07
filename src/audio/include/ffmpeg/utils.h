#ifndef FFMPEG_UTILS_H
#define FFMPEG_UTILS_H

#include <chrono>
#include <future>

#include <QUrl>

namespace ffmpeg {

inline bool url_is_valid(const std::string &media_url) {
  return !media_url.empty() && QUrl(media_url.c_str()).isValid();
}

template <typename T, typename Rep, typename Per>
bool future_ready(std::future<T> &future,
                  const std::chrono::duration<Rep, Per> &duration) {
  return future.valid() &&
         future.wait_for(duration) == std::future_status::ready;
}

template <typename T>
bool future_ready(std::future<T> &future) {
  return future_ready(future, std::chrono::seconds(0));
}

template <typename T, typename Rep, typename Per>
bool future_ready(std::shared_future<T> &future,
                  const std::chrono::duration<Rep, Per> &duration) {
  return future.valid() &&
         future.wait_for(duration) == std::future_status::ready;
}

template <typename T>
bool future_ready(std::shared_future<T> &future) {
  return future_ready(future, std::chrono::seconds(0));
}

}  // namespace ffmpeg

#endif /* !FFMPEG_UTILS_H */
