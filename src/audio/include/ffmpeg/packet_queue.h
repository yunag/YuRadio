#ifndef FFMPEG_PACKET_QUEUE_H
#define FFMPEG_PACKET_QUEUE_H

#include "ffmpeg/packet.h"

#include <condition_variable>
#include <deque>
#include <mutex>

namespace ffmpeg {

class packet_queue {
public:
  packet_queue() = default;
  packet_queue(const packet_queue &) = delete;
  packet_queue(packet_queue &&) = delete;
  packet_queue &operator=(const packet_queue &) = delete;
  packet_queue &operator=(packet_queue &&) = delete;
  ~packet_queue() = default;

  void enqueue(ffmpeg::packet &&packet);
  ffmpeg::packet dequeue();
  bool empty() const;

  void abort();
  void reset();

private:
  std::deque<ffmpeg::packet> m_packets;
  std::size_t m_maximum_packets_count = 20;
  std::size_t m_preferred_packets_count = m_maximum_packets_count / 2;

  mutable std::mutex m_mutex;
  std::condition_variable m_cond;
  bool m_abort = false;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_PACKET_QUEUE_H */
