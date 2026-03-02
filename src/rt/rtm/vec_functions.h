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
Vector<T, N> unit_vector(const Vector<T, N>& vector)
{
    T length = vector.length();
    if (length == T(0))
    {
        Vector<T, N> result;
        result[0] = T(1);
        return result;
    }
    return vector / length;
}

// Uniformly random direction on the unit sphere (rejection sampling)
template <typename T, std::size_t N>
Vector<T, N> random_unit_vector()
{
    while (true)
    {
        auto candidate   = Vector<T, N>::random(-1, 1);
        T squared_length = candidate.length_squared();
        if (std::numeric_limits<T>::epsilon() < squared_length && squared_length <= T(1))
        {
            return candidate / std::sqrt(squared_length);
        }
    }
}

// Uniformly random point inside the unit disk
template <typename T>
Vector<T, 2> random_in_unit_disk()
{
    while (true)
    {
        auto candidate = Vector<T, 2>::random(-1, 1);
        if (candidate.length_squared() < T(1))
        {
            return candidate;
        }
    }
}

// Random direction around +z with density cos(theta) / pi
template <typename T>
Vector<T, 3> random_cosine_direction()
{
    T first_sample  = random_real<T>();
    T second_sample = random_real<T>();

    T phi = 2 * PI * first_sample;
    T x   = std::cos(phi) * std::sqrt(second_sample);
    T y   = std::sin(phi) * std::sqrt(second_sample);
    T z   = std::sqrt(1 - second_sample);

    return Vector<T, 3>(x, y, z);
}

// Mirror v about the surface normal n
template <typename T, std::size_t N>
Vector<T, N> reflect(const Vector<T, N>& direction, const Vector<T, N>& normal)
{
    return direction - 2 * dot(direction, normal) * normal;
}

// Refract unit direction uv through a surface with normal n; eta_ratio = eta_in / eta_out
template <typename T>
Vector<T, 3> refract(const Vector<T, 3>& direction, const Vector<T, 3>& normal, T eta_ratio)
{
    T cos_theta                = std::fmin(dot(-direction, normal), T(1));
    Vector<T, 3> perpendicular = eta_ratio * (direction + cos_theta * normal);
    Vector<T, 3> parallel = -std::sqrt(std::fabs(T(1) - perpendicular.length_squared())) * normal;
    return perpendicular + parallel;
}

template <typename T, std::size_t N>
std::ostream& operator<<(std::ostream& stream, const Vector<T, N>& vector)
{
    stream << '(';
    for (std::size_t i = 0; i < N; ++i)
    {
        stream << (i ? ", " : "") << vector[i];
    }
    return stream << ')';
}

} // namespace rt
