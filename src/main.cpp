#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>

#include "rt/util/timer.h"
#include "scenes/scenes.h"

int main(int argc, char** argv)
{
    /*
     * Usage: ./main [scene] [output_directory]
     *   scene             which scene to render, e.g. 7 (defaults to 9)
     *   output_directory  where the image is written (defaults to images/, created if missing)
     */
    int scene                              = argc > 1 ? std::atoi(argv[1]) : 9;
    std::filesystem::path output_directory = argc > 2 ? argv[2] : "images";

    // Scene setup (model loading, BVH builds) and rendering together
    rt::ScopedTimer timer("Total");
    try
    {
        switch (scene)
        {
            case 1:
                scenes::spheres_scene(output_directory / "spheres.png");
                break;
            case 2:
                scenes::checkered_spheres(output_directory / "checked_spheres.png");
                break;
            case 3:
                scenes::earth(output_directory / "earth.png");
                break;
            case 4:
                scenes::perlin_spheres(output_directory / "perlin_spheres.png");
                break;
            case 5:
                scenes::quads(output_directory / "quads.png");
                break;
            case 6:
                scenes::simple_light(output_directory / "simple_light.png");
                break;
            case 7:
                scenes::cornell_box(output_directory / "cornell_box.png");
                break;
            case 8:
                scenes::final_scene(1000, 2000, 40, output_directory / "final_scene.png");
                break;
            case 9:
                scenes::three_D_model(output_directory / "3d_model.png");
                break;
            case 10:
                scenes::multiple_models(output_directory / "multiple_3d_models.png");
                break;
            case 11:
                scenes::dragon(output_directory / "dragon.png");
                break;
            case 12:
                scenes::sculpture_gallery(output_directory / "sculpture_gallery.png");
                break;
            case 13:
                scenes::neon_stage(output_directory / "neon_stage.png");
                break;
            case 14:
                scenes::bunny_campfire(output_directory / "bunny_campfire.png");
                break;
            default:
                scenes::final_scene(400, 250, 4, output_directory / "final_scene.png");
                break;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
