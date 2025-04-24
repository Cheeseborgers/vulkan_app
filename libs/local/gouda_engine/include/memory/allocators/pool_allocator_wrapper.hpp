#pragma once

#include <memory>
#include <string>
#include <vector>

#include "memory_tracker.hpp"
#include "pool_allocator.hpp"

namespace Gouda {
namespace Memory {

/**
 * @brief Wrapper for PoolAllocator to be used with STL containers.
 *
 * This wrapper allows STL containers like std::vector and std::string
 * to use the PoolAllocator.
 */
template <typename T>
struct PoolAllocatorWrapper {
    using value_type = T;
    PoolAllocator<T> *poolAllocator;

    PoolAllocatorWrapper(PoolAllocator<T> *allocator) : poolAllocator(allocator) {}

    template <typename U>
    PoolAllocatorWrapper(const PoolAllocatorWrapper<U> &other) noexcept : poolAllocator(other.poolAllocator)
    {
    }

    /**
     * @brief Allocates memory using the PoolAllocator and registers the allocation.
     */
    T *Allocate(size_t n)
    {
        void *ptr = poolAllocator->Allocate(n);
        MemoryTracker::Get().RegisterAllocation("PoolAllocator", n * sizeof(T));
        return static_cast<T *>(ptr);
    }

    /**
     * @brief Deallocates memory using the PoolAllocator (no-op).
     */
    void Deallocate(T *ptr, size_t n) noexcept
    {
        poolAllocator->Deallocate(ptr, n);
        MemoryTracker::Get().RegisterDeallocation("PoolAllocator", n * sizeof(T));
    }
};

// Define Pool-backed std::vector and std::string
template <typename T>
using PoolVector = std::vector<T, PoolAllocatorWrapper<T>>;

using PoolString = std::basic_string<char, std::char_traits<char>, PoolAllocatorWrapper<char>>;

/*
int main()
{
    // Create a PoolAllocator for integers with default pool size of 1024
    Gouda::Memory::PoolAllocator<int> poolAllocator;

    // Create a PoolVector using the PoolAllocatorWrapper
    Gouda::Memory::PoolVector<int> poolVec(&poolAllocator);

    // Allocate space for 10 integers
    poolVec.resize(10);

    // Create a PoolString using the PoolAllocatorWrapper
    Gouda::Memory::PoolString poolStr(&poolAllocator);

    // Assign a string value
    poolStr = "Pool-allocated string";

    // Print memory statistics
    Gouda::Memory::MemoryTracker::getInstance().printStats();

    return 0;
}
*/

} // namespace Memory end
} // namespace Gouda end