#pragma once
#include "def.hpp"
#include "rtw_stb_image.hpp"
#include "perlin.hpp"

namespace rt
{
class texture {
public:
    texture() {}
    virtual ~texture() = default;

    virtual color value(float u, float v, const point3f& p) const = 0;
};

class solid_color : public texture {
public:
    solid_color(const color& albedo) : albedo(albedo) {}

    solid_color(float r, float g, float b) : solid_color(color(r, g, b)) {}

    color value(float u, float v, const point3f& p) const override
    {
        return albedo;
    }

private:
    color albedo;
};

class checker_texture : public texture {
public:
    checker_texture(float scale, shared_ptr<texture> even, shared_ptr<texture> odd)
        : inv_scale(1.0f / scale), even(even), odd(odd) {}

    checker_texture(float scale, const color& c1, const color& c2)
        : checker_texture(scale, make_shared<solid_color>(c1), make_shared<solid_color>(c2)) {}

    color value(float u, float v, const point3f& p) const override
    {
        auto xInteger = static_cast<int>(floor(inv_scale * p.x()));
        auto yInteger = static_cast<int>(floor(inv_scale * p.y()));
        auto zInteger = static_cast<int>(floor(inv_scale * p.z()));

        bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

        return isEven ? even->value(u, v, p) : odd->value(u, v, p);
    }
private:
    float inv_scale;
    shared_ptr<texture> even, odd;
};

class image_texture : public texture {
public:
    image_texture(const char* filename) : image(filename) {}

    color value(float u, float v, const point3f& p) const override
    {
        // If no texture data, return solid cyan
        if(image.height() <= 0) return color(0, 1, 1);

        u = interval(0, 1).clamp(u);
        v = 1.0f - interval(0, 1).clamp(v);

        auto i = static_cast<int>(u * image.width());
        auto j = static_cast<int>(v * image.height());
        auto pixel = image.pixel_data(i, j);
        auto color_scale = 1.0f / 255.0f;
        return color(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
    }
private:
    rtw_image image;
};

class noise_texture : public texture {
public:
    noise_texture(float scale) : scale(scale) {}

    color value(float u, float v, const point3f& p) const override 
    {
        return color(0.5f, 0.5f, 0.5f) * (1.0f + sin(scale * p.z() + 10.0f * noise.turbulence(p, 7)));
    }

private:
    perlin noise;
    float scale;
};

} // namespace rt
