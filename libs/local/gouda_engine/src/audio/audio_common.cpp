#include "audio/audio_common.hpp"

#include <alext.h>

#include <string_view>
#include <unordered_set>

#include "utils/filesystem.hpp"

namespace gouda::audio {

const char *FormatName(ALenum format)
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
        default:
            return "Unknown Format";
    }
}

bool IsValidAudioExtension(std::string_view filepath)
{
    static const std::unordered_set<std::string> valid_extensions{".mp3", ".wav"};
    return valid_extensions.contains(fs::GetFileExtension(filepath));
}

bool HandleError(SNDFILE *&p_file)
{
    sf_close(p_file);
    p_file = nullptr;
    return false;
}

}