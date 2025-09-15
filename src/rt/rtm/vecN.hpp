#pragma once

#include <iostream>
#include <cmath>
#include <type_traits>
#include <array>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <initializer_list>
#include <cassert>

#include "random.hpp"
#include "comparison.hpp"

namespace rt {
template <typename T, size_t N>
class VecN {
private:
    // Ensure numerical type and valid dimension
    static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");
    static_assert(N > 0, "Vector dimension must be greater than 0");

    template <bool Cond, typename U = T>
    using enable_if_t = typename std::enable_if<Cond, U>::type;

    /// Core
    std::array<T, N> e;

    // Makes all other VecN<U, N> specializations friends
    template <typename, size_t> friend class VecN;
public:
#if defined(_MSC_VER)
#pragma region _constructors_
#endif
    // Default constructor - zero vector
    constexpr VecN() : e{} {}

    // Fill constructor - initialize all components with same value
    constexpr explicit VecN(T value){ e.fill(value); }

    // Variadic constructor for direct component initialization
    // Used for "parameter packs"
    // VecN v(1, 2, 3, ...)
    template<typename... Args,
         typename = typename std::enable_if<(std::conjunction<std::is_convertible<Args, T>...>::value)>::type>
    constexpr explicit VecN(Args&&... args) : e{static_cast<T>(args)...}
    {
        static_assert(sizeof...(args) == N, "Number of arguments must match vector dimension");
    }

    // Initializer list constructor
    // VecN v = {1, 2, 3, ...}
    constexpr explicit VecN(std::initializer_list<T> init_list)
    {
        if (init_list.size() != N)
            throw std::invalid_argument("Initializer list size must match vector dimension");
        std::copy(init_list.begin(), init_list.end(), e.begin());
    }

    // Constructor to accept classic C-style arrays
    constexpr explicit VecN(const T (&arr)[N])
    {
        std::copy(arr, arr + N, e.begin());
    }

    // Constructor to accept array pointer
    constexpr explicit VecN(const T* arr)
    {
        if (arr == nullptr)
            throw std::invalid_argument("Null pointer passed to VecN constructor");
        std::copy(arr, arr + N, e.begin());
    }

    // Constructor to accept std::array
    constexpr explicit VecN(const std::array<T, N>& arr) : e(arr) {}

    // Copy constructor with type conversion
    // VecN v(u)
    template <typename U>
    constexpr explicit VecN(const VecN<U, N>& other) 
    {
        for (size_t i = 0; i < N; ++i) {
            e[i] = static_cast<T>(other.e[i]);
        }
    }
#if defined(_MSC_VER)
#pragma endregion // Constructors Definition
#endif

    // Component access methods (only available for appropriate dimensions)
    // Access only, no mutation. Use [] for mutation
    template<size_t D = N>
    constexpr enable_if_t<(D >= 1)> x() const noexcept { return e[0]; }

    template<size_t D = N>
    constexpr enable_if_t<(D >= 1)> r() const noexcept { return e[0]; }

    template<size_t D = N>
    constexpr enable_if_t<(D >= 2)> y() const noexcept { return e[1]; }

    template<size_t D = N>
    constexpr enable_if_t<(D >= 2)> g() const noexcept { return e[1]; }

    template<size_t D = N>
    constexpr enable_if_t<(D >= 3)> z() const noexcept { return e[2]; }

    template<size_t D = N>
    constexpr enable_if_t<(D >= 3)> b() const noexcept { return e[2]; }

    template<size_t D = N>
    constexpr enable_if_t<(D >= 4)> w() const noexcept { return e[3]; }

    template<size_t D = N>
    constexpr enable_if_t<(D >= 4)> a() const noexcept { return e[3]; }

    // Efficient array access
    // Allow mutation
    T& operator[](size_t i) { assert(i < N && "Index out of bounds"); return e[i]; }
    const T& operator[](size_t i) const { assert(i < N && "Index out of bounds"); return e[i]; }

    // for SIMD
    T* data() noexcept { return e.data(); }
    const T* data() const noexcept { return e.data(); }

    // === UNARY OPERATORS ===
    // Negate operator
    constexpr VecN<T, N> operator-() const noexcept 
    { 
        VecN<T, N> result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = -e[i];
        }
        return result;
    }

    // Assignment operator (use default = of std::array)
    VecN& operator=(const VecN&) = default;

    // Compound assignment operators
    VecN<T, N>& operator+=(const VecN<T, N> &v) noexcept
    {
        for (size_t i = 0; i < N; ++i) {
            e[i] += v[i];
        }
        return *this;
    }

    VecN<T, N>& operator-=(const VecN<T, N> &v) noexcept
    {
        for (size_t i = 0; i < N; ++i) {
            e[i] -= v[i];
        }
        return *this;
    }

    // Hadamard product
    VecN<T, N>& operator*=(const VecN<T, N> &v) noexcept
    {
        for (size_t i = 0; i < N; ++i) {
            e[i] *= v[i];
        }
        return *this;
    }

    // Scalar multiplication
    VecN<T, N>& operator*=(T t) noexcept 
    {
        for (size_t i = 0; i < N; ++i) {
            e[i] *= t;
        }
        return *this;
    }

    // Scalar division
    VecN<T, N>& operator/=(T t)
    {
        if (t == T(0))
            throw std::invalid_argument("Division by zero");
    
        if constexpr (std::is_floating_point<T>::value) {
            return *this *= T(1) / t;
        } else {
            for (size_t i = 0; i < N; ++i) {
                e[i] /= t;
            }
            return *this;
        }
    }

    // use double if T is int, float if T is float
    using magnitude_t = typename std::conditional<std::is_floating_point<T>::value, T, double>::type;
    [[nodiscard]] magnitude_t length() const noexcept 
    {
        return std::sqrt(static_cast<magnitude_t>(length_squared()));
    }

    [[nodiscard]] constexpr T length_squared() const noexcept
    {
        T sum = T(0);
        for (size_t i = 0; i < N; ++i) {
            sum += e[i] * e[i];
        }
        return sum;
    }

    bool near_zero() const 
    {
        static_assert(std::is_floating_point<T>::value, "Type T when using near_zero() must be of floating point type");
        constexpr T epsilon = std::numeric_limits<T>::epsilon();
        for (size_t i = 0; i < N; ++i) {
            if (std::fabs(e[i]) >= epsilon) return false;
        }
        return true;
    }

    /**
     * @brief static function to fill the vector with 
     * random values from [0 to 1)
     */
    static VecN<T, N> random()
    {
        VecN<T, N> result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = random_real<T>();
        }
        return result;
    }

    /**
     * @brief static function to fill the vector with random values
     * @param min, max Ranges
     */
    static VecN<T, N> random(T min, T max)
    {
        VecN<T, N> result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = random_real<T>(min, max);
        }
        return result;
    }

    // Get vector dimension
    static constexpr size_t dimension() noexcept { return N; }


};

// === BINARY VECTOR OPERATORS ===
// Addition
template <typename T, typename U, size_t N>
constexpr inline auto operator+(const VecN<T, N> &v, const VecN<U, N> &u) noexcept
{
    using R = decltype(T{} + U{});
    VecN<R, N> result;
    for (size_t i = 0; i < N; ++i) {
        result[i] = v[i] + u[i];
    }
    return result;
}

// Subtraction
template <typename T, typename U, size_t N>
constexpr inline auto operator-(const VecN<T, N> &v, const VecN<U, N> &u) noexcept
{
    using R = decltype(T{} - U{});
    VecN<R, N> result;
    for (size_t i = 0; i < N; ++i) {
        result[i] = v[i] - u[i];
    }
    return result;
}

// Hadamard Product
template <typename T, typename U, size_t N>
constexpr inline auto operator*(const VecN<T, N> &v, const VecN<U, N> &u) noexcept
{
    using R = decltype(T{} * U{});
    VecN<R, N> result;
    for (size_t i = 0; i < N; ++i) {
        result[i] = v[i] * u[i];
    }
    return result;
}

// Scalar Multiplication
template <typename T, typename U, size_t N>
constexpr inline auto operator*(T t, const VecN<U, N> &v) noexcept
{
    using R = decltype(T{} * U{});
    VecN<R, N> result;
    for (size_t i = 0; i < N; ++i) {
        result[i] = t * v[i];
    }
    return result;
}

template <typename T, typename U, size_t N>
constexpr inline auto operator*(const VecN<T, N> &v, U t) noexcept
{
    return t * v;
}

// Scalar Division
template <typename T, typename U, size_t N>
inline auto operator/(const VecN<T, N> &v, U t)
{
    using R = decltype(T{} / U{});
    if (t == U(0))
        throw std::invalid_argument("Division by zero");
    
    VecN<R, N> result;
    for (size_t i = 0; i < N; ++i) {
        result[i] = v[i] / t;
    }
    return result;
}

// Dot Product
template <typename T, typename U, size_t N> [[nodiscard]]
constexpr inline auto dot(const VecN<T, N> &u, const VecN<U, N> &v) noexcept
{
    using R = decltype(T{} * U{});
    R sum = R(0);
    for (size_t i = 0; i < N; ++i) {
        sum += static_cast<R>(u[i]) * v[i];
    }
    return sum;
}

/// @brief Cross Product (only for 3D vectors)
template <typename T, typename U> [[nodiscard]]
constexpr inline auto cross(const VecN<T, 3> &u, const VecN<U, 3> &v) noexcept
{
    using R = decltype(T{} * U{});
    return VecN<R, 3>(
        u[1] * v[2] - u[2] * v[1],
        u[2] * v[0] - u[0] * v[2],
        u[0] * v[1] - u[1] * v[0]);
}

// === EQUALITY and INEQUALITY ===
// Equality operators for floating-point types
template <typename T, size_t N>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type
operator==(const VecN<T, N>& a, const VecN<T, N>& b)
{
    for (size_t i = 0; i < N; ++i) {
        if (!nearlyEqual(a[i], b[i])) return false;
    }
    return true;
}

template <typename T, size_t N>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type
operator!=(const VecN<T, N>& a, const VecN<T, N>& b)
{
    return !(a == b);
}

// Exact comparison for integral types
template <typename T, size_t N>
typename std::enable_if<std::is_integral<T>::value, bool>::type
operator==(const VecN<T, N>& a, const VecN<T, N>& b)
{
    for (size_t i = 0; i < N; ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

template <typename T, size_t N>
typename std::enable_if<std::is_integral<T>::value, bool>::type
operator!=(const VecN<T, N>& a, const VecN<T, N>& b)
{
    return !(a == b);
}

} // namespace rt
