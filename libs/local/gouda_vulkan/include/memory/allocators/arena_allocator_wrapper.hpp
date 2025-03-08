#pragma once

#include <memory>
#include <string>
#include <vector>

#include "arena_allocator.hpp"
#include "memory_tracker.hpp"

namespace Gouda {
namespace Memory {

/**
 * @brief Wrapper for ArenaAllocator to be used with STL containers.
 *
 * This wrapper allows STL containers like std::vector and std::string
 * to use the ArenaAllocator.
 *
 */
template <typename T>
struct ArenaAllocatorWrapper {
    using value_type = T;
    ArenaAllocator *arenaAllocator;

    ArenaAllocatorWrapper(ArenaAllocator *allocator) : arenaAllocator(allocator) {}

    template <typename U>
    ArenaAllocatorWrapper(const ArenaAllocatorWrapper<U> &other) noexcept : arenaAllocator(other.arenaAllocator)
    {
    }

    /**
     * @brief Allocates memory using ArenaAllocator and registers the allocation.
     */
    T *Allocate(size_t n)
    {
        void *ptr = arenaAllocator->Allocate(n * sizeof(T), alignof(T));
        MemoryTracker::Get().RegisterAllocation("ArenaAllocator", n * sizeof(T));
        return static_cast<T *>(ptr);
    }

    /**
     * @brief Deallocates memory using ArenaAllocator (no-op).
     *
     * Since ArenaAllocator manages memory in blocks and resets it as a whole,
     * no action is taken when deallocating individual elements.
     */
    void Deallocate(T *ptr, size_t n) noexcept
    {
        // No-op: ArenaAllocator resets all memory at once
    }
};

// Define Arena-backed std::vector and std::string
template <typename T>

using ArenaString = std::basic_string<char, std::char_traits<char>, ArenaAllocatorWrapper<char>>;

/*

int main()
{
    // Create an ArenaAllocator with a block size of 4096 bytes
    Gouda::Memory::ArenaAllocator arena(4096);

    // Create an ArenaVector using the ArenaAllocatorWrapper
    Gouda::Memory::ArenaVector<int> arenaVec(&arena);

    // Allocate space for 10 integers
    arenaVec.resize(10);

    // Create an ArenaString using the ArenaAllocatorWrapper
    Gouda::Memory::ArenaString arenaStr(&arena);

    // Assign a string value
    arenaStr = "Arena-allocated string";

    // Print memory statistics
    Gouda::Memory::MemoryTracker::getInstance().printStats();

    return 0;
}
*/

} // namespace Memory
} // namespace Gouda