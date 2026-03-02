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

// multithreading
#include <omp.h>

#include "rtm/constants.h"
#include "rtm/random.h"
#include "rtm/vector.h"

#include "hittable.h"
#include "color.h"
#include "material.h"
#include "pdf.h"
#include "save_file.h"

namespace rt
{
class Camera
{
public:
    float aspect_ratio    = 1.0f; // Ratio of image width over height
    int image_width       = 100;  // Rendered image width in px
    int samples_per_pixel = 10;   // Count of random samples for each pixel
    int max_depth = 10; // Max number of ray bounces into scene for the recursive ray_color function
    color background;   // Scene background color

    float field_of_view = 90.0f;             // Vertical view angle (field of view)
    point3f look_from   = point3f(0, 0, 0);  // Point camera is looking from
    point3f look_at     = point3f(0, 0, -1); // Point camera is looking at
    vec3f view_up       = vec3f(0, 1, 0);    // Camera-relative "up" direction

    float defocus_angle  = 0.0f;  // Variational angle of rays through each pixel
    float focus_distance = 10.0f; // Distance from camera look_from point to plane of perfect focus
    std::string output_filename = "output.png";

    /*
     * Renders the image in parallel (OpenMP, one row per task) and saves it.
     *   render(world)          - bounces sample only the material's own pdf.
     *                            Use for scenes lit by the background / sky.
     *   render(world, lights)  - also samples `lights` directly with shadow rays,
     *                            combined with material sampling by MIS (see
     *                            ray_color). Much less noise for small lights.
     *                            `lights` holds copies of the emitters' geometry;
     *                            their material is ignored.
     * For a single-threaded run, set OMP_NUM_THREADS=1.
     */
    void render(const hittable& world) { render_impl(world, nullptr); }

    void render(const hittable& world, const hittable& lights) { render_impl(world, &lights); }

private:
    static constexpr int ROULETTE_MIN_BOUNCES = 3; // Bounces before Russian roulette kicks in
    static constexpr float ROULETTE_MAX_SURVIVAL =
        0.95f; // Cap so bright paths still terminate eventually

    int image_height;          // Rendered image height
    int sample_count;          // Samples per pixel actually taken (at least 1)
    int strata_per_side;       // Side of the stratified sample grid
    float pixel_samples_scale; // Color scale factor (1 / sample_count)
    float stratum_size;        // 1 / strata_per_side
    point3f camera_center;     // Camera center
    point3f pixel00_location;  // Location of pixel (0, 0)
    vec3f pixel_delta_u;       // Offset to pixel to the right
    vec3f pixel_delta_v;       // Offset to pixel below
    vec3f right, up, backward; // Camera frame basis vector
    vec3f defocus_disk_u;      // Defocus disk horizontal radius
    vec3f defocus_disk_v;      // Defocus disk vertical radius
    std::vector<color> framebuffer;

    void render_impl(const hittable& world, const hittable* lights)
    {
        initialize();
        // The main thread built the scene from the fixed scene seed; its share of the
        // rendering should still get fresh random numbers each run
        seed_rng(clock_seed());
        int rows_done = 0;

// Rows are handed out dynamically, so faster cores simply take more of them
#pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < image_height; ++j)
        {
            for (int i = 0; i < image_width; ++i)
            {
                framebuffer[j * image_width + i] = sample_pixel(i, j, world, lights);
            }

#pragma omp critical
            std::clog << "\rProgress: " << std::fixed << std::setprecision(1)
                      << (++rows_done * 100.0f / image_height) << '%' << std::flush;
        }
        finish();
    }

    void finish()
    {
        std::clog << "\rDone. \n";
        // Write to file
        save_framebuffer(framebuffer, image_width, image_height, output_filename);
        std::clog << "Image saved to " << std::filesystem::current_path() / output_filename
                  << std::endl;
    }

    void initialize()
    {
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        // initialize framebuffer (sized, so pixels can be written by index)
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

        // Determine viewport dimensions
        auto theta           = degrees_to_radians(field_of_view);
        auto half_height     = std::tan(theta / 2);
        auto viewport_height = 2.0f * half_height * focus_distance;
        auto viewport_width  = viewport_height * (static_cast<float>(image_width) / image_height);

        // Calculate the right, up, backward unit basis vectors for the camera coordinate frame.
        backward = unit_vector(look_from - look_at);
        right    = unit_vector(cross(view_up, backward));
        up       = cross(backward, right);

        // Calculate the vectors across the horizontal and down the vertical viewport edges
        vec3f viewport_u = viewport_width * right;
        vec3f viewport_v = viewport_height * (-up);

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left =
            camera_center - (focus_distance * backward) - viewport_u / 2 - viewport_v / 2;
        pixel00_location = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

        // Calculate the camera defocus disk basis vectors
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

    // Camera ray through a uniformly random point in pixel (i, j)
    ray get_ray(int i, int j) const { return make_ray(i, j, sample_squared()); }

    // Camera ray through a random point in sub-pixel cell (stratum_i, stratum_j) of pixel (i, j)
    ray get_ray(int i, int j, int stratum_i, int stratum_j) const
    {
        return make_ray(i, j, sample_square_stratified(stratum_i, stratum_j));
    }

    // Construct a camera ray originating from the defocus disk and directed at
    // pixel (i, j) displaced by `offset` (in pixel units, within [-.5, .5]).
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
        // Random point in the unit square [-.5, .5] x [-.5, .5]
        return vec3f(random_float() - 0.5f, random_float() - 0.5f, 0.0f);
    }

    vec3f sample_square_stratified(int stratum_i, int stratum_j) const
    {
        // Random point in sub-cell (stratum_i, stratum_j) of the unit square [-.5, .5] x [-.5, .5]
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
     * Radiance arriving along r.
     *
     * At diffuse-like hits, light is gathered two ways and combined with multiple
     * importance sampling (MIS):
     *   1. Light sampling: pick a direction toward `lights` and trace a shadow ray. It only
     *      counts if the first thing it hits emits, so lights hidden behind glass or other
     *      objects contribute nothing (instead of inflating the other samples).
     *   2. Material sampling: continue the path along a direction drawn from the
     *      material's pdf. Emission found at the next hit is scaled by `emission_weight`,
     *      that direction's MIS weight, so lights aren't counted twice.
     */
    color ray_color(const ray& incoming, int depth, const hittable& world, const hittable* lights,
                    float emission_weight = 1.0f) const
    {
        // If we've exceeded the ray bounce limit, no more light is gathered
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
