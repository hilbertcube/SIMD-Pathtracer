#pragma once

#include "stb/stb_image.h"

#include <cstdlib>
#include <iostream>

namespace rt
{

class rtw_image
{
public:
    rtw_image() {}

    // Looks in $RTW_IMAGES, then the current directory, then images/ here and in up to six
    // parent directories. If nothing loads, width() and height() return 0.
    rtw_image(const char* image_filename, std::string image_directory = "images/")
    {
        auto filename              = std::string(image_filename);
        auto environment_directory = getenv("RTW_IMAGES");

        if (environment_directory &&
            load(std::string(environment_directory) + "/" + image_filename))
        {
            return;
        }

        if (load(filename))
        {
            return;
        }
        std::string parent_directory = "../";

        if (load(filename))
        {
            return;
        }
        if (load(image_directory + filename))
        {
            return;
        }
        if (load("../" + image_directory + filename))
        {
            return;
        }
        if (load("../../" + image_directory + filename))
        {
            return;
        }
        if (load("../../../" + image_directory + filename))
        {
            return;
        }
        if (load("../../../../" + image_directory + filename))
        {
            return;
        }
        if (load("../../../../../" + image_directory + filename))
        {
            return;
        }
        if (load("../../../../../../" + image_directory + filename))
        {
            return;
        }

        std::cerr << "warning: could not load image file '" << image_filename
                  << "'; using solid cyan instead\n";
    }

    ~rtw_image()
    {
        delete[] byte_data;
        stbi_image_free(float_data);
    }

    // Loads linear (gamma 1) float data and a byte copy for pixel_data(); false on failure.
    bool load(const std::string& filename)
    {
        // stb requires this out-parameter; the value is unused
        auto original_components = bytes_per_pixel;
        float_data = stbi_loadf(filename.c_str(), &image_width, &image_height, &original_components,
                                bytes_per_pixel);
        if (float_data == nullptr)
        {
            return false;
        }

        bytes_per_scanline = image_width * bytes_per_pixel;
        convert_to_bytes();
        return true;
    }

    int width() const { return (float_data == nullptr) ? 0 : image_width; }

    int height() const { return (float_data == nullptr) ? 0 : image_height; }

    // Linear RGB floats at (x, y), clamped to the image, unclipped (for HDR); needs width() > 0
    const float* float_pixel(int x, int y) const
    {
        x = clamp(x, 0, image_width);
        y = clamp(y, 0, image_height);
        return float_data + (static_cast<size_t>(y) * image_width + x) * bytes_per_pixel;
    }

    const unsigned char* pixel_data(int x, int y) const
    {
        // Three RGB bytes at (x, y), or magenta when no image is loaded.
        static unsigned char magenta[] = {255, 0, 255};
        if (byte_data == nullptr)
        {
            return magenta;
        }

        x = clamp(x, 0, image_width);
        y = clamp(y, 0, image_height);

        return byte_data + y * bytes_per_scanline + x * bytes_per_pixel;
    }

private:
    const int bytes_per_pixel = 3;
    float* float_data         = nullptr; // Linear floating point pixel data
    unsigned char* byte_data  = nullptr; // Linear 8-bit pixel data
    int image_width           = 0;
    int image_height          = 0;
    int bytes_per_scanline    = 0;

    static int clamp(int x, int low, int high)
    {
        // Clamps x to [low, high)
        if (x < low)
        {
            return low;
        }
        if (x < high)
        {
            return x;
        }
        return high - 1;
    }

    static unsigned char float_to_byte(float value)
    {
        if (value <= 0.0)
        {
            return 0;
        }
        if (1.0 <= value)
        {
            return 255;
        }
        return static_cast<unsigned char>(256.0 * value);
    }

    void convert_to_bytes()
    {
        int total_bytes = image_width * image_height * bytes_per_pixel;
        byte_data       = new unsigned char[total_bytes];

        auto* destination = byte_data;
        auto* source      = float_data;
        for (auto i = 0; i < total_bytes; i++, source++, destination++)
        {
            *destination = float_to_byte(*source);
        }
    }
};

} // namespace rt
