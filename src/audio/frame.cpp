#include "ffmpeg/frame.h"
#include "ffmpeg/audio_format_p.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <cassert>
#include <utility>

namespace ffmpeg {

frame::frame() : m_frame(av_frame_alloc()) {}

frame::~frame() {
  av_frame_free(&m_frame);
}

frame::frame(const frame &other) : frame() {
  av_frame_ref(m_frame, other.m_frame);
};

frame &frame::operator=(const frame &other) {
  frame(other).swap(*this);
  return *this;
};

frame::frame(frame &&other) noexcept : m_frame(other.m_frame) {
  other.m_frame = nullptr;
};

frame &frame::operator=(frame &&other) noexcept {
  frame(std::move(other)).swap(*this);
  return *this;
};

audio_format frame::audio_format() const {
  ffmpeg::audio_format fmt;
  fmt.sample_rate = m_frame->sample_rate;
  fmt.channel_count = m_frame->ch_layout.nb_channels;
  fmt.sample_format =
    get_sample_format(static_cast<AVSampleFormat>(m_frame->format));

  return fmt;
}

AVFrame *frame::avframe() {
  return m_frame;
}

const AVFrame *frame::avframe() const {
  return m_frame;
}

void frame::unref() {
  av_frame_unref(m_frame);
}

void frame::swap(frame &other) noexcept {
  std::swap(m_frame, other.m_frame);
}

}  // namespace ffmpeg
