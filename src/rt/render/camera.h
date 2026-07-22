#pragma once

#include <iostream>
#include <vector>
#include <stdint.h>
#include <sstream>
#include <functional>
#include <cstring> // For memcpy
#include <iomanip>
#include <filesystem>
#include <algorithm>

#include <omp.h>

#include "rt/math/constants.h"
#include "rt/math/random.h"
#include "rt/math/vector.h"

#include "rt/geometry/hittable.h"
#include "rt/image/color.h"
#include "rt/materials/material.h"
#include "rt/materials/pdf.h"
#include "rt/image/save_file.h"
#include "rt/util/progress.h"
#include "rt/util/timer.h"

namespace rt
{
class Camera
{
public:
    float aspect_ratio    = 1.0f; // Width over height
    int image_width       = 100;
    int samples_per_pixel = 10;
    int max_depth         = 10; // Maximum bounces per path
    color background;

    float field_of_view = 90.0f; // Vertical, in degrees
    point3f look_from   = point3f(0, 0, 0);
    point3f look_at     = point3f(0, 0, -1);
    vec3f view_up       = vec3f(0, 1, 0); // Camera-relative "up" direction

    float defocus_angle               = 0.0f;         // In degrees; 0 disables depth of field
    float focus_distance              = 10.0f;        // Distance to the plane of perfect focus
    std::filesystem::path output_path = "output.png"; // its directory is created if missing

    /*
     * Renders in parallel (OpenMP, one row per task) and saves the image. render(world) samples
     * only the material's own pdf; render(world, lights) also samples `lights` with shadow rays
     * and combines the two by MIS (see ray_color), which removes most noise from small lights.
     * `lights` is geometry only; its materials are ignored. Set OMP_NUM_THREADS=1 to render
     * single-threaded.
     */
    void render(const hittable& world) { render_impl(world, nullptr); }

    void render(const hittable& world, const hittable& lights) { render_impl(world, &lights); }

private:
    static constexpr int ROULETTE_MIN_BOUNCES = 3; // Bounces before Russian roulette kicks in
    static constexpr float ROULETTE_MAX_SURVIVAL =
        0.95f; // Cap so bright paths still terminate eventually

    int image_height;
    int sample_count;          // Samples per pixel actually taken (at least 1)
    int strata_per_side;       // Side of the stratified sample grid
    float pixel_samples_scale; // Color scale factor (1 / sample_count)
    float stratum_size;        // 1 / strata_per_side
    point3f camera_center;
    point3f pixel00_location;
    vec3f pixel_delta_u; // Offset to pixel to the right
    vec3f pixel_delta_v; // Offset to pixel below
    vec3f right, up, backward;
    vec3f defocus_disk_u; // Defocus disk horizontal radius
    vec3f defocus_disk_v; // Defocus disk vertical radius
    std::vector<color> framebuffer;

    void render_impl(const hittable& world, const hittable* lights)
    {
        initialize();
        // Fresh sampling noise each run; each row reseeds from it (see the loop below)
        const uint32_t run_seed = clock_seed();

        // Create the output directory first, so a bad path fails now instead of after the render
        if (output_path.has_parent_path())
        {
            std::filesystem::create_directories(output_path.parent_path());
        }
        std::clog << "Rendering " << output_path.string() << " (" << image_width << 'x'
                  << image_height << ", " << sample_count << " samples/pixel, max depth "
                  << max_depth << ", " << omp_get_max_threads() << " threads)\n";

        ProgressBar progress(image_height);

// Dynamic scheduling: faster cores simply take more rows
#pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < image_height; ++j)
        {
            // Seeded per row, so the result doesn't depend on which thread renders it
            seed_rng(mix_seed(run_seed, static_cast<uint32_t>(j)));
            for (int i = 0; i < image_width; ++i)
            {
                framebuffer[j * image_width + i] = sample_pixel(i, j, world, lights);
            }
            progress.advance();
        }

        progress.finish();
        report_render_time(progress.elapsed());
        save();
    }

    void report_render_time(std::chrono::nanoseconds elapsed) const
    {
        double seconds = std::chrono::duration<double>(elapsed).count();
        double samples = static_cast<double>(image_width) * image_height * sample_count;
        char rate[32];
        std::snprintf(rate, sizeof(rate), "%.1f M", samples / seconds / 1e6);
        std::clog << "Rendered in " << format_duration(elapsed) << " (" << rate << " samples/s)\n";
    }

    void save() const
    {
        save_framebuffer(framebuffer, image_width, image_height, output_path.string());
        std::clog << "Saved " << output_path.string() << std::endl;
    }

    void initialize()
    {
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        framebuffer.assign(static_cast<size_t>(image_width) * image_height, color(0, 0, 0));

        // Stratified sampling uses a strata_per_side x strata_per_side grid; any remaining samples
        // (when samples_per_pixel is not a perfect square) are taken uniformly at random.
        sample_count    = std::max(1, samples_per_pixel);
        strata_per_side = static_cast<int>(std::sqrt(static_cast<float>(sample_count)));
        while ((strata_per_side + 1) * (strata_per_side + 1) <= sample_count)
        {
            ++strata_per_side; // guard float rounding
        }
        pixel_samples_scale = 1.0f / sample_count;
        stratum_size        = 1.0f / strata_per_side;

        camera_center = look_from;

        auto theta           = degrees_to_radians(field_of_view);
        auto half_height     = std::tan(theta / 2);
        auto viewport_height = 2.0f * half_height * focus_distance;
        auto viewport_width  = viewport_height * (static_cast<float>(image_width) / image_height);

        backward = unit_vector(look_from - look_at);
        right    = unit_vector(cross(view_up, backward));
        up       = cross(backward, right);

        vec3f viewport_u = viewport_width * right;
        vec3f viewport_v = viewport_height * (-up);

        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        auto viewport_upper_left =
            camera_center - (focus_distance * backward) - viewport_u / 2 - viewport_v / 2;
        pixel00_location = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

        auto defocus_radius = focus_distance * std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u      = right * defocus_radius;
        defocus_disk_v      = up * defocus_radius;
    }

    // Average of all samples for pixel (i, j)
    color sample_pixel(int i, int j, const hittable& world, const hittable* lights) const
    {
        color pixel_color(0, 0, 0);
        for (int stratum_j = 0; stratum_j < strata_per_side; ++stratum_j)
        {
            for (int stratum_i = 0; stratum_i < strata_per_side; ++stratum_i)
            {
                pixel_color +=
                    ray_color(get_ray(i, j, stratum_i, stratum_j), max_depth, world, lights);
            }
        }

        for (int sample = strata_per_side * strata_per_side; sample < sample_count; ++sample)
        {
            pixel_color += ray_color(get_ray(i, j), max_depth, world, lights);
        }

        return pixel_samples_scale * pixel_color;
    }

    // Camera ray through a uniformly random point in the pixel
    ray get_ray(int i, int j) const { return make_ray(i, j, sample_squared()); }

    // Camera ray through a random point in sub-pixel cell (stratum_i, stratum_j)
    ray get_ray(int i, int j, int stratum_i, int stratum_j) const
    {
        return make_ray(i, j, sample_square_stratified(stratum_i, stratum_j));
    }

    // Ray from a point on the defocus disk through pixel (i, j), shifted by `offset` pixels
    ray make_ray(int i, int j, const vec3f& offset) const
    {
        auto pixel_sample =
            pixel00_location + (i + offset.x()) * pixel_delta_u + (j + offset.y()) * pixel_delta_v;
        auto ray_origin    = (defocus_angle <= 0) ? camera_center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;
        auto ray_time      = random_float();
        return ray(ray_origin, ray_direction, ray_time);
    }

    vec3f sample_squared() const
    {
        // Random point in [-.5, .5] x [-.5, .5]
        return vec3f(random_float() - 0.5f, random_float() - 0.5f, 0.0f);
    }

    vec3f sample_square_stratified(int stratum_i, int stratum_j) const
    {
        // Random point in sub-cell (stratum_i, stratum_j) of that square
        float offset_x = ((stratum_i + random_float()) * stratum_size) - 0.5f;
        float offset_y = ((stratum_j + random_float()) * stratum_size) - 0.5f;
        return vec3f(offset_x, offset_y, 0.0f);
    }

    point3f defocus_disk_sample() const
    {
        auto disk_point = random_in_unit_disk<float>();
        return camera_center + (disk_point[0] * defocus_disk_u) + (disk_point[1] * defocus_disk_v);
    }

    // Power heuristic: how much of a sample to credit to the strategy with density `a`
    // when another strategy could have produced the same direction with density `b`.
    static float mis_weight(float density, float other_density)
    {
        return density * density / (density * density + other_density * other_density);
    }

    /**
     * Radiance arriving along `incoming`. At diffuse hits, light is gathered two ways and
     * combined with multiple importance sampling (MIS):
     *   1. Light sampling: a shadow ray toward `lights`. It counts only if it reaches an
     *      emitter, so lights hidden behind glass or other objects contribute nothing.
     *   2. Material sampling: the path continues along a direction from the material's pdf.
     *      Emission found at the next hit is scaled by `emission_weight` (that direction's
     *      MIS weight), so lights aren't counted twice.
     */
    color ray_color(const ray& incoming, int depth, const hittable& world, const hittable* lights,
                    float emission_weight = 1.0f) const
    {
        if (depth <= 0)
        {
            return color(0, 0, 0);
        }

        hit_record record;
        if (!world.hit(incoming, interval(0.001f, INF), record))
        {
            return background;
        }

        scatter_record scattering;
        color emission = emission_weight * record.surface_material->emitted(
                                               incoming, record, record.u, record.v, record.point);
        if (!record.surface_material->scatter(incoming, record, scattering))
        {
            return emission;
        }

        // Specular materials (metal, glass) pick their own ray: nothing to importance-sample
        if (scattering.is_specular)
        {
            if (!russian_roulette(depth, scattering.attenuation))
            {
                return emission;
            }
            return emission + scattering.attenuation *
                                  ray_color(scattering.specular_ray, depth - 1, world, lights);
        }

        const pdf& material_pdf = *scattering.direction_pdf;
        color direct_light(0, 0, 0);

        // 1. Light sampling with a shadow ray
        if (lights)
        {
            ray to_light(record.point, lights->random(record.point), incoming.time());
            float light_density = lights->pdf_value(record.point, to_light.direction());
            float scattering_density =
                light_density > 0.0f
                    ? record.surface_material->scattering_pdf(incoming, record, to_light)
                    : 0.0f;

            hit_record light_record;
            if (scattering_density > 0.0f &&
                world.hit(to_light, interval(0.001f, INF), light_record))
            {
                color light_emission = light_record.surface_material->emitted(
                    to_light, light_record, light_record.u, light_record.v, light_record.point);
                float weight = mis_weight(light_density, material_pdf.value(to_light.direction()));
                direct_light = scattering.attenuation * light_emission *
                               (scattering_density * weight / light_density);
            }
        }

        // 2. Material sampling, continuing the path
        if (!russian_roulette(depth, scattering.attenuation))
        {
            return emission + direct_light;
        }

        ray scattered(record.point, material_pdf.generate(), incoming.time());
        float sample_density = material_pdf.value(scattered.direction());

        // A zero (or NaN) density carries no usable energy; dividing would give inf/NaN
        if (!(sample_density > 0.0f))
        {
            return emission + direct_light;
        }

        float scattering_density =
            record.surface_material->scattering_pdf(incoming, record, scattered);
        float weight = lights ? mis_weight(sample_density,
                                           lights->pdf_value(record.point, scattered.direction()))
                              : 1.0f;

        color indirect_light = ray_color(scattered, depth - 1, world, lights, weight);
        return emission + direct_light +
               scattering.attenuation * indirect_light * (scattering_density / sample_density);
    }

    // Russian roulette: after a few bounces, end the path with probability 1 - p and boost
    // survivors by 1/p. Unbiased, and dark surfaces stop wasting bounces early.
    bool russian_roulette(int depth, color& attenuation) const
    {
        if (max_depth - depth < ROULETTE_MIN_BOUNCES)
        {
            return true;
        }
        float survival_probability =
            std::fmin(ROULETTE_MAX_SURVIVAL,
                      std::fmax(attenuation.r(), std::fmax(attenuation.g(), attenuation.b())));
        if (random_float() >= survival_probability)
        {
            return false;
        }
        attenuation /= survival_probability;
        return true;
    }
};

} // namespace rt
