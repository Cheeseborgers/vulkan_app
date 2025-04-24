#pragma once

#include "AL/al.h"
#include "AL/alext.h"
#include "sndfile.h"

#include "core/types.hpp"

namespace gouda {
namespace audio {

const char *FormatName(ALenum format);
bool IsValidAudioExtension(std::string_view filepath);
bool HandleError(SNDFILE *&p_file);

} // namespace audio end
} // namespace gouda end
