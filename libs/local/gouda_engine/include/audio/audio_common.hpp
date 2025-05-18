#pragma once

#include "AL/al.h"
#include "sndfile.h"

namespace gouda::audio {

const char *FormatName(ALenum format);
bool IsValidAudioExtension(std::string_view filepath);
bool HandleError(SNDFILE *&p_file);

}