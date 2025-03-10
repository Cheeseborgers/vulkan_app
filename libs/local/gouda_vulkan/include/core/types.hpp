#pragma once

#ifndef _WIN64
#include <unistd.h>
#endif

#include <chrono>
#include <concepts>
#include <cstdint>
#include <functional>
#include <sstream>
#include <string>

using uint = unsigned int;
using ushort = unsigned short;
using uchar = unsigned char;

using u8 = uint8_t;
using i8 = int8_t;
using u16 = uint16_t;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

using HighResClock = std::chrono::high_resolution_clock;
using SystemClock = std::chrono::system_clock;
using SteadyClock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;
using Microseconds = std::chrono::microseconds;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
using DateTime = std::chrono::system_clock::time_point;

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

template <typename T>
concept NumericType = std::integral<T> || std::floating_point<T>;

template <NumericType T>
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

template <typename... Args>
using CallbackFunction = std::function<void(Args...)>;

using FrameBufferSize = Dimensions<int>;
using WindowSize = Dimensions<int>;
using ImageSize = Dimensions<int>;
