#pragma once

#include "vecN.hpp"

namespace rt {
// === BASIC UTILITIES ===
/// @brief Unit vector (normalize)
template <typename T, size_t N> [[nodiscard]]
inline VecN<T, N> unit_vector(const VecN<T, N> &v)
{
    auto len = v.length();
    if (len == T(0)) {
        VecN<T, N> result;
        result[0] = T(1);  // Default to first basis vector
        return result;
    }
    return v / len;
}

// Random unit vector (only for floating-point types)
template <typename T, size_t N> [[nodiscard]] 
inline VecN<T, N> random_unit_vector()
{
    static_assert(std::is_floating_point<T>::value, "random_unit_vector requires floating-point type");
    constexpr T epsilon = std::numeric_limits<T>::epsilon();

    while (true) {
        auto p = VecN<T, N>::random(-1, 1);
        auto lensq = p.length_squared();
        
        if (epsilon < lensq && lensq <= T(1)) {
            T inv_len = std::sqrt(T(1) / lensq);
            return p * inv_len;
        }
    }
}

// Random on hemisphere (only for 3D vectors)
template <typename T> [[nodiscard]] 
inline VecN<T, 3> random_on_hemisphere(const VecN<T, 3>& normal)
{
    VecN<T, 3> on_unit_sphere = random_unit_vector<T, 3>();
    if(dot(on_unit_sphere, normal) > T(0))
        return on_unit_sphere;
    else
        return -on_unit_sphere;
}

// Reflection
/**
 * @param v Incoming ray
 * @param n Normal vector
 */
template <typename T, typename U, size_t N> [[nodiscard]] 
inline VecN<T, N> reflect(const VecN<T, N>& v, const VecN<U, N>& n) 
{
    using R = typename std::common_type<T, U, float>::type;
    VecN<R, N> vr = static_cast<VecN<R, N>>(v);
    VecN<R, N> nr = static_cast<VecN<R, N>>(n);
    VecN<R, N> tmp = vr - static_cast<R>(2) * dot(vr, nr) * nr;
    return VecN<T, N>(tmp); // converting ctor is explicit
}

// Random in unit disk (only for 2D vectors)
template <typename T>
inline VecN<T, 2> random_in_unit_disk() 
{
    while(true) {
        auto p = VecN<T, 2>(random_real<T>(-1, 1), random_real<T>(-1, 1));
        if(p.length_squared() < 1)
            return p;
    }
}

// === ADDITIONAL FUNCTIONS ===
// Angle Between Vectors
template <typename T, typename U, size_t N> [[nodiscard]]
auto angle_between(const VecN<T, N> &u, const VecN<U, N> &v)
{
    using R = typename std::common_type<T, U, float>::type;
    R dot_product = dot(u, v);
    R len_product = static_cast<R>(u.length()) * static_cast<R>(v.length());
    if (len_product == R(0)) return R(0);
    return std::acos(std::clamp(dot_product / len_product, R(-1), R(1)));
}

/**
 * @brief Projection (of a onto b)
 * @param a First vector
 * @param b Second vector
 * @return dot(a, b) * b / |b|
 */
template <typename T, typename U, size_t N> [[nodiscard]]
auto project(const VecN<T, N> &u, const VecN<U, N> &v)
{
    static_assert(std::is_floating_point<U>::value, "Projection vector must be a floating-point type");
    static_assert(std::is_floating_point<T>::value, "Input vector must be a floating-point type");

    return (dot(u, v) / v.length_squared()) * v;
}

// Refraction (3D only)
template <typename T, typename U>
[[nodiscard]]
auto refract(const VecN<T, 3>& uv, const VecN<T, 3>& n, U eta_ratio)
{
    using R = typename std::common_type<T, U, float>::type;

    R cos_theta = std::fmin(dot(-uv, n), R(1));
    VecN<R, 3> r_out_perp = eta_ratio * (uv + cos_theta * n);
    VecN<R, 3> r_out_parallel = -std::sqrt(std::abs(R(1) - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}

template <typename T>
inline VecN<T, 3> random_cosine_direction() 
{
    auto r1 = random_float();
    auto r2 = random_float();

    auto phi = 2 * PI * r1;
    auto x = std::cos(phi) * std::sqrt(r2);
    auto y = std::sin(phi) * std::sqrt(r2);
    auto z = std::sqrt(1-r2);

    return VecN<T, 3>(x, y, z);
}

// === OUTPUT ===
template<typename T, size_t N>
std::ostream &operator<<(std::ostream &out, const VecN<T, N> &v)
{
    out << "(";
    for (size_t i = 0; i < N; ++i) {
        out << v[i];
        if (i < N - 1) out << ", ";
    }
    out << ")";
    return out;
}

}