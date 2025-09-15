#pragma once

#include "../def.hpp"
#include <cstdint>
#include <type_traits>
#include <thread>
#include <chrono>
#include <random>
#include <functional>

// RNG BACKEND SELECTION
// Define one of these before including random.hpp
//   USE_XORSHIFT32   -> fast RNG, good for rendering
//   USE_MT19937      -> standard RNG, slower but higher quality RNG
// Default = USE_XORSHIFT32

#if !defined(USE_XORSHIFT32) && !defined(USE_MT19937)
#define USE_XORSHIFT32
#endif

// XorShift32 (fast PRNG)
#ifdef USE_XORSHIFT32
struct XorShift32 {
    uint32_t state;

    FORCE_INLINE explicit XorShift32(uint32_t seed = 88675123u) : state(seed) {}

    FORCE_INLINE uint32_t next_u32() {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }

    FORCE_INLINE float next_float() {
        // Convert to [0,1)
        return (next_u32() >> 8) * (1.0f / 16777216.0f);
    }
};

inline thread_local XorShift32 tls_rng{
    static_cast<uint32_t>(
        std::hash<std::thread::id>{}(std::this_thread::get_id()) ^
        static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count())
    )
};
#endif // USE_XORSHIFT32


// std::mt19937 (standard PRNG)
#ifdef USE_MT19937
inline thread_local std::mt19937 tls_rng{
    static_cast<uint32_t>(
        std::hash<std::thread::id>{}(std::this_thread::get_id()) ^
        static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count())
    )
};
#endif // USE_MT19937


// API FUNCTIONS
// Random integer in [min, max]
template<typename T>
FORCE_INLINE T random_int(T min, T max) {
    static_assert(std::is_integral<T>::value, "T must be integral");
#ifdef USE_XORSHIFT32
    uint32_t r = tls_rng.next_u32();
    return static_cast<T>(min + (r % (max - min + 1)));
#else
    std::uniform_int_distribution<T> dist(min, max);
    return dist(tls_rng);
#endif
}

// Random real in [min, max)
template<typename T>
FORCE_INLINE T random_real(T min = T(0), T max = T(1)) {
    static_assert(std::is_floating_point<T>::value, "T must be floating point");
#ifdef USE_XORSHIFT32
    return min + (max - min) * static_cast<T>(tls_rng.next_float());
#else
    std::uniform_real_distribution<T> dist(min, max);
    return dist(tls_rng);
#endif
}

// Convenience wrappers
FORCE_INLINE float  random_float()                    { return random_real<float>(); }
FORCE_INLINE float  random_float(float a, float b)    { return random_real<float>(a, b); }
FORCE_INLINE double random_double()                   { return random_real<double>(); }
FORCE_INLINE double random_double(double a, double b) { return random_real<double>(a, b); }
