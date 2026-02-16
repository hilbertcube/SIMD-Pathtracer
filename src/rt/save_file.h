#pragma once
#include <vector>
#include <algorithm>
#include <fstream>
#include <cctype>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../external/stb/stb_image_write.h"

#include "rtm/vector.h"
#include "rtm/interval.h"
#include "rtm/random.h"
#include "color.h"

namespace rt
{

// Replace NaN components with zero (a stray NaN sample would otherwise poison the pixel)
inline float sanitize(float x)
{
    return (x != x) ? 0.0f : x;
}

inline void write_color_ppm(std::ostream& out, const color& pixel_color)
{
    // Apply a linear to gamma transform for gamma 2
    auto r = linear_to_gamma(sanitize(pixel_color.r()));
    auto g = linear_to_gamma(sanitize(pixel_color.g()));
    auto b = linear_to_gamma(sanitize(pixel_color.b()));

    // Translate the [0, 1] component values to the byte range [0, 255]
    static const interval intensity(0.000f, 0.999f);
    int rbyte = static_cast<int>(256 * intensity.clamp(r));
    int gbyte = static_cast<int>(256 * intensity.clamp(g));
    int bbyte = static_cast<int>(256 * intensity.clamp(b));

    // Write out pixel components
    out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}

// format extractor with edge cases handle (although not needed)
inline std::string extract_file_format(const std::string& filename)
{
    // transform to lowercase
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    for (int i = static_cast<int>(lower.size()) - 1; i >= 0; --i)
    {
        if (lower[i] == '.')
        {
            // dot is first char: treat as hidden file, not extension
            if (i == 0)
            {
                return "";
            }
            return lower.substr(i + 1);
        }
    }
    return ""; // no dot found
}

inline void save_framebuffer(const std::vector<color>& framebuffer, const int width,
                             const int height, const std::string& filename)
{
    const std::string format = extract_file_format(filename);

    // not recommended, very slow and produces very large files
    if (format == "ppm")
    {
        std::ofstream out(filename);
        out << "P3\n" << width << " " << height << "\n255\n";
        for (const color& c : framebuffer)
        {
            write_color_ppm(out, c);
        }
        return;
    }

    // Convert framebuffer -> unsigned char buffer
    std::vector<unsigned char> image_buffer(width * height * 3);
    static const interval intensity(0.000f, 0.999f);

    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < width; ++i)
        {
            const color& pixel_color = framebuffer[j * width + i];

            auto r = linear_to_gamma(sanitize(pixel_color.r()));
            auto g = linear_to_gamma(sanitize(pixel_color.g()));
            auto b = linear_to_gamma(sanitize(pixel_color.b()));

            int rbyte = static_cast<int>(256 * intensity.clamp(r));
            int gbyte = static_cast<int>(256 * intensity.clamp(g));
            int bbyte = static_cast<int>(256 * intensity.clamp(b));

            int idx               = (j * width + i) * 3;
            image_buffer[idx + 0] = static_cast<unsigned char>(rbyte);
            image_buffer[idx + 1] = static_cast<unsigned char>(gbyte);
            image_buffer[idx + 2] = static_cast<unsigned char>(bbyte);
        }
    }

    if (format == "png")
    {
        stbi_write_png(filename.c_str(), width, height, 3, image_buffer.data(), width * 3);
    }
    else if (format == "jpg" || format == "jpeg")
    {
        stbi_write_jpg(filename.c_str(), width, height, 3, image_buffer.data(), 90); // quality
    }
    else if (format == "bmp")
    {
        stbi_write_bmp(filename.c_str(), width, height, 3, image_buffer.data());
    }
    else if (format == "tga")
    {
        stbi_write_tga(filename.c_str(), width, height, 3, image_buffer.data());
    }
    else
    {
        throw std::runtime_error("Unsupported format: " + format);
    }
}

} // namespace rt
