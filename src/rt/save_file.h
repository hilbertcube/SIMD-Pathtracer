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

inline void write_color_ppm(std::ostream& stream, const color& pixel_color)
{
    // Apply a linear to gamma transform for gamma 2
    auto red   = linear_to_gamma(sanitize(pixel_color.r()));
    auto green = linear_to_gamma(sanitize(pixel_color.g()));
    auto blue  = linear_to_gamma(sanitize(pixel_color.b()));

    // Translate the [0, 1] component values to the byte range [0, 255]
    static const interval intensity(0.000f, 0.999f);
    int red_byte   = static_cast<int>(256 * intensity.clamp(red));
    int green_byte = static_cast<int>(256 * intensity.clamp(green));
    int blue_byte  = static_cast<int>(256 * intensity.clamp(blue));

    // Write out pixel components
    stream << red_byte << ' ' << green_byte << ' ' << blue_byte << '\n';
}

// format extractor with edge cases handle (although not needed)
inline std::string extract_file_format(const std::string& filename)
{
    // transform to lowercase
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char character) { return std::tolower(character); });

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
        std::ofstream file(filename);
        file << "P3\n" << width << " " << height << "\n255\n";
        for (const color& pixel : framebuffer)
        {
            write_color_ppm(file, pixel);
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

            auto red   = linear_to_gamma(sanitize(pixel_color.r()));
            auto green = linear_to_gamma(sanitize(pixel_color.g()));
            auto blue  = linear_to_gamma(sanitize(pixel_color.b()));

            int red_byte   = static_cast<int>(256 * intensity.clamp(red));
            int green_byte = static_cast<int>(256 * intensity.clamp(green));
            int blue_byte  = static_cast<int>(256 * intensity.clamp(blue));

            int index               = (j * width + i) * 3;
            image_buffer[index + 0] = static_cast<unsigned char>(red_byte);
            image_buffer[index + 1] = static_cast<unsigned char>(green_byte);
            image_buffer[index + 2] = static_cast<unsigned char>(blue_byte);
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
