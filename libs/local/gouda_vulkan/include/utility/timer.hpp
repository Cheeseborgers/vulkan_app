#pragma once

#include <chrono>
#include <iostream>
#include <thread>

namespace Gouda {

using SteadyClock = std::chrono::steady_clock;
using f32 = float;
using f64 = double;

/**
 * @class FrameTimer
 * @brief Tracks delta time (time between frames).
 *
 * This class calculates the time elapsed between two frames.
 * It is useful for animations, movement updates, and frame-dependent calculations.
 *
 * @example
 * @code
 * FrameTimer timer;
 * while (running) {
 *     timer.Update();
 *     float deltaTime = timer.GetDeltaTime();
 *     MoveCharacter(deltaTime);
 * }
 * @endcode
 */
class FrameTimer {
public:
    FrameTimer() : previous_time(SteadyClock::now()), delta_time(0.0f) {}

    /// Updates the timer, calculating delta time since the last frame.
    void Update()
    {
        auto current_time = SteadyClock::now();
        std::chrono::duration<f32> frame_time = current_time - previous_time;
        previous_time = current_time;
        delta_time = frame_time.count();
    }

    /// Gets the time in seconds since the last frame.
    f32 GetDeltaTime() const { return delta_time; }

private:
    SteadyClock::time_point previous_time;
    f32 delta_time;
};

/**
 * @class FixedTimer
 * @brief Ensures fixed timestep updates for physics simulations.
 *
 * Fixed timesteps prevent instability and make physics simulations consistent across different frame rates.
 *
 * @example
 * @code
 * FixedTimer physicsTimer(1.0f / 60.0f);
 * while (running) {
 *     float deltaTime = frameTimer.GetDeltaTime();
 *     physicsTimer.UpdateAccumulator(deltaTime);
 *     while (physicsTimer.ShouldUpdate()) {
 *         PhysicsUpdate(1.0f / 60.0f);
 *         physicsTimer.Advance();
 *     }
 * }
 * @endcode
 */
class FixedTimer {
public:
    explicit FixedTimer(f32 timestep) : fixed_timestep(timestep), accumulator(0.0f) {}

    /// Adds the delta time to the accumulator.
    void UpdateAccumulator(f32 delta_time)
    {
        accumulator += delta_time;
        if (accumulator > 0.25f)
            accumulator = 0.25f; // Prevents physics explosions
    }

    /// Checks if enough time has accumulated for a physics update.
    bool ShouldUpdate() const { return accumulator >= fixed_timestep; }

    /// Advances the accumulator after a physics update.
    void Advance() { accumulator -= fixed_timestep; }

    /// Returns the fixed time step value.
    f32 GetFixedTimeStep() const { return fixed_timestep; }

private:
    f32 fixed_timestep;
    f32 accumulator;
};

/**
 * @class FrameRateLimiter
 * @brief Caps the frame rate to a target FPS.
 *
 * Helps in reducing CPU/GPU load and prevents unnecessary rendering.
 *
 * @example
 * @code
 * FrameRateLimiter limiter(60.0f);
 * while (running) {
 *     float deltaTime = frameTimer.GetDeltaTime();
 *     Render();
 *     limiter.Limit(deltaTime);
 * }
 * @endcode
 */
class FrameRateLimiter {
public:
    explicit FrameRateLimiter(f32 target_fps) : target_frame_time(1.0f / target_fps) {}

    /// Delays the frame to maintain the target FPS.
    void Limit(f32 delta_time) const
    {
        f32 frame_time_remaining = target_frame_time - delta_time;
        if (frame_time_remaining > 0.0f) {
            std::this_thread::sleep_for(std::chrono::duration<f32>(frame_time_remaining));
        }
    }

private:
    f32 target_frame_time;
};

/**
 * @class ScopedTimer
 * @brief Measures execution time of a function or block.
 *
 * Useful for profiling performance.
 *
 * @example
 * @code
 * void SomeExpensiveFunction() {
 *     ScopedTimer timer("SomeExpensiveFunction");
 *     PerformHeavyComputation();
 * }
 * @endcode
 */
class ScopedTimer {
public:
    explicit ScopedTimer(const char *name) : label(name), start(SteadyClock::now()) {}

    ~ScopedTimer()
    {
        auto end = SteadyClock::now();
        std::chrono::duration<f32> duration = end - start;
        std::cout << label << " took " << duration.count() << " seconds\n";
    }

private:
    const char *label;
    SteadyClock::time_point start;
};

/**
 * @class GameClock
 * @brief Manages time scaling and pausing functionality.
 *
 * Allows time to be sped up, slowed down, or paused entirely.
 *
 * @example
 * @code
 * GameClock clock;
 * clock.SetTimeScale(0.5f); // Slow motion (50% speed)
 * while (running) {
 *     float deltaTime = frameTimer.GetDeltaTime();
 *     deltaTime = clock.ApplyTimeScale(deltaTime);
 *     UpdateGame(deltaTime);
 * }
 * @endcode
 */
class GameClock {
public:
    GameClock() : time_scale(1.0f), paused(false) {}

    /// Sets the time scaling factor (e.g., 0.5x for slow motion).
    void SetTimeScale(f32 scale) { time_scale = scale; }

    /// Gets the current time scale factor.
    f32 GetTimeScale() const { return time_scale; }

    /// Pauses the game clock.
    void Pause() { paused = true; }

    /// Resumes the game clock.
    void Resume() { paused = false; }

    /// Checks if the game is paused.
    bool IsPaused() const { return paused; }

    /// Applies time scaling to a given delta time.
    f32 ApplyTimeScale(f32 delta_time) const { return paused ? 0.0f : delta_time * time_scale; }

private:
    f32 time_scale;
    bool paused;
};

/**
 * @class InterpolationTimer
 * @brief Computes interpolation factor for smooth rendering.
 *
 * Allows interpolation between physics updates for smoother visual representation.
 *
 * @example
 * @code
 * InterpolationTimer interp(1.0f / 60.0f);
 * while (running) {
 *     float deltaTime = frameTimer.GetDeltaTime();
 *     interp.Update(deltaTime);
 *     float alpha = interp.GetInterpolationFactor();
 *     RenderScene(alpha);
 * }
 * @endcode
 */
class InterpolationTimer {
public:
    explicit InterpolationTimer(f32 timestep) : fixed_timestep(timestep), accumulator(0.0f) {}

    /// Adds the delta time to the accumulator.
    void Update(f32 delta_time) { accumulator += delta_time; }

    /// Returns the interpolation factor (between 0 and 1).
    f32 GetInterpolationFactor() const { return accumulator / fixed_timestep; }

private:
    f32 fixed_timestep;
    f32 accumulator;
};

} // namespace Gouda end