#pragma once

#include "hittable.h"
#include "texture.h"
#include "pdf.h"
#include "rtm/vector.h"

namespace rt
{

/**
 * @brief What a material reports when a ray scatters off it.
 * The pdf lives inside the record (no heap allocation per bounce); pdf_ptr points
 * at one of the members, so the record must not be copied.
 */
class scatter_record
{
public:
    color attenuation;            // Color reduction factor (albedo)
    const pdf* pdf_ptr = nullptr; // Distribution of scattered directions (diffuse-like materials)
    bool skip_pdf      = false;   // true for specular materials: use skip_pdf_ray directly
    ray skip_pdf_ray;             // The scattered ray when skip_pdf is true

    // Storage for the pdfs materials can pick from
    cosine_pdf cosine;
    sphere_pdf sphere;

    scatter_record()                                 = default;
    scatter_record(const scatter_record&)            = delete;
    scatter_record& operator=(const scatter_record&) = delete;
};

class material
{
public:
    material()          = default;
    virtual ~material() = default;

    /**
     * @brief Computes how an incoming ray interacts with the material.
     * @param r_in Incoming ray.
     * @param rec Surface hit record (point, normal, uv, etc.).
     * @param srec Output: attenuation, plus either a pdf to sample the scattered
     *             direction from, or (skip_pdf) a fixed specular ray.
     * @return True if scattered, false if absorbed.
     */
    virtual bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const
    {
        return false;
    }

    /**
     * @brief Computes emitted light from the material.
     * @param r_in Incoming ray.
     * @param rec Surface hit record (front_face decides which side emits).
     * @param u Surface u texture coordinate.
     * @param v Surface v texture coordinate.
     * @param p Hit point position in world space.
     * @return The emitted color (default: black).
     */
    virtual color emitted(const ray& r_in, const hit_record& rec, float u, float v,
                          const point3f& p) const
    {
        return color(0, 0, 0);
    }

    /// @brief The material's true scattering density for `scattered` (BRDF * cosine).
    virtual float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const
    {
        return 0.0f;
    }
};

class lambertian : public material
{
private:
    // Albedo refers to how much light a surface reflects
    shared_ptr<texture> tex;

public:
    // Construct from a solid color (albedo = base reflectivity)
    lambertian(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}

    // Construct from an arbitrary texture (e.g. checkerboard, image)
    lambertian(shared_ptr<texture> tex) : tex(tex) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override
    {
        srec.attenuation = tex->value(rec.u, rec.v, rec.p);
        srec.cosine      = cosine_pdf(rec.normal);
        srec.pdf_ptr     = &srec.cosine;
        srec.skip_pdf    = false;
        return true;
    }

    float scattering_pdf(const ray& r_in, const hit_record& rec,
                         const ray& scattered) const override
    {
        float cosine = dot(rec.normal, unit_vector(scattered.direction()));
        return cosine < 0 ? 0.0f : cosine / PI;
    }
};

class metal : public material
{
private:
    color albedo;
    float fuzz;

public:
    metal(const color& albedo, float fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override
    {
        vec3f reflected = reflect(r_in.direction(), rec.normal);
        reflected       = unit_vector(reflected) + (fuzz * random_unit_vector<float, 3>());

        srec.attenuation  = albedo;
        srec.pdf_ptr      = nullptr;
        srec.skip_pdf     = true;
        srec.skip_pdf_ray = ray(rec.p, reflected, r_in.time());

        // Absorb fuzzed reflections that end up below the surface
        return dot(reflected, rec.normal) > 0;
    }
};

class dielectrics : public material
{
private:
    float refraction_index;

    static float reflectance(float cosine, float refraction_index)
    {
        // Use Schlick's approximation for reflectance
        auto r0 = (1.0f - refraction_index) / (1 + refraction_index);
        r0 *= r0;
        return r0 + (1.0f - r0) * std::pow((1 - cosine), 5);
    }

public:
    dielectrics(float refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override
    {
        srec.attenuation = color(1.0f, 1.0f, 1.0f);
        srec.pdf_ptr     = nullptr;
        srec.skip_pdf    = true;
        float ri         = rec.front_face ? (1.0f / refraction_index) : refraction_index;

        vec3f unit_direction = unit_vector(r_in.direction());
        float cos_theta      = std::fmin(dot(-unit_direction, rec.normal), 1.0f);
        float sin_theta      = std::sqrt(1.0f - cos_theta * cos_theta);

        bool cannot_refract = (ri * sin_theta > 1.0f);
        vec3f direction;

        direction = (cannot_refract || reflectance(cos_theta, ri) > random_float())
                        ? reflect(unit_direction, rec.normal)
                        : refract(unit_direction, rec.normal, ri);

        srec.skip_pdf_ray = ray(rec.p, direction, r_in.time());
        return true;
    }
};

class diffuse_light : public material
{
public:
    diffuse_light(shared_ptr<texture> tex) : tex(tex) {}

    diffuse_light(const color& emit) : tex(make_shared<solid_color>(emit)) {}

    // One-sided: only the front face (the side the surface normal points to) emits.
    color emitted(const ray& r_in, const hit_record& rec, float u, float v,
                  const point3f& p) const override
    {
        if (!rec.front_face)
            return color(0, 0, 0);
        return tex->value(u, v, p);
    }

private:
    shared_ptr<texture> tex;
};

class isotropic : public material
{
public:
    isotropic(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}

    isotropic(shared_ptr<texture> tex) : tex(tex) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override
    {
        srec.attenuation = tex->value(rec.u, rec.v, rec.p);
        srec.pdf_ptr     = &srec.sphere;
        srec.skip_pdf    = false;
        return true;
    }

    float scattering_pdf(const ray& r_in, const hit_record& rec,
                         const ray& scattered) const override
    {
        return 1.0f / (4.0f * PI);
    }

private:
    shared_ptr<texture> tex;
};

} // namespace rt
