#pragma once

#include <iostream>
#include <mutex>
#include <unordered_map>

#include "logger.hpp"

namespace Gouda {

class MemoryTracker {
public:
    static MemoryTracker &Get()
    {
        static MemoryTracker instance;
        return instance;
    }

    void RegisterAllocation(const std::string &name, size_t bytes)
    {
        std::lock_guard<std::mutex> lock(mutex);
        stats[name].allocations++;
        stats[name].bytesAllocated += bytes;
    }

    void RegisterDeallocation(const std::string &name, size_t bytes)
    {
        std::lock_guard<std::mutex> lock(mutex);
        stats[name].deallocations++;
        stats[name].bytesFreed += bytes;
    }

    void PrintStats()
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "==== Memory Stats ====\n";
        for (const auto &[name, stat] : stats) {
            std::cout << name << ": " << stat.allocations << " allocations, " << stat.bytesAllocated
                      << " bytes allocated, " << stat.deallocations << " deallocations, " << stat.bytesFreed
                      << " bytes freed\n";
        }
    }

private:
    struct Stats {
        size_t allocations = 0;
        size_t deallocations = 0;
        size_t bytesAllocated = 0;
        size_t bytesFreed = 0;
    };

    std::unordered_map<std::string, Stats> stats;
    std::mutex mutex;

    MemoryTracker() = default;
};

} // namespace Gouda end
