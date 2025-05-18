#pragma once

#include "audio_common.hpp"

#include "sndfile.h"

namespace gouda::audio {

/**
 * @class MusicTrack
 * @brief Represents a music track for streaming audio.
 *
 * This class is used for loading, reading, and managing a music track's audio data.
 * It interfaces with the `libsndfile` library to read audio files and prepares audio data
 * for playback through OpenAL.
 */
class MusicTrack {
public:
    /**
     * @brief Default constructor for MusicTrack.
     *
     * Initializes the music track object, but does not load any data.
     */
    MusicTrack();

    /**
     * @brief Destructor for MusicTrack.
     *
     * Frees any resources associated with the music track, such as the opened file.
     */
    ~MusicTrack();

    // No copying, allow moving
    MusicTrack(const MusicTrack &) = delete;            ///< Deleted copy constructor to prevent copying.
    MusicTrack &operator=(const MusicTrack &) = delete; ///< Deleted copy assignment operator.

    /**
     * @brief Move constructor for MusicTrack.
     *
     * Transfers ownership of the underlying resources from another MusicTrack object.
     *
     * @param other The MusicTrack object to move from.
     */
    MusicTrack(MusicTrack &&other) noexcept;

    /**
     * @brief Move assignment operator for MusicTrack.
     *
     * Transfers ownership of the underlying resources from another MusicTrack object.
     *
     * @param other The MusicTrack object to move from.
     * @return A reference to this object.
     */
    MusicTrack &operator=(MusicTrack &&other) noexcept;

    /**
     * @brief Loads a music track from a file.
     *
     * This function opens the music track file, retrieves its audio properties, and prepares it for streaming.
     *
     * @param filepath The path to the music file to load.
     * @return True if the music track was loaded successfully, false otherwise.
     */
    bool Load(std::string_view filepath);

    /**
     * @brief Reads a chunk of audio data for streaming.
     *
     * This function reads the specified number of frames from the audio file and stores it in the provided buffer.
     *
     * @param buffer The buffer to store the read audio data.
     * @param frames The number of frames to read from the file.
     * @return The number of frames actually read from the file.
     */
    size_t ReadFrames(float *buffer, size_t frames);

    /**
     * @brief Gets the audio file handle for the track.
     *
     * This function provides access to the underlying file handle used by `libsndfile` to read the audio data.
     *
     * @return The file handle (`SNDFILE *`) for the track.
     */
    [[nodiscard]] SNDFILE *GetFile() const { return p_sndfile; }

    /**
     * @brief Gets the audio format for the track.
     *
     * This function provides the audio format used by OpenAL for playback (e.g., `AL_FORMAT_STEREO16`).
     *
     * @return The audio format (`ALenum`) of the track.
     */
    [[nodiscard]] ALenum GetFormat() const { return m_format; }

    /**
     * @brief Gets the sample rate of the music track.
     *
     * This function provides the sample rate (samples per second) of the track.
     *
     * @return The sample rate (in Hz).
     */
    [[nodiscard]] int GetSampleRate() const { return m_sample_rate; }

    /**
     * @brief Gets the number of channels in the music track.
     *
     * This function provides the number of channels in the track (e.g., 1 for mono, 2 for stereo).
     *
     * @return The number of channels (e.g., 1 or 2).
     */
    [[nodiscard]] int GetChannels() const { return m_channels; }

    /**
     * @brief Checks if the music track has finished playing.
     *
     * This function checks if the track has finished streaming.
     *
     * @return True if the track is finished, false otherwise.
     */
    [[nodiscard]] bool IsFinished() const { return m_finished; }

    /**
     * @brief Sets the finished status of the music track.
     *
     * This function sets whether the track is finished or not. This is useful to mark when the track has finished
     * streaming.
     *
     * @param finished The status to set (true if finished, false otherwise).
     */
    void SetFinished(const bool finished) { m_finished = finished; }

private:
    SNDFILE *p_sndfile; ///< The file handle for the audio file.
    SF_INFO m_sfinfo{};   ///< Information structure containing the file's metadata (e.g., sample rate, channels).
    ALenum m_format;    ///< The audio format used by OpenAL.
    int m_sample_rate;  ///< The sample rate of the audio track.
    int m_channels;     ///< The number of channels (mono or stereo).
    bool m_finished;    ///< Whether the track has finished playing or streaming.
};

}