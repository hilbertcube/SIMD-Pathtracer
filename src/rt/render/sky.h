#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

#include "rt/image/color.h"
#include "rt/image/rtw_stb_image.h"
#include "rt/math/constants.h"
#include "rt/math/vector.h"

namespace rt
{

/*
 * An equirectangular sky image (usually .hdr), seen by rays that hit nothing; +y is up. It is not
 * sampled as a light, so it suits soft skies. For a sky with a sun, clip the sun out with
 * `max_radiance` and add it back as a distant sphere light.
 */
class sky_image
{
public:
    // `strength` scales the image, `rotation_degrees` turns it about +y, and `max_radiance` clips
    // each channel before scaling
    sky_image(const std::string& filename, float strength = 1.0f, float rotation_degrees = 0.0f,
              float max_radiance = INF)
        : image(filename.c_str()), strength(strength), rotation(rotation_degrees * PI / 180.0f),
          max_radiance(max_radiance)
    {
        if (image.width() <= 0)
        {
            throw std::runtime_error("could not load the sky '" + filename + "'");
        }
    }

    sky_image(const sky_image&)            = delete;
    sky_image& operator=(const sky_image&) = delete;

    // Radiance from `direction` (any length), bilinearly filtered
    color value(const vec3f& direction) const
    {
        const vec3f d = unit_vector(direction);
        float u       = (std::atan2(d.z(), d.x()) + rotation) / (2.0f * PI);
        u -= std::floor(u);
        const float v    = std::acos(std::clamp(d.y(), -1.0f, 1.0f)) / PI;
        const int width  = image.width();
        const int height = image.height();

        // Wrap horizontally, clamp vertically
        const float x = u * width - 0.5f, y = v * height - 0.5f;
        const int x0 = static_cast<int>(std::floor(x)), y0 = static_cast<int>(std::floor(y));
        const float fx = x - x0, fy = y - y0;
        const int xa = (x0 % width + width) % width, xb = (xa + 1) % width;
        const int ya = std::clamp(y0, 0, height - 1), yb = std::clamp(y0 + 1, 0, height - 1);

        auto at = [&](int px, int py)
        {
            const float* p = image.float_pixel(px, py);
            return color(std::fmin(p[0], max_radiance), std::fmin(p[1], max_radiance),
                         std::fmin(p[2], max_radiance));
        };
        const color top    = (1 - fx) * at(xa, ya) + fx * at(xb, ya);
        const color bottom = (1 - fx) * at(xa, yb) + fx * at(xb, yb);
        return strength * ((1 - fy) * top + fy * bottom);
    }

private:
    rtw_image image;
    float strength;
    float rotation; // radians about +y
    float max_radiance;
};

} // namespace rt
