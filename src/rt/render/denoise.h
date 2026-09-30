#pragma once

#include <vector>

#include "rt/image/color.h"
#include "rt/math/vector.h"

namespace rt
{

/*
 * Denoises `image` in place with Intel Open Image Denoise. `albedo` and `normal` are noise-free
 * pictures of the surface color and shading normal at each pixel, which let the denoiser keep
 * edges and textures sharp. All three are width * height linear (HDR) colors.
 *
 * Returns false, leaving `image` as it was, if the program was built without Open Image
 * Denoise or the denoiser failed; the reason is printed.
 */
bool denoise_image(std::vector<vec3f>& image, const std::vector<vec3f>& albedo,
                   const std::vector<vec3f>& normal, int width, int height);

} // namespace rt
