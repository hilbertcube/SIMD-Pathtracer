#pragma once

#include "rtm/vector.h"
#include "rtm/interval.h"
#include "rtm/random.h"

namespace rt
{

inline float linear_to_gamma(float linear_component)
{
    if (linear_component > 0)
        return std::sqrt(linear_component);
    return 0.0f;
}

} // namespace rt
