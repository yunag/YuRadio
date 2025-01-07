add_library(
  FFmpegPlayer STATIC
  player.cpp
  demuxer.cpp
  muxer.cpp
  decoder.cpp
  error.cpp
  frame.cpp
  packet.cpp
  packet_queue.cpp
  ring_buffer.cpp
  audio_device.cpp
  audio_resampler.cpp
  audio_format.cpp
  miniaudio_audio_output.cpp)

target_sources(
  FFmpegPlayer
  PUBLIC include/ffmpeg/audio_device.h
         include/ffmpeg/audio_format.h
         include/ffmpeg/audio_format_p.h
         include/ffmpeg/audio_resampler.h
         include/ffmpeg/miniaudio_audio_output.h
         include/ffmpeg/decoder.h
         include/ffmpeg/demuxer.h
         include/ffmpeg/error.h
         include/ffmpeg/frame.h
         include/ffmpeg/maybe.h
         include/ffmpeg/muxer.h
         include/ffmpeg/packet.h
         include/ffmpeg/packet_queue.h
         include/ffmpeg/pimpl.h
         include/ffmpeg/player.h
         include/ffmpeg/ring_buffer.h
         include/ffmpeg/scope_guard.h
         include/ffmpeg/utils.h)

include(CompilerWarnings)
target_set_warnings(FFmpegPlayer)

target_compile_features(FFmpegPlayer PUBLIC cxx_std_20)
set_target_properties(
  FFmpegPlayer
  PROPERTIES CXX_SCAN_FOR_MODULES OFF
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF)

find_package(FFmpeg REQUIRED COMPONENTS avcodec avformat avutil avfilter
                                        swresample)

# NOTE: Qt dependencies only needed for logging
find_package(Qt6 REQUIRED COMPONENTS Core)

target_link_libraries(
  FFmpegPlayer
  PRIVATE FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil FFmpeg::avfilter
          FFmpeg::swresample
  PRIVATE Qt6::Core miniaudio)
target_include_directories(FFmpegPlayer
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
