#pragma once

/// @brief Floating-point comparison (epsilon-based). \n
/// @brief Reference: https://floating-point-gui.de/errors/comparison/
template <typename T>
inline bool nearlyEqual(T a, T b, T epsilon = std::numeric_limits<T>::epsilon())
{
    static_assert(std::is_floating_point_v<T>,
                  "T must be floating-point");
    const T absA = std::fabs(a);
    const T absB = std::fabs(b);
    const T diff = std::fabs(a - b);

    if (a == b) {
        return true;    // Shortcut: handles infinities and exact matches
    } else if (a == 0 || b == 0 || (absA + absB < std::numeric_limits<T>::min())) {
        // Handles cases where one or both numbers are near zero
        return diff < (epsilon * std::numeric_limits<T>::min());
    } else {
        // Relative error
        return diff / std::min((absA + absB), std::numeric_limits<T>::max()) < epsilon;
    }
}

template <typename T>
inline bool isNegative(T a, T epsilon = std::numeric_limits<T>::epsilon())
{
    static_assert(std::is_floating_point_v<T>,
                  "T must be floating-point");

    if (std::fabs(a) <= epsilon) {
        // Treat as zero
        return false;
    }
    return a < 0;
}

template <typename T>
inline bool isPositive(T a, T epsilon = std::numeric_limits<T>::epsilon())
{
    static_assert(std::is_floating_point_v<T>,
                  "T must be floating-point");
    if (std::fabs(a) > epsilon) {
        // Treat as zero
        return false;
    }
    return a > 0;
}