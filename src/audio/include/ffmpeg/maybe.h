#ifndef FFMPEG_MAYBE_H
#define FFMPEG_MAYBE_H

#include <optional>
#include <system_error>

namespace ffmpeg {

template <typename T>
class maybe {
public:
  template <typename U>
  maybe(U errc)
    requires(std::is_error_code_enum_v<U>)
      : m_error_code(errc) {}

  maybe(std::error_code error_code) : m_error_code(error_code) {}

  maybe(const T &value) : m_value(value) {}

  maybe(T &&value) : m_value(std::move(value)) {}

  maybe(const maybe &) = default;
  maybe(maybe &&) = delete;
  maybe &operator=(const maybe &) = default;
  maybe &operator=(maybe &&) = delete;

  constexpr operator bool() const { return m_value.has_value(); }

  constexpr T *operator->() noexcept { return std::addressof(value()); }

  constexpr const T *operator->() const noexcept {
    return std::addressof(value());
  }

  constexpr T &operator*() & noexcept { return value(); }

  constexpr const T &operator*() const & noexcept { return value(); }

  constexpr T &value() {
    assert(m_value.has_value());
    return *m_value;
  }

  constexpr const T &value() const {
    assert(m_value.has_value());
    return *m_value;
  }

  constexpr const std::error_code &error() const { return m_error_code; }

private:
  std::error_code m_error_code;
  std::optional<T> m_value;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_MAYBE_H */
