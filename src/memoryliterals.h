#include <cstddef>

namespace MemoryLiterals {

constexpr std::size_t operator""_KiB(unsigned long long int x) {
  return 1024ULL * x;
}

constexpr std::size_t operator""_MiB(unsigned long long int x) {
  return 1024_KiB * x;
}

constexpr std::size_t operator""_GiB(unsigned long long int x) {
  return 1024_MiB * x;
}

constexpr std::size_t operator""_TiB(unsigned long long int x) {
  return 1024_GiB * x;
}

constexpr std::size_t operator""_PiB(unsigned long long int x) {
  return 1024_TiB * x;
}

}  // namespace MemoryLiterals
