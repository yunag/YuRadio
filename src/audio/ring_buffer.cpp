#include <QDebug>

#include "ffmpeg/ring_buffer.h"

#include <algorithm>
#include <cstring>

namespace ffmpeg {

ring_buffer::ring_buffer(std::size_t capacity) : m_capacity(capacity) {
  m_buffer.resize(capacity);
}

std::size_t ring_buffer::read(char *data, std::size_t size) {
  size = std::min(size, m_size);
  const std::size_t bytes_read = std::min(size, m_capacity - m_front);

  std::memcpy(data, m_buffer.data() + m_front, bytes_read);
  std::memcpy(data + bytes_read, m_buffer.data(), size - bytes_read);

  advance_index(m_front, size);

  m_size -= size;

  return size;
}

std::size_t ring_buffer::write(const char *data, std::size_t size) {
  size = std::min(size, m_capacity - m_size);
  const std::size_t bytes_write = std::min(size, m_capacity - m_rear);

  std::memcpy(m_buffer.data() + m_rear, data, bytes_write);
  std::memcpy(m_buffer.data(), data + bytes_write, size - bytes_write);

  advance_index(m_rear, size);

  m_size += size;

  return size;
}

void ring_buffer::clear() {
  m_front = 0;
  m_rear = 0;
  m_size = 0;
}

void ring_buffer::clear(std::size_t newcapacity) {
  m_capacity = newcapacity;
  m_buffer.resize(m_capacity);
}

void ring_buffer::advance_index(std::size_t &index, std::size_t size) const {
  index = (index + size) % m_capacity;
}

std::size_t ring_buffer::capacity() const {
  return m_capacity;
}

std::size_t ring_buffer::size() const {
  return m_size;
}

}  // namespace ffmpeg
