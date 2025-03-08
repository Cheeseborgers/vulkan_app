#pragma once

#include <cstddef>
#include <malloc.h>
#include <stdexcept>
#include <vector>

#include "memory_tracker.hpp"

namespace Gouda {
namespace Memory {

/**
 * @brief ArenaAllocator is a memory allocator for long-lived allocations.
 *
 * It allocates large blocks of memory and provides an allocation strategy
 * that reuses chunks of memory, suitable for game assets or persistent data
 * that remain in memory for the lifetime of the application or until explicitly reset.
 */
class ArenaAllocator {
public:
    struct Block {
        char *memory;    ///< Pointer to the allocated memory block
        size_t capacity; ///< Total capacity of the memory block
        size_t offset;   ///< Current offset for the next allocation

        Block(size_t size) : capacity(size), offset(0)
        {
            memory = static_cast<char *>(std::malloc(size));
            if (!memory)
                throw std::bad_alloc();
        }

        ~Block() { std::free(memory); }

        void Reset() { offset = 0; } ///< Resets the allocator to reuse the memory block
    };

    ArenaAllocator(size_t blockSize = 1024) : blockSize(blockSize) { AllocateBlock(); }
    ~ArenaAllocator()
    {
        for (auto *block : blocks)
            delete block;
    }

    /**
     * @brief Allocates memory of the requested size.
     *
     * This allocator provides a strategy to fit the requested memory into
     * an existing block if space allows, or allocate a new larger block if needed.
     */
    void *Allocate(size_t size, size_t alignment = alignof(std::max_align_t))
    {
        for (auto *block : blocks) {
            size_t padding = (alignment - (block->offset % alignment)) % alignment;
            if (block->offset + padding + size <= block->capacity) {
                void *ptr = block->memory + block->offset + padding;
                block->offset += padding + size;
                return ptr;
            }
        }
        AllocateBlock(std::max(size, blockSize)); // Allocate a new block if needed
        return Allocate(size, alignment);         // Retry after adding a new block
    }

    void Reset()
    {
        for (auto *block : blocks)
            block->Reset();
    }

private:
    std::vector<Block *> blocks;
    size_t blockSize;

    void AllocateBlock(size_t size = 0) { blocks.push_back(new Block(size ? size : blockSize)); }
};

void *ArenaAlloc(ArenaAllocator &allocator, size_t size)
{
    void *ptr = allocator.Allocate(size);
    MemoryTracker::Get().RegisterAllocation("ArenaAllocator", size);
    return ptr;
}

void ArenaDealloc(ArenaAllocator &allocator, void *ptr, size_t size)
{
    // Arena deallocations are typically a reset operation, no-op. bellow is pointless :)
    // MemoryTracker::getInstance().registerDeallocation("ArenaAllocator", size);
}

} // namespace Memory end
} // namespace Gouda end
