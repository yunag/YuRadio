#ifndef FFMPEG_SCOPE_GUARD_H
#define FFMPEG_SCOPE_GUARD_H

#include <type_traits>
#include <utility>

namespace ffmpeg {

template <typename F>
class scope_guard {
public:
  explicit scope_guard(F &&f) noexcept : m_func(std::move(f)) {}

  explicit scope_guard(const F &f) noexcept : m_func(f) {}

  scope_guard(scope_guard &&other) noexcept : m_func(std::move(other.m_func)) {}

  ~scope_guard() noexcept { m_func(); }

private:
  F m_func;
};

template <typename F>
scope_guard(F (&)()) -> scope_guard<F (*)()>;

template <typename F>
scope_guard<std::decay_t<F>> make_scope_guard(F &&f) {
  return scope_guard<std::decay_t<F>>(std::forward<F>(f));
}

}  // namespace ffmpeg

#endif  // FFMPEG_SCOPE_GUARD_H
