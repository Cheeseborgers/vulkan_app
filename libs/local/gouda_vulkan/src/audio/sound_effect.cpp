#include "audio/sound_effect.hpp"

#include <filesystem>
#include <memory>

#include <sndfile.h>

#include "logger.hpp"

namespace GoudaVK {

SoundEffect::SoundEffect() : m_buffer{0}
{
    // Default constructor initializes to "unloaded" state
}

SoundEffect::~SoundEffect() { alDeleteBuffers(1, &m_buffer); }

SoundEffect::SoundEffect(SoundEffect &&other) noexcept : m_buffer{other.m_buffer} { other.m_buffer = 0; }

SoundEffect &SoundEffect::operator=(SoundEffect &&other) noexcept
{
    if (this != &other) {
        if (m_buffer)
            alDeleteBuffers(1, &m_buffer);
        m_buffer = other.m_buffer;
        other.m_buffer = 0;
    }
    return *this;
}

bool SoundEffect::Load(std::string_view filename)
{
    ENGINE_LOG_INFO("Attempting to load sound effect: {}", filename);

    if (m_buffer) {
        alDeleteBuffers(1, &m_buffer); // Clear any existing buffer
        m_buffer = 0;
    }

    enum FormatType { Int16, Float };
    FormatType sample_format{Int16};
    ALenum format;
    SF_INFO sfinfo;

    // Ensure filename is valid
    if (filename.empty()) {
        ENGINE_LOG_ERROR("Empty filename provided");
        return false;
    }

    SNDFILE *sndfile = sf_open(filename.data(), SFM_READ, &sfinfo);
    if (!sndfile) {
        ENGINE_LOG_ERROR("Could not open audio in {}: {}", filename, sf_strerror(nullptr));
        return false;
    }

    if (!alcGetCurrentContext()) {
        ENGINE_LOG_ERROR("No OpenAL context current before buffer creation");
        sf_close(sndfile);
        return false;
    }

    ENGINE_LOG_DEBUG("File info: frames={}, channels={}, samplerate={}, format={:x}", sfinfo.frames, sfinfo.channels,
                     sfinfo.samplerate, sfinfo.format);

    if (sfinfo.frames < 1) {
        ENGINE_LOG_ERROR("Bad sample count in {} ({} frames)", filename, sfinfo.frames);
        sf_close(sndfile);
        return false;
    }

    // Format detection
    if ((sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_PCM_16 ||
        (sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_PCM_24 ||
        (sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_PCM_32) {
        sample_format = Int16;
    }
    else if ((sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT ||
             (sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_OPUS ||
             (sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_MPEG_LAYER_III) {
        if (alIsExtensionPresent("AL_EXT_FLOAT32")) {
            sample_format = Float;
        }
        else {
            ENGINE_LOG_ERROR("Float format not supported by OpenAL (AL_EXT_FLOAT32 missing)");
            sf_close(sndfile);
            return false;
        }
    }
    else {
        ENGINE_LOG_ERROR("Unsupported audio format in {}: {:x}", filename, sfinfo.format);
        sf_close(sndfile);
        return false;
    }

    if (sfinfo.channels == 1) {
        format = (sample_format == Int16) ? AL_FORMAT_MONO16 : AL_FORMAT_MONO_FLOAT32;
    }
    else if (sfinfo.channels == 2) {
        format = (sample_format == Int16) ? AL_FORMAT_STEREO16 : AL_FORMAT_STEREO_FLOAT32;
    }
    else {
        ENGINE_LOG_ERROR("Unsupported channel count: {}", sfinfo.channels);
        sf_close(sndfile);
        return false;
    }

    size_t sample_size{(sample_format == Int16) ? 2 : 4};
    size_t alloc_size{static_cast<size_t>(sfinfo.frames) * sfinfo.channels * sample_size};
    ENGINE_LOG_DEBUG("Allocating {} bytes for audio data", alloc_size);

    auto membuf{std::unique_ptr<void, void (*)(void *)>(malloc(alloc_size), free)};
    if (!membuf) {
        ENGINE_LOG_ERROR("Memory allocation failed for audio buffer (size: {})", alloc_size);
        sf_close(sndfile);
        return false;
    }

    sf_count_t num_frames;
    if (sample_format == Int16) {
        ENGINE_LOG_DEBUG("Reading as Int16");
        num_frames = sf_readf_short(sndfile, static_cast<short *>(membuf.get()), sfinfo.frames);
    }
    else {
        ENGINE_LOG_DEBUG("Reading as Float");
        num_frames = sf_readf_float(sndfile, static_cast<float *>(membuf.get()), sfinfo.frames);
    }

    if (num_frames < 1) {
        ENGINE_LOG_ERROR("Failed to read samples in {} ({} frames returned)", filename, num_frames);
        sf_close(sndfile);
        return false;
    }

    ALsizei num_bytes{static_cast<ALsizei>(num_frames * sfinfo.channels * sample_size)};
    ENGINE_LOG_DEBUG("Read {} frames, {} bytes", num_frames, num_bytes);

    alGenBuffers(1, &m_buffer);
    alBufferData(m_buffer, format, membuf.get(), num_bytes, sfinfo.samplerate);

    ALenum err = alGetError();
    if (err != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("OpenAL Error: {}", alGetString(err));
        alDeleteBuffers(1, &m_buffer);
        m_buffer = 0;
    }
    else {
        ENGINE_LOG_INFO("Created sound effect from: {}, format: {}", filename, FormatName(format));
    }

    sf_close(sndfile);

    return true;
}

} // end namespace GoudaVK
