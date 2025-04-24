#include "audio/audio_manager.hpp"

#include <algorithm> // for std::shuffle
#include <stdexcept>
#include <utility> // for std::exchange

#include "AL/alext.h"
#include "AL/efx-presets.h"

#include "debug/debug.hpp"
#include "math/random.hpp"

namespace gouda {
namespace audio {

AudioManager::AudioManager()
    : p_device{nullptr},
      p_context{nullptr},
      m_music_source{0},
      p_current_track{nullptr},
      m_current_index{0},
      m_master_sound_volume{1.0f},
      m_master_music_volume{1.0f},
      m_music_looping{false},
      m_queue_looping{false},
      m_effects_loaded{false},
      m_effects_pointers_loaded{false}
{
    // Default constructor initializes to "unloaded" state
}

AudioManager::~AudioManager()
{
    // Stop and delete sound effect sources
    for (ALuint source : m_active_sources) {
        alSourceStop(source);
        alDeleteSources(1, &source);
    }
    for (ALuint source : m_source_pool) {
        alDeleteSources(1, &source);
    }

    // Stop and delete music resources
    StopMusic();

    DestroyAudioEffects();

    // Destroy OpenAL context and device
    alcMakeContextCurrent(nullptr);
    if (p_context) {
        alcDestroyContext(p_context);
    }
    if (p_device) {
        alcCloseDevice(p_device);
    }

    ENGINE_LOG_DEBUG("Audio manager destroyed");
}

void AudioManager::Initialize(f32 sound_volume, f32 music_volume)
{
    p_device = alcOpenDevice(nullptr); // Default device
    if (!p_device) {
        ENGINE_LOG_ERROR("Failed to open audio device");
        ENGINE_THROW("Failed to open audio device");
    }

    p_context = alcCreateContext(p_device, nullptr);
    if (!p_context || !alcMakeContextCurrent(p_context)) {
        alcDestroyContext(p_context);
        alcCloseDevice(p_device);
        ENGINE_LOG_ERROR("Failed to create or set audio context");
        ENGINE_THROW("Failed to create or set audio context");
    }

    InitializeSources();

    if (!alcIsExtensionPresent(alcGetContextsDevice(alcGetCurrentContext()), "ALC_EXT_EFX")) {
        ENGINE_LOG_ERROR("EFX extension not supported by audio device");
    }
    else {
        ENGINE_LOG_DEBUG("EFX extension supported");

        LoadAudioEffectFunctions();

        if (!m_effects_pointers_loaded) {
            ENGINE_LOG_ERROR("Failed to load effect function pointers");
        }
        else {
            InitializeAudioEffects();
        }
    }

    SetMasterSoundVolume(sound_volume);
    SetMasterMusicVolume(music_volume);

    ENGINE_LOG_DEBUG("Audio manager initialized");
}

void AudioManager::PlaySoundEffect(const SoundEffect &sound, const std::vector<AudioEffectType> &effects, f32 volume,
                                   f32 pitch)
{
    ALuint buffer{sound.GetBuffer()};
    if (!alIsBuffer(buffer)) {
        ENGINE_LOG_ERROR("Invalid buffer ID {}", buffer);
        return;
    }

    ALuint source{GetFreeSource()};
    if (source == 0) {
        ENGINE_LOG_WARNING("No sources available to play sound effect");
        return;
    }

    f32 effective_volume{volume * m_master_sound_volume};

    ENGINE_LOG_DEBUG("Attaching buffer {} to source {}", buffer, source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcef(source, AL_GAIN, effective_volume);
    alSourcef(source, AL_PITCH, pitch);
    alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f); // Center
    alSourcei(source, AL_LOOPING, AL_FALSE);           // One-shot

    if (!effects.empty()) {
        ApplyAudioEffects(source, effects);
        ENGINE_LOG_DEBUG("EFFECTS APPLIED");
    }

    alSourcePlay(source);
    ALenum result{alGetError()};
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to play sound effect: {}", alGetString(result));
        alDeleteSources(1, &source);
        return;
    }

    ALint state{0};
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    ENGINE_LOG_DEBUG("Source {} state: {}", source, state == AL_PLAYING ? "PLAYING" : "NOT PLAYING");
    m_active_sources.push_back(source);
}

void AudioManager::PlaySoundEffectAt(const SoundEffect &sound, const Vec3 &position,
                                     const std::vector<AudioEffectType> &effects, f32 volume, f32 pitch, bool loop)
{
    ALuint source{GetFreeSource()};
    if (source == 0)
        return;

    ALuint buffer{sound.GetBuffer()};
    if (!alIsBuffer(buffer)) {
        ENGINE_LOG_ERROR("Invalid buffer ID {}", buffer);
        return;
    }

    f32 effective_volume{volume * m_master_sound_volume};
    alSourcei(source, AL_BUFFER, buffer);
    alSourcef(source, AL_GAIN, effective_volume);
    alSourcef(source, AL_PITCH, pitch);
    alSource3f(source, AL_POSITION, position.x, position.y, position.z);
    alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);

    if (!effects.empty()) {
        ApplyAudioEffects(source, effects);
    }

    alSourcePlay(source);
    ALenum result{alGetError()};
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to play sound effect: {}", alGetString(result));
        alDeleteSources(1, &source);
        return;
    }
    m_active_sources.push_back(source);
}

void AudioManager::QueueMusic(MusicTrack &track, bool play_immediately)
{
    m_music_tracks.push_back(&track);
    ENGINE_LOG_DEBUG("Queued music track");
    if (play_immediately && !p_current_track) {
        PlayNextTrack();
    }
}

void AudioManager::PlayMusic(bool shuffle)
{
    if (!p_current_track && !m_music_tracks.empty()) {
        if (shuffle) {
            ShuffleTracks();
        }

        PlayNextTrack();
    }
}

void AudioManager::PauseMusic()
{
    if (m_music_source) {
        alSourcePause(m_music_source);
        ALenum result{alGetError()};
        if (result != AL_NO_ERROR) {
            ENGINE_LOG_ERROR("Failed to pause music: {}", alGetString(result));
        }
        else {
            ENGINE_LOG_DEBUG("Paused music on source {}", m_music_source);
        }
    }
}

void AudioManager::ResumeMusic()
{
    if (m_music_source) {
        alSourcePlay(m_music_source);
        ALenum result{alGetError()};
        if (result != AL_NO_ERROR) {
            ENGINE_LOG_ERROR("Failed to resume music: {}", alGetString(result));
        }
        else {
            ENGINE_LOG_DEBUG("Resumed music on source {}", m_music_source);
        }
    }
}

void AudioManager::StopMusic()
{
    if (m_music_source) {
        StopCurrentTrack();
    }

    p_current_track = nullptr;
    m_music_looping = false;
    m_music_tracks.clear();
    m_current_index = 0;

    ENGINE_LOG_DEBUG("Stopped music and cleared queue");
}

void AudioManager::StopCurrentTrack()
{
    if (m_music_source) {

        ALuint source_to_log{m_music_source};
        alSourceStop(m_music_source);

        ALenum result{alGetError()};
        if (result != AL_NO_ERROR) {
            ENGINE_LOG_ERROR("Failed to stop music source {}: {}", source_to_log, alGetString(result));
        }
        alDeleteSources(1, &m_music_source);
        result = alGetError();
        if (result != AL_NO_ERROR) {
            ENGINE_LOG_ERROR("Failed to delete music source {}: {}", source_to_log, alGetString(result));
        }
        m_music_source = 0;
        for (ALuint buffer : m_music_buffers) {
            alDeleteBuffers(1, &buffer);
            result = alGetError();
            if (result != AL_NO_ERROR) {
                ENGINE_LOG_ERROR("Failed to delete buffer {}: {}", buffer, alGetString(result));
            }
        }
        m_music_buffers.clear();
        ENGINE_LOG_DEBUG("Stopped current track on source {}", source_to_log);
    }
}

void AudioManager::ShuffleTracks()
{
    if (m_current_index < m_music_tracks.size()) {
        std::shuffle(m_music_tracks.begin() + m_current_index, m_music_tracks.end(), math::GetGlobalRNG());
    }

    ENGINE_LOG_DEBUG("Shuffled music queue with {} tracks", m_music_tracks.size());
}

void AudioManager::Update()
{
    // Clean up sound m_effects
    for (auto it = m_active_sources.begin(); it != m_active_sources.end();) {
        ALint state;
        alGetSourcei(*it, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
            m_source_pool.push_back(*it);
            ENGINE_LOG_DEBUG("Returned source {} to pool", *it);
            it = m_active_sources.erase(it);
        }
        else {
            ++it;
        }
    }

    // Stream music
    if (m_music_source && p_current_track) {
        ALint processed{0};
        alGetSourcei(m_music_source, AL_BUFFERS_PROCESSED, &processed);
        while (processed--) {
            ALuint buffer{0};
            alSourceUnqueueBuffers(m_music_source, 1, &buffer);
            ALenum result{alGetError()};
            if (result != AL_NO_ERROR) {
                ENGINE_LOG_ERROR("alSourceUnqueueBuffers failed: {}", alGetString(result));
                continue;
            }

            std::vector<f32> temp(FRAMES_PER_BUFFER * p_current_track->GetChannels());
            size_t frames_read{p_current_track->ReadFrames(temp.data(), FRAMES_PER_BUFFER)};
            if (frames_read > 0) {
                alBufferData(buffer, p_current_track->GetFormat(), temp.data(),
                             frames_read * p_current_track->GetChannels() * sizeof(f32),
                             p_current_track->GetSampleRate());
                alSourceQueueBuffers(m_music_source, 1, &buffer);
            }
            else if (p_current_track->IsFinished()) {

                if (m_music_looping) {
                    sf_seek(p_current_track->GetFile(), 0, SEEK_SET); // Reset to start
                    p_current_track->SetFinished(false);
                    frames_read = p_current_track->ReadFrames(temp.data(), FRAMES_PER_BUFFER);
                    if (frames_read > 0) {
                        alBufferData(buffer, p_current_track->GetFormat(), temp.data(),
                                     frames_read * p_current_track->GetChannels() * sizeof(f32),
                                     p_current_track->GetSampleRate());
                        alSourceQueueBuffers(m_music_source, 1, &buffer);
                    }
                }
                else {
                    ENGINE_LOG_DEBUG("Music track finished");
                    PlayNextTrack();
                    break;
                }
            }
        }

        ALint state{0};
        alGetSourcei(m_music_source, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING && !m_music_buffers.empty()) {
            alSourcePlay(m_music_source);
            ENGINE_LOG_DEBUG("Restarted music playback");
        }
    }
}

void AudioManager::SetMusicPitch(f32 pitch)
{
    if (m_music_source) {
        alSourcef(m_music_source, AL_PITCH, pitch);
        ALenum result{alGetError()};
        if (result != AL_NO_ERROR) {
            ENGINE_LOG_ERROR("Failed to set music pitch: {}", alGetString(result));
        }
        else {
            ENGINE_LOG_DEBUG("Set music pitch to {} for source {}", pitch, m_music_source);
        }
    }
    else {
        ENGINE_LOG_WARNING("No music source active to set pitch");
    }
}

static f32 ClampVolume(f32 volume) { return std::clamp(volume, 0.0f, 1.0f); }

void AudioManager::SetMasterSoundVolume(f32 volume)
{
    m_master_sound_volume = ClampVolume(volume);

    for (ALuint source : m_active_sources) {
        ALint buffer{0};
        alGetSourcei(source, AL_BUFFER, &buffer);
        f32 current_volume{0.0f};
        alGetSourcef(source, AL_GAIN, &current_volume);
        alSourcef(source, AL_GAIN, current_volume * m_master_sound_volume);
    }

    ENGINE_LOG_DEBUG("Set master sound volume to {}", m_master_sound_volume);
}

void AudioManager::SetMasterMusicVolume(f32 volume)
{
    m_master_music_volume = ClampVolume(volume);
    if (m_music_source) {
        f32 current_volume{0.0f};
        alGetSourcef(m_music_source, AL_GAIN, &current_volume);
        alSourcef(m_music_source, AL_GAIN, current_volume * volume);
    }

    ENGINE_LOG_DEBUG("Set master music volume to {} (volume): {}", m_master_music_volume, volume);
}

void AudioManager::SetMusicLooping(bool loop)
{
    m_music_looping = loop; // Update state even if no source active
    if (m_music_source) {
        alSourcei(m_music_source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        ALenum result{alGetError()};
        if (result != AL_NO_ERROR) {
            ENGINE_LOG_ERROR("Failed to set music looping: {}", alGetString(result));
        }
        else {
            ENGINE_LOG_DEBUG("Set music looping to {}", loop ? "true" : "false");
        }
    }
}

void AudioManager::SetQueueLooping(bool loop)
{
    m_queue_looping = loop;
    ENGINE_LOG_DEBUG("Set queue looping to {}", loop ? "true" : "false");
}

void AudioManager::SetListenerPosition(const Vec3 &position)
{
    alListener3f(AL_POSITION, position.x, position.y, position.z);
    ALenum result{alGetError()};
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set listener position: {}", alGetString(result));
    }
}

void AudioManager::SetListenerVelocity(const Vec3 &velocity)
{
    alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    ALenum result{alGetError()};
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set listener velocity: {}", alGetString(result));
    }
}

// Private functions --------------------------------------------------------------------------
void AudioManager::LoadAudioEffectFunctions()
{
#define LOAD_PROC(T, x) ((x) = (T)alGetProcAddress(#x))
    LOAD_PROC(LPALGENEFFECTS, alGenEffects);
    LOAD_PROC(LPALDELETEEFFECTS, alDeleteEffects);
    LOAD_PROC(LPALEFFECTI, alEffecti);
    LOAD_PROC(LPALEFFECTF, alEffectf);
    LOAD_PROC(LPALEFFECTFV, alEffectfv);
    LOAD_PROC(LPALGENAUXILIARYEFFECTSLOTS, alGenAuxiliaryEffectSlots);
    LOAD_PROC(LPALDELETEAUXILIARYEFFECTSLOTS, alDeleteAuxiliaryEffectSlots);
    LOAD_PROC(LPALAUXILIARYEFFECTSLOTI, alAuxiliaryEffectSloti);
    LOAD_PROC(LPALAUXILIARYEFFECTSLOTF, alAuxiliaryEffectSlotf); // Ensure this is loaded
    LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTI, alGetAuxiliaryEffectSloti);
    LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTF, alGetAuxiliaryEffectSlotf);

    ENGINE_LOG_DEBUG("Effects functions loaded");

#undef LOAD_PROC

    // Verify that critical functions loaded
    if (!alGenEffects || !alEffecti || !alEffectf || !alGenAuxiliaryEffectSlots || !alAuxiliaryEffectSloti ||
        !alAuxiliaryEffectSlotf || !alGetAuxiliaryEffectSloti || !alGetAuxiliaryEffectSlotf) {
        ENGINE_LOG_ERROR("Failed to load one or more EFX function pointers:");
        ENGINE_LOG_ERROR("alGenEffects: {}", alGenEffects ? "loaded" : "not loaded");
        ENGINE_LOG_ERROR("alEffecti: {}", alEffecti ? "loaded" : "not loaded");
        ENGINE_LOG_ERROR("alEffectf: {}", alEffectf ? "loaded" : "not loaded");
        ENGINE_LOG_ERROR("alGenAuxiliaryEffectSlots: {}", alGenAuxiliaryEffectSlots ? "loaded" : "not loaded");
        ENGINE_LOG_ERROR("alAuxiliaryEffectSloti: {}", alAuxiliaryEffectSloti ? "loaded" : "not loaded");
        ENGINE_LOG_ERROR("alAuxiliaryEffectSlotf: {}", alAuxiliaryEffectSlotf ? "loaded" : "not loaded");

        m_effects_pointers_loaded = false;
    }
    else {
        m_effects_pointers_loaded = true;
    }
}

void AudioManager::InitializeSources()
{
    if (!p_device) {
        ENGINE_LOG_ERROR("Cannot initialize sources: no audio device");
        return;
    }

    // Query device for maximum mono sources
    ALCint max_mono_sources{0};
    alcGetIntegerv(p_device, ALC_MONO_SOURCES, 1, &max_mono_sources);
    ALenum result = alcGetError(p_device);
    if (result != ALC_NO_ERROR || max_mono_sources <= 0) {
        ENGINE_LOG_WARNING("Failed to query ALC_MONO_SOURCES or invalid value ({}), falling back to trial",
                           max_mono_sources);
        max_mono_sources = -1; // Flag to use trial-and-resultor
    }

    // Trial-and-resultor if query failed
    int detected_max_sources{0};
    constexpr int absolute_max_sources{256};

    if (max_mono_sources == -1) {
        std::vector<ALuint> temp_sources;
        temp_sources.reserve(absolute_max_sources);           // Reasonable upper bound
        while (detected_max_sources < absolute_max_sources) { // Cap at 256 to avoid excessive allocation
            ALuint source;
            alGenSources(1, &source);
            if (alGetError() != AL_NO_ERROR) {
                ENGINE_LOG_DEBUG("Detected max sources at {}", detected_max_sources);
                break;
            }
            temp_sources.push_back(source);
            detected_max_sources++;
        }
        // Transfer successful sources to pool
        m_source_pool = std::move(temp_sources);
        for (ALuint source : m_source_pool) {
            alDeleteSources(1, &source); // Clean up temporary sources
        }
        m_source_pool.clear();
    }
    else {
        detected_max_sources = max_mono_sources;
    }

    // Set pool size (using half the max, capped for safety)
    const int min_pool_size{16};                   // Minimum reasonable pool
    const int max_pool_size{absolute_max_sources}; // Maximum reasonable pool
    int initial_pool_size{std::max(min_pool_size, std::min(detected_max_sources / 2, max_pool_size))};

    ENGINE_LOG_DEBUG("Audio device supports up to {} sources; setting pool size to {}", detected_max_sources,
                     initial_pool_size);

    // Allocate the pool
    m_source_pool.reserve(initial_pool_size);
    int sources_created{0};
    for (int i = 0; i < initial_pool_size; ++i) {
        ALuint source;
        alGenSources(1, &source);
        if (alGetError() == AL_NO_ERROR) {
            m_source_pool.push_back(source);
            sources_created++;
        }
        else {
            ENGINE_LOG_ERROR("Failed to generate source {} of {}", i, initial_pool_size);
            break;
        }
    }
}

void AudioManager::InitializeAudioEffects()
{
    // --- Reverb Effect ---
    alGenEffects(1, &m_effects.reverb.effect_id);
    if (alGetError() != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to generate reverb effect");
        return;
    }
    alEffecti(m_effects.reverb.effect_id, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
    ALenum result{alGetError()};
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set reverb type: {}", alGetString(result));
        CleanupEffect(m_effects.reverb.effect_id, m_effects.reverb.slot_id, "reverb");
        return;
    }
    alEffectf(m_effects.reverb.effect_id, AL_REVERB_DENSITY, 1.0f); // Min: 0.0, Max: 1.0 - Full density for rich reverb
    alEffectf(m_effects.reverb.effect_id, AL_REVERB_DIFFUSION,
              1.0f); // Min: 0.0, Max: 1.0 - Full diffusion for natural spread
    alEffectf(m_effects.reverb.effect_id, AL_REVERB_GAIN, 0.32f);   // Min: 0.0, Max: 1.0 - Moderate gain to blend
    alEffectf(m_effects.reverb.effect_id, AL_REVERB_GAINHF, 0.89f); // Min: 0.0, Max: 1.0 - Slight HF roll-off
    alEffectf(m_effects.reverb.effect_id, AL_REVERB_DECAY_TIME,
              1.49f); // Min: 0.1, Max: 20.0 - Short decay for small space
    alEffectf(m_effects.reverb.effect_id, AL_REVERB_DECAY_HFRATIO, 0.83f); // Min: 0.1, Max: 2.0 - Balanced HF decay
    alEffectf(m_effects.reverb.effect_id, AL_REVERB_REFLECTIONS_GAIN,
              0.05f); // Min: 0.0, Max: 3.16 - Subtle early reflections
    alEffectf(m_effects.reverb.effect_id, AL_REVERB_REFLECTIONS_DELAY,
              0.007f); // Min: 0.0, Max: 0.3 - Quick reflection onset
    alEffectf(m_effects.reverb.effect_id, AL_REVERB_LATE_REVERB_GAIN,
              1.26f); // Min: 0.0, Max: 10.0 - Moderate late reverb
    alEffectf(m_effects.reverb.effect_id, AL_REVERB_LATE_REVERB_DELAY, 0.011f); // Min: 0.0, Max: 0.1 - Short late delay
    alEffectf(m_effects.reverb.effect_id, AL_REVERB_AIR_ABSORPTION_GAINHF,
              0.994f); // Min: 0.892, Max: 1.0 - Minimal HF absorption
    alEffectf(m_effects.reverb.effect_id, AL_REVERB_ROOM_ROLLOFF_FACTOR, 0.0f); // Min: 0.0, Max: 10.0 - No rolloff
    alEffecti(m_effects.reverb.effect_id, AL_REVERB_DECAY_HFLIMIT, AL_TRUE); // Min: 0, Max: 1 - Enable HF decay limit
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set reverb params: {}", alGetString(result));
        CleanupEffect(m_effects.reverb.effect_id, m_effects.reverb.slot_id, "reverb");
        return;
    }
    alGenAuxiliaryEffectSlots(1, &m_effects.reverb.slot_id);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to generate reverb slot: {}", alGetString(result));
        CleanupEffect(m_effects.reverb.effect_id, m_effects.reverb.slot_id, "reverb");
        return;
    }
    alAuxiliaryEffectSloti(m_effects.reverb.slot_id, AL_EFFECTSLOT_EFFECT, m_effects.reverb.effect_id);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to attach reverb to slot: {}", alGetString(result));
        CleanupEffect(m_effects.reverb.effect_id, m_effects.reverb.slot_id, "reverb");
        return;
    }
    alAuxiliaryEffectSlotf(m_effects.reverb.slot_id, AL_EFFECTSLOT_GAIN, 1.0f); // Min: 0.0, Max: 1.0 - Full output
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set reverb gain: {}", alGetString(result));
        CleanupEffect(m_effects.reverb.effect_id, m_effects.reverb.slot_id, "reverb");
        return;
    }

    ALfloat reverb_gain{0.0f};
    alGetAuxiliaryEffectSlotf(m_effects.reverb.slot_id, AL_EFFECTSLOT_GAIN, &reverb_gain);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to get reverb gain: {}", alGetString(result));
    }
    else {
        ENGINE_LOG_DEBUG("Reverb slot gain set to: {}", reverb_gain);
    }
    ENGINE_LOG_DEBUG("Initialized reverb effect with effect_id={} and slot_id={}", m_effects.reverb.effect_id,
                     m_effects.reverb.slot_id);

    // --- Echo Effect ---
    alGenEffects(1, &m_effects.echo.effect_id);
    if (alGetError() != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to generate echo effect");
        return;
    }
    alEffecti(m_effects.echo.effect_id, AL_EFFECT_TYPE, AL_EFFECT_ECHO);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set echo type: {}", alGetString(result));
        CleanupEffect(m_effects.echo.effect_id, m_effects.echo.slot_id, "echo");
        return;
    }
    alEffectf(m_effects.echo.effect_id, AL_ECHO_DELAY, 0.1f);    // Min: 0.0, Max: 0.207 - Moderate echo onset
    alEffectf(m_effects.echo.effect_id, AL_ECHO_LRDELAY, 0.1f);  // Min: 0.0, Max: 0.404 - Subtle stereo spread
    alEffectf(m_effects.echo.effect_id, AL_ECHO_DAMPING, 0.5f);  // Min: 0.0, Max: 0.99 - Balanced HF decay
    alEffectf(m_effects.echo.effect_id, AL_ECHO_FEEDBACK, 0.5f); // Min: 0.0, Max: 1.0 - Moderate repetitions
    alEffectf(m_effects.echo.effect_id, AL_ECHO_SPREAD, -0.5f);  // Min: -1.0, Max: 1.0 - Moderate stereo enhancement
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set echo params: {}", alGetString(result));
        CleanupEffect(m_effects.echo.effect_id, m_effects.echo.slot_id, "echo");
        return;
    }
    alGenAuxiliaryEffectSlots(1, &m_effects.echo.slot_id);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to generate echo slot: {}", alGetString(result));
        CleanupEffect(m_effects.echo.effect_id, m_effects.echo.slot_id, "echo");
        return;
    }
    alAuxiliaryEffectSloti(m_effects.echo.slot_id, AL_EFFECTSLOT_EFFECT, m_effects.echo.effect_id);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to attach echo to slot: {}", alGetString(result));
        CleanupEffect(m_effects.echo.effect_id, m_effects.echo.slot_id, "echo");
        return;
    }
    alAuxiliaryEffectSlotf(m_effects.echo.slot_id, AL_EFFECTSLOT_GAIN, 1.0f); // Min: 0.0, Max: 1.0 - Full output
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set echo gain: {}", alGetString(result));
        CleanupEffect(m_effects.echo.effect_id, m_effects.echo.slot_id, "echo");
        return;
    }
    ALfloat echo_gain{0.0f};
    alGetAuxiliaryEffectSlotf(m_effects.echo.slot_id, AL_EFFECTSLOT_GAIN, &echo_gain);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to get echo gain: {}", alGetString(result));
    }
    else {
        ENGINE_LOG_DEBUG("Echo slot gain set to: {}", echo_gain);
    }
    ENGINE_LOG_DEBUG("Initialized echo effect with effect_id={} and slot_id={}", m_effects.echo.effect_id,
                     m_effects.echo.slot_id);

    // --- Distortion Effect ---
    alGenEffects(1, &m_effects.distortion.effect_id);
    if (alGetError() != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to generate distortion effect");
        return;
    }
    alEffecti(m_effects.distortion.effect_id, AL_EFFECT_TYPE, AL_EFFECT_DISTORTION);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set distortion type: {}", alGetString(result));
        CleanupEffect(m_effects.distortion.effect_id, m_effects.distortion.slot_id, "distortion");
        return;
    }
    alEffectf(m_effects.distortion.effect_id, AL_DISTORTION_EDGE, 0.2f); // Min: 0.0, Max: 1.0 - Subtle grit
    alEffectf(m_effects.distortion.effect_id, AL_DISTORTION_GAIN, 0.5f); // Min: 0.01, Max: 1.0 - Moderate boost
    alEffectf(m_effects.distortion.effect_id, AL_DISTORTION_LOWPASS_CUTOFF,
              8000.0f); // Min: 80.0, Max: 24000.0 - Preserve clarity
    alEffectf(m_effects.distortion.effect_id, AL_DISTORTION_EQCENTER,
              3600.0f); // Min: 80.0, Max: 24000.0 - Mid-range focus
    alEffectf(m_effects.distortion.effect_id, AL_DISTORTION_EQBANDWIDTH,
              3600.0f); // Min: 80.0, Max: 24000.0 - Broad effect
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set distortion params: {}", alGetString(result));
        CleanupEffect(m_effects.distortion.effect_id, m_effects.distortion.slot_id, "distortion");
        return;
    }
    alGenAuxiliaryEffectSlots(1, &m_effects.distortion.slot_id);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to generate distortion slot: {}", alGetString(result));
        CleanupEffect(m_effects.distortion.effect_id, m_effects.distortion.slot_id, "distortion");
        return;
    }
    alAuxiliaryEffectSloti(m_effects.distortion.slot_id, AL_EFFECTSLOT_EFFECT, m_effects.distortion.effect_id);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to attach distortion to slot: {}", alGetString(result));
        CleanupEffect(m_effects.distortion.effect_id, m_effects.distortion.slot_id, "distortion");
        return;
    }
    alAuxiliaryEffectSlotf(m_effects.distortion.slot_id, AL_EFFECTSLOT_GAIN, 1.0f); // Min: 0.0, Max: 1.0 - Full output
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set distortion gain: {}", alGetString(result));
        CleanupEffect(m_effects.distortion.effect_id, m_effects.distortion.slot_id, "distortion");
        return;
    }
    ALfloat distortion_gain{0.0f};
    alGetAuxiliaryEffectSlotf(m_effects.distortion.slot_id, AL_EFFECTSLOT_GAIN, &distortion_gain);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to get distortion gain: {}", alGetString(result));
    }
    else {
        ENGINE_LOG_DEBUG("Distortion slot gain set to: {}", distortion_gain);
    }
    ENGINE_LOG_DEBUG("Initialized distortion effect with effect_id={} and slot_id={}", m_effects.distortion.effect_id,
                     m_effects.distortion.slot_id);

    // --- Chorus Effect ---
    alGenEffects(1, &m_effects.chorus.effect_id);
    if (alGetError() != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to generate chorus effect");
        return;
    }
    alEffecti(m_effects.chorus.effect_id, AL_EFFECT_TYPE, AL_EFFECT_CHORUS);
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set chorus effect type: {}", alGetString(result));
        CleanupEffect(m_effects.chorus.effect_id, m_effects.chorus.slot_id, "chorus");
        return;
    }
    alEffecti(m_effects.chorus.effect_id, AL_CHORUS_WAVEFORM,
              AL_CHORUS_WAVEFORM_SINUSOID); // Min: 0 (triangle), Max: 1 (sinusoid) - Smooth modulation for lush sound
    alEffecti(m_effects.chorus.effect_id, AL_CHORUS_PHASE, 90);  // Min: -180, Max: 180 - Phase shift for stereo width
    alEffectf(m_effects.chorus.effect_id, AL_CHORUS_RATE, 1.1f); // Min: 0.0, Max: 10.0 - Modulation rate: gentle sweep
    alEffectf(m_effects.chorus.effect_id, AL_CHORUS_DEPTH,
              0.1f); // Min: 0.0, Max: 1.0 - Modulation depth: subtle effect
    alEffectf(m_effects.chorus.effect_id, AL_CHORUS_FEEDBACK,
              0.25f); // Min: -1.0, Max: 1.0 - Feedback: moderate enhancement
    alEffectf(m_effects.chorus.effect_id, AL_CHORUS_DELAY,
              0.016f); // Min: 0.0, Max: 0.016 - Base delay: classic chorus effect
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set chorus parameters: {}", alGetString(result));
        CleanupEffect(m_effects.chorus.effect_id, m_effects.chorus.slot_id, "chorus");
        return;
    }
    alGenAuxiliaryEffectSlots(1, &m_effects.chorus.slot_id);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to generate chorus slot: {}", alGetString(result));
        CleanupEffect(m_effects.chorus.effect_id, m_effects.chorus.slot_id, "chorus");
        return;
    }
    alAuxiliaryEffectSloti(m_effects.chorus.slot_id, AL_EFFECTSLOT_EFFECT, m_effects.chorus.effect_id);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to attach chorus effect to slot: {}", alGetString(result));
        CleanupEffect(m_effects.chorus.effect_id, m_effects.chorus.slot_id, "chorus");
        return;
    }
    alAuxiliaryEffectSlotf(m_effects.chorus.slot_id, AL_EFFECTSLOT_GAIN,
                           1.0f); // Min: 0.0, Max: 1.0 - Output level: full audibility
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set chorus slot gain: {}", alGetString(result));
        CleanupEffect(m_effects.chorus.effect_id, m_effects.chorus.slot_id, "chorus");
        return;
    }
    ALfloat gain{0.0f};
    alGetAuxiliaryEffectSlotf(m_effects.chorus.slot_id, AL_EFFECTSLOT_GAIN, &gain);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to get chorus slot gain: {}", alGetString(result));
    }
    else {
        ENGINE_LOG_DEBUG("Chorus slot gain set to: {}", gain);
    }
    ENGINE_LOG_DEBUG("Initialized chorus effect with effect_id={} and slot_id={}", m_effects.chorus.effect_id,
                     m_effects.chorus.slot_id);

    // --- Flanger Effect ---
    alGenEffects(1, &m_effects.flanger.effect_id);
    if (alGetError() != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to generate flanger effect");
        return;
    }
    alEffecti(m_effects.flanger.effect_id, AL_EFFECT_TYPE, AL_EFFECT_FLANGER);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set flanger effect type: {}", alGetString(result));
        CleanupEffect(m_effects.flanger.effect_id, m_effects.flanger.slot_id, "flanger");
        return;
    }
    alEffecti(m_effects.flanger.effect_id, AL_FLANGER_WAVEFORM,
              AL_FLANGER_WAVEFORM_SINUSOID); // Min: 0 (triangle), Max: 1 (sinusoid) - Smooth sweep for flanging
    alEffecti(m_effects.flanger.effect_id, AL_FLANGER_PHASE, 0);    // Min: -180, Max: 180 - Phase: centered stereo
    alEffectf(m_effects.flanger.effect_id, AL_FLANGER_RATE, 0.27f); // Min: 0.0, Max: 10.0 - Modulation rate: slow sweep
    alEffectf(m_effects.flanger.effect_id, AL_FLANGER_DEPTH,
              1.0f); // Min: 0.0, Max: 1.0 - Modulation depth: full effect
    alEffectf(m_effects.flanger.effect_id, AL_FLANGER_FEEDBACK,
              -0.5f); // Min: -1.0, Max: 1.0 - Feedback: moderate metallic tone
    alEffectf(m_effects.flanger.effect_id, AL_FLANGER_DELAY,
              0.002f); // Min: 0.0, Max: 0.004 - Base delay: typical flanger comb
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set flanger parameters: {}", alGetString(result));
        CleanupEffect(m_effects.flanger.effect_id, m_effects.flanger.slot_id, "flanger");
        return;
    }
    alGenAuxiliaryEffectSlots(1, &m_effects.flanger.slot_id);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to generate flanger slot: {}", alGetString(result));
        CleanupEffect(m_effects.flanger.effect_id, m_effects.flanger.slot_id, "flanger");
        return;
    }
    alAuxiliaryEffectSloti(m_effects.flanger.slot_id, AL_EFFECTSLOT_EFFECT, m_effects.flanger.effect_id);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to attach flanger effect to slot: {}", alGetString(result));
        CleanupEffect(m_effects.flanger.effect_id, m_effects.flanger.slot_id, "flanger");
        return;
    }
    alAuxiliaryEffectSlotf(m_effects.flanger.slot_id, AL_EFFECTSLOT_GAIN,
                           1.0f); // Min: 0.0, Max: 1.0 - Output level: full audibility
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to set flanger slot gain: {}", alGetString(result));
        CleanupEffect(m_effects.flanger.effect_id, m_effects.flanger.slot_id, "flanger");
        return;
    }
    ALfloat flanger_gain{0.0f};
    alGetAuxiliaryEffectSlotf(m_effects.flanger.slot_id, AL_EFFECTSLOT_GAIN, &flanger_gain);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to get flanger slot gain: {}", alGetString(result));
    }
    else {
        ENGINE_LOG_DEBUG("Flanger slot gain set to: {}", flanger_gain);
    }
    ENGINE_LOG_DEBUG("Initialized flanger effect with effect_id={} and slot_id={}", m_effects.flanger.effect_id,
                     m_effects.flanger.slot_id);

    m_effects_loaded = true; // Flag effects as loaded

    ENGINE_LOG_DEBUG("All audio effects initialized successfully");
}

void AudioManager::CleanupEffect(ALuint effect_id, ALuint slot_id, std::string_view name)
{
    if (effect_id && alDeleteEffects) {
        alDeleteEffects(1, &effect_id);
        ALenum result = alGetError();
        if (result != AL_NO_ERROR) {
            ENGINE_LOG_ERROR("Failed to delete {} effect: {}", name, alGetString(result));
        }
        else {
            ENGINE_LOG_DEBUG("Deleted {} effect ID {}", name, effect_id);
            effect_id = std::exchange(effect_id, 0); // Ensure it is reset
        }
    }

    if (slot_id && alDeleteAuxiliaryEffectSlots) {
        alDeleteAuxiliaryEffectSlots(1, &slot_id);
        ALenum result = alGetError();
        if (result != AL_NO_ERROR) {
            ENGINE_LOG_ERROR("Failed to delete {} slot: {}", name, alGetString(result));
        }
        else {
            ENGINE_LOG_DEBUG("Deleted {} slot ID {}", name, slot_id);
            slot_id = std::exchange(slot_id, 0); // Ensure it is reset
        }
    }
}

void AudioManager::DestroyAudioEffects()
{
    CleanupEffect(m_effects.reverb.effect_id, m_effects.reverb.slot_id, "reverb");
    CleanupEffect(m_effects.echo.effect_id, m_effects.echo.slot_id, "echo");
    CleanupEffect(m_effects.distortion.effect_id, m_effects.distortion.slot_id, "distortion");
    CleanupEffect(m_effects.chorus.effect_id, m_effects.chorus.slot_id, "chorus");
    CleanupEffect(m_effects.flanger.effect_id, m_effects.flanger.slot_id, "flanger");

    m_effects_loaded = false;

    ENGINE_LOG_DEBUG("Audio effects destroyed");
}

void AudioManager::ApplyAudioEffects(ALuint source, const std::vector<AudioEffectType> &effects)
{

    if (source == 0) {
        ENGINE_LOG_ERROR("Could not apply audio effects. Invalid source provided");
        return;
    }

    if (!m_effects_pointers_loaded) {
        ENGINE_LOG_ERROR("Could not apply audio effects. Audio effects pointers not loaded");
        return;
    }

    if (!m_effects_loaded) {
        ENGINE_LOG_ERROR("Could not apply audio effects. No audio effects are loaded");
        return;
    }

    ALenum result{alGetError()}; // Clear any prior errors
    ALCint max_sends{0};
    alcGetIntegerv(p_device, ALC_MAX_AUXILIARY_SENDS, 1, &max_sends);
    ENGINE_LOG_DEBUG("Max auxiliary sends supported: {}", max_sends);
    if (max_sends <= 0) {
        ENGINE_LOG_WARNING("No auxiliary sends supported; effects will not be applied");
        return;
    }

    size_t num_effects{std::min(effects.size(), static_cast<size_t>(max_sends))};
    ENGINE_LOG_DEBUG("Applying {} effects to source {}", num_effects, source);
    for (size_t i = 0; i < num_effects; ++i) {

        AudioEffectType effect_type{effects[i]};
        ALuint slot_id{m_effects[effect_type].slot_id};
        ALuint effect_id{m_effects[effect_type].effect_id};

        ENGINE_LOG_DEBUG("Effect {}: slot_id={}, effect_id={}", static_cast<int>(effect_type), slot_id, effect_id);

        if (slot_id == 0 || effect_id == 0) {
            ENGINE_LOG_WARNING("Effect {} not initialized (slot_id={}, effect_id={}); skipping",
                               static_cast<int>(effect_type), slot_id, effect_id);
            continue;
        }

        // Check slot state
        ALint slot_gain{0};
        alGetAuxiliaryEffectSloti(slot_id, AL_EFFECTSLOT_GAIN, &slot_gain);
        ENGINE_LOG_DEBUG("Slot {} gain: {}", slot_id, slot_gain);

        alSource3i(source, AL_AUXILIARY_SEND_FILTER, slot_id, static_cast<ALint>(i), AL_FILTER_NULL);
        result = alGetError();
        if (result != AL_NO_ERROR) {
            ENGINE_LOG_ERROR("Failed to attach effect {} to source {} on send {}: {}", static_cast<int>(effect_type),
                             source, i, alGetString(result));
        }
        else {
            ENGINE_LOG_DEBUG("Attached effect {} (slot {}) to source {} on send {}", static_cast<int>(effect_type),
                             slot_id, source, i);
        }
    }
}

ALuint AudioManager::GetFreeSource()
{
    if (!m_source_pool.empty()) {
        ALuint source{m_source_pool.back()};
        m_source_pool.pop_back();
        ENGINE_LOG_DEBUG("Reusing source {}", source);
        return source;
    }

    ALuint source{0};
    alGenSources(1, &source);
    if (alGetError() != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("No free sources available");
        return 0;
    }
    ENGINE_LOG_DEBUG("Created new source {}", source);
    return source;
}

void AudioManager::PlayNextTrack()
{
    if (m_current_index >= m_music_tracks.size()) {
        if (m_queue_looping && !m_music_tracks.empty()) {
            m_current_index = 0; // Reset to start for looping
        }
        else {
            ENGINE_LOG_DEBUG("No more tracks to play");
            StopMusic();
            return;
        }
    }

    if (m_music_source) {
        StopCurrentTrack(); // Stop and clean up the current track
    }

    p_current_track = m_music_tracks[m_current_index];
    m_current_index++;

    alGenSources(1, &m_music_source);
    ALenum result{alGetError()};
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to create music source: {}", alGetString(result));
        m_music_source = 0;
        return;
    }

    m_music_buffers.resize(NUM_STREAM_BUFFERS);
    alGenBuffers(NUM_STREAM_BUFFERS, m_music_buffers.data());
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to create music buffers: {}", alGetString(result));
        alDeleteSources(1, &m_music_source);
        m_music_source = 0;
        return;
    }

    std::vector<f32> temp(FRAMES_PER_BUFFER * p_current_track->GetChannels());
    int buffers_queued{0};
    for (ALuint buffer : m_music_buffers) {
        size_t frames_read{p_current_track->ReadFrames(temp.data(), FRAMES_PER_BUFFER)};
        ENGINE_LOG_DEBUG("Read {} frames for buffer {}", frames_read, buffer);
        if (frames_read > 0) {
            alBufferData(buffer, p_current_track->GetFormat(), temp.data(),
                         frames_read * p_current_track->GetChannels() * sizeof(f32), p_current_track->GetSampleRate());
            result = alGetError();
            if (result != AL_NO_ERROR) {
                ENGINE_LOG_ERROR("alBufferData failed: {}", alGetString(result));
                StopMusic();
                return;
            }
            alSourceQueueBuffers(m_music_source, 1, &buffer);
            result = alGetError();
            if (result != AL_NO_ERROR) {
                ENGINE_LOG_ERROR("alSourceQueueBuffers failed: {}", alGetString(result));
                StopMusic();
                return;
            }
            buffers_queued++;
        }
    }

    if (buffers_queued == 0) {
        ENGINE_LOG_ERROR("No buffers queued; cannot play music");
        StopMusic();
        return;
    }

    alSourcef(m_music_source, AL_GAIN, m_master_music_volume);
    alSourcei(m_music_source, AL_LOOPING, m_music_looping ? AL_TRUE : AL_FALSE);
    alSourcePlay(m_music_source);
    result = alGetError();
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to start music playback: {}", alGetString(result));
        StopMusic();
    }
    else {
        ENGINE_LOG_DEBUG("Started music playback on source {} with {} buffers", m_music_source, buffers_queued);
    }
}

} // namespace audio end
} // namespace gouda end