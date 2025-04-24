#pragma once

#ifndef _WIN64
#include <unistd.h>
#endif

#include <chrono>
#include <concepts>
#include <expected>
#include <filesystem>
#include <functional>
#include <limits>

using uint = unsigned int;
using ushort = unsigned short;
using uchar = unsigned char;

// Integer type definitions
using u8 = uint8_t;
using s8 = int8_t;
using u16 = uint16_t;
using s16 = int16_t;
using u32 = uint32_t;
using s32 = int32_t;
using u64 = uint64_t;
using s64 = int64_t;

// Floating point types
using f32 = float;
using f64 = double;

struct Constants {
    static constexpr uint8_t u8_max = std::numeric_limits<uint8_t>::max();
    static constexpr uint16_t u16_max = std::numeric_limits<uint16_t>::max();
    static constexpr uint32_t u32_max = std::numeric_limits<uint32_t>::max();
    static constexpr uint64_t u64_max = std::numeric_limits<uint64_t>::max();

    static constexpr int8_t s8_max = std::numeric_limits<int8_t>::max();
    static constexpr int16_t s16_max = std::numeric_limits<int16_t>::max();
    static constexpr int32_t s32_max = std::numeric_limits<int32_t>::max();
    static constexpr int64_t s64_max = std::numeric_limits<int64_t>::max();

    static constexpr float f32_max = std::numeric_limits<float>::max();
    static constexpr double f64_max = std::numeric_limits<double>::max();
};

// Chrono types
using HighResClock = std::chrono::high_resolution_clock;
using SystemClock = std::chrono::system_clock;
using SteadyClock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;
using Microseconds = std::chrono::microseconds;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
using DateTime = std::chrono::system_clock::time_point;
using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

using FilePath = std::filesystem::path;

template <typename A, typename B>
using Expect = std::expected<A, B>;

template <typename... Args>
using CallbackFunction = std::function<void(Args...)>;

/**
 * @brief Concept that checks if a type is a floating-point type.
 * @tparam T The type to check.
 */
template <typename T>
concept FloatingPointT = std::is_floating_point_v<T>;

/**
 * @brief Concept that checks if a type is a integral type.
 * @tparam T The type to check.
 */
template <typename T>
concept IntegerT = std::is_integral_v<T>;

/**
 * @brief Concept that checks if a type is an integer or floating-point type.
 * @tparam T The type to check.
 */
template <typename T>
concept NumericT = std::is_arithmetic_v<T>;

struct SemVer {
    u32 major{0};
    u32 minor{0};
    u32 patch{0};
    u32 variant{0}; // Variant is typically 0 for Vulkan

    std::string ToString() const
    {
        std::ostringstream oss;
        oss << major << "." << minor << "." << patch;
        return oss.str();
    }

    bool operator==(const SemVer &other) const
    {
        return major == other.major && minor == other.minor && patch == other.patch && variant == other.variant;
    }

    bool operator<(const SemVer &other) const
    {
        return std::tie(major, minor, patch, variant) < std::tie(other.major, other.minor, other.patch, other.variant);
    }
};

template <NumericT T>
struct Dimensions {
    T width;
    T height;

    Dimensions(T w = T(), T h = T()) : width(w), height(h) {}

    // Equality comparison
    bool operator==(const Dimensions &other) const { return width == other.width && height == other.height; }

    // Inequality comparison
    bool operator!=(const Dimensions &other) const { return !(*this == other); }

    T area() const { return width * height; }

    std::string ToString() { return std::string(std::format("{}x{}", width, height)); }
};

template <NumericT T>
struct Rect {
    T left;
    T right;
    T bottom;
    T top;

    Rect(T left_ = T(), T right_ = T(), T bottom_ = T(), T top_ = T())
        : left{left_}, right{right_}, bottom{bottom_}, top{top_}
    {
    }

    // Equality comparison
    bool operator==(const Rect &other) const
    {
        return left == other.left && right == other.right && bottom == other.bottom && top == other.top;
    }

    // Inequality comparison
    bool operator!=(const Rect &other) const { return !(*this == other); }

    std::string ToString()
    {
        return std::string(std::format("left: {}, right: {}, bottom: {}, top: {}", left, right, bottom, top));
    }
};

template <NumericT T>
struct Colour {
    T value[4];

    Colour(T r, T g, T b, T a)
    {
        value[0] = r;
        value[1] = g;
        value[2] = b;
        value[3] = a;
    }
};

using FrameBufferSize = Dimensions<int>;
using WindowSize = Dimensions<int>;
using ImageSize = Dimensions<int>;
