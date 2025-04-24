/**
 * @file math/random.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-13
 * @brief A Threadsafe and non pcg random implementation
 */
#include "math/random.hpp"

#include <array>
#include <numeric> // For std::accumulate
#include <random>

namespace gouda {
namespace math {

u32 GenerateSeed()
{
    std::random_device rd;
    std::array<u32, 4> seeds{rd(), rd(), rd(), rd()};
    return std::accumulate(seeds.begin(), seeds.end(), 0u, std::bit_xor<>());
}

BaseRNG::BaseRNG(u32 seed) : rng(seed) {}

u32 BaseRNG::GetUint() { return rng(); }

int BaseRNG::GetInt(int min, int max)
{
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

f32 BaseRNG::GetFloat(f32 min, f32 max)
{
    std::uniform_real_distribution<f32> dist(min, max);
    return dist(rng);
}

ThreadSafeRNG::ThreadSafeRNG(u32 seed) : BaseRNG(seed) {}

u32 ThreadSafeRNG::operator()()
{
    return WithLock([this] { return BaseRNG::operator()(); });
}

u32 ThreadSafeRNG::GetUint()
{
    return WithLock([this] { return BaseRNG::GetUint(); });
}

int ThreadSafeRNG::GetInt(int min, int max)
{
    return WithLock([this, min, max] { return BaseRNG::GetInt(min, max); });
}

f32 ThreadSafeRNG::GetFloat(f32 min, f32 max)
{
    return WithLock([this, min, max] { return BaseRNG::GetFloat(min, max); });
}

BaseRNG &GetGlobalRNG()
{
    static BaseRNG rng(GenerateSeed());
    return rng;
}

ThreadSafeRNG &GetGlobalThreadSafeRNG()
{
    static ThreadSafeRNG rng(GenerateSeed());
    return rng;
}

} // namespace math
} // namespace gouda