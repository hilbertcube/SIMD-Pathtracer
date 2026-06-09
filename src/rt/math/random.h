#pragma once

#include <chrono>
#include <cstdint>
#include <type_traits>

#include "rt/util/def.h"

namespace rt
{

/*
 * Every thread's generator starts from SCENE_SEED, so scenes built on the main thread (some
 * use random layouts, box heights, Perlin noise) come out the same on every run. The camera
 * reseeds at the start of each image row from a per-run seed and the row number (see
 * mix_seed): noise differs between runs, but not with which thread renders which row.
 */
constexpr uint32_t SCENE_SEED = 88675123u;

// XorShift32: small and fast, good enough for Monte Carlo sampling
struct XorShift32
{
    uint32_t state;

    // A zero state would stay zero forever, so it is replaced with 1
    constexpr explicit XorShift32(uint32_t seed) : state(seed != 0 ? seed : 1u) {}

    FORCE_INLINE uint32_t next_u32()
    {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }

    // Uniform in [0, 1), from the top 24 bits (a float's full mantissa)
    FORCE_INLINE float next_float() { return (next_u32() >> 8) * (1.0f / 16777216.0f); }
};

// Constant-initialized, so accessing it needs no per-thread "set up yet?" check
inline thread_local XorShift32 tls_rng{SCENE_SEED};

inline void seed_rng(uint32_t seed)
{
    tls_rng = XorShift32(seed);
}

inline uint32_t clock_seed()
{
    auto ticks = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return static_cast<uint32_t>(ticks) ^ static_cast<uint32_t>(ticks >> 32);
}

// Derives an independent seed for stream `index` (an image row) from `seed`; the
// MurmurHash3 finalizer keeps neighboring indices from giving similar states.
inline uint32_t mix_seed(uint32_t seed, uint32_t index)
{
    uint32_t x = seed ^ (index * 0x9E3779B9u);
    x ^= x >> 16;
    x *= 0x85EBCA6Bu;
    x ^= x >> 13;
    x *= 0xC2B2AE35u;
    x ^= x >> 16;
    return x;
}

// Random integer in [min, max]
template <typename T>
FORCE_INLINE T random_int(T min, T max)
{
    static_assert(std::is_integral<T>::value, "T must be integral");
    uint32_t bits = tls_rng.next_u32();
    return static_cast<T>(min + (bits % (max - min + 1)));
}

// Random real in [min, max)
template <typename T>
FORCE_INLINE T random_real(T min = T(0), T max = T(1))
{
    static_assert(std::is_floating_point<T>::value, "T must be floating point");
    return min + (max - min) * static_cast<T>(tls_rng.next_float());
}

FORCE_INLINE float random_float()
{
    return random_real<float>();
}

FORCE_INLINE float random_float(float min, float max)
{
    return random_real<float>(min, max);
}

} // namespace rt
