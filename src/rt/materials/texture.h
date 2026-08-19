#pragma once
#include "rt/util/def.h"
#include "rt/image/rtw_stb_image.h"
#include "rt/materials/perlin.h"

namespace rt
{
class texture
{
public:
    texture() {}

    virtual ~texture() = default;

    virtual color value(float u, float v, const point3f& point) const = 0;

    /// Whether value() reads the (u, v) texture coordinates. Hits on surfaces whose
    /// textures don't can skip computing them; the default is the safe answer.
    virtual bool needs_uv() const { return true; }
};

class solid_color : public texture
{
public:
    solid_color(const color& albedo) : albedo(albedo) {}

    solid_color(float red, float green, float blue) : solid_color(color(red, green, blue)) {}

    color value(float u, float v, const point3f& point) const override { return albedo; }

    bool needs_uv() const override { return false; }

private:
    color albedo;
};

class checker_texture : public texture
{
public:
    checker_texture(float scale, shared_ptr<texture> even, shared_ptr<texture> odd)
        : inverse_scale(1.0f / scale), even(even), odd(odd)
    {
    }

    checker_texture(float scale, const color& even_color, const color& odd_color)
        : checker_texture(scale, make_shared<solid_color>(even_color),
                          make_shared<solid_color>(odd_color))
    {
    }

    color value(float u, float v, const point3f& point) const override
    {
        auto cell_x = static_cast<int>(floor(inverse_scale * point.x()));
        auto cell_y = static_cast<int>(floor(inverse_scale * point.y()));
        auto cell_z = static_cast<int>(floor(inverse_scale * point.z()));

        bool is_even = (cell_x + cell_y + cell_z) % 2 == 0;

        return is_even ? even->value(u, v, point) : odd->value(u, v, point);
    }

    bool needs_uv() const override { return even->needs_uv() || odd->needs_uv(); }

private:
    float inverse_scale;
    shared_ptr<texture> even, odd;
};

class image_texture : public texture
{
public:
    image_texture(const char* filename) : image(filename) {}

    color value(float u, float v, const point3f& point) const override
    {
        // If no texture data, return solid cyan
        if (image.height() <= 0)
        {
            return color(0, 1, 1);
        }

        u = interval(0, 1).clamp(u);
        v = 1.0f - interval(0, 1).clamp(v);

        auto column      = static_cast<int>(u * image.width());
        auto row         = static_cast<int>(v * image.height());
        auto pixel       = image.pixel_data(column, row);
        auto color_scale = 1.0f / 255.0f;
        return color(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
    }

private:
    rtw_image image;
};

class noise_texture : public texture
{
public:
    // `scale` sets the stripes along z. `frequency` scales space for the noise that wobbles
    // them, so its features are 1 / frequency units across: the default 1 suits objects a few
    // units across, and a hundred-unit object wants about 0.05.
    noise_texture(float scale, float frequency = 1.0f) : scale(scale), frequency(frequency) {}

    color value(float u, float v, const point3f& point) const override
    {
        return color(0.5f, 0.5f, 0.5f) *
               (1.0f +
                std::sin(scale * point.z() + 10.0f * noise.turbulence(frequency * point, 7)));
    }

    bool needs_uv() const override { return false; }

private:
    perlin noise;
    float scale;
    float frequency;
};

} // namespace rt
