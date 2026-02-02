#pragma once

#include <cmath>
#include <limits>
#include <ostream>

#include "vector.h"
#include "constants.h"
#include "random.h"

namespace rt
{

// Normalized v; a zero vector maps to (1, 0, ...) instead of NaN
template <typename T, std::size_t N>
Vector<T, N> unit_vector(const Vector<T, N>& v)
{
    T len = v.length();
    if (len == T(0))
    {
        Vector<T, N> result;
        result[0] = T(1);
        return result;
    }
    return v / len;
}

// Uniformly random direction on the unit sphere (rejection sampling)
template <typename T, std::size_t N>
Vector<T, N> random_unit_vector()
{
    while (true)
    {
        auto p  = Vector<T, N>::random(-1, 1);
        T lensq = p.length_squared();
        if (std::numeric_limits<T>::epsilon() < lensq && lensq <= T(1))
            return p / std::sqrt(lensq);
    }
}

// Uniformly random point inside the unit disk
template <typename T>
Vector<T, 2> random_in_unit_disk()
{
    while (true)
    {
        auto p = Vector<T, 2>::random(-1, 1);
        if (p.length_squared() < T(1))
            return p;
    }
}

// Random direction around +z with density cos(theta) / pi
template <typename T>
Vector<T, 3> random_cosine_direction()
{
    T r1 = random_real<T>();
    T r2 = random_real<T>();

    T phi = 2 * PI * r1;
    T x   = std::cos(phi) * std::sqrt(r2);
    T y   = std::sin(phi) * std::sqrt(r2);
    T z   = std::sqrt(1 - r2);

    return Vector<T, 3>(x, y, z);
}

// Mirror v about the surface normal n
template <typename T, std::size_t N>
Vector<T, N> reflect(const Vector<T, N>& v, const Vector<T, N>& n)
{
    return v - 2 * dot(v, n) * n;
}

// Refract unit direction uv through a surface with normal n; eta_ratio = eta_in / eta_out
template <typename T>
Vector<T, 3> refract(const Vector<T, 3>& uv, const Vector<T, 3>& n, T eta_ratio)
{
    T cos_theta                 = std::fmin(dot(-uv, n), T(1));
    Vector<T, 3> r_out_perp     = eta_ratio * (uv + cos_theta * n);
    Vector<T, 3> r_out_parallel = -std::sqrt(std::fabs(T(1) - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}

template <typename T, std::size_t N>
std::ostream& operator<<(std::ostream& out, const Vector<T, N>& v)
{
    out << '(';
    for (std::size_t i = 0; i < N; ++i)
        out << (i ? ", " : "") << v[i];
    return out << ')';
}

} // namespace rt
