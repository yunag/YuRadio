/**
 * @file
 * @brief TODO: Implement
 */

#include "ffmpeg/audio_device.h"

namespace ffmpeg {

class audio_device_private {
public:
};

audio_device::audio_device() : d(make_pimpl<audio_device_private>()) {};

audio_device::~audio_device() = default;

bool audio_device::is_null() const {
  return true;
}

}  // namespace ffmpeg
