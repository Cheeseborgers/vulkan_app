#pragma once

/**
 * @file profiler.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-27
 * @brief Engine module
 *
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <chrono>
#include <fstream>
#include <mutex>
#include <source_location>
#include <string_view>
#include <thread>

#include "core/types.hpp"

namespace gouda::internal::profiler {

/**
 * @struct ProfileResult
 * @brief Holds information about a single profiling event.
 */
struct ProfileResult {
    std::string_view name;           // Name of the profiled function or scope
    FloatingPointMicroseconds start; // Start time in microseconds
    Microseconds elapsed_time;       // Duration of the event
    std::thread::id thread_id;       // ID of the thread executing the event
};

/**
 * @struct ProfilingSession
 * @brief Holds metadata about the current profiling session.
 */
struct ProfilingSession {
    std::string name; // Name of the session
};

/**
 * @class Profiler
 * @brief Singleton class to manage profiling sessions and write results.
 */
class Profiler {
public:
    Profiler(const Profiler &) = delete;
    Profiler(Profiler &&) = delete;

    /**
     * @brief Starts a new profiling session.
     * @param name Name of the session.
     * @param filepath Path to the output file for profiling results.
     */
    void BeginSession(std::string_view name, std::string_view filepath);

    /**
     * @brief Ends the current profiling session.
     */
    void EndSession();

    /**
     * @brief Writes a profiling result to the output file.
     * @param result The profiling result to write.
     */
    void Write(const ProfileResult &result);

    /**
     * @brief Gets the singleton instance of the Profiler.
     * @return Reference to the Profiler instance.
     */
    static Profiler &Get()
    {
        static Profiler instance;
        return instance;
    }

private:
    Profiler();
    ~Profiler() { EndSession(); }

    void WriteHeader();
    void WriteFooter();
    void InternalEndSession();

private:
    std::mutex m_mutex;                                  // Protects session state and output stream
    std::unique_ptr<ProfilingSession> p_current_session; // Current session metadata
    std::ofstream m_output_stream;                       // Output stream for profiling data
};

/**
 * @class ProfilingTimer
 * @brief Measures the duration of a scope and records it with the Profiler.
 */
class ProfilingTimer {
public:
    /**
     * @brief Constructs a timer with a name or function name from source location.
     * @param name Name of the scope (optional).
     * @param loc Source location for automatic function name retrieval.
     */
    ProfilingTimer(std::string_view name, std::source_location loc = std::source_location::current())
        : m_name{loc.function_name()}, m_start_timepoint{SteadyClock::now()}, m_stopped{false}
    {
    }

    ~ProfilingTimer()
    {
        if (!m_stopped)
            Stop();
    }

    /**
     * @brief Stops the timer and records the result.
     */
    void Stop()
    {
        auto endTimepoint = SteadyClock::now();
        auto highResStart = FloatingPointMicroseconds{m_start_timepoint.time_since_epoch()};
        auto elapsedTime = std::chrono::time_point_cast<Microseconds>(endTimepoint).time_since_epoch() -
                           std::chrono::time_point_cast<Microseconds>(m_start_timepoint).time_since_epoch();

        Profiler::Get().Write({m_name, highResStart, elapsedTime, std::this_thread::get_id()});
        m_stopped = true;
    }

private:
    std::string_view m_name;                                // Name of the scope or function
    std::chrono::time_point<SteadyClock> m_start_timepoint; // Start time of the measurement
    bool m_stopped;                                         // Flag to prevent double-stopping
};

/**
 * @class ProfileSessionGuard
 * @brief RAII class to manage the lifetime of a profiling session.
 */
class ProfileSessionGuard {
public:
    /**
     * @brief Starts a profiling session.
     * @param name Name of the session.
     * @param filepath Path to the output file.
     */
    ProfileSessionGuard(std::string_view name, std::string_view filepath)
    {
        Profiler::Get().BeginSession(name, filepath);
    }

    ~ProfileSessionGuard() { Profiler::Get().EndSession(); }
};

} // namespace gouda::internal::profiler

// Profiling Macros
#define ENGINE_PROFILE

#ifdef ENGINE_PROFILE
/**
 * @brief Starts a profiling session.
 * @param name Session name.
 * @param filepath Output file path.
 */
#define ENGINE_PROFILE_SESSION(name, filepath)                                                                         \
    gouda::internal::profiler::ProfileSessionGuard guard { name, filepath }

/**
 * @brief Profiles a named scope.
 * @param name Name of the scope.
 */
#define ENGINE_PROFILE_SCOPE(name)                                                                                     \
    gouda::internal::profiler::ProfilingTimer timer { name }

/**
 * @brief Profiles the current function using source location.
 */
#define ENGINE_PROFILE_FUNCTION()                                                                                      \
    gouda::internal::profiler::ProfilingTimer timer { std::source_location::current() }
#else
#define ENGINE_PROFILE_SESSION(name, filepath)
#define ENGINE_PROFILE_SCOPE(name)
#define ENGINE_PROFILE_FUNCTION()
#endif