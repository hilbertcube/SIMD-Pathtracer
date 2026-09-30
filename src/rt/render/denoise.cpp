#include "rt/render/denoise.h"

#include <cmath>
#include <iostream>

#ifdef RT_HAVE_OIDN
#include <OpenImageDenoise/oidn.hpp>
#endif

namespace rt
{

#ifdef RT_HAVE_OIDN

namespace
{

// The pixels as consecutive floats, with anything non-finite replaced by zero
std::vector<float> to_floats(const std::vector<vec3f>& pixels)
{
    std::vector<float> floats(pixels.size() * 3);
    for (size_t i = 0; i < pixels.size(); ++i)
    {
        for (int channel = 0; channel < 3; ++channel)
        {
            const float value       = pixels[i][channel];
            floats[3 * i + channel] = std::isfinite(value) ? value : 0.0f;
        }
    }
    return floats;
}

} // namespace

bool denoise_image(std::vector<vec3f>& image, const std::vector<vec3f>& albedo,
                   const std::vector<vec3f>& normal, int width, int height)
{
    try
    {
        oidn::DeviceRef device = oidn::newDevice();
        device.commit();

        const std::vector<float> noisy_floats  = to_floats(image);
        const std::vector<float> albedo_floats = to_floats(albedo);
        const std::vector<float> normal_floats = to_floats(normal);

        const size_t bytes            = noisy_floats.size() * sizeof(float);
        oidn::BufferRef color_buffer  = device.newBuffer(bytes);
        oidn::BufferRef albedo_buffer = device.newBuffer(bytes);
        oidn::BufferRef normal_buffer = device.newBuffer(bytes);
        oidn::BufferRef output_buffer = device.newBuffer(bytes);
        color_buffer.write(0, bytes, noisy_floats.data());
        albedo_buffer.write(0, bytes, albedo_floats.data());
        normal_buffer.write(0, bytes, normal_floats.data());

        oidn::FilterRef filter = device.newFilter("RT");
        filter.setImage("color", color_buffer, oidn::Format::Float3, width, height);
        filter.setImage("albedo", albedo_buffer, oidn::Format::Float3, width, height);
        filter.setImage("normal", normal_buffer, oidn::Format::Float3, width, height);
        filter.setImage("output", output_buffer, oidn::Format::Float3, width, height);
        filter.set("hdr", true);
        filter.set("quality", oidn::Quality::High);
        filter.commit();
        filter.execute();

        const char* message = nullptr;
        if (device.getError(message) != oidn::Error::None)
        {
            std::cerr << "Denoising failed: " << message << '\n';
            return false;
        }

        std::vector<float> denoised(noisy_floats.size());
        output_buffer.read(0, bytes, denoised.data());
        for (size_t i = 0; i < image.size(); ++i)
        {
            image[i] = vec3f(denoised[3 * i], denoised[3 * i + 1], denoised[3 * i + 2]);
        }
        return true;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Denoising failed: " << error.what() << '\n';
        return false;
    }
}

#else

bool denoise_image(std::vector<vec3f>&, const std::vector<vec3f>&, const std::vector<vec3f>&, int,
                   int)
{
    std::cerr << "This build has no Open Image Denoise; see the README to add it.\n";
    return false;
}

#endif

} // namespace rt
