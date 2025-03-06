#pragma once

#include "audio_common.hpp"

#include "sndfile.h"

namespace GoudaVK {

class MusicTrack {
public:
    MusicTrack();
    ~MusicTrack();

    // No copying, allow moving
    MusicTrack(const MusicTrack &) = delete;
    MusicTrack &operator=(const MusicTrack &) = delete;
    MusicTrack(MusicTrack &&other) noexcept;
    MusicTrack &operator=(MusicTrack &&other) noexcept;

    // Initialize the music track by opening the file
    bool Load(std::string_view filename);

    // Read a chunk of audio data for streaming
    size_t ReadFrames(float *buffer, size_t frames);

    // Get audio properties for buffer configuration
    SNDFILE *GetFile() const { return m_file; }
    ALenum GetFormat() const { return m_format; }
    int GetSampleRate() const { return m_sample_rate; }
    int GetChannels() const { return m_channels; }
    bool IsFinished() const { return m_finished; }

    void SetFinished(bool finished) { m_finished = finished; }

private:
    SNDFILE *m_file;
    SF_INFO m_sfinfo;
    ALenum m_format;
    int m_sample_rate;
    int m_channels;
    bool m_finished;
};

} // end namespace GoudaVK