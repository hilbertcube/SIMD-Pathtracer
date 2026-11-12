#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "rt/materials/material.h"
#include "stb/stb_image.h"

namespace rt
{

/*
 * Another material with holes cut where a mask is clear, for leaf and twig cards. Textured mesh
 * triangles report no hit where masked_out() is true, so all rays pass through the gaps.
 */
class cutout : public material
{
public:
    // `mask_file` is an image with alpha, or a grey opacity map; texels below `threshold` are cut
    cutout(shared_ptr<material> base, const std::string& mask_file, float threshold = 0.5f)
        : base(base), threshold(static_cast<unsigned char>(threshold * 255.0f))
    {
        int channels          = 0;
        unsigned char* pixels = stbi_load(mask_file.c_str(), &width, &height, &channels, 4);
        if (pixels == nullptr)
        {
            throw std::runtime_error("could not load the cut-out mask '" + mask_file + "'");
        }
        // Alpha if the image has it, otherwise its brightness
        const bool has_alpha = channels == 2 || channels == 4;
        alpha.resize(static_cast<size_t>(width) * height);
        for (size_t i = 0; i < alpha.size(); ++i)
        {
            alpha[i] = has_alpha ? pixels[4 * i + 3] : pixels[4 * i];
        }
        stbi_image_free(pixels);
    }

    // Reads the same texel as image_texture
    bool masked_out(float u, float v) const override
    {
        u           = std::clamp(u, 0.0f, 1.0f);
        v           = 1.0f - std::clamp(v, 0.0f, 1.0f);
        const int x = std::min(static_cast<int>(u * width), width - 1);
        const int y = std::min(static_cast<int>(v * height), height - 1);
        return alpha[static_cast<size_t>(y) * width + x] < threshold;
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        return base->scatter(incoming, record, scattering);
    }

    color emitted(const ray& incoming, const hit_record& record, float u, float v,
                  const point3f& point) const override
    {
        return base->emitted(incoming, record, u, v, point);
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        return base->scattering_pdf(incoming, record, scattered);
    }

    bool needs_uv() const override { return true; }

private:
    shared_ptr<material> base;
    unsigned char threshold;
    int width = 0, height = 0;
    std::vector<unsigned char> alpha;
};

} // namespace rt
