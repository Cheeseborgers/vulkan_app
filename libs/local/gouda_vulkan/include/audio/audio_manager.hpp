#pragma once

#include <vector>

#include "core/types.hpp"
#include "music_track.hpp"
#include "sound_effect.hpp"

// TODO: Add paused state m_music_paused
// TODO: Test all functions
// TODO: Sort/group music and sound members
// TODO: Use allocator for member data
// TODO: Add clear queue if needed (for level/state changes)
// TODO: Add assertions to all audio classes and functions
// TODO: Move each audio effect init logic to own seperate functions?
// TODO: Add comments to the rest of functions and members in this file

namespace gouda {
namespace audio {

/**
 * @brief Represents a 3D vector used for sound positioning.
 */
struct Vec3 {
    f32 x; ///< X-coordinate of the vector.
    f32 y; ///< Y-coordinate of the vector.
    f32 z; ///< Z-coordinate of the vector.
};

enum class AudioEffectType : u8 { Reverb, Echo, Distortion, Chorus, Flanger, None };

struct AudioEffects {
    struct EffectData {
        ALuint effect_id{0};
        ALuint slot_id{0};
    };

    EffectData reverb;
    EffectData echo;
    EffectData distortion;
    EffectData chorus;
    EffectData flanger;

    EffectData &operator[](AudioEffectType type)
    {
        switch (type) {
            case AudioEffectType::Reverb:
                return reverb;
            case AudioEffectType::Echo:
                return echo;
            case AudioEffectType::Distortion:
                return distortion;
            case AudioEffectType::Chorus:
                return chorus;
            case AudioEffectType::Flanger:
                return flanger;
            case AudioEffectType::None:
                [[fallthrough]];
            default:
                static EffectData nullEffect; // Default to a null effect for safety
                return nullEffect;
        }
    }
};

/**
 * @class AudioManager
 * @brief Manages audio playback including sound effects and music tracks.
 *
 * The AudioManager class allows for playing sound effects, music tracks, and managing the audio settings.
 * It supports functionalities like volume control, pitch adjustments, music looping, and positioning of sound effects
 * in 3D space.
 */
class AudioManager {
public:
    /**
     * @brief Default constructor for AudioManager.
     */
    AudioManager();

    /**
     * @brief Destructor for AudioManager.
     */
    ~AudioManager();

    // Prevent copying and moving
    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;
    AudioManager(AudioManager &&other) = delete;
    AudioManager &operator=(AudioManager &&other) = delete;

    /**
     * @brief Initializes the audio system.
     *
     * This function sets up the OpenAL device and context, prepares audio sources, and initializes buffers for music
     * streaming.
     */
    void Initialize();

    /**
     * @brief Plays a sound effect.
     *
     * @param sound The sound effect to play.
     * @param effects Audio effects to apply to the sound (Default is empty).
     * @param volume Volume of the sound (default is 1.0f).
     * @param pitch Pitch of the sound (default is 1.0f).
     *
     * @note This function plays the sound effect using a free audio source from the pool.
     */
    void PlaySoundEffect(const SoundEffect &sound, const std::vector<AudioEffectType> &effects = {}, f32 volume = 1.0f,
                         f32 pitch = 1.0f);

    /**
     * @brief Plays a sound effect at a specific 3D position.
     *
     * @param sound The sound effect to play.
     * @param position The 3D position where the sound will be played.
     * @param effects Audio effects to apply to the sound (Default is empty).
     * @param volume Volume of the sound (default is 1.0f).
     * @param pitch Pitch of the sound (default is 1.0f).
     * @param loop Whether the sound should loop (default is false).
     *
     * @note This function plays the sound effect at the given position using a free audio source.
     */
    void PlaySoundEffectAt(const SoundEffect &sound, const Vec3 &position,
                           const std::vector<AudioEffectType> &effects = {}, f32 volume = 1.0f, f32 pitch = 1.0f,
                           bool loop = false);

    /**
     * @brief Queues a music track for playback.
     *
     * @param track The music track to queue.
     * @param play_immediately Whether the music should start playing immediately (default is true).
     */
    void QueueMusic(MusicTrack &track, bool play_immediately = true);

    /**
     * @brief Starts playing the queued music track.
     */
    void PlayMusic();

    /**
     * @brief Pauses the currently playing music.
     */
    void PauseMusic();

    /**
     * @brief Resumes the paused music.
     */
    void ResumeMusic();

    /**
     * @brief Stops the currently playing music.
     */
    void StopMusic();

    /**
     * @brief Stops the current music track and goes back to the first track in the queue.
     */
    void StopCurrentTrack();

    /**
     * @brief Shuffles the remaining music tracks in the queue.
     */
    void ShuffleRemainingTracks();

    /**
     * @brief Updates the audio system (should be called every frame).
     *
     * This function updates the state of music playback and checks for buffer updates.
     */
    void Update();

    /**
     * @brief Sets the pitch of the currently playing music.
     *
     * @param pitch The pitch factor to set.
     */
    void SetMusicPitch(f32 pitch);

    /**
     * @brief Sets the master volume for sound effects.
     *
     * @param volume The master volume to set (range [0.0f, 1.0f]).
     */
    void SetMasterSoundVolume(f32 volume);

    /**
     * @brief Sets the master volume for music tracks.
     *
     * @param volume The master volume to set (range [0.0f, 1.0f]).
     */
    void SetMasterMusicVolume(f32 volume);

    /**
     * @brief Enables or disables music looping.
     *
     * @param loop Whether the music should loop when it finishes (default is false).
     */
    void SetMusicLooping(bool loop);

    /**
     * @brief Enables or disables queue looping.
     *
     * @param loop Whether the music queue should loop when all tracks have been played (default is false).
     */
    void SetQueueLooping(bool loop);

    /**
     * @brief Sets the listener's position in 3D space.
     *
     * @param position The new position of the listener.
     */
    void SetListenerPosition(const Vec3 &position);

    /**
     * @brief Sets the listener's velocity in 3D space.
     *
     * @param velocity The new velocity of the listener.
     */
    void SetListenerVelocity(const Vec3 &velocity);

private:
    /**
     * @brief Loads OpenAL function pointer. This must be called before using audio effects
     *
     */
    void LoadAudioEffectFunctions();

    /**
     * @brief Initializes audio sources.
     *
     * This function prepares audio sources for playback and sets them up with the necessary settings.
     */
    void InitializeSources();

    /**
     * @brief Initializes audio effects.
     *
     * This function prepares audio effects for playback and sets them up with the necessary settings.
     */
    void InitializeAudioEffects();

    /**
     * @brief Cleans up a single audio effect.
     *
     * @param effect_id The audio effect identifier.
     * @param slot_id The audio effect slot identifier.
     * @param name The audio effect name.
     */
    void CleanupEffect(ALuint effect_id, ALuint slot_id, std::string_view name);

    /**
     * @brief Destroys and free resources for all loaded OpenAL effects.
     *
     * This function deletes the effects and auxilary effect slots
     */
    void DestroyAudioEffects();

    /**
     * @brief Sets audio effects on the given audio source.
     *
     * @param source The audio source to apply effects on.
     * @param effects A vector of effects to apply to the source.
     */
    void ApplyAudioEffects(ALuint source, const std::vector<AudioEffectType> &effects);

    /**
     * @brief Gets a free audio source.
     *
     * @return A free audio source that can be used for sound effects.
     */
    ALuint GetFreeSource();

    /**
     * @brief Plays the next track in the queue.
     *
     * This function starts the playback of the next music track in the queue.
     */
    void PlayNextTrack();

private:
    // Effect function pointers
    typedef void(AL_APIENTRY *LPALGENEFFECTS)(ALsizei, ALuint *);
    typedef void(AL_APIENTRY *LPALDELETEEFFECTS)(ALsizei, const ALuint *);
    typedef void(AL_APIENTRY *LPALEFFECTI)(ALuint, ALenum, ALint);
    typedef void(AL_APIENTRY *LPALEFFECTF)(ALuint, ALenum, ALfloat);
    typedef void(AL_APIENTRY *LPALEFFECTFV)(ALuint, ALenum, const ALfloat *);
    typedef void(AL_APIENTRY *LPALGENAUXILIARYEFFECTSLOTS)(ALsizei, ALuint *);
    typedef void(AL_APIENTRY *LPALDELETEAUXILIARYEFFECTSLOTS)(ALsizei, const ALuint *);
    typedef void(AL_APIENTRY *LPALAUXILIARYEFFECTSLOTI)(ALuint, ALenum, ALint);
    typedef void(AL_APIENTRY *LPALAUXILIARYEFFECTSLOTF)(ALuint, ALenum, ALfloat);
    typedef void(AL_APIENTRY *LPALGETAUXILIARYEFFECTSLOTI)(ALuint, ALenum, ALint *);
    typedef void(AL_APIENTRY *LPALGETAUXILIARYEFFECTSLOTF)(ALuint, ALenum, ALfloat *);

    LPALGENEFFECTS alGenEffects{nullptr};
    LPALDELETEEFFECTS alDeleteEffects{nullptr};
    LPALEFFECTI alEffecti{nullptr};
    LPALEFFECTF alEffectf{nullptr};
    LPALEFFECTFV alEffectfv{nullptr};
    LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots{nullptr};
    LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots{nullptr};
    LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti{nullptr};
    LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf{nullptr};
    LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti{nullptr};
    LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf{nullptr};

private:
    ALCdevice *p_device;   ///< OpenAL device for audio output.
    ALCcontext *p_context; ///< OpenAL context for controlling audio output.

    // Sound effect members
    std::vector<ALuint> m_source_pool;    ///< Pool of available sources for sound effects.
    std::vector<ALuint> m_active_sources; ///< Sources currently playing sound effects.

    // Music streaming members
    ALuint m_music_source;                            ///< Dedicated source for music playback.
    std::vector<ALuint> m_music_buffers;              ///< Buffers for streaming music chunks.
    std::vector<MusicTrack *> m_music_tracks;         ///< Queue of music tracks to be played.
    size_t m_current_index = 0;                       ///< Index of the next track to play in the queue.
    MusicTrack *p_current_track;                      ///< The current music track being streamed.
    static constexpr size_t NUM_STREAM_BUFFERS = 3;   ///< Number of buffers for music streaming.
    static constexpr size_t FRAMES_PER_BUFFER = 4096; ///< Number of frames per buffer (for streaming music).

    f32 m_master_sound_volume; ///< Master volume for sound effects.
    f32 m_master_music_volume; ///< Master volume for music.

    bool m_music_looping; ///< Whether music should loop after finishing.
    bool m_queue_looping; ///< Whether the music queue should loop when finished.

    AudioEffects m_effects;
    bool m_effects_loaded;
    bool m_effects_pointers_loaded;
};

} // namespace audio end
} // namespace gouda end