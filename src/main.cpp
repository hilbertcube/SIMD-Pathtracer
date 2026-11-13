#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "rt/render/render_settings.h"
#include "rt/util/timer.h"
#include "scripting/scene_script.h"

int main(int argc, char** argv)
{
    /*
     * Usage: ./main scene [output_directory] [--spp N] [--width N] [--denoise]
     *   scene             the scene script, e.g. scenes/07_cornell_box.lua
     *   output_directory  where the image is written (defaults to images/, created if missing)
     *   --spp N           samples per pixel for the whole render, instead of the scene's own
     *   --width N         image width, instead of the scene's own (the aspect ratio stays)
     *   --denoise         clean the image with Intel Open Image Denoise, which makes a low
     *                     sample count (say 64) look like a much higher one
     */
    std::vector<std::string> positional;
    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--denoise")
        {
            rt::global_render_settings().denoise = true;
        }
        else if (argument == "--spp" && i + 1 < argc)
        {
            rt::global_render_settings().samples = std::atoi(argv[++i]);
        }
        else if (argument == "--width" && i + 1 < argc)
        {
            rt::global_render_settings().width = std::atoi(argv[++i]);
        }
        else
        {
            positional.push_back(argument);
        }
    }
    if (positional.empty())
    {
        std::cerr << "usage: " << argv[0]
                  << " scene.lua [output_directory] [--spp N] [--width N] [--denoise]\n";
        return 1;
    }
    const std::filesystem::path scene      = positional[0];
    std::filesystem::path output_directory = positional.size() > 1 ? positional[1] : "images";

    // Scene setup (model loading, BVH builds) and rendering together
    rt::ScopedTimer timer("Total");
    try
    {
        scripting::render_scene(scene, output_directory);
    }
    catch (const std::exception& error)
    {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
