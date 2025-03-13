#include "audio/sound_effect.hpp"

#include <memory>
#include <string_view>
#include <unordered_set>

#include <sndfile.h>

#include "logger.hpp"
#include "math/math.hpp"
#include "utility/filesystem.hpp"

namespace gouda {
namespace audio {

SoundEffect::SoundEffect() : m_buffer{0}
{
    // Default constructor initializes to "unloaded" state
}

SoundEffect::~SoundEffect() { alDeleteBuffers(1, &m_buffer); }

SoundEffect::SoundEffect(SoundEffect &&other) noexcept : m_buffer{other.m_buffer} { other.m_buffer = 0; }

SoundEffect &SoundEffect::operator=(SoundEffect &&other) noexcept
{
    if (this != &other) {
        // Free existing buffer
        if (m_buffer) {
            alDeleteBuffers(1, &m_buffer);
        }

        // Move ownership
        m_buffer = other.m_buffer;
        other.m_buffer = 0; // Reset other
    }
    return *this;
}

bool SoundEffect::Load(std::string_view filepath)
{
    ENGINE_LOG_DEBUG("Attempting to load sound effect: {}", filepath);

    if (filepath.empty()) {
        ENGINE_LOG_ERROR("Empty filename provided");
        return false;
    }

    if (!IsValidAudioExtension(filepath)) {
        ENGINE_LOG_ERROR("Unsupported audio type");
        return false;
    }

    if (m_buffer) {
        alDeleteBuffers(1, &m_buffer);
        m_buffer = 0;
    }

    ALenum format;
    SF_INFO sfinfo;
    SNDFILE *p_sndfile{sf_open(filepath.data(), SFM_READ, &sfinfo)};
    if (!p_sndfile) {
        ENGINE_LOG_ERROR("Could not open audio in {}: {}", filepath, sf_strerror(nullptr));
        return false;
    }

    if (!alcGetCurrentContext()) {
        ENGINE_LOG_ERROR("No OpenAL context current before buffer creation");
        return HandleError(p_sndfile);
    }

    ENGINE_LOG_DEBUG("File info: frames={}, channels={}, samplerate={}, format={:x}", sfinfo.frames, sfinfo.channels,
                     sfinfo.samplerate, sfinfo.format);

    if (sfinfo.frames < 1) {
        ENGINE_LOG_ERROR("Bad sample count in {} ({} frames)", filepath, sfinfo.frames);
        return HandleError(p_sndfile);
    }

    // Format detection
    bool is_float_format = false;
    if ((sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_PCM_16 ||
        (sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_PCM_24 ||
        (sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_PCM_32) {
        // Int16 format
        is_float_format = false;
    }
    else if ((sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT ||
             (sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_OPUS ||
             (sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_MPEG_LAYER_III) {
        // Float format
        if (alIsExtensionPresent("AL_EXT_FLOAT32")) {
            is_float_format = true;
        }
        else {
            ENGINE_LOG_ERROR("Float format not supported by OpenAL (AL_EXT_FLOAT32 missing)");
            return HandleError(p_sndfile);
        }
    }
    else {
        ENGINE_LOG_ERROR("Unsupported audio format in {}: {:x}", filepath, sfinfo.format);
        return HandleError(p_sndfile);
    }

    // Determine the OpenAL format based on channels and sample format
    if (sfinfo.channels == 1) {
        format = is_float_format ? AL_FORMAT_MONO_FLOAT32 : AL_FORMAT_MONO16;
    }
    else if (sfinfo.channels == 2) {
        format = is_float_format ? AL_FORMAT_STEREO_FLOAT32 : AL_FORMAT_STEREO16;
    }
    else {
        ENGINE_LOG_ERROR("Unsupported channel count: {}", sfinfo.channels);
        return HandleError(p_sndfile);
    }

    size_t sample_size{is_float_format ? 4 : 2};
    size_t alloc_size{static_cast<size_t>(sfinfo.frames) * sfinfo.channels * sample_size};
    ENGINE_LOG_DEBUG("Allocating {} bytes ({}kb) for audio data", alloc_size, math::bytes_to_kb(alloc_size));

    auto membuf{std::unique_ptr<void, void (*)(void *)>(malloc(alloc_size), free)};
    if (!membuf) {
        ENGINE_LOG_ERROR("Memory allocation failed for audio buffer (size: {})", alloc_size);
        return HandleError(p_sndfile);
    }

    sf_count_t num_frames = 0;
    if (is_float_format) {
        num_frames = sf_readf_float(p_sndfile, static_cast<f32 *>(membuf.get()), sfinfo.frames);
    }
    else {
        num_frames = sf_readf_short(p_sndfile, static_cast<short *>(membuf.get()), sfinfo.frames);
    }

    if (num_frames < 1) {
        ENGINE_LOG_ERROR("Failed to read samples in {} ({} frames returned)", filepath, num_frames);
        return HandleError(p_sndfile);
    }

    ALsizei num_bytes = static_cast<ALsizei>(num_frames * sfinfo.channels * sample_size);
    ENGINE_LOG_DEBUG("Read {} frames, {} bytes ({}kb)", num_frames, num_bytes, math::bytes_to_kb(num_bytes));

    alGenBuffers(1, &m_buffer);
    alBufferData(m_buffer, format, membuf.get(), num_bytes, sfinfo.samplerate);

    ALenum result{alGetError()};
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("OpenAL Error: {}", alGetString(result));
        alDeleteBuffers(1, &m_buffer);
        m_buffer = 0;
    }
    else {
        ENGINE_LOG_DEBUG("Created sound effect from: {}, format: {}", filepath, FormatName(format));
    }

    sf_close(p_sndfile);

    return true;
}

} // namespace audio end
} // namespace gouda end
