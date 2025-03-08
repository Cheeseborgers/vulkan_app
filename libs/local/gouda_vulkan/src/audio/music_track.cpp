#include "audio/music_track.hpp"

#include <stdexcept>

#include "logger.hpp"

namespace Gouda {
namespace Audio {

MusicTrack::MusicTrack() : m_file{nullptr}, m_format{AL_NONE}, m_sample_rate{0}, m_channels{0}, m_finished{true}
{
    // Default constructor initializes to "unloaded" state
}

MusicTrack::~MusicTrack()
{
    if (m_file) {
        sf_close(m_file);
    }
}

MusicTrack::MusicTrack(MusicTrack &&other) noexcept
    : m_file{other.m_file},
      m_sfinfo{other.m_sfinfo},
      m_format{other.m_format},
      m_sample_rate{other.m_sample_rate},
      m_channels{other.m_channels},
      m_finished{other.m_finished}
{
    other.m_file = nullptr;
    other.m_finished = true;
}

MusicTrack &MusicTrack::operator=(MusicTrack &&other) noexcept
{
    if (this != &other) {
        if (m_file)
            sf_close(m_file);
        m_file = other.m_file;
        m_sfinfo = other.m_sfinfo;
        m_format = other.m_format;
        m_sample_rate = other.m_sample_rate;
        m_channels = other.m_channels;
        m_finished = other.m_finished;
        other.m_file = nullptr;
        other.m_finished = true;
    }
    return *this;
}

bool MusicTrack::Load(std::string_view filename)
{
    ENGINE_LOG_INFO("Attempting to load music track: {}", filename);
    if (m_file) {
        sf_close(m_file);
        m_file = nullptr;
    }

    m_file = sf_open(filename.data(), SFM_READ, &m_sfinfo);
    if (!m_file) {
        ENGINE_LOG_ERROR("Could not open music file {}: {}", filename, sf_strerror(nullptr));
        return false;
    }

    if (m_sfinfo.frames < 1) {
        ENGINE_LOG_ERROR("Invalid frame count: {}", m_sfinfo.frames);
        sf_close(m_file);
        m_file = nullptr;
        return false;
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
            sf_close(m_file);
            m_file = nullptr;
            return false;
        }
    }
    else {
        ENGINE_LOG_ERROR("Unsupported audio format: {:x}", m_sfinfo.format);
        sf_close(m_file);
        m_file = nullptr;
        return false;
    }

    m_sample_rate = m_sfinfo.samplerate;
    m_channels = m_sfinfo.channels;
    m_finished = false;
    ENGINE_LOG_INFO("Loaded music track: format={}, rate={}, channels={}", FormatName(m_format), m_sample_rate,
                    m_channels);
    return true;
}

size_t MusicTrack::ReadFrames(float *buffer, size_t frames)
{
    if (!m_file || m_finished) {
        ENGINE_LOG_DEBUG("Music track finished or not loaded");
        return 0;
    }

    sf_count_t read{sf_readf_float(m_file, buffer, frames)};
    if (read < static_cast<sf_count_t>(frames)) {
        m_finished = true;
        ENGINE_LOG_INFO("Reached end of music track");
    }
    // ENGINE_LOG_DEBUG("Read {} frames from music track", read);
    return static_cast<size_t>(read);
}

} // namespace Audio end
} // namespace Gouda end