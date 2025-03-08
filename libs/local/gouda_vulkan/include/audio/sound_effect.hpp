#pragma once

#include "audio_common.hpp"

namespace Gouda {
namespace Audio {

/**
 * @class SoundEffect
 * @brief Represents a sound effect that can be loaded and played using OpenAL.
 *
 * This class encapsulates a sound effect buffer that can be loaded from a file and used for playback.
 * It manages the underlying OpenAL buffer, providing easy access to it.
 */
class SoundEffect {
public:
    /**
     * @brief Default constructor for SoundEffect.
     *
     * Initializes the sound effect object, but does not load any data.
     */
    SoundEffect();

    /**
     * @brief Destructor for SoundEffect.
     *
     * Frees any resources associated with the sound effect, such as the OpenAL buffer.
     */
    ~SoundEffect();

    // No copying, allow moving
    SoundEffect(const SoundEffect &) = delete;            ///< Deleted copy constructor to prevent copying.
    SoundEffect &operator=(const SoundEffect &) = delete; ///< Deleted copy assignment operator.

    /**
     * @brief Move constructor for SoundEffect.
     *
     * Transfers ownership of the underlying resources from another SoundEffect object.
     *
     * @param other The SoundEffect object to move from.
     */
    SoundEffect(SoundEffect &&other) noexcept;

    /**
     * @brief Move assignment operator for SoundEffect.
     *
     * Transfers ownership of the underlying resources from another SoundEffect object.
     *
     * @param other The SoundEffect object to move from.
     * @return A reference to this object.
     */
    SoundEffect &operator=(SoundEffect &&other) noexcept;

    /**
     * @brief Loads a sound effect from a file.
     *
     * This function loads the sound effect data from the specified file and stores it in an OpenAL buffer.
     *
     * @param filename The path to the sound file to load.
     * @return True if the sound effect was loaded successfully, false otherwise.
     */
    bool Load(std::string_view filename);

    /**
     * @brief Gets the OpenAL buffer for the sound effect.
     *
     * This function provides access to the internal OpenAL buffer, which can be used to play the sound.
     *
     * @return The OpenAL buffer identifier.
     */
    ALuint GetBuffer() const { return m_buffer; }

private:
    ALuint m_buffer; ///< OpenAL buffer that stores the sound effect data.
};

} // namespace Audio end
} // namespace Gouda end
