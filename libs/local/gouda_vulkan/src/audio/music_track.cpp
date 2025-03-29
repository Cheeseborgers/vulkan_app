#include "audio/music_track.hpp"

#include <stdexcept>

#include "debug/logger.hpp"

namespace gouda {
namespace audio {

MusicTrack::MusicTrack() : p_sndfile{nullptr}, m_format{AL_NONE}, m_sample_rate{0}, m_channels{0}, m_finished{true}
{
    // Default constructor initializes to "unloaded" state
}

MusicTrack::~MusicTrack()
{
    if (p_sndfile) {
        sf_close(p_sndfile);
    }
}

MusicTrack::MusicTrack(MusicTrack &&other) noexcept
    : p_sndfile{other.p_sndfile},
      m_sfinfo{other.m_sfinfo},
      m_format{other.m_format},
      m_sample_rate{other.m_sample_rate},
      m_channels{other.m_channels},
      m_finished{other.m_finished}
{
    other.p_sndfile = nullptr;
    other.m_finished = true;
}

MusicTrack &MusicTrack::operator=(MusicTrack &&other) noexcept
{
    if (this != &other) {
        if (p_sndfile) {
            sf_close(p_sndfile);
        }
        p_sndfile = other.p_sndfile;
        m_sfinfo = other.m_sfinfo;
        m_format = other.m_format;
        m_sample_rate = other.m_sample_rate;
        m_channels = other.m_channels;
        m_finished = other.m_finished;
        other.p_sndfile = nullptr;
        other.m_finished = true;
    }
    return *this;
}

bool MusicTrack::Load(std::string_view filepath)
{
    ENGINE_LOG_INFO("Attempting to load music track: {}", filepath);

    if (filepath.empty()) {
        ENGINE_LOG_ERROR("Empty filepath provided");
        return false;
    }

    if (!IsValidAudioExtension(filepath)) {
        ENGINE_LOG_ERROR("Unsupported audio type: {}", filepath);
        return false;
    }

    if (p_sndfile) {
        sf_close(p_sndfile);
        p_sndfile = nullptr;
    }

    p_sndfile = sf_open(filepath.data(), SFM_READ, &m_sfinfo);
    if (!p_sndfile) {
        ENGINE_LOG_ERROR("Could not open music file {}: {}", filepath, sf_strerror(nullptr));
        return false;
    }

    if (m_sfinfo.frames < 1) {
        ENGINE_LOG_ERROR("Invalid frame count: {}", m_sfinfo.frames);
        return HandleError(p_sndfile);
    }

    // Format detection
    if ((m_sfinfo.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_WAV ||
        (m_sfinfo.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_MPEG) {
        if (m_sfinfo.channels == 1) {
            m_format = AL_FORMAT_MONO_FLOAT32;
        }
        else if (m_sfinfo.channels == 2) {
            m_format = AL_FORMAT_STEREO_FLOAT32;
        }
        else {
            ENGINE_LOG_ERROR("Unsupported channel count: {}", m_sfinfo.channels);
            return HandleError(p_sndfile);
        }
    }
    else {
        ENGINE_LOG_ERROR("Unsupported audio format: {:x}", m_sfinfo.format);
        return HandleError(p_sndfile);
    }

    m_sample_rate = m_sfinfo.samplerate;
    m_channels = m_sfinfo.channels;
    m_finished = false;
    ENGINE_LOG_DEBUG("Loaded music track: format={}, rate={}, channels={}", FormatName(m_format), m_sample_rate,
                     m_channels);
    return true;
}

size_t MusicTrack::ReadFrames(float *buffer, size_t frames)
{
    if (!p_sndfile || m_finished) {
        ENGINE_LOG_DEBUG("Music track finished or not loaded");
        return 0;
    }

    sf_count_t read{sf_readf_float(p_sndfile, buffer, frames)};
    if (read < static_cast<sf_count_t>(frames)) {
        m_finished = true;
        ENGINE_LOG_DEBUG("Reached end of music track");
    }
    // ENGINE_LOG_DEBUG("Read {} frames from music track", read);
    return static_cast<size_t>(read);
}

} // namespace audio end
} // namespace gouda end