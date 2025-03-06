#include "audio/audio_manager.hpp"

#include <algorithm> // for std::shuffle
#include <random>    // for std::random_device AND std::mt19937
#include <stdexcept>

#include "logger.hpp"

#include "utility/random.hpp"

namespace GoudaVK {

AudioManager::AudioManager()
    : p_device{nullptr},
      p_context{nullptr},
      m_music_source{0},
      p_current_track{nullptr},
      m_current_index{0},
      m_master_sound_volume{1.0f},
      m_master_music_volume{1.0f},
      m_music_looping{false},
      m_queue_looping{false}
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

void AudioManager::Initialize()
{
    p_device = alcOpenDevice(nullptr); // Default device
    if (!p_device) {
        ENGINE_LOG_ERROR("Failed to open audio device");
        throw std::runtime_error("Failed to open audio device");
    }
    p_context = alcCreateContext(p_device, nullptr);
    if (!p_context || !alcMakeContextCurrent(p_context)) {
        alcDestroyContext(p_context);
        alcCloseDevice(p_device);
        ENGINE_LOG_ERROR("Failed to create or set audio context");
        throw std::runtime_error("Failed to create or set audio context");
    }

    InitializeSources();

    ENGINE_LOG_DEBUG("Audio manager initialized");
}

void AudioManager::PlaySoundEffect(const SoundEffect &sound, f32 volume, f32 pitch)
{
    ALuint buffer = sound.GetBuffer();
    if (!alIsBuffer(buffer)) {
        ENGINE_LOG_ERROR("Invalid buffer ID {}", buffer);
        return;
    }

    ALuint source = GetFreeSource();
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
    alSourcePlay(source);

    ALenum err = alGetError();
    if (err != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to play sound effect: {}", alGetString(err));
        alDeleteSources(1, &source);
        return;
    }

    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    ENGINE_LOG_DEBUG("Source {} state: {}", source, state == AL_PLAYING ? "PLAYING" : "NOT PLAYING");
    m_active_sources.push_back(source);
}

void GoudaVK::AudioManager::PlaySoundEffectAt(const SoundEffect &sound, const Vec3 &position, f32 volume, f32 pitch,
                                              bool loop)
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
    alSourcePlay(source);

    ALenum result{alGetError()};
    if (result != AL_NO_ERROR) {
        ENGINE_LOG_ERROR("Failed to play sound effect: {}", alGetString(result));
        alDeleteSources(1, &source);
        return;
    }
    m_active_sources.push_back(source);
}

void AudioManager::QueueMusic(MusicTrack &track)
{
    m_music_tracks.push_back(&track);
    ENGINE_LOG_DEBUG("Queued music track");
    if (!p_current_track) { // Start playback if nothing is playing
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
        ALuint source_to_log = m_music_source;
        alSourceStop(m_music_source);
        ALenum result = alGetError();
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

void AudioManager::ShuffleRemainingTracks()
{
    if (m_current_index < m_music_tracks.size()) {
        std::shuffle(m_music_tracks.begin() + m_current_index, m_music_tracks.end(), GetGlobalRNG());
    }

    ENGINE_LOG_DEBUG("Shuffled music queue with {} tracks", m_music_tracks.size());
}

void AudioManager::Update()
{
    // Clean up sound effects
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
        ALint processed;
        alGetSourcei(m_music_source, AL_BUFFERS_PROCESSED, &processed);
        while (processed--) {
            ALuint buffer;
            alSourceUnqueueBuffers(m_music_source, 1, &buffer);
            ALenum err = alGetError();
            if (err != AL_NO_ERROR) {
                ENGINE_LOG_ERROR("alSourceUnqueueBuffers failed: {}", alGetString(err));
                continue;
            }

            std::vector<f32> temp(FRAMES_PER_BUFFER * p_current_track->GetChannels());
            size_t frames_read = p_current_track->ReadFrames(temp.data(), FRAMES_PER_BUFFER);
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

        ALint state;
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
        ALint buffer;
        alGetSourcei(source, AL_BUFFER, &buffer);
        f32 current_volume;
        alGetSourcef(source, AL_GAIN, &current_volume);
        alSourcef(source, AL_GAIN, current_volume * m_master_sound_volume);
    }

    ENGINE_LOG_DEBUG("Set master sound volume to {}", m_master_sound_volume);
}

void AudioManager::SetMasterMusicVolume(f32 volume)
{
    m_master_music_volume = ClampVolume(volume);
    if (m_music_source) {
        f32 current_volume;
        alGetSourcef(m_music_source, AL_GAIN, &current_volume);
        alSourcef(m_music_source, AL_GAIN, current_volume * m_master_music_volume);
    }

    ENGINE_LOG_DEBUG("Set master music volume to {}", m_master_music_volume);
}

void AudioManager::SetMusicLooping(bool loop)
{
    m_music_looping = loop; // Update state even if no source active
    if (m_music_source) {
        alSourcei(m_music_source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        ALenum err = alGetError();
        if (err != AL_NO_ERROR) {
            ENGINE_LOG_ERROR("Failed to set music looping: {}", alGetString(err));
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
        max_mono_sources = -1; // Flag to use trial-and-error
    }

    // Trial-and-error if query failed
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

ALuint AudioManager::GetFreeSource()
{
    if (!m_source_pool.empty()) {
        ALuint source{m_source_pool.back()};
        m_source_pool.pop_back();
        ENGINE_LOG_DEBUG("Reusing source {}", source);
        return source;
    }

    ALuint source;
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
    ALenum result = alGetError();
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
    int buffers_queued = 0;
    for (ALuint buffer : m_music_buffers) {
        size_t frames_read = p_current_track->ReadFrames(temp.data(), FRAMES_PER_BUFFER);
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

} // end namespace GoudaVK