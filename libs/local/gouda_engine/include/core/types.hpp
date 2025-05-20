#pragma once
/**
 * @file types.hpp
 * @author GoudaCheeseburgers
 * @date 2025-02-24
 * @brief Engine type def module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <chrono>
#include <expected>
#include <filesystem>
#include <functional>
#include <limits>
#include <span>
#include <array>

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

namespace constants {
// Unsigned int numerical limits
inline constexpr uint8_t u8_max = std::numeric_limits<uint8_t>::max();
inline constexpr uint16_t u16_max = std::numeric_limits<uint16_t>::max();
inline constexpr uint32_t u32_max = std::numeric_limits<uint32_t>::max();
inline constexpr uint64_t u64_max = std::numeric_limits<uint64_t>::max();

// Signed int numerical limits
inline constexpr int8_t s8_max = std::numeric_limits<int8_t>::max();
inline constexpr int16_t s16_max = std::numeric_limits<int16_t>::max();
inline constexpr int32_t s32_max = std::numeric_limits<int32_t>::max();
inline constexpr int64_t s64_max = std::numeric_limits<int64_t>::max();

inline constexpr float f32_max = std::numeric_limits<float>::max();
inline constexpr double f64_max = std::numeric_limits<double>::max();

inline constexpr size_t size_t_max = std::numeric_limits<size_t>::max();

inline constexpr size_t kb = 1024;
inline constexpr size_t mb = kb * 1024;
inline constexpr size_t gb = mb * 1024;

inline constexpr float gravity = -9.81f;
inline constexpr float epsilon = 1e-6f;
inline constexpr float infinity = std::numeric_limits<float>::infinity();
inline constexpr float pi = 3.14159265359f;

} // namespace constants

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

using String = std::basic_string<char>;
using StringView = std::basic_string_view<char>;
using FilePath = std::filesystem::path;
using FileTimeType = std::filesystem::file_time_type;

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
 * @brief Concept that checks if a type is an integral type.
 * @tparam T The type to check.
 */
template <typename T>
concept IntegerT = std::is_integral_v<T>;

/**
 * @brief Concept that checks if a type is an integer or floating-point type.
 * @tparam T The type to check.
 */
template <typename T>
concept NumericT = std::integral<T> || std::floating_point<T>;

struct SemVer {
    u32 major;
    u32 minor;
    u32 patch;
    u32 variant;

    explicit constexpr SemVer() : major{0}, minor{0}, patch{0}, variant{0} {}

    explicit constexpr SemVer(const u32 major_, const u32 minor_, const u32 patch_, const u32 variant_)
        : major(major_), minor(minor_), patch(patch_), variant(variant_)
    {
    }

    // Equality comparison
    [[nodiscard]] constexpr bool operator==(const SemVer &other) const
    {
        return major == other.major && minor == other.minor && patch == other.patch && variant == other.variant;
    }

    // Inequality comparison
    [[nodiscard]] constexpr bool operator!=(const SemVer &other) const { return !(*this == other); }

    [[nodiscard]] std::string ToString() const
    {
        return std::string(std::format("major: {}, minor: {}, patch: {}, variant: {}", major, minor, patch, variant));
    }
};

template <NumericT T>
struct Dimensions {
    T width;
    T height;

    explicit constexpr Dimensions(T w = T(), T h = T()) : width(w), height(h) {}

    // Equality comparison
    [[nodiscard]] constexpr bool operator==(const Dimensions &other) const { return width == other.width && height == other.height; }

    // Inequality comparison
    [[nodiscard]] constexpr bool operator!=(const Dimensions &other) const { return !(*this == other); }

    [[nodiscard]] constexpr T area() const { return width * height; }

    [[nodiscard]] std::string ToString() const { return std::string(std::format("{}x{}", width, height)); }
};

struct SpriteRect {
    float x;
    float y;
    float width;
    float height;

    explicit constexpr SpriteRect(const float x_ = 0, const float y_ = 0, const float width_ = 0, const float height_ = 0)
        : x{x_}, y{y_}, width{width_}, height{height_}
    {
    }

    explicit constexpr SpriteRect(const float value = 0) : x{value}, y{value}, width{value}, height{value} {}

    // Equality comparison
    [[nodiscard]] constexpr bool operator==(const SpriteRect &other) const
    { return width == other.width && height == other.height && x == other.x && y == other.y; }

    // Inequality comparison
    [[nodiscard]] constexpr bool operator!=(const SpriteRect &other) const { return !(*this == other); }

    [[nodiscard]] constexpr float area() const { return width * height; }

    [[nodiscard]] std::string ToString() const
    {
        return std::string(std::format("x: {}, y: {}, width: {}, height: {}", x, y, width, height));
    }
};

template <NumericT T>
struct UVRect {
    T u_min;
    T v_min;
    T u_max;
    T v_max;

    explicit constexpr UVRect(T u_min_ = T(), T v_min_ = T(), T u_max_ = T(), T v_max_ = T())
        : u_min{u_min_}, v_min{v_min_}, u_max{u_max_}, v_max{v_max_}
    {
    }

    explicit constexpr UVRect(T value) : u_min{value}, v_min{value}, u_max{value}, v_max{value} {}

    // Equality comparison
    [[nodiscard]] constexpr bool operator==(const UVRect &other) const
    { return u_min == other.u_min && v_min == other.v_min && u_max == other.u_max && v_max == other.v_max; }

    // Inequality comparison
    [[nodiscard]] constexpr bool operator!=(const SpriteRect &other) const { return !(*this == other); }

    [[nodiscard]] std::string ToString() const
    {
        return std::string(std::format("u_min: {}, v_min: {}, u_max: {}, v_max: {}", u_min, v_min, u_min, v_max));
    }
};

template <NumericT T>
struct Rect {
    T left;
    T right;
    T bottom;
    T top;

    explicit constexpr Rect(T left_ = T(), T right_ = T(), T bottom_ = T(), T top_ = T())
        : left{left_}, right{right_}, bottom{bottom_}, top{top_}
    {
    }

    // Equality comparison
    [[nodiscard]] bool operator==(const Rect &other) const
    {
        return left == other.left && right == other.right && bottom == other.bottom && top == other.top;
    }

    // Inequality comparison
    [[nodiscard]] bool operator!=(const Rect &other) const { return !(*this == other); }

    [[nodiscard]] std::string ToString() const
    {
        return std::string(std::format("left: {}, right: {}, bottom: {}, top: {}", left, right, bottom, top));
    }
};

template <NumericT T>
struct Colour {
    std::array<T, 4> value{};

    Colour(T r, T g, T b, T a) : value{r, g, b, a} {}
    explicit constexpr Colour(T all = T{}) : value{all, all, all, all} {}

    // Access as span
    [[nodiscard]] std::span<T, 4> as_span() { return std::span<T, 4>(value); }
    [[nodiscard]] std::span<const T, 4> as_span() const { return std::span<const T, 4>(value); }

    // Index access
    T& operator[](size_t i) { return value[i]; }
    const T& operator[](size_t i) const { return value[i]; }

    [[nodiscard]] std::string ToString() const {
        return std::format("r: {}, g: {}, b: {}, a: {}", value[0], value[1], value[2], value[3]);
    }
};

using FrameBufferSize = Dimensions<int>;
using WindowSize = Dimensions<int>;
using ImageSize = Dimensions<int>;
using AtlasSize = Dimensions<float>;
