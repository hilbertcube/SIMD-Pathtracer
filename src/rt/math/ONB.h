#pragma once

#include <cmath>
#include "rt/math/vector.h"

namespace rt
{

/// @brief Orthonormal basis built around a single direction (used as the local w axis).
class ONB
{
public:
    ONB() = default;

    /*
     * Branchless construction from Duff et al., "Building an Orthonormal Basis, Revisited"
     * (JCGT 2017). Once the direction is normalized the two other axes follow without
     * another square root.
     */
    explicit ONB(const vec3f& direction)
    {
        axis[2]             = unit_vector(direction);
        const vec3f& normal = axis[2];
        float sign          = std::copysign(1.0f, normal.z());
        float scale         = -1.0f / (sign + normal.z());
        float shared_term   = normal.x() * normal.y() * scale;
        axis[0] = vec3f(1.0f + sign * normal.x() * normal.x() * scale, sign * shared_term,
                        -sign * normal.x());
        axis[1] = vec3f(shared_term, sign + normal.y() * normal.y() * scale, -normal.y());
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
