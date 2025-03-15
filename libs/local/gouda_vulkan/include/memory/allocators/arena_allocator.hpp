#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>
#include <stdexcept>
#include <vector>

#include "memory/memory_tracker.hpp"

namespace gouda {
namespace memory {

class ArenaAllocator {
public:
    struct Block {
        std::vector<char> memory; ///< Memory block storage
        size_t capacity;          ///< Total capacity of the block
        size_t offset;            ///< Current offset for the next allocation

        explicit Block(size_t size) : memory(size), capacity(size), offset(0) {}

        void Reset() { offset = 0; } ///< Resets the allocator for reuse
    };

    explicit ArenaAllocator(size_t blockSize = 1024) : blockSize(blockSize) { AllocateBlock(blockSize); }

    void *Allocate(size_t size, size_t alignment = alignof(std::max_align_t))
    {
        for (auto &blockPtr : blocks) {
            Block &block = *blockPtr;

            size_t padding = (alignment - (block.offset % alignment)) % alignment;
            if (block.offset + padding + size <= block.capacity) {
                void *ptr = block.memory.data() + block.offset + padding;
                block.offset += padding + size;
                return ptr;
            }
        }

        // No existing block has enough space; allocate a new one
        AllocateBlock(std::max(size, blockSize));
        return Allocate(size, alignment);
    }

    void Reset()
    {

        for (auto &blockPtr : blocks) {
            MemoryTracker::Get().RegisterDeallocation("ArenaAllocator", blockPtr->capacity);
            blockPtr->Reset();
        }
    }

private:
    std::vector<std::unique_ptr<Block>> blocks;
    size_t blockSize;

    void AllocateBlock(size_t size)
    {
        blocks.push_back(std::make_unique<Block>(size));
        MemoryTracker::Get().RegisterAllocation("ArenaAllocator", size);
    }
};

inline void *ArenaAlloc(ArenaAllocator &allocator, size_t size) { return allocator.Allocate(size); }

} // namespace memory
} // namespace gouda
