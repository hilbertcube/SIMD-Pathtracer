#pragma once

#include "rt/math/vector.h"
#include "rt/math/interval.h"
#include "rt/math/random.h"

namespace rt
{

inline float linear_to_gamma(float linear_component)
{
    if (linear_component > 0)
    {
        return std::sqrt(linear_component);
    }
    return 0.0f;
}

} // namespace rt
