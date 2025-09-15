#pragma once

#include <memory>
using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;

#include <cmath>
using std::sqrt;
using std::floor;

// compiler detection macros
#if defined(_MSC_VER)
    #define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define FORCE_INLINE inline __attribute__((always_inline))
#else
    #define FORCE_INLINE inline
#endif