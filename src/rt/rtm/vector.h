#pragma once

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <type_traits>

#include "random.h"

namespace rt
{

// Fixed-size math vector, e.g. Vector<float, 3> for points, directions and colors.
template <typename T, std::size_t N>
class Vector
{
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
    static_assert(N > 0, "N must be greater than 0");

    std::array<T, N> components{};

public:
    // Zero vector
    constexpr Vector() = default;

    // Every component set to value: Vector<float, 3>(0.5f)
    constexpr explicit Vector(T value)
    {
        for (T& component : components)
        {
            component = value;
        }
    }

    // One value per component: Vector<float, 3>(1, 2, 3)
    template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == N && (N > 1) &&
                                                            (std::is_arithmetic_v<Args> && ...)>>
    constexpr explicit Vector(Args... args) : components{static_cast<T>(args)...}
    {
    }

    constexpr T x() const { return components[0]; }

    constexpr T y() const
    {
        static_assert(N >= 2);
        return components[1];
    }

    constexpr T z() const
    {
        static_assert(N >= 3);
        return components[2];
    }

    constexpr T r() const { return x(); }

    constexpr T g() const { return y(); }

    constexpr T b() const { return z(); }

    constexpr T& operator[](std::size_t i)
    {
        assert(i < N);
        return components[i];
    }

    constexpr T operator[](std::size_t i) const
    {
        assert(i < N);
        return components[i];
    }

    constexpr Vector operator-() const
    {
        Vector result;
        for (std::size_t i = 0; i < N; ++i)
        {
            result.components[i] = -components[i];
        }
        return result;
    }

    constexpr Vector& operator+=(const Vector& v)
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            components[i] += v.components[i];
        }
        return *this;
    }

    constexpr Vector& operator-=(const Vector& v)
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            components[i] -= v.components[i];
        }
        return *this;
    }

    constexpr Vector& operator*=(const Vector& v)
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            components[i] *= v.components[i];
        }
        return *this;
    } // component-wise

    constexpr Vector& operator*=(T scalar)
    {
        for (T& component : components)
        {
            component *= scalar;
        }
        return *this;
    }

    constexpr Vector& operator/=(T scalar)
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            return *this *= T(1) / scalar; // one division instead of N
        }
        for (T& component : components)
        {
            component /= scalar;
        }
        return *this;
    }

    constexpr T length_squared() const { return dot(*this, *this); }

    T length() const { return std::sqrt(length_squared()); }

    // Components uniformly random in [0, 1) or [min, max)
    static Vector random() { return random(T(0), T(1)); }

    static Vector random(T min, T max)
    {
        Vector result;
        for (T& component : result.components)
        {
            component = random_real<T>(min, max);
        }
        return result;
    }
};

// Vector-vector arithmetic (* is component-wise)
template <typename T, std::size_t N>
constexpr Vector<T, N> operator+(Vector<T, N> left, const Vector<T, N>& right)
{
    return left += right;
}

template <typename T, std::size_t N>
constexpr Vector<T, N> operator-(Vector<T, N> left, const Vector<T, N>& right)
{
    return left -= right;
}

template <typename T, std::size_t N>
constexpr Vector<T, N> operator*(Vector<T, N> left, const Vector<T, N>& right)
{
    return left *= right;
}

// Scalar arithmetic; the scalar is converted to the vector's type, so 0.5 * vec3f stays a vec3f
template <typename T, std::size_t N, typename S,
          typename = std::enable_if_t<std::is_arithmetic_v<S>>>
constexpr Vector<T, N> operator*(Vector<T, N> vector, S scalar)
{
    return vector *= static_cast<T>(scalar);
}

template <typename T, std::size_t N, typename S,
          typename = std::enable_if_t<std::is_arithmetic_v<S>>>
constexpr Vector<T, N> operator*(S scalar, Vector<T, N> vector)
{
    return vector *= static_cast<T>(scalar);
}

template <typename T, std::size_t N, typename S,
          typename = std::enable_if_t<std::is_arithmetic_v<S>>>
constexpr Vector<T, N> operator/(Vector<T, N> vector, S scalar)
{
    return vector /= static_cast<T>(scalar);
}

template <typename T, std::size_t N>
constexpr T dot(const Vector<T, N>& left, const Vector<T, N>& right)
{
    T sum = T(0);
    for (std::size_t i = 0; i < N; ++i)
    {
        sum += left[i] * right[i];
    }
    return sum;
}

template <typename T>
constexpr Vector<T, 3> cross(const Vector<T, 3>& left, const Vector<T, 3>& right)
{
    return Vector<T, 3>(left[1] * right[2] - left[2] * right[1],
                        left[2] * right[0] - left[0] * right[2],
                        left[0] * right[1] - left[1] * right[0]);
}

using vec2f   = Vector<float, 2>;
using vec3f   = Vector<float, 3>;
using point3f = Vector<float, 3>;
using color   = Vector<float, 3>;
} // namespace rt

#include "vec_functions.h"
