/**
 * @file utility/random.hpp
 * @brief Random Number Generator (RNG) utilities for GoudaVK
 *
 * This file defines both a non-thread-safe and a thread-safe random number generator (RNG)
 * using the PCG (Permuted Congruential Generator) algorithm. These generators are compatible
 * with STL functions like std::shuffle.
 */

#pragma once

#include <mutex>
#include <random>

#include "gouda_types.hpp"
#include "pcg_random.hpp"

// TODO: Change namespace!!!

namespace GoudaVK {

/// Define the RNG type (PCG 32-bit)
using RNG = pcg32;

/**
 * @class BaseRNG
 * @brief Non-thread-safe RNG class.
 *
 * This class provides a simple RNG implementation using the PCG algorithm.
 * It is compatible with STL functions like std::shuffle.
 */
class BaseRNG {
public:
    using result_type = uint32_t; ///< Type required for STL random operations

public:
    /**
     * @brief Constructs the RNG with a given seed.
     * @param seed The seed value to initialize the RNG.
     */
    explicit BaseRNG(u32 seed);

    /**
     * @brief Generates a random number (needed for std::shuffle compatibility).
     * @return A randomly generated 32-bit unsigned integer.
     */
    result_type operator()() { return rng(); }

    /**
     * @brief Generates a random unsigned integer.
     * @return A random u32 value.
     */
    u32 GetUint();

    /**
     * @brief Generates a random signed integer in the range [min, max].
     * @param min The minimum possible value.
     * @param max The maximum possible value.
     * @return A random integer in [min, max].
     */
    int GetInt(int min, int max);

    /**
     * @brief Generates a random float in the range [min, max].
     * @param min The minimum possible value.
     * @param max The maximum possible value.
     * @return A random float in [min, max].
     */
    f32 GetFloat(f32 min, f32 max);

    /// @brief Returns the minimum possible value (required by STL algorithms).
    static constexpr BaseRNG::result_type min() { return RNG::min(); }

    /// @brief Returns the maximum possible value (required by STL algorithms).
    static constexpr BaseRNG::result_type max() { return RNG::max(); }

private:
    RNG rng; ///< Internal PCG RNG instance
};

/**
 * @class ThreadSafeRNG
 * @brief Thread-safe version of BaseRNG.
 *
 * Uses a mutex to ensure thread safety during RNG operations.
 */
class ThreadSafeRNG : public BaseRNG {
public:
    using result_type = BaseRNG::result_type; ///< Inherit result_type for std::shuffle compatibility

    /**
     * @brief Constructs the thread-safe RNG with a given seed.
     * @param seed The seed value to initialize the RNG.
     */
    explicit ThreadSafeRNG(u32 seed);

    /**
     * @brief Generates a random number in a thread-safe manner.
     * @return A randomly generated 32-bit unsigned integer.
     */
    u32 operator()();

    /// @copydoc BaseRNG::GetUint()
    u32 GetUint();

    /// @copydoc BaseRNG::GetInt()
    int GetInt(int min, int max);

    /// @copydoc BaseRNG::GetFloat()
    f32 GetFloat(f32 min, f32 max);

private:
    template <typename Func>
    auto WithLock(Func &&func)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return func();
    }

private:
    mutable std::mutex m_mutex; ///< Mutex for thread safety
};

/**
 * @brief Retrieves a global instance of a non-thread-safe RNG.
 * @return Reference to a static BaseRNG instance.
 * @note Uses a random device to seed the RNG.
 */
BaseRNG &GetGlobalRNG();

/**
 * @brief Retrieves a global instance of a thread-safe RNG.
 * @return Reference to a static ThreadSafeRNG instance.
 * @note Uses a random device to seed the RNG.
 */
ThreadSafeRNG &GetGlobalThreadSafeRNG();

} // end namespace GoudaVK
