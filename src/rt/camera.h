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
        float aspect_ratio = 1.0f;  // Ratio of image width over height
        int image_width = 100;      // Rendered image width in px
        int samples_per_pixel = 10; // Count of random samples for each pixel
        int max_depth = 10;         // Max number of ray bounces into scene for the recursive ray_color function
        color background;           // Scene background color

        float vfov = 90.0f;                  // Vertical view angle (field of view)
        point3f lookfrom = point3f(0, 0, 0); // Point camera is looking from
        point3f lookat = point3f(0, 0, -1);  // Point camera is looking at
        vec3f vup = vec3f(0, 1, 0);          // Camera-relative "up" direction

        float defocus_angle = 0.0f; // Variational angle of rays through each pixel
        float focus_dist = 10.0f;   // Distance from camera lookfrom point to plane of perfect focus
        std::string output_filename = "output.png";

        // Renders the image in parallel (OpenMP, one row per task) and saves it.
        //   render(world)          - bounces sample only the material's own pdf.
        //                            Use for scenes lit by the background / sky.
        //   render(world, lights)  - importance-samples `lights` (mixed 50/50 with the
        //                            material pdf). Much less noise for small lights.
        //                            `lights` holds copies of the emitters' geometry;
        //                            their material is ignored.
        // For a single-threaded run, set OMP_NUM_THREADS=1.
        void render(const hittable &world) { render_impl(world, nullptr); }
        void render(const hittable &world, const hittable &lights) { render_impl(world, &lights); }

    private:
        static constexpr int   RR_MIN_BOUNCES  = 3;     // Bounces before Russian roulette kicks in
        static constexpr float RR_MAX_SURVIVAL = 0.95f; // Cap so bright paths still terminate eventually

        int image_height;          // Rendered image height
        int spp;                   // Samples per pixel actually taken (at least 1)
        int sqrt_spp;              // Side of the stratified sample grid
        float pixel_samples_scale; // Color scale factor (1 / spp)
        float recip_sqrt_spp;      // 1 / sqrt_spp
        point3f camera_center;     // Camera center
        point3f pixel00_loc;       // Location of pixel (0, 0)
        vec3f pixel_delta_u;       // Offset to pixel to the right
        vec3f pixel_delta_v;       // Offset to pixel below
        vec3f u, v, w;             // Camera frame basis vector
        vec3f defocus_disk_u;      // Defocus disk horizontal radius
        vec3f defocus_disk_v;      // Defocus disk vertical radius
        std::vector<color> framebuffer;

        void render_impl(const hittable &world, const hittable *lights)
        {
            initialize();
            int rows_done = 0;

            // Rows are handed out dynamically, so faster cores simply take more of them
            #pragma omp parallel for schedule(dynamic)
            for (int j = 0; j < image_height; ++j)
            {
                for (int i = 0; i < image_width; ++i)
                    framebuffer[j * image_width + i] = sample_pixel(i, j, world, lights);

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
            std::clog << "Image saved to " << std::filesystem::current_path() / output_filename << std::endl;
        }

        void initialize()
        {
            image_height = static_cast<int>(image_width / aspect_ratio);
            image_height = (image_height < 1) ? 1 : image_height;

            // initialize framebuffer (sized, so pixels can be written by index)
            framebuffer.assign(static_cast<size_t>(image_width) * image_height, color(0, 0, 0));

            // Stratified sampling uses a sqrt_spp x sqrt_spp grid; any remaining samples
            // (when samples_per_pixel is not a perfect square) are taken uniformly at random.
            spp = std::max(1, samples_per_pixel);
            sqrt_spp = static_cast<int>(std::sqrt(static_cast<float>(spp)));
            while ((sqrt_spp + 1) * (sqrt_spp + 1) <= spp) ++sqrt_spp; // guard float rounding
            pixel_samples_scale = 1.0f / spp;
            recip_sqrt_spp = 1.0f / sqrt_spp;

            camera_center = lookfrom;

            // Determine viewport dimensions
            auto theta = degrees_to_radians(vfov);
            auto h = std::tan(theta / 2);
            auto viewport_height = 2.0f * h * focus_dist;
            auto viewport_width = viewport_height * (static_cast<float>(image_width) / image_height);

            // Calculate the u, v, w unit basis vectors for the camera coordinate frame.
            w = unit_vector(lookfrom - lookat);
            u = unit_vector(cross(vup, w));
            v = cross(w, u);

            // Calculate the vectors across the horizontal and down the vertical viewport edges
            vec3f viewport_u = viewport_width * u;
            vec3f viewport_v = viewport_height * (-v);

            // Calculate the horizontal and vertical delta vectors from pixel to pixel.
            pixel_delta_u = viewport_u / image_width;
            pixel_delta_v = viewport_v / image_height;

            // Calculate the location of the upper left pixel.
            auto viewport_upper_left = camera_center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
            pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

            // Calculate the camera defocus disk basis vectors
            auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
            defocus_disk_u = u * defocus_radius;
            defocus_disk_v = v * defocus_radius;
        }

        // Average of all samples for pixel (i, j)
        color sample_pixel(int i, int j, const hittable &world, const hittable *lights) const
        {
            color pixel_color(0, 0, 0);
            for (int s_j = 0; s_j < sqrt_spp; ++s_j)
                for (int s_i = 0; s_i < sqrt_spp; ++s_i)
                    pixel_color += ray_color(get_ray(i, j, s_i, s_j), max_depth, world, lights);

            for (int s = sqrt_spp * sqrt_spp; s < spp; ++s)
                pixel_color += ray_color(get_ray(i, j), max_depth, world, lights);

            return pixel_samples_scale * pixel_color;
        }

        // Camera ray through a uniformly random point in pixel (i, j)
        ray get_ray(int i, int j) const
        {
            return make_ray(i, j, sample_squared());
        }

        // Camera ray through a random point in sub-pixel cell (s_i, s_j) of pixel (i, j)
        ray get_ray(int i, int j, int s_i, int s_j) const
        {
            return make_ray(i, j, sample_square_stratified(s_i, s_j));
        }

        // Construct a camera ray originating from the defocus disk and directed at
        // pixel (i, j) displaced by `offset` (in pixel units, within [-.5, .5]).
        ray make_ray(int i, int j, const vec3f &offset) const
        {
            auto pixel_sample = pixel00_loc + (i + offset.x()) * pixel_delta_u + (j + offset.y()) * pixel_delta_v;
            auto ray_origin = (defocus_angle <= 0) ? camera_center : defocus_disk_sample();
            auto ray_direction = pixel_sample - ray_origin;
            auto ray_time = random_float();
            return ray(ray_origin, ray_direction, ray_time);
        }

        vec3f sample_squared() const
        {
            // Random point in the unit square [-.5, .5] x [-.5, .5]
            return vec3f(random_float() - 0.5f, random_float() - 0.5f, 0.0f);
        }

        vec3f sample_square_stratified(int s_i, int s_j) const
        {
            // Random point in sub-cell (s_i, s_j) of the unit square [-.5, .5] x [-.5, .5]
            float px = ((s_i + random_float()) * recip_sqrt_spp) - 0.5f;
            float py = ((s_j + random_float()) * recip_sqrt_spp) - 0.5f;
            return vec3f(px, py, 0.0f);
        }

        point3f defocus_disk_sample() const
        {
            auto p = random_in_unit_disk<float>();
            return camera_center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
        }

        color ray_color(const ray &r, int depth, const hittable &world, const hittable *lights) const
        {
            // If we've exceeded the ray bounce limit, no more light is gathered
            if (depth <= 0)
                return color(0, 0, 0);

            hit_record rec;

            if (!world.hit(r, interval(0.001f, INF), rec))
                return background;

            scatter_record srec;
            color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

            if (!rec.mat->scatter(r, rec, srec))
                return color_from_emission;

            // Russian roulette: after a few bounces, end the path with probability 1 - p and
            // boost survivors by 1/p. Unbiased, and dark surfaces stop wasting bounces early.
            if (max_depth - depth >= RR_MIN_BOUNCES)
            {
                const color& a = srec.attenuation;
                float p = std::fmin(RR_MAX_SURVIVAL, std::fmax(a.r(), std::fmax(a.g(), a.b())));
                if (random_float() >= p)
                    return color_from_emission;
                srec.attenuation /= p;
            }

            // Specular materials (metal, glass) pick their own ray; no pdf weighting
            if (srec.skip_pdf)
                return color_from_emission + srec.attenuation * ray_color(srec.skip_pdf_ray, depth - 1, world, lights);

            // Diffuse-like materials: sample a direction from a pdf and weight by
            // (material density / sampling density)
            ray scattered;
            float pdf_value;
            if (lights)
            {
                hittable_pdf light_pdf(*lights, rec.p);
                mixture_pdf p(light_pdf, *srec.pdf_ptr);
                scattered = ray(rec.p, p.generate(), r.time());
                pdf_value = p.value(scattered.direction());
            }
            else
            {
                scattered = ray(rec.p, srec.pdf_ptr->generate(), r.time());
                pdf_value = srec.pdf_ptr->value(scattered.direction());
            }

            // A zero (or NaN) density carries no usable energy; dividing would give inf/NaN
            if (!(pdf_value > 0.0f))
                return color_from_emission;

            float scattering_pdf = rec.mat->scattering_pdf(r, rec, scattered);

            color sample_color = ray_color(scattered, depth - 1, world, lights);
            color color_from_scatter = (srec.attenuation * scattering_pdf * sample_color) / pdf_value;

            return color_from_emission + color_from_scatter;
        }
    };

}
