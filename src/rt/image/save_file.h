#pragma once
#include <vector>
#include <algorithm>
#include <fstream>
#include <cctype>
#include <stdexcept>
#include <string>

#include "stb/stb_image_write.h"

#include "rt/math/vector.h"
#include "rt/math/interval.h"
#include "rt/math/random.h"
#include "rt/image/color.h"

namespace rt
{

// Replace NaN components with zero (a stray NaN sample would otherwise poison the pixel)
inline float sanitize(float x)
{
    return (x != x) ? 0.0f : x;
}

inline void write_color_ppm(std::ostream& stream, const color& pixel_color)
{
    auto red   = linear_to_gamma(sanitize(pixel_color.r()));
    auto green = linear_to_gamma(sanitize(pixel_color.g()));
    auto blue  = linear_to_gamma(sanitize(pixel_color.b()));

    static const interval intensity(0.000f, 0.999f);
    int red_byte   = static_cast<int>(256 * intensity.clamp(red));
    int green_byte = static_cast<int>(256 * intensity.clamp(green));
    int blue_byte  = static_cast<int>(256 * intensity.clamp(blue));

    stream << red_byte << ' ' << green_byte << ' ' << blue_byte << '\n';
}

// The file extension in lower case, or "" if there is none
inline std::string extract_file_format(const std::string& filename)
{
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
    return "";
}

/*
 * Writes the framebuffer; the extension picks the format (png, jpg/jpeg, bmp, tga, ppm).
 * Throws std::runtime_error for an unknown format or a failed write.
 */
inline void save_framebuffer(const std::vector<color>& framebuffer, const int width,
                             const int height, const std::string& filename)
{
    const std::string format = extract_file_format(filename);

    // Slow and produces very large files
    if (format == "ppm")
    {
        std::ofstream file(filename);
        file << "P3\n" << width << " " << height << "\n255\n";
        for (const color& pixel : framebuffer)
        {
            write_color_ppm(file, pixel);
        }
        if (!file)
        {
            throw std::runtime_error("could not write '" + filename + "'");
        }
        return;
    }

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

    int written = 0; // stb returns 0 on failure
    if (format == "png")
    {
        written =
            stbi_write_png(filename.c_str(), width, height, 3, image_buffer.data(), width * 3);
    }
    else if (format == "jpg" || format == "jpeg")
    {
        written = stbi_write_jpg(filename.c_str(), width, height, 3, image_buffer.data(),
                                 90); // quality
    }
    else if (format == "bmp")
    {
        written = stbi_write_bmp(filename.c_str(), width, height, 3, image_buffer.data());
    }
    else if (format == "tga")
    {
        written = stbi_write_tga(filename.c_str(), width, height, 3, image_buffer.data());
    }
    else
    {
        throw std::runtime_error("unsupported image format '" + format + "' for '" + filename +
                                 "' (use png, jpg, bmp, tga or ppm)");
    }

    if (written == 0)
    {
        throw std::runtime_error("could not write '" + filename + "'");
    }
}

} // namespace rt
