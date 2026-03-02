#pragma once

#include <cmath>
#include "vector.h"

namespace rt
{

/// @brief Orthonormal basis built around a single direction (used as the local w axis).
class ONB
{
public:
    ONB() = default;

    explicit ONB(const vec3f& direction)
    {
        axis[2]      = unit_vector(direction);
        vec3f helper = (std::fabs(axis[2].x()) > 0.9f) ? vec3f(0, 1, 0) : vec3f(1, 0, 0);
        axis[1]      = unit_vector(cross(axis[2], helper));
        axis[0]      = cross(axis[2], axis[1]);
    }

    const vec3f& tangent() const { return axis[0]; }

    const vec3f& bitangent() const { return axis[1]; }

    const vec3f& normal() const { return axis[2]; }

    /// @brief Transform from basis coordinates to world space.
    vec3f transform(const vec3f& local) const
    {
        return (local[0] * axis[0]) + (local[1] * axis[1]) + (local[2] * axis[2]);
    }

private:
    vec3f axis[3];
};

} // namespace rt
