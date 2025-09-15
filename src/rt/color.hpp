#pragma once

#include "rtm/vector.hpp"
#include "rtm/interval.hpp"
#include "rtm/random.hpp"

namespace rt {

inline float linear_to_gamma(float linear_component)
{
    if(linear_component > 0)
        return std::sqrt(linear_component);
    return 0.0f;
}

void write_color(std::ostream& out, const color& pixel_color)
{
    // Apply a linear to gamma transform for gamma 2
    auto r = linear_to_gamma(pixel_color.r());
    auto g = linear_to_gamma(pixel_color.g());
    auto b = linear_to_gamma(pixel_color.b());
 
    // Translate the [0, 1] component values to the byte range [0, 255]
    static const interval intensity(0.000f, 0.999f);
    int rbyte = static_cast<int>(256 * intensity.clamp(r));
    int gbyte = static_cast<int>(256 * intensity.clamp(g));
    int bbyte = static_cast<int>(256 * intensity.clamp(b));

    // Write out pixel components
    out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}


}

