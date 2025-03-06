#pragma once

#include "audio_common.hpp"

namespace GoudaVK {

class SoundEffect {
public:
    SoundEffect();
    ~SoundEffect();

    // No copying, allow moving
    SoundEffect(const SoundEffect &) = delete;
    SoundEffect &operator=(const SoundEffect &) = delete;
    SoundEffect(SoundEffect &&other) noexcept;
    SoundEffect &operator=(SoundEffect &&other) noexcept;

    bool Load(std::string_view filename);

    ALuint GetBuffer() const { return m_buffer; }

private:
    ALuint m_buffer;
};

} // end namespace GoudaVK
