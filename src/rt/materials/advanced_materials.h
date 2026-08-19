#pragma once

#include <algorithm>
#include <cmath>

#include "rt/materials/material.h"
#include "rt/materials/perlin.h"

namespace rt
{

// (1 - cosine)^5, the angular part of Schlick's approximation of the Fresnel reflectance
inline float schlick_fifth_power(float cosine)
{
    const float m  = 1.0f - cosine;
    const float m2 = m * m;
    return m2 * m2 * m;
}

/*
 * The ray after hitting a glass-like surface: it reflects or refracts, with the reflection chosen
 * by Schlick's Fresnel probability (or always, past the critical angle).
 */
inline ray dielectric_ray(const ray& incoming, const hit_record& record, float refraction_index)
{
    const float ratio          = record.front_face ? 1.0f / refraction_index : refraction_index;
    const vec3f unit_direction = unit_vector(incoming.direction());
    const float cos_theta      = std::fmin(dot(-unit_direction, record.normal), 1.0f);
    const float sin_theta      = std::sqrt(1.0f - cos_theta * cos_theta);

    float normal_reflectance = (1.0f - ratio) / (1.0f + ratio);
    normal_reflectance *= normal_reflectance;
    const float reflectance =
        normal_reflectance + (1.0f - normal_reflectance) * schlick_fifth_power(cos_theta);

    const bool reflects   = ratio * sin_theta > 1.0f || reflectance > random_float();
    const vec3f direction = reflects ? reflect(unit_direction, record.normal)
                                     : refract(unit_direction, record.normal, ratio);
    return ray(record.point, direction, incoming.time());
}

/*
 * A diffuse base under a clear glossy coat: plastic, ceramic, glaze, paint. The coat reflects
 * with Schlick's Fresnel probability, which rises toward grazing angles; otherwise the light
 * reaches the diffuse base. Each outcome is its own kind of scatter, so lights are still sampled
 * directly on the diffuse part.
 */
class coated : public material
{
public:
    coated(const color& albedo, float coat_index = 1.5f)
        : coated(make_shared<solid_color>(albedo), coat_index)
    {
    }

    coated(shared_ptr<texture> albedo_texture, float coat_index = 1.5f)
        : albedo_texture(albedo_texture),
          normal_reflectance(((coat_index - 1.0f) / (coat_index + 1.0f)) *
                             ((coat_index - 1.0f) / (coat_index + 1.0f)))
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const vec3f unit_direction = unit_vector(incoming.direction());
        const float cosine         = std::fmax(0.0f, dot(-unit_direction, record.normal));
        const float fresnel =
            normal_reflectance + (1.0f - normal_reflectance) * schlick_fifth_power(cosine);

        if (random_float() < fresnel)
        {
            scattering.attenuation   = color(1.0f, 1.0f, 1.0f);
            scattering.direction_pdf = nullptr;
            scattering.is_specular   = true;
            scattering.specular_ray =
                ray(record.point, reflect(unit_direction, record.normal), incoming.time());
            return true;
        }

        scattering.attenuation   = albedo_texture->value(record.u, record.v, record.point);
        scattering.cosine        = cosine_pdf(record.normal);
        scattering.direction_pdf = &scattering.cosine;
        scattering.is_specular   = false;
        return true;
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        const float projection = dot(record.normal, scattered.direction());
        return projection <= 0.0f ? 0.0f : projection / (scattered.direction().length() * PI);
    }

    bool needs_uv() const override { return albedo_texture->needs_uv(); }

private:
    shared_ptr<texture> albedo_texture;
    float normal_reflectance; // Fresnel reflectance of the coat at normal incidence
};

/*
 * Glass that absorbs light as it travels through (Beer-Lambert), so thick parts are darker and
 * more saturated than thin ones. `tint` is the color of white light after it has crossed
 * `distance` units of the glass.
 */
class tinted_dielectric : public material
{
public:
    tinted_dielectric(float refraction_index, const color& tint, float distance)
        : refraction_index(refraction_index),
          absorption(-std::log(std::fmax(tint.r(), 1e-4f)) / distance,
                     -std::log(std::fmax(tint.g(), 1e-4f)) / distance,
                     -std::log(std::fmax(tint.b(), 1e-4f)) / distance)
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        scattering.attenuation = color(1.0f, 1.0f, 1.0f);
        if (!record.front_face)
        {
            // The ray reached this surface from inside the glass, over `distance` units
            const float distance = record.t * incoming.direction().length();
            scattering.attenuation =
                color(std::exp(-absorption.r() * distance), std::exp(-absorption.g() * distance),
                      std::exp(-absorption.b() * distance));
        }
        scattering.direction_pdf = nullptr;
        scattering.is_specular   = true;
        scattering.specular_ray  = dielectric_ray(incoming, record, refraction_index);
        return true;
    }

    bool needs_uv() const override { return false; }

private:
    float refraction_index;
    color absorption; // per unit distance, for red, green and blue
};

/*
 * Glass whose refractive index differs for red, green and blue (blue bends most, as in real
 * glass), so edges split into colors like a prism.
 *
 * A path can only follow one color at a time, so each path picks a color channel and carries only
 * that channel's light (weighted by 3, so the average over paths is unbiased). There is no
 * per-path state in the renderer, so the channel is kept in the ray's time: a camera ray's time
 * is a random number in [0, 1), whose third gives the channel. The first scatter here adds 1 to
 * the time to record that the weight has been applied. This means the material can't be combined
 * with motion blur.
 */
class dispersive_dielectric : public material
{
public:
    dispersive_dielectric(float red_index, float green_index, float blue_index)
        : indices{red_index, green_index, blue_index}
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const bool weighted = incoming.time() >= 1.0f;
        const float time    = weighted ? incoming.time() - 1.0f : incoming.time();
        const int channel   = std::min(2, static_cast<int>(time * 3.0f));

        scattering.attenuation = color(1.0f, 1.0f, 1.0f);
        if (!weighted)
        {
            scattering.attenuation = color(channel == 0 ? 3.0f : 0.0f, channel == 1 ? 3.0f : 0.0f,
                                           channel == 2 ? 3.0f : 0.0f);
        }

        const ray refracted      = dielectric_ray(incoming, record, indices[channel]);
        scattering.direction_pdf = nullptr;
        scattering.is_specular   = true;
        scattering.specular_ray  = ray(refracted.origin(), refracted.direction(), time + 1.0f);
        return true;
    }

    bool needs_uv() const override { return false; }

private:
    float indices[3];
};

/*
 * A conductor (metal) with GGX microfacet roughness and Fresnel reflection: the color is the
 * reflectance at normal incidence, and it turns whiter toward grazing angles. Roughness 0 is a
 * perfect mirror. Rough metals are sampled like diffuse surfaces, so lights are sampled directly
 * and highlights converge much faster than with a fuzzy mirror.
 */
class conductor : public material
{
public:
    // roughness in [0, 1]; the GGX width alpha is its square
    conductor(const color& reflectance, float roughness)
        : reflectance(reflectance), alpha(roughness * roughness)
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const vec3f view   = -unit_vector(incoming.direction());
        const float cosine = dot(view, record.normal);
        if (cosine <= 0.0f)
        {
            return false;
        }

        const float edge       = schlick_fifth_power(cosine);
        scattering.attenuation = reflectance + edge * (color(1.0f, 1.0f, 1.0f) - reflectance);
        scattering.is_specular = alpha < 1e-3f;
        if (scattering.is_specular)
        {
            scattering.direction_pdf = nullptr;
            scattering.specular_ray =
                ray(record.point, reflect(-view, record.normal), incoming.time());
        }
        else
        {
            scattering.direction_pdf = scattering.emplace<ggx_pdf>(record.normal, view, alpha);
        }
        return true;
    }

    // The microfacet BRDF times the cosine, without the Fresnel color (that is the attenuation)
    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        const vec3f view             = -unit_vector(incoming.direction());
        const vec3f light            = unit_vector(scattered.direction());
        const float normal_dot_view  = dot(record.normal, view);
        const float normal_dot_light = dot(record.normal, light);
        if (normal_dot_view <= 0.0f || normal_dot_light <= 0.0f)
        {
            return 0.0f;
        }
        const float alpha_squared   = alpha * alpha;
        const float normal_dot_half = dot(record.normal, unit_vector(light + view));
        return ggx_distribution(normal_dot_half, alpha_squared) *
               ggx_masking(normal_dot_light, alpha_squared) *
               ggx_masking(normal_dot_view, alpha_squared) / (4.0f * normal_dot_view);
    }

    bool needs_uv() const override { return false; }

private:
    color reflectance;
    float alpha;
};

/*
 * A pearly base under a thin transparent film, like mother of pearl, oil on water or a soap
 * film. The film reflects light that interferes with the light reflected from beneath it, and
 * the colors that survive depend on the viewing angle.
 */
class iridescent : public material
{
public:
    // thickness_nm is the film's thickness in nanometers (a few hundred), film_index its
    // refractive index, and coat_reflectance how strongly it reflects when viewed head on
    iridescent(const color& base_albedo, float thickness_nm = 480.0f, float film_index = 1.4f,
               float coat_reflectance = 0.3f)
        : base_albedo(base_albedo), thickness_nm(thickness_nm), film_index(film_index),
          coat_reflectance(coat_reflectance)
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const vec3f unit_direction = unit_vector(incoming.direction());
        const float cosine         = std::fmax(0.0f, dot(-unit_direction, record.normal));
        const float fresnel =
            coat_reflectance + (1.0f - coat_reflectance) * schlick_fifth_power(cosine);

        if (random_float() < fresnel)
        {
            // Light that bounced inside the film travels an extra 2 * n * d * cos(theta) before
            // rejoining the surface reflection, and each wavelength is reinforced or cancelled
            const float sin_inside     = std::sqrt(1.0f - cosine * cosine) / film_index;
            const float cos_inside     = std::sqrt(1.0f - sin_inside * sin_inside);
            const float path           = 2.0f * film_index * thickness_nm * cos_inside;
            const float wavelengths[3] = {650.0f, 550.0f, 450.0f};
            float tint[3];
            for (int channel = 0; channel < 3; ++channel)
            {
                tint[channel] = 1.0f + std::cos(2.0f * PI * path / wavelengths[channel]);
            }
            scattering.attenuation   = color(tint[0], tint[1], tint[2]);
            scattering.direction_pdf = nullptr;
            scattering.is_specular   = true;
            scattering.specular_ray =
                ray(record.point, reflect(unit_direction, record.normal), incoming.time());
            return true;
        }

        scattering.attenuation   = base_albedo;
        scattering.cosine        = cosine_pdf(record.normal);
        scattering.direction_pdf = &scattering.cosine;
        scattering.is_specular   = false;
        return true;
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        const float projection = dot(record.normal, scattered.direction());
        return projection <= 0.0f ? 0.0f : projection / (scattered.direction().length() * PI);
    }

    bool needs_uv() const override { return false; }

private:
    color base_albedo;
    float thickness_nm;
    float film_index;
    float coat_reflectance;
};

/*
 * Cloth like velvet or satin: a diffuse surface whose color shifts toward a sheen color at
 * grazing angles, where fibers standing up from the surface catch the light.
 */
class velvet : public material
{
public:
    // The sheen's strength is strength * (1 - cos(angle to the normal))^power
    velvet(const color& albedo, const color& sheen, float power = 3.0f, float strength = 1.0f)
        : albedo(albedo), sheen(sheen), power(power), strength(strength)
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const float cosine =
            std::fmax(0.0f, dot(-unit_vector(incoming.direction()), record.normal));
        const float weight = std::fmin(1.0f, strength * std::pow(1.0f - cosine, power));

        scattering.attenuation   = (1.0f - weight) * albedo + weight * sheen;
        scattering.cosine        = cosine_pdf(record.normal);
        scattering.direction_pdf = &scattering.cosine;
        scattering.is_specular   = false;
        return true;
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        const float projection = dot(record.normal, scattered.direction());
        return projection <= 0.0f ? 0.0f : projection / (scattered.direction().length() * PI);
    }

    bool needs_uv() const override { return false; }

private:
    color albedo, sheen;
    float power, strength;
};

/*
 * Bump mapping: tilts the surface normal as if the surface had small procedural dents and
 * bumps, then shades with another material. Hammered metal, orange peel, rough stone, with no
 * extra geometry.
 */
class bumped : public material
{
public:
    // feature_scale is the number of bumps per unit length; height the bumps' height in units
    bumped(shared_ptr<material> base, float feature_scale, float height)
        : base(base), feature_scale(feature_scale), height(height)
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        return base->scatter(incoming, perturbed(incoming, record), scattering);
    }

    color emitted(const ray& incoming, const hit_record& record, float u, float v,
                  const point3f& point) const override
    {
        return base->emitted(incoming, perturbed(incoming, record), u, v, point);
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        return base->scattering_pdf(incoming, perturbed(incoming, record), scattered);
    }

    bool needs_uv() const override { return base->needs_uv(); }

private:
    shared_ptr<material> base;
    perlin noise;
    float feature_scale;
    float height;

    // The hit with its normal tilted away from the uphill direction of the bump field
    hit_record perturbed(const ray& incoming, const hit_record& record) const
    {
        const point3f p      = feature_scale * record.point;
        const float step     = 0.05f;
        const vec3f gradient = (height * feature_scale / (2.0f * step)) *
                               vec3f(noise.noise_smooth(p + vec3f(step, 0, 0)) -
                                         noise.noise_smooth(p - vec3f(step, 0, 0)),
                                     noise.noise_smooth(p + vec3f(0, step, 0)) -
                                         noise.noise_smooth(p - vec3f(0, step, 0)),
                                     noise.noise_smooth(p + vec3f(0, 0, step)) -
                                         noise.noise_smooth(p - vec3f(0, 0, step)));

        hit_record shaded = record;
        const vec3f tilted =
            unit_vector(record.normal - (gradient - dot(gradient, record.normal) * record.normal));
        if (dot(tilted, incoming.direction()) < 0.0f) // keep facing the ray
        {
            shaded.normal = tilted;
        }
        return shaded;
    }
};

/*
 * A scattering medium whose scattering favors one direction (mist, clouds, smoke, steam):
 * g > 0 scatters mostly forward, which makes it glow when lit from behind. Use it as the phase
 * function of a constant_medium.
 */
class mist : public material
{
public:
    mist(const color& albedo, float g) : albedo_texture(make_shared<solid_color>(albedo)), g(g) {}

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        scattering.attenuation = albedo_texture->value(record.u, record.v, record.point);
        scattering.direction_pdf =
            scattering.emplace<henyey_greenstein_pdf>(unit_vector(incoming.direction()), g);
        scattering.is_specular = false;
        return true;
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        return henyey_greenstein(
            dot(unit_vector(incoming.direction()), unit_vector(scattered.direction())), g);
    }

    bool needs_uv() const override { return false; }

private:
    shared_ptr<texture> albedo_texture;
    float g;
};

/*
 * A scattering medium that also glows: fire, embers, a lamp's haze. The light is emitted where
 * the medium scatters, so a denser or thicker medium glows brighter. Use it as the phase
 * function of a constant_medium.
 */
class glowing_medium : public isotropic
{
public:
    glowing_medium(const color& albedo, const color& emission)
        : isotropic(albedo), emission(emission)
    {
    }

    color emitted(const ray& incoming, const hit_record& record, float u, float v,
                  const point3f& point) const override
    {
        return emission;
    }

private:
    color emission;
};

/*
 * Frosted glass: glass whose surface is microscopically rough, so it scatters the light it
 * reflects and refracts. Each ray picks a GGX microfacet normal and reflects or refracts about
 * it, with Schlick's Fresnel choosing between the two. Roughness 0 is clear glass.
 */
class rough_dielectric : public material
{
public:
    // roughness in [0, 1]; the GGX width alpha is its square
    rough_dielectric(float refraction_index, float roughness)
        : refraction_index(refraction_index),
          alpha_squared(roughness * roughness * roughness * roughness)
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const vec3f view            = -unit_vector(incoming.direction());
        const float normal_dot_view = dot(view, record.normal);
        if (normal_dot_view <= 0.0f)
        {
            return false;
        }

        // A microfacet normal from the GGX distribution
        const float u = random_float();
        const float cos_theta =
            std::fmax(1e-4f, std::sqrt((1.0f - u) / (1.0f + (alpha_squared - 1.0f) * u)));
        const float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);
        const float phi       = 2.0f * PI * random_float();
        const vec3f half =
            ONB(record.normal)
                .transform(vec3f(sin_theta * std::cos(phi), sin_theta * std::sin(phi), cos_theta));

        const float view_dot_half = dot(view, half);
        if (view_dot_half <= 0.0f)
        {
            return false;
        }

        const float ratio        = record.front_face ? 1.0f / refraction_index : refraction_index;
        const float sin_view     = std::sqrt(1.0f - std::fmin(view_dot_half, 1.0f) * view_dot_half);
        float normal_reflectance = (1.0f - ratio) / (1.0f + ratio);
        normal_reflectance *= normal_reflectance;
        const float fresnel =
            normal_reflectance + (1.0f - normal_reflectance) * schlick_fifth_power(view_dot_half);

        const bool reflects   = ratio * sin_view > 1.0f || fresnel > random_float();
        const vec3f direction = reflects ? reflect(-view, half) : refract(-view, half, ratio);

        // The new direction must stay on the right side of the surface
        const float normal_dot_light = dot(direction, record.normal);
        if (reflects ? normal_dot_light <= 0.0f : normal_dot_light >= 0.0f)
        {
            return false;
        }

        // Importance sampling by the microfacet normal leaves only the shadowing-masking term
        const float weight = ggx_masking(normal_dot_view, alpha_squared) *
                             ggx_masking(std::fabs(normal_dot_light), alpha_squared) *
                             view_dot_half / (normal_dot_view * cos_theta);
        scattering.attenuation   = color(weight, weight, weight);
        scattering.direction_pdf = nullptr;
        scattering.is_specular   = true;
        scattering.specular_ray  = ray(record.point, direction, incoming.time());
        return true;
    }

    bool needs_uv() const override { return false; }

private:
    float refraction_index;
    float alpha_squared;
};

// The GGX distribution stretched along the tangent and bitangent, with widths alpha_t and
// alpha_b. The arguments are the half vector's components along tangent, bitangent and normal.
inline float anisotropic_ggx_distribution(float half_t, float half_b, float half_n, float alpha_t,
                                          float alpha_b)
{
    const float sum = half_t * half_t / (alpha_t * alpha_t) +
                      half_b * half_b / (alpha_b * alpha_b) + half_n * half_n;
    return 1.0f / (PI * alpha_t * alpha_b * sum * sum);
}

// Smith masking for that distribution, for a direction with the given components
inline float anisotropic_ggx_masking(float direction_t, float direction_b, float direction_n,
                                     float alpha_t, float alpha_b)
{
    const float lambda = 0.5f * (std::sqrt(1.0f + (alpha_t * alpha_t * direction_t * direction_t +
                                                   alpha_b * alpha_b * direction_b * direction_b) /
                                                      (direction_n * direction_n)) -
                                 1.0f);
    return 1.0f / (1.0f + lambda);
}

// A surface frame for anisotropic materials, and the GGX sampling that goes with it
struct anisotropic_frame
{
    vec3f tangent, bitangent, normal;

    // The tangent runs around `axis`, as the grooves of a turned and brushed object do
    anisotropic_frame(const vec3f& normal, const vec3f& axis) : normal(normal)
    {
        vec3f around = cross(axis, normal);
        if (around.length_squared() < 1e-6f) // at the poles of the axis
        {
            around = cross(std::fabs(normal.x()) < 0.9f ? vec3f(1, 0, 0) : vec3f(0, 1, 0), normal);
        }
        tangent   = unit_vector(around);
        bitangent = cross(normal, tangent);
    }
};

/*
 * The directions a brushed conductor reflects into: pick a microfacet normal from the
 * anisotropic GGX distribution, then mirror the view direction about it.
 */
class anisotropic_ggx_pdf : public pdf
{
public:
    anisotropic_ggx_pdf(const anisotropic_frame& frame, const vec3f& view, float alpha_t,
                        float alpha_b)
        : frame(frame), view(view), alpha_t(alpha_t), alpha_b(alpha_b)
    {
    }

    float value(const vec3f& direction) const override
    {
        const vec3f light = unit_vector(direction);
        if (dot(light, frame.normal) <= 0.0f)
        {
            return 0.0f;
        }
        const vec3f half            = unit_vector(light + view);
        const float normal_dot_half = dot(half, frame.normal);
        const float view_dot_half   = dot(view, half);
        if (normal_dot_half <= 0.0f || view_dot_half <= 0.0f)
        {
            return 0.0f;
        }
        return anisotropic_ggx_distribution(dot(half, frame.tangent), dot(half, frame.bitangent),
                                            normal_dot_half, alpha_t, alpha_b) *
               normal_dot_half / (4.0f * view_dot_half);
    }

    vec3f generate() const override
    {
        const float angle   = 2.0f * PI * random_float();
        const float phi     = std::atan2(alpha_b * std::sin(angle), alpha_t * std::cos(angle));
        const float cos_phi = std::cos(phi), sin_phi = std::sin(phi);

        const float u = random_float();
        const float spread =
            cos_phi * cos_phi / (alpha_t * alpha_t) + sin_phi * sin_phi / (alpha_b * alpha_b);
        const float cos_theta = 1.0f / std::sqrt(1.0f + u / ((1.0f - u) * spread));
        const float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);

        const vec3f half = sin_theta * cos_phi * frame.tangent +
                           sin_theta * sin_phi * frame.bitangent + cos_theta * frame.normal;
        return reflect(-view, half);
    }

private:
    anisotropic_frame frame;
    vec3f view;
    float alpha_t, alpha_b;
};

/*
 * Brushed metal: a conductor whose roughness depends on the direction, like steel or aluminum
 * with fine parallel grooves. Highlights stretch across the grooves into long streaks and bands.
 * The grooves run around `axis`, so with the axis along a turned object they are rings.
 */
class brushed_metal : public material
{
public:
    // Roughness (as for conductor) along the grooves, which is small, and across them, which is
    // larger and sets how long the streaks are
    brushed_metal(const color& reflectance, float roughness_along, float roughness_across,
                  const vec3f& axis = vec3f(0, 1, 0))
        : reflectance(reflectance), alpha_t(std::fmax(0.02f, roughness_along * roughness_along)),
          alpha_b(std::fmax(0.02f, roughness_across * roughness_across)), axis(unit_vector(axis))
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const vec3f view   = -unit_vector(incoming.direction());
        const float cosine = dot(view, record.normal);
        if (cosine <= 0.0f)
        {
            return false;
        }

        const float edge         = schlick_fifth_power(cosine);
        scattering.attenuation   = reflectance + edge * (color(1.0f, 1.0f, 1.0f) - reflectance);
        scattering.is_specular   = false;
        scattering.direction_pdf = scattering.emplace<anisotropic_ggx_pdf>(
            anisotropic_frame(record.normal, axis), view, alpha_t, alpha_b);
        return true;
    }

    // The microfacet BRDF times the cosine, without the Fresnel color (that is the attenuation)
    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        const vec3f view             = -unit_vector(incoming.direction());
        const vec3f light            = unit_vector(scattered.direction());
        const float normal_dot_view  = dot(record.normal, view);
        const float normal_dot_light = dot(record.normal, light);
        if (normal_dot_view <= 0.0f || normal_dot_light <= 0.0f)
        {
            return 0.0f;
        }
        const anisotropic_frame frame(record.normal, axis);
        const vec3f half = unit_vector(light + view);
        return anisotropic_ggx_distribution(dot(half, frame.tangent), dot(half, frame.bitangent),
                                            dot(half, frame.normal), alpha_t, alpha_b) *
               anisotropic_ggx_masking(dot(light, frame.tangent), dot(light, frame.bitangent),
                                       normal_dot_light, alpha_t, alpha_b) *
               anisotropic_ggx_masking(dot(view, frame.tangent), dot(view, frame.bitangent),
                                       normal_dot_view, alpha_t, alpha_b) /
               (4.0f * normal_dot_view);
    }

    bool needs_uv() const override { return false; }

private:
    color reflectance;
    float alpha_t, alpha_b;
    vec3f axis;
};

/*
 * Rough diffuse (Oren-Nayar): a surface made of many tiny facets, like clay, plaster, sand or the
 * moon. Unlike a Lambertian one it looks flat and bright when lit from the viewer's side, and
 * brightens toward its edges, since the facets facing the light are seen from any angle.
 */
class oren_nayar : public material
{
public:
    // sigma is the facets' angular spread in radians: 0 is Lambertian, 0.5 to 1 is very rough
    oren_nayar(const color& albedo, float sigma)
        : oren_nayar(make_shared<solid_color>(albedo), sigma)
    {
    }

    oren_nayar(shared_ptr<texture> albedo_texture, float sigma)
        : albedo_texture(albedo_texture), a(1.0f - 0.5f * sigma * sigma / (sigma * sigma + 0.33f)),
          b(0.45f * sigma * sigma / (sigma * sigma + 0.09f))
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        scattering.attenuation   = albedo_texture->value(record.u, record.v, record.point);
        scattering.cosine        = cosine_pdf(record.normal);
        scattering.direction_pdf = &scattering.cosine;
        scattering.is_specular   = false;
        return true;
    }

    // The BRDF times the cosine, divided by the albedo
    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        const vec3f view             = -unit_vector(incoming.direction());
        const vec3f light            = unit_vector(scattered.direction());
        const float normal_dot_light = dot(record.normal, light);
        const float normal_dot_view  = dot(record.normal, view);
        if (normal_dot_light <= 0.0f || normal_dot_view <= 0.0f)
        {
            return 0.0f;
        }

        // How much the two directions agree around the normal, from their parts in the surface
        const float light_sine_squared =
            std::fmax(0.0f, 1.0f - normal_dot_light * normal_dot_light);
        const float view_sine_squared = std::fmax(0.0f, 1.0f - normal_dot_view * normal_dot_view);
        float azimuth_cosine          = 0.0f;
        if (light_sine_squared > 1e-6f && view_sine_squared > 1e-6f)
        {
            azimuth_cosine =
                std::fmax(0.0f, (dot(light, view) - normal_dot_light * normal_dot_view) /
                                    std::sqrt(light_sine_squared * view_sine_squared));
        }

        // sin(max angle) * tan(min angle)
        const float larger_cosine  = std::fmax(normal_dot_light, normal_dot_view);
        const float smaller_cosine = std::fmin(normal_dot_light, normal_dot_view);
        const float sine_tangent   = std::sqrt(1.0f - smaller_cosine * smaller_cosine) *
                                   std::sqrt(1.0f - larger_cosine * larger_cosine) / larger_cosine;

        return normal_dot_light / PI * (a + b * azimuth_cosine * sine_tangent);
    }

    bool needs_uv() const override { return albedo_texture->needs_uv(); }

private:
    shared_ptr<texture> albedo_texture;
    float a, b;
};

} // namespace rt
