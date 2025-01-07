#include "ffmpeg/packet_queue.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace ffmpeg {

void packet_queue::enqueue(ffmpeg::packet &&packet) {
  {
    std::unique_lock locker(m_mutex);

    if (m_packets.size() >= m_maximum_packets_count) {
      m_cond.wait(locker, [this]() {
        return m_packets.size() <= m_preferred_packets_count || m_abort;
      });

      if (m_abort) {
        return;
      }
    }

    m_packets.push_back(std::move(packet));
  }

  m_cond.notify_one();
}

ffmpeg::packet packet_queue::dequeue() {
  ffmpeg::packet packet;

  {
    std::unique_lock locker(m_mutex);

    m_cond.wait(locker, [this]() {
      return !m_packets.empty() || m_abort;
    });

    if (m_abort) {
      return ffmpeg::packet(nullptr);
    }

    packet = std::move(m_packets.front());
    m_packets.pop_front();
  }

  m_cond.notify_one();

  return packet;
}

void packet_queue::abort() {
  {
    const std::unique_lock locker(m_mutex);
    m_abort = true;
  }

  m_cond.notify_all();
}

void packet_queue::reset() {
  const std::unique_lock locker(m_mutex);

  m_packets.clear();

  m_abort = false;
}

bool packet_queue::empty() const {
  const std::unique_lock locker(m_mutex);
  return m_packets.empty();
}

}  // namespace ffmpeg
