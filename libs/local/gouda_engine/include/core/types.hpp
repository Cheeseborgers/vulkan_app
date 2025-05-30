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
inline constexpr u8 u8_max = std::numeric_limits<uint8_t>::max();
inline constexpr u16 u16_max = std::numeric_limits<uint16_t>::max();
inline constexpr u32 u32_max = std::numeric_limits<uint32_t>::max();
inline constexpr u64 u64_max = std::numeric_limits<uint64_t>::max();

// Signed int numerical limits
inline constexpr s8 s8_max = std::numeric_limits<int8_t>::max();
inline constexpr s16 s16_max = std::numeric_limits<int16_t>::max();
inline constexpr s32 s32_max = std::numeric_limits<int32_t>::max();
inline constexpr s64 s64_max = std::numeric_limits<int64_t>::max();

inline constexpr f32 f32_max = std::numeric_limits<float>::max();
inline constexpr f64 f64_max = std::numeric_limits<double>::max();

inline constexpr size_t size_t_max = std::numeric_limits<size_t>::max();

inline constexpr size_t kb = 1024;
inline constexpr size_t mb = kb * 1024;
inline constexpr size_t gb = mb * 1024;

inline constexpr f32 gravity = -9.81f;
inline constexpr f32 epsilon = 1e-6f;
inline constexpr f32 infinity = std::numeric_limits<float>::infinity();
inline constexpr f32 pi = 3.14159265359f;
inline constexpr f32 double_pi = 2 * pi;

inline constexpr f32 z_max = 0.0f;
inline constexpr f32 z_min = -0.9f;

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

    [[nodiscard]] constexpr bool operator==(const SemVer &other) const
    {
        return major == other.major && minor == other.minor && patch == other.patch && variant == other.variant;
    }

    [[nodiscard]] constexpr bool operator!=(const SemVer &other) const { return !(*this == other); }

    [[nodiscard]] std::string ToString() const
    {
        return std::format("major: {}, minor: {}, patch: {}, variant: {}", major, minor, patch, variant);
    }
};

template <NumericT T>
struct Dimensions {
    T width;
    T height;

    explicit constexpr Dimensions(T w = T(), T h = T()) : width(w), height(h) {}

    [[nodiscard]] constexpr bool operator==(const Dimensions &other) const { return width == other.width && height == other.height; }

    [[nodiscard]] constexpr bool operator!=(const Dimensions &other) const { return !(*this == other); }

    [[nodiscard]] constexpr T area() const { return width * height; }

    [[nodiscard]] std::string ToString() const { return std::format("{}x{}", width, height); }
};

struct SpriteRect {
    float x;
    float y;
    float width;
    float height;

    constexpr SpriteRect() : x(0), y(0), width(0), height(0) {}

    explicit constexpr SpriteRect(const float x_ = 0, const float y_ = 0, const float width_ = 0, const float height_ = 0)
        : x{x_}, y{y_}, width{width_}, height{height_}
    {
    }

    explicit constexpr SpriteRect(const float value = 0) : x{value}, y{value}, width{value}, height{value} {}

    [[nodiscard]] constexpr bool operator==(const SpriteRect &other) const
    { return width == other.width && height == other.height && x == other.x && y == other.y; }

    [[nodiscard]] constexpr bool operator!=(const SpriteRect &other) const { return !(*this == other); }

    [[nodiscard]] constexpr float area() const { return width * height; }

    [[nodiscard]] std::string ToString() const
    {
        return std::format("x: {}, y: {}, width: {}, height: {}", x, y, width, height);
    }
};

template <NumericT T>
struct UVRect {
    T u_min;
    T v_min;
    T u_max;
    T v_max;

    constexpr UVRect() : u_min{0}, v_min{0}, u_max{0}, v_max{0} {}

    explicit constexpr UVRect(T u_min_, T v_min_, T u_max_, T v_max_)
        : u_min{u_min_}, v_min{v_min_}, u_max{u_max_}, v_max{v_max_}
    {
    }

    explicit constexpr UVRect(T value) : u_min{value}, v_min{value}, u_max{value}, v_max{value} {}

    [[nodiscard]] constexpr bool operator==(const UVRect &other) const
    { return u_min == other.u_min && v_min == other.v_min && u_max == other.u_max && v_max == other.v_max; }

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

    constexpr Rect() : left{0}, right{0}, bottom{0}, top{0} {}

    explicit constexpr Rect(T left_ = T(), T right_ = T(), T bottom_ = T(), T top_ = T())
        : left{left_}, right{right_}, bottom{bottom_}, top{top_}
    {
    }

    [[nodiscard]] bool operator==(const Rect &other) const
    {
        return left == other.left && right == other.right && bottom == other.bottom && top == other.top;
    }

    [[nodiscard]] bool operator!=(const Rect &other) const { return !(*this == other); }

    [[nodiscard]] std::string ToString() const
    {
        return std::string(std::format("left: {}, right: {}, bottom: {}, top: {}", left, right, bottom, top));
    }

    [[nodiscard]] bool IsZero() const
    {
        return left == T() && right == T() && top == T() && bottom == T();
    }
};


template <NumericT T>
struct Colour {
    union {
        struct {
            T r, g, b, a;
        };
        std::array<T, 4> data;
    };

    Colour(T r_, T g_, T b_, T a_) : r{r_}, g{g_}, b{b_}, a{a_} {}
    explicit constexpr Colour(T all = T{}) : r{all}, g{all}, b{all}, a{all} {}

    [[nodiscard]] constexpr T& operator[](std::size_t i) { return data[i]; }
    [[nodiscard]] constexpr const T& operator[](std::size_t i) const { return data[i]; }

    [[nodiscard]] constexpr std::span<T, 4> as_span() { return data; }
    [[nodiscard]] constexpr std::span<const T, 4> as_span() const { return data; }

    [[nodiscard]] std::string ToString() const {
        return std::format("r: {}, g: {}, b: {}, a: {}", r, g, b, a);
    }
};

using FrameBufferSize = Dimensions<int>;
using WindowSize = Dimensions<int>;
using ImageSize = Dimensions<int>;
using AtlasSize = Dimensions<float>;
