#pragma once

#include <filesystem>
#include <string>

#include "rt/geometry/hittable_list.h"
#include "rt/render/camera.h"

/*
 * Scenes are Lua scripts in scenes/. A script builds the scene by calling the
 * functions this file gives it: materials, shapes and models, add() and light() to put them in the
 * scene, and camera(). load_scene runs one; render_scene also renders the result.
 */
namespace scripting
{

struct scene_description
{
    rt::hittable_list world;  // in one BVH
    rt::hittable_list lights; // what the shadow rays aim at; empty when the scene names none
    rt::Camera camera;        // its output_path not set yet
    std::string output;       // the image's file name, from output() or the script's name
};

scene_description load_scene(const std::filesystem::path& script);

// Run the script and render its scene to `output_directory` / its output name
void render_scene(const std::filesystem::path& script,
                  const std::filesystem::path& output_directory);

} // namespace scripting
