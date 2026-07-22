#pragma once

#include <filesystem>

// Every scene builds its world, renders it and writes the image to `output_path`. Each one
// lives in its own file in this directory, and main() picks one by number.
namespace scenes
{

// Ray Tracing in One Weekend's final scene: a checkered ground, many small random
// spheres (some moving) and three large ones
void spheres_scene(const std::filesystem::path& output_path);

// Two large checkered spheres, one on top of the other
void checkered_spheres(const std::filesystem::path& output_path);

// A globe with an earth texture
void earth(const std::filesystem::path& output_path);

// Spheres with a marble (Perlin noise) texture
void perlin_spheres(const std::filesystem::path& output_path);

// Five colored quads arranged around the view
void quads(const std::filesystem::path& output_path);

// A marble sphere on a marble ground, lit by a sphere light and a quad light
void simple_light(const std::filesystem::path& output_path);

// The Cornell box with two turned boxes, a sphere and a pyramid
void cornell_box(const std::filesystem::path& output_path);

// The final scene of Ray Tracing: The Next Week: a field of boxes, an area light, fog, glass,
// metal, an earth globe, a marble sphere and a cluster of spheres. The image size and sampling are
// parameters, so a quick preview shares the code.
void final_scene(int image_width, int samples_per_pixel, int max_depth,
                 const std::filesystem::path& output_path);

// The Cornell box with a blue metal teapot
void three_D_model(const std::filesystem::path& output_path);

// The Cornell box with a gold dragon and mirrors on the side and back walls
void dragon(const std::filesystem::path& output_path);

// The Cornell box with Suzanne and the teapot on two turned cubes and the cow on the
// floor
void multiple_models(const std::filesystem::path& output_path);

// Six statues on marble pedestals under spotlights, in two staggered rows
void sculpture_gallery(const std::filesystem::path& output_path);

// A chrome armadillo and a glass bunny lit only by neon tubes, over a glossy black floor
void neon_stage(const std::filesystem::path& output_path);

// A ring of colorful bunnies around a glowing orb at night, with a horse watching
void bunny_campfire(const std::filesystem::path& output_path);

} // namespace scenes
