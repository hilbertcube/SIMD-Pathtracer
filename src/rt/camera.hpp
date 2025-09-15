#pragma once

#include <iostream>
#include <vector>
#include <stdint.h>
#include <sstream>
#include <thread>
#include <mutex>
#include <functional>
#include <memory>
#include <immintrin.h>  // For SIMD
#include <cstring>      // For memcpy
#include <sstream>      // string stream
#include <iomanip>
#include <filesystem>

// multithreading
#include <thread>
#include <atomic>
#include <omp.h>
#include <mutex>
#include <functional>

#include "rtm/constants.hpp"
#include "rtm/random.hpp"
#include "rtm/vector.hpp"

#include "hittable.hpp"
#include "color.hpp"
#include "material.hpp"
#include "save_file.hpp"

namespace rt {
class Camera {
public:
    float    aspect_ratio = 1.0f;  // Ratio of image width over height
    int image_width       = 100;   // Rendered image width in px
    int samples_per_pixel = 10;    // Count of maximum random samples of each pixel
    int max_depth         = 10;    // Max number of ray bounces into scene for the recursive ray_color function
    color  background;             // Scene background color

    float   vfov     = 90.0f;               // Vertical view angle (field of view)
    point3f lookfrom = point3f(0, 0, 0);    // Point camera is looking from
    point3f lookat   = point3f(0, 0, -1);   // Point camera is looking at
    vec3f   vup      = vec3f(0, 0, 0);      // Camera-relative "up" direction
    
    float defocus_angle  = 0.0f;     // Variational angle of rays through each pixel
    float focus_dist     = 10.0f;       // Distance from camera lookfrom point to plane of perfect focus
    std::string output_filename = "output.png";

    void render_serial(const hittable& world)
    {   
        initialize();
        std::vector<color> framebuffer(image_width * image_height);

        for (int j{0}; j < image_height; ++j) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i{0}; i < image_width; ++i)
            {
                color pixel_color(0, 0, 0);
                for (int sample{0}; sample < samples_per_pixel; ++sample)
                {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }
                framebuffer[j * image_width + i] = pixel_samples_scale * pixel_color;
            }
        }

        std::clog << "\rDone. \n";
        // Write to file
        save_framebuffer(framebuffer, image_width, image_height, output_filename);
        std::clog << "\nImage saved to " << std::filesystem::current_path() / output_filename << std::endl;
    }

    void render_omp(const hittable& world)
    {
        initialize();
        std::vector<color> framebuffer(image_width * image_height);

        #pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < image_height; ++j) {
            #pragma omp critical
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;

            for (int i = 0; i < image_width; ++i) {
                color pixel_color(0, 0, 0);
                for (int sample = 0; sample < samples_per_pixel; ++sample) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }
                framebuffer[j * image_width + i] = pixel_samples_scale * pixel_color;
            }
        }
        std::clog << "\rDone. \n";
        // Write to file
        save_framebuffer(framebuffer, image_width, image_height, output_filename);
        std::clog << "\nImage saved to " << std::filesystem::current_path() / output_filename << std::endl;
    }

    // Structure to represent a tile in the image
    struct Tile {
        int x0, y0;      // Top-left corner
        int x1, y1;      // Bottom-right corner
        Tile(int _x0, int _y0, int _x1, int _y1) 
            : x0(_x0), y0(_y0), x1(_x1), y1(_y1) {}
    };

    void render_tiles(const hittable& world)
    {
        initialize();
        std::vector<color> framebuffer(image_width * image_height);
        
        // Calculate optimal tile size based on cache size
        // Typical L1 cache is 32KB-64KB. Each pixel is 3 floats (12 bytes)
        // Use 32x32 tiles (~12 KB/tile)
        const int TILE_SIZE = 32;
        
        // Create tiles
        std::vector<Tile> tiles;
        for (int y = 0; y < image_height; y += TILE_SIZE) {
            for (int x = 0; x < image_width; x += TILE_SIZE) {
                tiles.emplace_back(
                    x, y,
                    std::min(x + TILE_SIZE, image_width),
                    std::min(y + TILE_SIZE, image_height)
                );
            }
        }

        // Progress tracking
        std::atomic<int> tiles_completed(0);
        const int total_tiles = tiles.size();

        // Render tiles in parallel
        #pragma omp parallel
        {
            // Each thread gets its own random number generator
            thread_local std::mt19937 gen(std::random_device{}());
            thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);

            #pragma omp for schedule(dynamic, 1)
            for (int tile_idx = 0; tile_idx < tiles.size(); ++tile_idx) {
                const Tile& tile = tiles[tile_idx];
                
                // Process each pixel in the tile
                for (int j = tile.y0; j < tile.y1; ++j) {
                    for (int i = tile.x0; i < tile.x1; ++i) {
                        color pixel_color(0, 0, 0);
                        
                        // Sample the pixel
                        for (int sample = 0; sample < samples_per_pixel; ++sample) {
                            ray r = get_ray(i, j);
                            pixel_color += ray_color(r, max_depth, world);
                        }
                        
                        framebuffer[j * image_width + i] = pixel_samples_scale * pixel_color;
                    }
                }
                int completed = ++tiles_completed;
                #pragma omp critical
                {
                    float progress = (completed * 100.0f) / total_tiles;
                    std::clog << "\rProgress: " << std::fixed << std::setprecision(1) 
                             << progress << "% [" << completed << "/" << total_tiles 
                             << " tiles]" << std::flush;
                }
            }
        }
        std::clog << "\rDone. \n";
        // Write to file
        save_framebuffer(framebuffer, image_width, image_height, output_filename);
        std::clog << "Image saved to " << std::filesystem::current_path() / output_filename << std::endl;
    }

private:
    float pixel_samples_scale;  // Color scale factor
    int image_height;           // Rendered image height
    point3f camera_center;      // Camera center
    point3f pixel00_loc;        // Location of pixel (0, 0)
    vec3f pixel_delta_u;        // Offset to pixel to the right
    vec3f pixel_delta_v;        // Offset to pixel below
    vec3f u, v, w;              // Camera frame basis vector
    vec3f defocus_disk_u;       // Defocus disk horizontal radius
    vec3f defocus_disk_v;       // Defocus disk vertical radius

    void initialize()
    {
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        pixel_samples_scale = 1.0f / samples_per_pixel;

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

    // Construct a camera ray originating from the defocus disk and directed at a randomly
    // sampled point around the pixel at location (i, j)
    ray get_ray(int i, int j) const
    {
        auto offset = sample_squared();
        auto pixel_sample = pixel00_loc + (i + offset.x()) * pixel_delta_u + (j + offset.y()) * pixel_delta_v;
        auto ray_origin = (defocus_angle <= 0)? camera_center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;
        auto ray_time = random_float();
        return ray(ray_origin, ray_direction, ray_time);
    }

    vec3f sample_squared() const
    {
        // use random float between 0 and 1
        return vec3f(random_float() - 0.5f, random_float() - 0.5f, 0.0f);
    }

    point3f defocus_disk_sample() const
    {
        auto p = random_in_unit_disk<float>();
        return camera_center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

    color ray_color(const ray& r, int depth, const hittable& world) const
    {
        // If we've exceeded the ray bounce limit, no more light is gathered
        if(depth == 0) return color(0, 0, 0);

        hit_record rec;

        if(!world.hit(r, interval(0.001f, INF), rec)) return background;

        ray scattered;
        color attenuation;
        color color_from_emission = rec.mat->emitted(rec.u, rec.v, rec.p);
        if (!rec.mat->scatter(r, rec, attenuation, scattered))
            return color_from_emission;
        
        // Recursive call
        color color_from_scatter = attenuation * ray_color(scattered, depth - 1, world);
        return color_from_emission + color_from_scatter;
    }
};

}