#pragma once

#include <format>
#include <source_location>
#include <stdexcept>
#include <string>

#if defined(__cpp_lib_stacktrace) && __cpp_lib_stacktrace >= 202011L
#include <stacktrace>
#endif

// Helper function to get the source location details
constexpr std::string get_source_info(const std::source_location &loc = std::source_location::current())
{
    return std::format("{}:{} ({})", loc.file_name(), loc.line(), loc.function_name());
}

// Helper function for optional stack trace (C++23 feature)
inline std::string get_stack_trace()
{
#if defined(__cpp_lib_stacktrace) && __cpp_lib_stacktrace >= 202011L
    return std::format("\nStacktrace:\n{}", std::stacktrace::current());
#else
    return "";
#endif
}

// Define throwing macros with formatted messages and source location
#define THROW_EXCEPTION(prefix, msg, ...)                                                                              \
    throw std::runtime_error(                                                                                          \
        std::format("{} {} - {}{}", prefix, std::format(msg, ##__VA_ARGS__), get_source_info(), get_stack_trace()));

// Application and Engine specific macros
#define APP_THROW(msg, ...) THROW_EXCEPTION("[APP ERROR]", msg, ##__VA_ARGS__)
#define ENGINE_THROW(msg, ...) THROW_EXCEPTION("[ENGINE ERROR]", msg, ##__VA_ARGS__)

// Overloads for cases without additional arguments
#define APP_THROW_NO_MSG() APP_THROW("An application error occurred")
#define ENGINE_THROW_NO_MSG() ENGINE_THROW("An engine error occurred")