#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "memory_tracker.hpp"

namespace Gouda {
namespace Memory {

/**
 * @brief PoolAllocator is a memory allocator for fixed-size objects.
 *
 * It maintains a pool of pre-allocated objects and reuses them when needed.
 * This allocator is especially useful for allocating small objects in fixed
 * sizes (e.g., font glyphs, fixed-size buffers).
 */
template <typename T, size_t PoolSize = 1024>
class PoolAllocator {
public:
    PoolAllocator()
    {
        pool.resize(PoolSize);
        for (auto &obj : pool) {
            freeList.push_back(&obj);
        }
    }

    /**
     * @brief Allocates a memory block from the pool.
     */
    T *Allocate(size_t n)
    {
        if (freeList.empty() || n > 1) {
            throw std::bad_alloc();
        }

        T *ptr = freeList.back();
        freeList.pop_back();
        return ptr;
    }

    /**
     * @brief Deallocates a memory block back to the pool.
     */
    void Deallocate(T *p, size_t) { freeList.push_back(p); }

private:
    std::vector<T> pool;       ///< Pool of pre-allocated objects
    std::vector<T *> freeList; ///< List of available free memory blocks
};

// Allocator wrappers for std::string & STL Containers

// Pool Allocator Wrapper for Fixed-Size Strings
template <size_t StringSize = 64>
struct FixedSizeString {
    char data[StringSize] = {0};

    FixedSizeString() = default;
    FixedSizeString(const char *str) { std::strncpy(data, str, StringSize - 1); }
    FixedSizeString &operator=(const char *str)
    {
        std::strncpy(data, str, StringSize - 1);
        return *this;
    }

    const char *c_str() const { return data; }
};

void *PoolAlloc(PoolAllocator<int> &allocator, size_t size)
{
    void *ptr = allocator.Allocate(size);
    MemoryTracker::Get().RegisterAllocation("PoolAllocator", size);
    return ptr;
}

void PoolDealloc(PoolAllocator<int> &allocator, void *ptr, size_t size)
{
    allocator.Deallocate(static_cast<int *>(ptr), size);
    MemoryTracker::Get().RegisterDeallocation("PoolAllocator", size);
}

} // namespace Memory end
} // namespace Gouda end