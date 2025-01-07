#ifndef FFMPEG_PIMPL_H
#define FFMPEG_PIMPL_H

#include <memory>
#include <type_traits>

namespace ffmpeg {

/**
 * @brief Point of this class is to introduce const correctness in PIMPL idiom
 */
template <typename pointer_type>
class propagate_const {
public:
  using value_type = std::remove_reference_t<decltype(*pointer_type{})>;

  template <typename T>
  constexpr propagate_const(T &&p)
    requires(std::is_convertible_v<T, pointer_type>)
      : ptr{std::forward<T>(p)} {}

  constexpr value_type &operator*() { return *ptr; }

  constexpr value_type const &operator*() const { return *ptr; }

  constexpr value_type *operator->() { return std::addressof(*ptr); }

  constexpr const value_type *operator->() const {
    return std::addressof(*ptr);
  }

  constexpr value_type *get() { return std::addressof(*ptr); }

  constexpr const value_type *get() const { return std::addressof(*ptr); }

private:
  pointer_type ptr;
};

template <typename T>
using pimpl = propagate_const<std::unique_ptr<T>>;

template <typename T, typename... Args>
pimpl<T> make_pimpl(Args &&...args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

}  // namespace ffmpeg

#endif /* !FFMPEG_PIMPL_H */
