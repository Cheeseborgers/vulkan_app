#pragma once

#include <format>
#include <iostream>
#include <source_location>
#include <string_view>

#define STATIC_ASSERT(expr) static_assert(expr, "Static assertion failed: " #expr);

#ifndef NDEBUG

namespace assert_internal {

#if defined(_WIN32) || defined(_WIN64)
#define ASSERT_INTERNAL_DEBUGBREAK() __debugbreak()
#elif defined(__linux__)
#include <signal.h>
#define ASSERT_INTERNAL_DEBUGBREAK() raise(SIGTRAP)
#else
#define ASSERT_INTERNAL_DEBUGBREAK()
#endif

void assert_print(const std::source_location &location);

void assert_print(const std::source_location &location, std::string_view format, const std::format_args &args);

void assert_impl(const std::source_location &location, bool check);

// Handle case where no format arguments are provided
inline void assert_impl(const std::source_location &location, bool check, std::string_view format)
{
    if (!check) {
        assert_print(location, format, std::make_format_args());
        ASSERT_INTERNAL_DEBUGBREAK();
    }
}

// Template to handle cases with format arguments
template <typename... Args>
inline void assert_impl(const std::source_location &location, bool check, std::string_view format, Args &&...args)
{
    if (!check) {
        // Here we create temporary references to handle the rvalue issue
        auto temp_args_ = std::make_tuple(std::forward<Args>(args)...);
        std::apply([&](auto &&...temp_args) { assert_print(location, format, std::make_format_args(temp_args...)); },
                   temp_args_);
        ASSERT_INTERNAL_DEBUGBREAK();
    }
}

} // namespace assert_internal

// Define ASSERT macro
#define ASSERT(...) assert_internal::assert_impl(std::source_location::current(), __VA_ARGS__ __VA_OPT__(, ) true)

#else
#define ASSERT(...)
#endif