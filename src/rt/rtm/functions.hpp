#pragma once

#include "../def.hpp"
#include "constants.hpp"

FORCE_INLINE float degrees_to_radians(float degrees)
{
    return degrees * PI / 180.0f;
}

FORCE_INLINE float radians_to_degrees(float radians)
{
    return radians * 180.0f / PI;
}

FORCE_INLINE float percentage(float total, float remaining)
{
    return (total - remaining) * 100.0f / total;
}
