#pragma once

namespace rt
{

/*
 * Options that apply to every render, set once from the command line (see main). The scenes
 * don't know about them: Camera reads them when it renders.
 */
struct render_settings
{
    bool denoise = false; // clean the image with Intel Open Image Denoise after rendering
    int samples  = 0;     // samples per pixel for every scene; 0 keeps each scene's own
};

inline render_settings& global_render_settings()
{
    static render_settings settings;
    return settings;
}

} // namespace rt
