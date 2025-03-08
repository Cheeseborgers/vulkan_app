#pragma once

#include <memory>
#include <string>
#include <vector>

#include "linear_allocator.hpp"
#include "memory_tracker.hpp"

namespace Gouda {
namespace Memory {

/**
 * @brief Wrapper for LinearAllocator to be used with STL containers.
 *
 * This wrapper allows STL containers like std::vector and std::string
 * to use the LinearAllocator.
 */
template <typename T>
struct LinearAllocatorWrapper {
    using value_type = T;
    LinearAllocator *linearAllocator;

    LinearAllocatorWrapper(LinearAllocator *allocator) : linearAllocator(allocator) {}

    template <typename U>
    LinearAllocatorWrapper(const LinearAllocatorWrapper<U> &other) noexcept : linearAllocator(other.linearAllocator)
    {
    }

    /**
     * @brief Allocates memory using LinearAllocator and registers the allocation.
     */
    T *Allocate(size_t n)
    {
        void *ptr = linearAllocator->Allocate(n * sizeof(T), alignof(T));
        MemoryTracker::Get().RegisterAllocation("LinearAllocator", n * sizeof(T));
        return static_cast<T *>(ptr);
    }

    /**
     * @brief Deallocates memory using LinearAllocator (no-op).
     */
    void Deallocate(T *ptr, size_t n) noexcept
    {
        // No-op, as LinearAllocator will reset memory as a whole
    }
};

// Define Linear-backed std::vector and std::string
template <typename T>
using LinearVector = std::vector<T, LinearAllocatorWrapper<T>>;

using LinearString = std::basic_string<char, std::char_traits<char>, LinearAllocatorWrapper<char>>;

/*
int main()
{
    // Create a LinearAllocator with a block size of 1024 bytes
    Gouda::Memory::LinearAllocator linearAllocator(1024);

    // Create a LinearVector using the LinearAllocatorWrapper
    Gouda::Memory::LinearVector<int> linearVec(&linearAllocator);

    // Allocate space for 10 integers
    linearVec.resize(10);

    // Create a LinearString using the LinearAllocatorWrapper
    Gouda::Memory::LinearString linearStr(&linearAllocator);

    // Assign a string value
    linearStr = "Linear-allocated string";

    // Print memory statistics
    Gouda::Memory::MemoryTracker::getInstance().printStats();

    return 0;
}
*/

} // namespace Memory end
} // namespace Gouda end