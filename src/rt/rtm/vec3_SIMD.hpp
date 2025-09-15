#pragma once

#include "VecN.hpp"  // Include the base VecN implementation

// SIMD capability detection
#ifdef _MSC_VER
    #include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
    #include <x86intrin.h>
#endif

// Feature detection macros
#if defined(__SSE__) || (defined(_M_X64) || defined(_M_IX86_FP) && _M_IX86_FP >= 1)
    #define VECN_HAS_SSE 1
    #include <xmmintrin.h>
#endif

#if defined(__SSE2__) || (defined(_M_X64) || defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    #define VECN_HAS_SSE2 1
    #include <emmintrin.h>
#endif

#if defined(__SSE3__)
    #define VECN_HAS_SSE3 1
    #include <pmmintrin.h>
#endif

#if defined(__SSE4_1__)
    #define VECN_HAS_SSE4_1 1
    #include <smmintrin.h>
#endif

#if defined(__AVX__)
    #define VECN_HAS_AVX 1
    #include <immintrin.h>
#endif

#if defined(__ARM_NEON)
    #define VECN_HAS_NEON 1
    #include <arm_neon.h>
#endif

namespace rt {

namespace simd_utils {

#ifdef VECN_HAS_SSE
    /// @brief SSE functions

    /// @brief Load data from std::array to __m128 register
    inline __m128 load_vec3(const float* ptr) {
        // Load 3 floats and set w to 0
        return _mm_set_ps(0.0f, ptr[2], ptr[1], ptr[0]);
    }

    inline __m128 load_scalar(float scalar) {
        return _mm_set1_ps(scalar);
    }
    
    // load vec onto ptr
    inline void store_vec3(__m128 vec, float* ptr) {
        // Store only first 3 components
        // _mm_store_ss(ptr, vec);
        // _mm_store_ss(ptr + 1, _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1,1,1,1)));
        // _mm_store_ss(ptr + 2, _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2,2,2,2)));
        _mm_storeu_ps(ptr, vec);
    }

    inline float horizontal_sum_sse(__m128 v) {
#ifdef VECN_HAS_SSE3
        v = _mm_hadd_ps(v, v);
        v = _mm_hadd_ps(v, v);
        return _mm_cvtss_f32(v);
#else
        __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));
        __m128 sums = _mm_add_ps(v, shuf);
        shuf = _mm_movehl_ps(shuf, sums);
        sums = _mm_add_ss(sums, shuf);
        return _mm_cvtss_f32(sums);
#endif
    }
#endif

#ifdef VECN_HAS_NEON
    inline float horizontal_sum_neon(float32x4_t v) {
        float32x2_t r = vadd_f32(vget_high_f32(v), vget_low_f32(v));
        return vget_lane_f32(vpadd_f32(r, r), 0);
    }
#endif

} // namespace simd_utils::

#ifdef VECN_HAS_SSE
// 16 bytes alignment
template<> class alignas(16) VecN<float, 3> {
private:
    std::array<float, 3> e;
    friend class VecN;
public:
#if defined(_MSC_VER)
#pragma region _constructors_
#endif
    // Constructors
    constexpr VecN() : e{0.0f, 0.0f, 0.0f} {}
    constexpr VecN(float value) : e{value, value, value}{}
    constexpr VecN(float x, float y, float z) : e{x, y, z} {}
    constexpr VecN(const std::array<float, 3>& arr) : e(arr) {}
    constexpr VecN(const float arr[3]) : e{arr[0], arr[1], arr[2]}{}
    constexpr VecN(const VecN<float, 3>& other) : e(other.e) {}
#if defined(_MSC_VER)
#pragma endregion // Constructors Definition
#endif
    constexpr float x() const noexcept { return e[0];}
    constexpr float y() const noexcept { return e[1];}
    constexpr float z() const noexcept { return e[2];}
    constexpr float r() const noexcept { return e[0];}
    constexpr float g() const noexcept { return e[1];}
    constexpr float b() const noexcept { return e[2];}

    float& operator[](size_t i) {
        assert(i < 3 && "Index out of bounds");
        return e[i];
    }

    const float& operator[](size_t i) const {
        assert(i < 3 && "Index out of bounds");
        return e[i];
    }

    float* data() noexcept { return e.data(); }
    const float* data() const noexcept { return e.data(); }

    // === UNARY OPERATORS ===
    VecN& operator=(const VecN&) = default;

    // Operations implemented with SIMD
    // ps = single precision floating point
    // pd = double precision floating point
    VecN<float, 3> operator-() const noexcept
    {
        __m128 v = simd_utils::load_vec3(e.data());
        __m128 zero = _mm_setzero_ps();
        __m128 result = _mm_sub_ps(zero, v);

        VecN<float, 3> ret;
        simd_utils::store_vec3(result, ret.data());
        return ret;
    }

    VecN<float, 3>& operator+=(const VecN<float, 3>& other) noexcept
    {
        __m128 a = simd_utils::load_vec3(e.data());
        __m128 b = simd_utils::load_vec3(other.data());
        __m128 result = _mm_add_ps(a, b);
        simd_utils::store_vec3(result, e.data());
        return *this;
    }

    VecN<float, 3>& operator-=(const VecN<float, 3>& other) noexcept {
        __m128 a = simd_utils::load_vec3(e.data());
        __m128 b = simd_utils::load_vec3(other.data());
        __m128 result = _mm_sub_ps(a, b);
        simd_utils::store_vec3(result, e.data());
        return *this;
    }

    /// @brief Hadamard product
    VecN<float, 3>& operator*=(const VecN<float, 3>& other) noexcept
    {
        __m128 a = simd_utils::load_vec3(e.data());
        __m128 b = simd_utils::load_vec3(other.data());
        __m128 result = _mm_mul_ps(a, b);   // element by element mult
        simd_utils::store_vec3(result, e.data());
        return *this;
    }

    VecN<float, 3>& operator*=(float scalar) noexcept
    {
        __m128 v = simd_utils::load_vec3(e.data());
        __m128 s = simd_utils::load_scalar(scalar);
        __m128 result = _mm_mul_ps(v, s);
        simd_utils::store_vec3(result, e.data());
        return *this;
    }

    VecN<float, 3>& operator/=(float scalar) {
        if (nearlyEqual(scalar, 0.0f))
            throw std::invalid_argument("Division by zero");
        
        __m128 v = simd_utils::load_vec3(e.data());
        __m128 s = _mm_set1_ps(1.0f / scalar);
        __m128 result = _mm_mul_ps(v, s);
        simd_utils::store_vec3(result, e.data());
        return *this;
    }

    [[nodiscard]] 
    float length() const noexcept {
        return std::sqrt(length_squared());
    }

    [[nodiscard]] 
    float length_squared() const noexcept {
        __m128 v = simd_utils::load_vec3(e.data());
        __m128 sq = _mm_mul_ps(v, v);
        return simd_utils::horizontal_sum_sse(sq);
    }

    bool near_zero() const {
        constexpr float epsilon = std::numeric_limits<float>::epsilon();
        return (nearlyEqual(e[0], 0.0f)) && 
               (nearlyEqual(e[1], 0.0f)) && 
               (nearlyEqual(e[2], 0.0f));
    }

    static VecN<float, 3> random()
    {
        VecN<float, 3> result;
        for (size_t i = 0; i < 3; ++i) {
            result[i] = random_real<float>();
        }
        return result;
    }

    static VecN<float, 3> random(float a, float b)
    {
        VecN<float, 3> result;
        for (size_t i = 0; i < 3; ++i) {
            result[i] = random_real<float>(a, b);
        }
        return result;
    }

    static constexpr size_t dimension() noexcept { return 3; }
};

// SIMD operators for Vec3<float>
inline VecN<float, 3> operator+(const VecN<float, 3>& a, const VecN<float, 3>& b) noexcept {
    __m128 va = simd_utils::load_vec3(a.data());
    __m128 vb = simd_utils::load_vec3(b.data());
    __m128 result = _mm_add_ps(va, vb);
    
    VecN<float, 3> ret;
    simd_utils::store_vec3(result, ret.data());
    return ret;
}

inline VecN<float, 3> operator-(const VecN<float, 3>& a, const VecN<float, 3>& b) noexcept {
    __m128 va = simd_utils::load_vec3(a.data());
    __m128 vb = simd_utils::load_vec3(b.data());
    __m128 result = _mm_sub_ps(va, vb);
    
    VecN<float, 3> ret;
    simd_utils::store_vec3(result, ret.data());
    return ret;
}

inline VecN<float, 3> operator*(const VecN<float, 3>& a, const VecN<float, 3>& b) noexcept {
    __m128 va = simd_utils::load_vec3(a.data());
    __m128 vb = simd_utils::load_vec3(b.data());
    __m128 result = _mm_mul_ps(va, vb);
    
    VecN<float, 3> ret;
    simd_utils::store_vec3(result, ret.data());
    return ret;
}

inline VecN<float, 3> operator*(float scalar, const VecN<float, 3>& v) noexcept {
    __m128 vv = simd_utils::load_vec3(v.data());
    __m128 s = _mm_set1_ps(scalar);
    __m128 result = _mm_mul_ps(vv, s);
    
    VecN<float, 3> ret;
    simd_utils::store_vec3(result, ret.data());
    return ret;
}

inline VecN<float, 3> operator*(const VecN<float, 3>& v, float scalar) noexcept {
    return scalar * v;
}

inline VecN<float, 3> operator/(const VecN<float, 3>& v, float scalar) {
    if (nearlyEqual(scalar, 0.0f))
        throw std::invalid_argument("Division by zero");
    
    __m128 vv = simd_utils::load_vec3(v.data());
    __m128 s = _mm_set1_ps(1.0f / scalar);
    __m128 result = _mm_mul_ps(vv, s);
    
    VecN<float, 3> ret;
    simd_utils::store_vec3(result, ret.data());
    return ret;
}

// SIMD dot product
[[nodiscard]]
inline float dot(const VecN<float, 3>& a, const VecN<float, 3>& b) noexcept {
    __m128 va = simd_utils::load_vec3(a.data());
    __m128 vb = simd_utils::load_vec3(b.data());
    __m128 mul = _mm_mul_ps(va, vb);
    return simd_utils::horizontal_sum_sse(mul);
}

// SIMD cross product
[[nodiscard]]
inline VecN<float, 3> cross(const VecN<float, 3>& a, const VecN<float, 3>& b) noexcept {
    __m128 va = simd_utils::load_vec3(a.data());
    __m128 vb = simd_utils::load_vec3(b.data());
    
    // a.yzx
    __m128 a_yzx = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 0, 2, 1));
    // b.zxy  
    __m128 b_zxy = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 1, 0, 2));
    // a.zxy
    __m128 a_zxy = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 1, 0, 2));
    // b.yzx
    __m128 b_yzx = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 0, 2, 1));
    
    __m128 result = _mm_sub_ps(_mm_mul_ps(a_yzx, b_zxy), _mm_mul_ps(a_zxy, b_yzx));
    
    VecN<float, 3> ret;
    simd_utils::store_vec3(result, ret.data());
    return ret;
}

#endif // VECN_HAS_SSE

}

