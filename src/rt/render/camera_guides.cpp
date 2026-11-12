#include <chrono>
#include <iostream>

#include "rt/render/camera.h"
#include "rt/render/denoise.h"

namespace rt
{

namespace
{
constexpr int GUIDE_SAMPLES    = 16; // camera rays per pixel for the guide images
constexpr int MAX_SPECULAR_HOP = 8;  // mirrors and glass followed before taking the surface behind

color clamp_unit(const color& value)
{
    return color(std::fmin(1.0f, std::fmax(0.0f, value.r())),
                 std::fmin(1.0f, std::fmax(0.0f, value.g())),
                 std::fmin(1.0f, std::fmax(0.0f, value.b())));
}
} // namespace

/*
 * The denoiser's guides: the color of the surface seen at each pixel, and its normal. Mirrors and
 * glass are followed to the first surface that is not one, since that is the detail the
 * denoiser should keep sharp. A few random camera rays are averaged, so edges stay antialiased.
 */
void Camera::render_guides(const hittable& world, std::vector<color>& albedo,
                           std::vector<color>& normal) const
{
    albedo.assign(framebuffer.size(), color(0, 0, 0));
    normal.assign(framebuffer.size(), color(0, 0, 0));
    const uint32_t guide_seed = clock_seed();

#pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < image_height; ++j)
    {
        seed_rng(mix_seed(guide_seed, static_cast<uint32_t>(j)));
        for (int i = 0; i < image_width; ++i)
        {
            color albedo_sum(0, 0, 0), normal_sum(0, 0, 0);
            for (int sample = 0; sample < GUIDE_SAMPLES; ++sample)
            {
                ray current = get_ray(i, j);
                color throughput(1, 1, 1);
                for (int hop = 0; hop <= MAX_SPECULAR_HOP; ++hop)
                {
                    hit_record record;
                    if (!world.hit(current, interval(0.001f, INF), record))
                    {
                        albedo_sum += throughput * clamp_unit(sky ? sky->value(current.direction())
                                                                  : background);
                        break;
                    }

                    const material& surface = *record.surface_material;
                    scatter_record scattering;
                    if (!surface.scatter(current, record, scattering))
                    {
                        // A light, or something that absorbs: its own color
                        albedo_sum +=
                            throughput * clamp_unit(surface.emitted(current, record, record.u,
                                                                    record.v, record.point));
                        normal_sum += record.normal;
                        break;
                    }
                    if (scattering.is_specular && hop < MAX_SPECULAR_HOP)
                    {
                        throughput = throughput * scattering.attenuation;
                        current    = scattering.specular_ray;
                        continue;
                    }
                    albedo_sum += throughput * clamp_unit(scattering.attenuation);
                    normal_sum += record.normal;
                    break;
                }
            }
            albedo[j * image_width + i] = clamp_unit(albedo_sum / float(GUIDE_SAMPLES));
            normal[j * image_width + i] = normal_sum / float(GUIDE_SAMPLES);
        }
    }
}

void Camera::denoise_framebuffer(const hittable& world)
{
    const auto start = std::chrono::steady_clock::now();
    std::vector<color> albedo, normal;
    render_guides(world, albedo, normal);
    if (denoise_image(framebuffer, albedo, normal, image_width, image_height))
    {
        const double seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        char text[32];
        std::snprintf(text, sizeof(text), "%.1f s", seconds);
        std::clog << "Denoised in " << text << '\n';
    }
}

} // namespace rt
