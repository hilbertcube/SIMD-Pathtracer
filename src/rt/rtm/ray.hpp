#pragma once

#include "vector.hpp"

namespace rt 
{

// Ray is a quantity with direction, P(t) = A + t * b
// Here, A is the ray origin and b is the direction vector

class ray {
public:
    ray() {}

    ray(const point3f& origin, const vec3f& direction, float time)
    : orig(origin), dir(direction), tm(time) {}

    ray(const point3f& origin, const vec3f& direction)
      : ray(origin, direction, 0) {}

    [[nodiscard]]
    const point3f& origin() const 
    { 
        return orig;
    }

    [[nodiscard]]
    const point3f& direction() const 
    { 
        return dir; 
    }

    [[nodiscard]]
    point3f at(float t) const 
    { 
        return orig + t * dir;
    }

    [[nodiscard]]
    float time() const { return tm; }
private:
    point3f orig;
    vec3f dir;
    float tm;
};

} // namespace rt

