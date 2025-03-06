#include <iostream>
#include <string>
#include <vector>

#if defined(__cpp_lib_stacktrace)
#include <stacktrace> // C++23 stacktrace support (Unsupported at least as of gcc 14.2.1)
#endif

#ifdef STACKTRACE_WINDOWS
#include <dbghelp.h>
#include <windows.h>
#pragma comment(lib, "dbghelp.lib") // Ensure linking with dbghelp.lib
#elif defined(STACKTRACE_LINUX) || defined(STACKTRACE_MACOS)
#include <execinfo.h> // glibc backtrace
#ifdef USE_LIBUNWIND
#include <libunwind.h> // Better stack unwinding support
#endif
#endif

/**
 * @brief Namespace for stack tracing functionality.
 */
namespace stacktrace {

/**
 * @brief Detects the current operating system at runtime.
 * @return A string representing the detected platform.
 */
inline std::string detect_platform()
{
#ifdef STACKTRACE_WINDOWS
    return "Windows";
#elif defined(STACKTRACE_LINUX)
    return "Linux";
#elif defined(STACKTRACE_MACOS)
    return "macOS";
#else
    return "Unknown";
#endif
}

/**
 * @brief Captures and prints the current stack trace.
 */
inline void print_stacktrace()
{
#if defined(__cpp_lib_stacktrace)
    // Use C++23 std::stacktrace if available
    std::cerr << "Stacktrace:\n" << std::stacktrace::current() << '\n';

#elif defined(STACKTRACE_WINDOWS)
    // Windows stack trace using CaptureStackBackTrace
    constexpr int maxFrames = 64;
    void *stack[maxFrames];
    USHORT frames = CaptureStackBackTrace(0, maxFrames, stack, nullptr);

    HANDLE process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);

    std::vector<SYMBOL_INFO *> symbols(frames);
    for (USHORT i = 0; i < frames; ++i) {
        symbols[i] = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
        symbols[i]->MaxNameLen = 255;
        symbols[i]->SizeOfStruct = sizeof(SYMBOL_INFO);

        if (SymFromAddr(process, (DWORD64)(stack[i]), 0, symbols[i])) {
            std::cerr << i << ": " << symbols[i]->Name << " - 0x" << std::hex << symbols[i]->Address << '\n';
        }
        else {
            std::cerr << i << ": [unknown function]\n";
        }
        free(symbols[i]);
    }
    SymCleanup(process);

#elif defined(STACKTRACE_LINUX) || defined(STACKTRACE_MACOS)
#ifdef USE_LIBUNWIND
    // Use libunwind for better stack tracing
    unw_cursor_t cursor;
    unw_context_t context;
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    std::cerr << "Stacktrace:\n";
    while (unw_step(&cursor) > 0) {
        unw_word_t offset, pc;
        char sym[256];

        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
            std::cerr << "  " << sym << " + " << offset << '\n';
        }
        else {
            std::cerr << "  [unknown function]\n";
        }
    }

#else
    // Use execinfo (glibc backtrace)
    constexpr int maxFrames = 64;
    void *callstack[maxFrames];

    int frames = backtrace(callstack, maxFrames);
    char **symbols = backtrace_symbols(callstack, frames);

    if (symbols) {
        std::cerr << "Stacktrace:\n";
        for (int i = 0; i < frames; ++i) {
            std::cerr << symbols[i] << '\n';
        }
        free(symbols);
    }
    else {
        std::cerr << "Failed to generate stacktrace\n";
    }
#endif

#else
    std::cerr << "Stacktrace not supported on this platform.\n";
#endif
}

} // namespace stacktrace