#pragma once

#include "AL/al.h"
#include "AL/alext.h"
#include "sndfile.h"

#include "core/types.hpp"

namespace gouda {
namespace audio {

const char *FormatName(ALenum format);
bool IsValidAudioExtension(const FilePath &filepath);
bool HandleError(SNDFILE *&p_file);

} // namespace audio end
} // namespace gouda end
