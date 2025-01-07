#ifndef FFMPEG_RING_BUFFER_H
#define FFMPEG_RING_BUFFER_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ffmpeg {

class ring_buffer {
public:
  ring_buffer(std::size_t capacity);
  ring_buffer(const ring_buffer &) = default;
  ring_buffer(ring_buffer &&) = default;
  ring_buffer &operator=(const ring_buffer &) = default;
  ring_buffer &operator=(ring_buffer &&) = default;

  std::size_t read(char *data, std::size_t size);
  std::size_t write(const char *data, std::size_t size);

  std::size_t capacity() const;
  std::size_t size() const;

  void clear();
  void clear(std::size_t newcapacity);

private:
  void advance_index(std::size_t &index, std::size_t size) const;

private:
  std::vector<std::uint8_t> m_buffer;
  std::size_t m_capacity;
  std::size_t m_rear = 0;
  std::size_t m_front = 0;
  std::size_t m_size = 0;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_RING_BUFFER_H */
