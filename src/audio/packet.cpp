#include "ffmpeg/packet.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <cassert>
#include <utility>

namespace ffmpeg {

packet::packet(AVPacket *avpacket) : m_packet(avpacket) {}

packet::packet() : m_packet(av_packet_alloc()) {}

packet::~packet() {
  /* NOTE: It's safe to free a nullptr */
  av_packet_free(&m_packet);
}

packet::packet(const packet &other) : packet() {
  av_packet_ref(m_packet, other.m_packet);
};

packet::packet(packet &&other) noexcept : m_packet(other.m_packet) {
  other.m_packet = nullptr;
}

packet &packet::operator=(const packet &other) {
  packet(other).swap(*this);
  return *this;
};

packet &packet::operator=(packet &&other) noexcept {
  packet(std::move(other)).swap(*this);
  return *this;
};

void packet::unref() {
  assert(!is_null());
  av_packet_unref(m_packet);
}

AVPacket *packet::avpacket() {
  return m_packet;
}

const AVPacket *packet::avpacket() const {
  return m_packet;
}

void packet::swap(packet &other) noexcept {
  std::swap(m_packet, other.m_packet);
}

std::chrono::duration<double> packet::duration() {
  assert(m_packet != nullptr);
  return std::chrono::duration<double>(static_cast<double>(m_packet->duration) *
                                       av_q2d(m_packet->time_base));
}

}  // namespace ffmpeg
