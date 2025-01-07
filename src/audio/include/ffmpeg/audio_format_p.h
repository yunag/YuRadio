#ifndef FFMPEG_AUDIO_FORMAT_P_H
#define FFMPEG_AUDIO_FORMAT_P_H

#include "ffmpeg/audio_format.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include <cassert>

namespace ffmpeg {

inline ffmpeg::sample_format get_sample_format(AVSampleFormat format) {
  switch (format) {
    case AV_SAMPLE_FMT_U8:
      return sample_format::u8;
    case AV_SAMPLE_FMT_S16:
      return sample_format::s16;
    case AV_SAMPLE_FMT_S32:
      return sample_format::s32;
    case AV_SAMPLE_FMT_FLT:
      return sample_format::f32;
    case AV_SAMPLE_FMT_DBL:
      return sample_format::f64;
    case AV_SAMPLE_FMT_U8P:
      return sample_format::u8p;
    case AV_SAMPLE_FMT_S16P:
      return sample_format::s16p;
    case AV_SAMPLE_FMT_S32P:
      return sample_format::s32p;
    case AV_SAMPLE_FMT_FLTP:
      return sample_format::f32p;
    case AV_SAMPLE_FMT_DBLP:
      return sample_format::f64p;
    case AV_SAMPLE_FMT_S64:
      return sample_format::s64;
    case AV_SAMPLE_FMT_S64P:
      return sample_format::s64p;
    default:
      return sample_format::unknown;
  }
}

inline AVSampleFormat get_av_sample_format(ffmpeg::sample_format format) {
  switch (format) {
    case sample_format::u8:
      return AV_SAMPLE_FMT_U8;
    case sample_format::s16:
      return AV_SAMPLE_FMT_S16;
    case sample_format::s32:
      return AV_SAMPLE_FMT_S32;
    case sample_format::f32:
      return AV_SAMPLE_FMT_FLT;
    case sample_format::f64:
      return AV_SAMPLE_FMT_DBL;
    case sample_format::u8p:
      return AV_SAMPLE_FMT_U8P;
    case sample_format::s16p:
      return AV_SAMPLE_FMT_S16P;
    case sample_format::s32p:
      return AV_SAMPLE_FMT_S32P;
    case sample_format::f32p:
      return AV_SAMPLE_FMT_FLTP;
    case sample_format::f64p:
      return AV_SAMPLE_FMT_DBLP;
    case sample_format::s64:
      return AV_SAMPLE_FMT_S64;
    case sample_format::s64p:
      return AV_SAMPLE_FMT_S64P;
    case sample_format::unknown:
    default:
      return AV_SAMPLE_FMT_NONE;
  }
}

}  // namespace ffmpeg

#endif /* !FFMPEG_AUDIO_FORMAT_P_H */
