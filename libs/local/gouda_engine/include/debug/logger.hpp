#pragma once
/**
 * @file debug/logger.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-27
 * @brief Engine logger module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <chrono>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <syncstream>

#include "containers/small_vector.hpp"
#include "core/types.hpp"
#include "debug/assert.hpp"
#include "debug/stacktrace.hpp"

// TODO: REMOVE THIS!!
#define APP_LOG_LEVEL_TRACE 1
#define ENGINE_LOG_LEVEL_TRACE 1

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace gouda {

enum class LogLevel : u8 { Trace, Debug, Info, Warning, Error, Fatal };

class Logger {
private:
    using Sink = std::function<void(StringView)>;

protected:
    explicit Logger(StringView file_path = "")
        : m_file_path(file_path),
          m_min_level{LogLevel::Trace},
          m_log_to_file(!file_path.empty()),
          m_buffered{false}
    {
        if (m_log_to_file) {
            m_file_stream.open(m_file_path, std::ios::out | std::ios::app);
            if (!m_file_stream) {
                std::print(std::cerr, "[Logger] Failed to open log file: {}\n", m_file_path);
                m_log_to_file = false;
            }
            else {
                m_sinks.push_back(
                    [this](StringView msg) { std::osyncstream(m_file_stream) << msg << std::endl; });
            }
        }
        m_sinks.push_back([](StringView msg) { std::osyncstream(std::cout) << msg << std::endl; });
    }

    ~Logger()
    {
        if (m_file_stream.is_open()) {
            m_file_stream.close();
        }
    }

    static std::string GetTimestamp()
    {
        const auto now = SystemClock::now();
        auto local_time = std::chrono::floor<Milliseconds>(now);
        return std::format("{:%Y-%m-%d %H:%M:%S}", local_time);
    }

    // Variadic template for formatted Logging
    template <typename... Args>
    void Log(LogLevel level, StringView prefix, StringView format_str, StringView tag = "",
             const std::source_location &loc = std::source_location::current(), Args &&...args)
    {
        // Create a tuple to store the arguments by value or move
        auto args_tuple = std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
        Log_impl(level, prefix, format_str, tag, loc, args_tuple, std::index_sequence_for<Args...>{});
    }

    // Helper function to unpack tuple into std::make_format_args
    template <typename... TupleArgs, std::size_t... I>
    void Log_impl(LogLevel level, StringView prefix, StringView format_str, StringView tag,
                  const std::source_location &loc, std::tuple<TupleArgs...> &args_tuple, std::index_sequence<I...>)
    {
        if (static_cast<u8>(level) < static_cast<u8>(m_min_level)) {
            return;
        }

        String timestamp{GetTimestamp()};
        static constexpr std::array<StringView, 6> level_strings{"[TRACE] ", "[DEBUG] ", "[INFO] ", "[WARNING] ", "[ERROR] ",
                                                                       "[FATAL] "};
        // Set and Ensure level is within bounds
        const u8 level_index{static_cast<u8>(level)};
        ASSERT(level_index < static_cast<u8>(level_strings.size()), "Log level is out of bounds.");

        String full_prefix{tag.empty() ? String(prefix) : std::format("{} [{}] ", prefix, tag)};
        String message{std::vformat(format_str, std::make_format_args(std::get<I>(args_tuple)...))};

        // Common Log format, optionally including source location
        String Log_message{
            level == LogLevel::Info || level == LogLevel::Debug
                ? std::format("{} {}{}{}", timestamp, full_prefix, level_strings[level_index], message)
                : std::format("{} {}{}{} ({}:{}:{})", timestamp, full_prefix, level_strings[level_index], message,
                              loc.file_name(), loc.line(), loc.column())};

        if (m_buffered) {
            std::lock_guard lock(m_buffer_mutex);
            m_buffer.push_back({level, Log_message});
        }
        else {
            //if (level == LogLevel::Trace) { // Trace does not need to set or reset console colour
            //    for (const auto &sink : m_sinks) {
             //       sink(Log_message);
             //   }

             //   return; // Return early, we got no further business here.
            //}

            SetConsoleColor(level);
            for (const auto &sink : m_sinks) {
                sink(Log_message);
            }
            ResetConsoleColor();
        }

        if (level == LogLevel::Fatal) {
            internal::print_stacktrace();
        }
    }

    void SetConsoleColor(LogLevel level)
    {

#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole == INVALID_HANDLE_VALUE)
            return;
        switch (level) {
            case LogLevel::Debug:
                SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
                break;
            case LogLevel::Info:
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                break;
            case LogLevel::Warning:
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
                break;
            case LogLevel::Error:
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                break;
            case LogLevel::Fatal:
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                break;
            case LogLevel::Trace:
            default:
                SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);
                break;
        }
#else
        if (isatty(STDOUT_FILENO)) {
            switch (level) {
                case LogLevel::Debug:
                    std::cout << "\033[34m";
                    break;
                case LogLevel::Info:
                    std::cout << "\033[32m";
                    break;
                case LogLevel::Warning:
                    std::cout << "\033[33m";
                    break;
                case LogLevel::Error:
                    std::cout << "\033[31m";
                    break;
                case LogLevel::Fatal:
                    std::cout << "\033[91m";
                    break;
                case LogLevel::Trace:
                default:
                    std::cout << "\033[0m";
                    break;
            }
        }
#endif
    }

    void ResetConsoleColor()
    {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
#else
        if (isatty(STDOUT_FILENO)) {
            std::cout << "\033[0m";
        }
#endif
    }

    std::ofstream m_file_stream;
    String m_file_path;
    SmallVector<std::pair<LogLevel, String>, 1> m_buffer;
    SmallVector<Sink, 2, GrowthPolicyAddOne> m_sinks;
    std::mutex m_buffer_mutex;
    LogLevel m_min_level;
    bool m_log_to_file;
    bool m_buffered;

public:
    void SetLogLevel(const LogLevel level) { m_min_level = level; }
    void SetBuffered(const bool enabled)
    {
        std::lock_guard lock(m_buffer_mutex);
        m_buffered = enabled;
        if (!enabled) {
            Flush();
        }
    }

    void Flush()
    {
        std::lock_guard lock(m_buffer_mutex);
        for (const auto &[level, msg] : m_buffer) {
            SetConsoleColor(level);
            for (const auto &sink : m_sinks) {
                sink(msg);
            }

            ResetConsoleColor();
        }

        m_buffer.clear();
    }

    void AddSink(const Sink& sink)
    {
        std::lock_guard lock(m_buffer_mutex);
        m_sinks.push_back(sink);
    }
};

// TODO: FIX THIS TO WORK IN EVERY BUILD
class EngineLogger : public Logger {
public:
    static EngineLogger &GetInstance(const String &Log_file_path = "")
    {
        static std::once_flag init_flag;
        std::call_once(init_flag, [&]() {
            instance = new EngineLogger(Log_file_path);
#ifdef ENGINE_LOG_LEVEL_DEBUG
            // instance->SetLogLevel(LogLevel::Debug);
#elif defined(ENGINE_LOG_LEVEL_INFO)
            // instance->SetLogLevel(LogLevel::Info);
#else
            // instance->SetLogLevel(LogLevel::Warning);  // Default Log level
#endif
        });
        return *instance;
    }

    // Variadic template for formatted Logging
    template <typename... Args>
    void Log(LogLevel level, StringView format_str, StringView tag = "",
             const std::source_location &loc = std::source_location::current(), Args &&...args)
    {
        Logger::Log(level, "[ENGINE]", format_str, tag, loc, std::forward<Args>(args)...);
    }

    // Plain string overload
    void Log(const LogLevel level, StringView message, StringView tag = "",
             const std::source_location &loc = std::source_location::current())
    {
        Logger::Log(level, "[ENGINE]", message, tag, loc);
    }

private:
    explicit EngineLogger(const String &file_path) : Logger(file_path) {}
    inline static EngineLogger *instance{nullptr};
};

class AppLogger : public Logger {
public:
    static AppLogger &GetInstance(const String &file_path = "")
    {
        static std::once_flag init_flag;
        std::call_once(init_flag, [&] {
            instance = new AppLogger(file_path);
#ifdef APP_LOG_LEVEL_DEBUG
            // instance->SetLogLevel(LogLevel::Debug);
#elif defined(APP_LOG_LEVEL_INFO)
            // instance->SetLogLevel(LogLevel::Info);
#else
            // instance->SetLogLevel(LogLevel::Warning);  // Default Log level
#endif
        });
        return *instance;
    }

    // Variadic template for formatted Logging
    template <typename... Args>
    void Log(LogLevel level, StringView format_str, StringView tag = "",
             const std::source_location &loc = std::source_location::current(), Args &&...args)
    {
        Logger::Log(level, "[APP]", format_str, tag, loc, std::forward<Args>(args)...);
    }

    // Plain string overload
    void Log(const LogLevel level, StringView message, StringView tag = "",
             const std::source_location &loc = std::source_location::current())
    {
        Logger::Log(level, "[APP]", message, tag, loc);
    }

private:
    explicit AppLogger(const String &file_path) : Logger(file_path) {}
    inline static AppLogger *instance{nullptr};
};

} // namespace gouda

// Macros supporting both plain and formatted calls
#ifdef ENABLE_ENGINE_LOGGING
#define ENGINE_LOG_PLAIN(level, msg) gouda::EngineLogger::GetInstance().Log(level, msg)
#define ENGINE_LOG_TAG_PLAIN(level, tag, msg) gouda::EngineLogger::GetInstance().Log(level, msg, tag)
#define ENGINE_LOG(level, fmt, ...)                                                                                    \
    gouda::EngineLogger::GetInstance().Log(level, fmt, "", std::source_location::current(), ##__VA_ARGS__)
#define ENGINE_LOG_TAG(level, tag, fmt, ...)                                                                           \
    gouda::EngineLogger::GetInstance().Log(level, fmt, tag, std::source_location::current(), ##__VA_ARGS__)

#if defined(ENGINE_LOG_LEVEL_TRACE)
#define ENGINE_LOG_TRACE(fmt, ...) ENGINE_LOG(gouda::LogLevel::Trace, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_TRACE_TAG(tag, fmt, ...) ENGINE_LOG_TAG(gouda::LogLevel::Trace, tag, fmt, ##__VA_ARGS__)
#else
#define ENGINE_LOG_TRACE(fmt, ...)                                                                                     \
do {                                                                                                               \
} while (0)
#define ENGINE_LOG_TRACE_TAG(tag, fmt, ...)                                                                            \
do {                                                                                                               \
} while (0)
#endif


#if defined(ENGINE_LOG_LEVEL_DEBUG)
#define ENGINE_LOG_DEBUG(fmt, ...) ENGINE_LOG(gouda::LogLevel::Debug, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_DEBUG_TAG(tag, fmt, ...) ENGINE_LOG_TAG(gouda::LogLevel::Debug, tag, fmt, ##__VA_ARGS__)
#else
#define ENGINE_LOG_DEBUG(fmt, ...)                                                                                     \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_DEBUG_TAG(tag, fmt, ...)                                                                            \
    do {                                                                                                               \
    } while (0)
#endif

#if defined(ENGINE_LOG_LEVEL_INFO)
#define ENGINE_LOG_INFO(fmt, ...) ENGINE_LOG(gouda::LogLevel::Info, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_INFO_TAG(tag, fmt, ...) ENGINE_LOG_TAG(gouda::LogLevel::Info, tag, fmt, ##__VA_ARGS__)
#else
#define ENGINE_LOG_INFO(fmt, ...)                                                                                      \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_INFO_TAG(tag, fmt, ...)                                                                             \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(ENGINE_LOG_LEVEL_WARNING)
#define ENGINE_LOG_WARNING(fmt, ...) ENGINE_LOG(gouda::LogLevel::Warning, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_WARNING_TAG(tag, fmt, ...) ENGINE_LOG_TAG(gouda::LogLevel::Warning, tag, fmt, ##__VA_ARGS__)
#else
#define ENGINE_LOG_WARNING(fmt, ...)                                                                                   \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_WARNING_TAG(tag, fmt, ...)                                                                          \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(ENGINE_LOG_LEVEL_ERROR)
#define ENGINE_LOG_ERROR(fmt, ...) ENGINE_LOG(gouda::LogLevel::Error, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_ERROR_TAG(tag, fmt, ...) ENGINE_LOG_TAG(gouda::LogLevel::Error, tag, fmt, ##__VA_ARGS__)
#else
#define ENGINE_LOG_ERROR(fmt, ...)                                                                                     \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_ERROR_TAG(tag, fmt, ...)                                                                            \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(ENGINE_LOG_LEVEL_FATAL)
#define ENGINE_LOG_FATAL(fmt, ...) ENGINE_LOG(gouda::LogLevel::Fatal, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_FATAL_TAG(tag, fmt, ...) ENGINE_LOG_TAG(gouda::LogLevel::Fatal, tag, fmt, ##__VA_ARGS__)
#else
#define ENGINE_LOG_FATAL(fmt, ...)                                                                                     \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_FATAL_TAG(tag, fmt, ...)                                                                            \
    do {                                                                                                               \
    } while (0)
#endif
#else
#define ENGINE_LOG_PLAIN(level, msg)                                                                                   \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_TAG_PLAIN(level, tag, msg)                                                                          \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_TAG(level, tag, fmt, ...)                                                                           \
    do {                                                                                                               \
    } while (0)

#define ENGINE_LOG_TRACE(fmt, ...)                                                                                     \
do {                                                                                                               \
} while (0)
#define ENGINE_LOG_TRACE_TAG(tag, fmt, ...)                                                                            \
do {                                                                                                               \
} while (0)
#define ENGINE_LOG_DEBUG(fmt, ...)                                                                                     \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_DEBUG_TAG(tag, fmt, ...)                                                                            \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_INFO(fmt, ...)                                                                                      \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_INFO_TAG(tag, fmt, ...)                                                                             \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_WARNING(fmt, ...)                                                                                   \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_WARNING_TAG(tag, fmt, ...)                                                                          \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_ERROR(fmt, ...)                                                                                     \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_ERROR_TAG(tag, fmt, ...)                                                                            \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_FATAL(fmt, ...)                                                                                     \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_FATAL_TAG(tag, fmt, ...)                                                                            \
    do {                                                                                                               \
    } while (0)
#endif

#ifdef ENABLE_APP_LOGGING
#define APP_LOG_PLAIN(level, msg) gouda::AppLogger::GetInstance().Log(level, msg)
#define APP_LOG_TAG_PLAIN(level, tag, msg) gouda::AppLogger::GetInstance().Log(level, msg, tag)
#define APP_LOG(level, fmt, ...)                                                                                       \
    gouda::AppLogger::GetInstance().Log(level, fmt, "", std::source_location::current(), ##__VA_ARGS__)
#define APP_LOG_TAG(level, tag, fmt, ...)                                                                              \
    gouda::AppLogger::GetInstance().Log(level, fmt, tag, std::source_location::current(), ##__VA_ARGS__)

#if defined(APP_LOG_LEVEL_TRACE)
#define APP_LOG_TRACE(fmt, ...) APP_LOG(gouda::LogLevel::Trace, fmt, ##__VA_ARGS__)
#define APP_LOG_TRACE_TAG(tag, fmt, ...) APP_LOG_TAG(gouda::LogLevel::Trace, tag, fmt, ##__VA_ARGS__)
#else
#define APP_LOG_TRACE(fmt, ...)                                                                                        \
do {                                                                                                               \
} while (0)
#define APP_LOG_TRACE_TAG(tag, fmt, ...)                                                                               \
do {                                                                                                               \
} while (0)
#endif
#if defined(APP_LOG_LEVEL_DEBUG)
#define APP_LOG_DEBUG(fmt, ...) APP_LOG(gouda::LogLevel::Debug, fmt, ##__VA_ARGS__)
#define APP_LOG_DEBUG_TAG(tag, fmt, ...) APP_LOG_TAG(gouda::LogLevel::Debug, tag, fmt, ##__VA_ARGS__)
#else
#define APP_LOG_DEBUG(fmt, ...)                                                                                        \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_DEBUG_TAG(tag, fmt, ...)                                                                               \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(APP_LOG_LEVEL_INFO)
#define APP_LOG_INFO(fmt, ...) APP_LOG(gouda::LogLevel::Info, fmt, ##__VA_ARGS__)
#define APP_LOG_INFO_TAG(tag, fmt, ...) APP_LOG_TAG(gouda::LogLevel::Info, tag, fmt, ##__VA_ARGS__)
#else
#define APP_LOG_INFO(fmt, ...)                                                                                         \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_INFO_TAG(tag, fmt, ...)                                                                                \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(APP_LOG_LEVEL_WARNING)
#define APP_LOG_WARNING(fmt, ...) APP_LOG(gouda::LogLevel::Warning, fmt, ##__VA_ARGS__)
#define APP_LOG_WARNING_TAG(tag, fmt, ...) APP_LOG_TAG(gouda::LogLevel::Warning, tag, fmt, ##__VA_ARGS__)
#else
#define APP_LOG_WARNING(fmt, ...)                                                                                      \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_WARNING_TAG(tag, fmt, ...)                                                                             \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(APP_LOG_LEVEL_ERROR)
#define APP_LOG_ERROR(fmt, ...) APP_LOG(gouda::LogLevel::Error, fmt, ##__VA_ARGS__)
#define APP_LOG_ERROR_TAG(tag, fmt, ...) APP_LOG_TAG(gouda::LogLevel::Error, tag, fmt, ##__VA_ARGS__)
#else
#define APP_LOG_ERROR(fmt, ...)                                                                                        \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_ERROR_TAG(tag, fmt, ...)                                                                               \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(APP_LOG_LEVEL_FATAL)
#define APP_LOG_FATAL(fmt, ...) APP_LOG(gouda::LogLevel::Fatal, fmt, ##__VA_ARGS__)
#define APP_LOG_FATAL_TAG(tag, fmt, ...) APP_LOG_TAG(gouda::LogLevel::Fatal, tag, fmt, ##__VA_ARGS__)
#else
#define APP_LOG_FATAL(fmt, ...)                                                                                        \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_FATAL_TAG(tag, fmt, ...)                                                                               \
    do {                                                                                                               \
    } while (0)
#endif
#else
#define APP_LOG_PLAIN(level, msg)                                                                                      \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_TAG_PLAIN(level, tag, msg)                                                                             \
    do {                                                                                                               \
    } while (0)
#define APP_LOG(level, fmt, ...)                                                                                       \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_TAG(level, tag, fmt, ...)                                                                              \
    do {                                                                                                               \
    } while (0)

#define APP_LOG_TRACE(fmt, ...)                                                                                        \
do {                                                                                                               \
} while (0)
#define APP_LOG_TRACE_TAG(tag, fmt, ...)                                                                               \
do {                                                                                                               \
} while (0)
#define APP_LOG_DEBUG(fmt, ...)                                                                                        \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_DEBUG_TAG(tag, fmt, ...)                                                                               \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_INFO(fmt, ...)                                                                                         \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_INFO_TAG(tag, fmt, ...)                                                                                \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_WARNING(fmt, ...)                                                                                      \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_WARNING_TAG(tag, fmt, ...)                                                                             \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_ERROR(fmt, ...)                                                                                        \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_ERROR_TAG(tag, fmt, ...)                                                                               \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_FATAL(fmt, ...)                                                                                        \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_FATAL_TAG(tag, fmt, ...)                                                                               \
    do {                                                                                                               \
    } while (0)
#endif