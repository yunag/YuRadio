#ifndef FFMPEG_PACKET_H
#define FFMPEG_PACKET_H

#include <chrono>

struct AVPacket;

namespace ffmpeg {

class packet_private;

class packet {
public:
  packet();
  explicit packet(AVPacket *avpacket);

  packet(const packet &);
  packet(packet &&) noexcept;
  packet &operator=(const packet &);
  packet &operator=(packet &&) noexcept;
  ~packet();

  std::chrono::duration<double> duration();
  void unref();

  const AVPacket *avpacket() const;
  AVPacket *avpacket();

  bool is_null() { return !m_packet; }

protected:
  void swap(packet &other) noexcept;

private:
  AVPacket *m_packet;
};

}  // namespace ffmpeg

#endif /* !FFMPEG_PACKET_H */
