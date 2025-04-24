#pragma once

#include <cstddef>
#include <stdexcept>

#include "memory_tracker.hpp"

namespace Gouda {
namespace Memory {

/**
 * @brief LinearAllocator is a fast memory allocator that works in a "linear" fashion.
 *
 * It allocates memory in a single contiguous block and simply advances an offset
 * pointer to allocate further memory. This is very fast but is only suitable for
 * temporary memory that can be reset each frame.
 */
class LinearAllocator {
public:
    LinearAllocator(size_t size) : p_memory(new char[size]), capacity(size), offset(0) {}
    ~LinearAllocator() { delete[] p_memory; }

    /**
     * @brief Allocates memory of the requested size, respecting alignment.
     *
     * Memory is allocated contiguously, and the offset is updated.
     */
    void *Allocate(size_t size, size_t alignment = alignof(std::max_align_t))
    {
        size_t padding{(alignment - (offset % alignment)) % alignment};
        if (offset + padding + size > capacity) {
            throw std::bad_alloc();
        }

        void *ptr = p_memory + offset + padding;
        offset += padding + size;
        return ptr;
    }

    void Reset() { offset = 0; } ///< Resets the memory by simply resetting the offset

private:
    char *p_memory;  ///< Pointer to the allocated memory block
    size_t capacity; ///< Total capacity of the allocated block
    size_t offset;   ///< Current offset for the next allocation
};

void *linearAlloc(LinearAllocator &allocator, size_t size)
{
    void *ptr = allocator.Allocate(size);
    MemoryTracker::Get().RegisterAllocation("LinearAllocator", size);
    return ptr;
}

void linearDealloc(LinearAllocator &allocator, void *ptr, size_t size)
{
    // LinearAllocator doesn't actually deallocate, it just resets memory
    MemoryTracker::Get().RegisterDeallocation("LinearAllocator", size);
}

} // namespace Memory end
} // namespace Gouda end
