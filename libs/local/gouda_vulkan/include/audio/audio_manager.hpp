#pragma once

#include <vector>

#include "gouda_types.hpp"
#include "music_track.hpp"
#include "sound_effect.hpp"

// TODO: add paused state m_music_paused
// TODO: Test all functions
// TODO: Add ability for queued music not to play straight away
// TODO: Sort/group music and sound members

namespace GoudaVK {

struct Vec3 {
    f32 x, y, z;
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // Prevent copying and moving
    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;
    AudioManager(AudioManager &&other) = delete;
    AudioManager &operator=(AudioManager &&other) = delete;

    void Initialize();

    void PlaySoundEffect(const SoundEffect &sound, f32 volume = 1.0f, f32 pitch = 1.0f);
    void PlaySoundEffectAt(const SoundEffect &sound, const Vec3 &position, f32 volume = 1.0f, f32 pitch = 1.0f,
                           bool loop = false);

    void QueueMusic(MusicTrack &track);
    void PauseMusic();
    void ResumeMusic();
    void StopMusic();
    void StopCurrentTrack();
    void ShuffleRemainingTracks();

    void Update();

    void SetMusicPitch(f32 pitch);
    void SetMasterSoundVolume(f32 volume);
    void SetMasterMusicVolume(f32 volume);
    void SetMusicLooping(bool loop);
    void SetQueueLooping(bool loop);
    void SetListenerPosition(const Vec3 &position);
    void SetListenerVelocity(const Vec3 &velocity);

private:
    void InitializeSources(); // Helper to set up sources
    ALuint GetFreeSource();   // Get or create a free source
    void PlayNextTrack();     // Helper to start the next queued track

private:
    ALCdevice *p_device;
    ALCcontext *p_context;

    // Sound effect members
    std::vector<ALuint> m_source_pool;    // Pool of available sources for sound effects
    std::vector<ALuint> m_active_sources; // Sources currently playing sound effects

    // Music streaming members
    ALuint m_music_source;                            // Dedicated source for music playback
    std::vector<ALuint> m_music_buffers;              // Buffers for streaming music chunks
    std::vector<MusicTrack *> m_music_tracks;         // Replaces m_music_queue
    size_t m_current_index = 0;                       // Tracks the next track to play
    MusicTrack *p_current_track;                      // Current music track being streamed
    static constexpr size_t NUM_STREAM_BUFFERS = 2;   // Double buffering
    static constexpr size_t FRAMES_PER_BUFFER = 4096; // ~93ms at 44.1kHz

    f32 m_master_sound_volume; // Master volume for sound effects
    f32 m_master_music_volume; // Master volume for music

    bool m_music_looping;
    bool m_queue_looping; // Controls whether the queue loops
};

} // end namespace GoudaVK