/**
 * @file debug/stacktrace.cpp
 * @author GoudaCheeseburgers
 * @date 2025-04-25
 * @brief Engine module implementation
 */
#include "debug/stacktrace.hpp"

#include <iostream>

#include "utils/defer.hpp"

#if defined(_WIN32)
#define STACKTRACE_WINDOWS
#include <dbghelp.h>
#include <windows.h>

#elif defined(__APPLE__)
#define STACKTRACE_MACOS
#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>

#elif defined(__linux__)
#define STACKTRACE_LINUX
#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>
#if defined(USE_LIBUNWIND)
#include <libunwind.h>
#endif
#endif

namespace gouda::internal {
void print_stacktrace()
{
#if defined(STACKTRACE_WINDOWS)
    void *stack[64];
    unsigned short frames;
    SYMBOL_INFO *symbol;
    HANDLE process = GetCurrentProcess();

    SymInitialize(process, NULL, TRUE);
    frames = CaptureStackBackTrace(0, 64, stack, NULL);
    symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256, 1);
    DEFER { free(symbols); });

    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    std::cerr << "Stacktrace:\n";
    for (unsigned int i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        std::cerr << i << ": " << symbol->Name << " - 0x" << std::hex << symbol->Address << std::dec << "\n";
    }

#elif defined(STACKTRACE_LINUX) || defined(STACKTRACE_MACOS)

#if defined(USE_LIBUNWIND)
    unw_cursor_t cursor;
    unw_context_t context;
    unw_word_t offset, pc;
    char sym[256];

    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    std::cerr << "Stacktrace:\n";
    while (unw_step(&cursor) > 0) {
        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        if (pc == 0) {
            break;
        }

        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
            int status{0};
            char *demangled{abi::__cxa_demangle(sym, nullptr, nullptr, &status)};
            std::cerr << (demangled ? demangled : sym) << " + 0x" << std::hex << offset << std::dec << "\n";
            free(demangled);
        }
        else {
            std::cerr << "-- error: unable to obtain symbol name\n";
        }
    }

#else
    void *array[64];
    size_t size{backtrace(array, 64)};
    char **symbols{backtrace_symbols(array, size)};
    DEFER { free(symbols); });

    std::cerr << "Stacktrace:\n";
    for (size_t i = 0; i < size; ++i) {
        std::cerr << symbols[i] << "\n";
    }

#endif

#else
    std::cerr << "Stacktrace not supported on this platform.\n";
#endif
}

} // namespace gouda::internal