// LoggerLib/include/Logger.hpp
#pragma once

#include <assert.h>
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
#include <vector>

// #include "stacktrace.hpp"

#ifdef __cpp_lib_stacktrace
#include <stacktrace>
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

enum class LogLevel : uint8_t { Debug, Info, Warning, Error, Fatal };

// TODO: Make sure all member vars are set in the constructor
// TODO: Sort stacktrace as a fallback for all supported platforms until gcc sorts its act out

class Logger {
protected:
    Logger(const std::string &log_file_path = "") : m_log_to_file(!log_file_path.empty()), m_file_path(log_file_path)
    {
        if (m_log_to_file) {
            m_file_stream.open(m_file_path, std::ios::out | std::ios::app);
            if (!m_file_stream) {
                std::print(std::cerr, "[Logger] Failed to open log file: {}\n", m_file_path);
                m_log_to_file = false;
            }
            else {
                m_sinks.push_back(
                    [this](std::string_view msg) { std::osyncstream(m_file_stream) << msg << std::endl; });
            }
        }
        m_sinks.push_back([](std::string_view msg) { std::osyncstream(std::cout) << msg << std::endl; });
    }

    ~Logger()
    {
        if (m_file_stream.is_open()) {
            m_file_stream.close();
        }
    }

    std::string get_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto local_time = std::chrono::floor<std::chrono::milliseconds>(now);
        return std::format("{:%Y-%m-%d %H:%M:%S}", local_time);
    }

    // Variadic template for formatted logging
    template <typename... Args>
    void log(LogLevel level, std::string_view prefix, std::string_view format_str, std::string_view tag = "",
             const std::source_location &loc = std::source_location::current(), Args &&...args)
    {
        // Create a tuple to store the arguments by value or move
        auto args_tuple = std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
        log_impl(level, prefix, format_str, tag, loc, args_tuple, std::index_sequence_for<Args...>{});
    }

    // Helper function to unpack tuple into std::make_format_args
    template <typename... TupleArgs, std::size_t... I>
    void log_impl(LogLevel level, std::string_view prefix, std::string_view format_str, std::string_view tag,
                  const std::source_location &loc, std::tuple<TupleArgs...> &args_tuple, std::index_sequence<I...>)
    {
        if (static_cast<int>(level) < static_cast<int>(m_min_level))
            return;

        // TODO: Add [DEBUG] flag to print crap when debugging
        std::string timestamp{get_timestamp()};
        static constexpr std::array<std::string_view, 5> level_strings = {"[DEBUG] ", "[INFO] ", "[WARNING] ",
                                                                          "[ERROR] ", "[FATAL] "};

        // Set and Ensure level is within bounds
        int level_index{static_cast<int>(level)};
        assert(level_index >= 0 && level_index < static_cast<int>(level_strings.size()));

        std::string full_prefix{tag.empty() ? std::string(prefix) : std::format("{} [{}] ", prefix, tag)};
        std::string message{std::vformat(format_str, std::make_format_args(std::get<I>(args_tuple)...))};

        // Common log format, optionally including source location
        std::string log_message{
            (level == LogLevel::Info || level == LogLevel::Debug)
                ? std::format("{} {}{}{}", timestamp, full_prefix, level_strings[level_index], message)
                : std::format("{} {}{}{} ({}:{}:{})", timestamp, full_prefix, level_strings[level_index], message,
                              loc.file_name(), loc.line(), loc.column())};

// TODO: Implement stacktraing for all platforms
#ifdef __cpp_lib_stacktrace
        if (level == LogLevel::Fatal) {
            stacktrace::print_stacktrace();
        }
#endif

        if (m_buffered) {
            std::lock_guard<std::mutex> lock(m_bufferMutex);
            m_buffer.push_back({level, log_message});
        }
        else {
            setConsoleColor(level);
            for (const auto &sink : m_sinks)
                sink(log_message);
            resetConsoleColor();
        }
    }

    void setConsoleColor(LogLevel level)
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
            }
        }
#endif
    }

    void resetConsoleColor()
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

    bool m_log_to_file;
    std::string m_file_path;
    std::ofstream m_file_stream;
    LogLevel m_min_level = LogLevel::Debug;
    bool m_buffered = false;
    std::mutex m_bufferMutex;
    std::vector<std::pair<LogLevel, std::string>> m_buffer;
    using Sink = std::function<void(std::string_view)>;
    std::vector<Sink> m_sinks;

public:
    void SetLogLevel(LogLevel level) { m_min_level = level; }
    void SetBuffered(bool enabled)
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffered = enabled;
        if (!enabled)
            flush();
    }
    void flush()
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        for (const auto &[level, msg] : m_buffer) {
            setConsoleColor(level);
            for (const auto &sink : m_sinks)
                sink(msg);
            resetConsoleColor();
        }
        m_buffer.clear();
    }
    void addSink(Sink sink)
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_sinks.push_back(sink);
    }
};

// TODO: FIX THIS TO WORK IN EVERY BUILD
class EngineLogger : public Logger {
public:
    static EngineLogger &GetInstance(const std::string &log_file_path = "")
    {
        static std::once_flag initFlag;
        std::call_once(initFlag, [&]() {
            instance = new EngineLogger(log_file_path);
#ifdef ENGINE_LOG_LEVEL_DEBUG
            // instance->SetLogLevel(LogLevel::Debug);
#elif defined(ENGINE_LOG_LEVEL_INFO)
            // instance->SetLogLevel(LogLevel::Info);
#else
            // instance->SetLogLevel(LogLevel::Warning);  // Default log level
#endif
        });
        return *instance;
    }

    // Variadic template for formatted logging
    template <typename... Args>
    void log(LogLevel level, std::string_view format_str, std::string_view tag = "",
             const std::source_location &loc = std::source_location::current(), Args &&...args)
    {
        Logger::log(level, "[ENGINE]", format_str, tag, loc, std::forward<Args>(args)...);
    }

    // Plain string overload
    void log(LogLevel level, std::string_view message, std::string_view tag = "",
             const std::source_location &loc = std::source_location::current())
    {
        Logger::log(level, "[ENGINE]", message, tag, loc);
    }

private:
    EngineLogger(const std::string &log_file_path) : Logger(log_file_path) {}
    inline static EngineLogger *instance = nullptr;
};

class AppLogger : public Logger {
public:
    static AppLogger &GetInstance(const std::string &log_file_path = "")
    {
        static std::once_flag initFlag;
        std::call_once(initFlag, [&]() {
            instance = new AppLogger(log_file_path);
#ifdef APP_LOG_LEVEL_DEBUG
            // instance->SetLogLevel(LogLevel::Debug);
#elif defined(APP_LOG_LEVEL_INFO)
            // instance->SetLogLevel(LogLevel::Info);
#else
            // instance->SetLogLevel(LogLevel::Warning);  // Default log level
#endif
        });
        return *instance;
    }

    // Variadic template for formatted logging
    template <typename... Args>
    void log(LogLevel level, std::string_view format_str, std::string_view tag = "",
             const std::source_location &loc = std::source_location::current(), Args &&...args)
    {
        Logger::log(level, "[GAME]", format_str, tag, loc, std::forward<Args>(args)...);
    }

    // Plain string overload
    void log(LogLevel level, std::string_view message, std::string_view tag = "",
             const std::source_location &loc = std::source_location::current())
    {
        Logger::log(level, "[GAME]", message, tag, loc);
    }

private:
    AppLogger(const std::string &log_file_path) : Logger(log_file_path) {}
    inline static AppLogger *instance = nullptr;
};

// Macros supporting both plain and formatted calls
#ifdef ENABLE_ENGINE_LOGGING
#define ENGINE_LOG_PLAIN(level, msg) EngineLogger::GetInstance().log(level, msg)
#define ENGINE_LOG_TAG_PLAIN(level, tag, msg) EngineLogger::GetInstance().log(level, msg, tag)
#define ENGINE_LOG(level, fmt, ...)                                                                                    \
    EngineLogger::GetInstance().log(level, fmt, "", std::source_location::current(), ##__VA_ARGS__)
#define ENGINE_LOG_TAG(level, tag, fmt, ...)                                                                           \
    EngineLogger::GetInstance().log(level, fmt, tag, std::source_location::current(), ##__VA_ARGS__)

#if defined(ENGINE_LOG_LEVEL_DEBUG)
#define ENGINE_LOG_DEBUG(fmt, ...) ENGINE_LOG(LogLevel::Debug, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_DEBUG_TAG(tag, fmt, ...) ENGINE_LOG_TAG(LogLevel::Debug, tag, fmt, ##__VA_ARGS__)
#else
#define ENGINE_LOG_DEBUG(fmt, ...)                                                                                     \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_DEBUG_TAG(tag, fmt, ...)                                                                            \
    do {                                                                                                               \
    } while (0)
#endif

#if defined(ENGINE_LOG_LEVEL_INFO)
#define ENGINE_LOG_INFO(fmt, ...) ENGINE_LOG(LogLevel::Info, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_INFO_TAG(tag, fmt, ...) ENGINE_LOG_TAG(LogLevel::Info, tag, fmt, ##__VA_ARGS__)
#else
#define ENGINE_LOG_INFO(fmt, ...)                                                                                      \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_INFO_TAG(tag, fmt, ...)                                                                             \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(ENGINE_LOG_LEVEL_WARNING)
#define ENGINE_LOG_WARNING(fmt, ...) ENGINE_LOG(LogLevel::Warning, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_WARNING_TAG(tag, fmt, ...) ENGINE_LOG_TAG(LogLevel::Warning, tag, fmt, ##__VA_ARGS__)
#else
#define ENGINE_LOG_WARNING(fmt, ...)                                                                                   \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_WARNING_TAG(tag, fmt, ...)                                                                          \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(ENGINE_LOG_LEVEL_ERROR)
#define ENGINE_LOG_ERROR(fmt, ...) ENGINE_LOG(LogLevel::Error, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_ERROR_TAG(tag, fmt, ...) ENGINE_LOG_TAG(LogLevel::Error, tag, fmt, ##__VA_ARGS__)
#else
#define ENGINE_LOG_ERROR(fmt, ...)                                                                                     \
    do {                                                                                                               \
    } while (0)
#define ENGINE_LOG_ERROR_TAG(tag, fmt, ...)                                                                            \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(ENGINE_LOG_LEVEL_FATAL)
#define ENGINE_LOG_FATAL(fmt, ...) ENGINE_LOG(LogLevel::Fatal, fmt, ##__VA_ARGS__)
#define ENGINE_LOG_FATAL_TAG(tag, fmt, ...) ENGINE_LOG_TAG(LogLevel::Fatal, tag, fmt, ##__VA_ARGS__)
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
#define APP_LOG_PLAIN(level, msg) AppLogger::GetInstance().log(level, msg)
#define APP_LOG_TAG_PLAIN(level, tag, msg) AppLogger::GetInstance().log(level, msg, tag)
#define APP_LOG(level, fmt, ...)                                                                                       \
    AppLogger::GetInstance().log(level, fmt, "", std::source_location::current(), ##__VA_ARGS__)
#define APP_LOG_TAG(level, tag, fmt, ...)                                                                              \
    AppLogger::GetInstance().log(level, fmt, tag, std::source_location::current(), ##__VA_ARGS__)

#if defined(APP_LOG_LEVEL_DEBUG)
#define APP_LOG_DEBUG(fmt, ...) APP_LOG(LogLevel::Debug, fmt, ##__VA_ARGS__)
#define APP_LOG_DEBUG_TAG(tag, fmt, ...) APP_LOG_TAG(LogLevel::Debug, tag, fmt, ##__VA_ARGS__)
#else
#define APP_LOG_DEBUG(fmt, ...)                                                                                        \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_DEBUG_TAG(tag, fmt, ...)                                                                               \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(APP_LOG_LEVEL_INFO)
#define APP_LOG_INFO(fmt, ...) APP_LOG(LogLevel::Info, fmt, ##__VA_ARGS__)
#define APP_LOG_INFO_TAG(tag, fmt, ...) APP_LOG_TAG(LogLevel::Info, tag, fmt, ##__VA_ARGS__)
#else
#define APP_LOG_INFO(fmt, ...)                                                                                         \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_INFO_TAG(tag, fmt, ...)                                                                                \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(APP_LOG_LEVEL_WARNING)
#define APP_LOG_WARNING(fmt, ...) APP_LOG(LogLevel::Warning, fmt, ##__VA_ARGS__)
#define APP_LOG_WARNING_TAG(tag, fmt, ...) APP_LOG_TAG(LogLevel::Warning, tag, fmt, ##__VA_ARGS__)
#else
#define APP_LOG_WARNING(fmt, ...)                                                                                      \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_WARNING_TAG(tag, fmt, ...)                                                                             \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(APP_LOG_LEVEL_ERROR)
#define APP_LOG_ERROR(fmt, ...) APP_LOG(LogLevel::Error, fmt, ##__VA_ARGS__)
#define APP_LOG_ERROR_TAG(tag, fmt, ...) APP_LOG_TAG(LogLevel::Error, tag, fmt, ##__VA_ARGS__)
#else
#define APP_LOG_ERROR(fmt, ...)                                                                                        \
    do {                                                                                                               \
    } while (0)
#define APP_LOG_ERROR_TAG(tag, fmt, ...)                                                                               \
    do {                                                                                                               \
    } while (0)
#endif
#if defined(APP_LOG_LEVEL_FATAL)
#define APP_LOG_FATAL(fmt, ...) APP_LOG(LogLevel::Fatal, fmt, ##__VA_ARGS__)
#define APP_LOG_FATAL_TAG(tag, fmt, ...) APP_LOG_TAG(LogLevel::Fatal, tag, fmt, ##__VA_ARGS__)
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