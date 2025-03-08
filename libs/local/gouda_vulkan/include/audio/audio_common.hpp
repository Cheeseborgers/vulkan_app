#pragma once

#include <string_view>

#include "AL/al.h"
#include "AL/alext.h"

namespace Gouda {
namespace Audio {

static const char *FormatName(ALenum format)
{
    switch (format) {
        case AL_FORMAT_MONO16:
            return "Mono, S16";
        case AL_FORMAT_MONO_FLOAT32:
            return "Mono, Float32";
        case AL_FORMAT_STEREO16:
            return "Stereo, S16";
        case AL_FORMAT_STEREO_FLOAT32:
            return "Stereo, Float32";
    }
    return "Unknown Format";
}

} // namespace Audio end
} // namespace Gouda end
