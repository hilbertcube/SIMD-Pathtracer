#pragma once

#include "rt/geometry/hittable.h"
#include "rt/materials/texture.h"
#include "rt/materials/pdf.h"
#include "rt/math/vector.h"

namespace rt
{

/**
 * @brief What a material reports when a ray scatters off it.
 * The pdf lives inside the record (no heap allocation per bounce); direction_pdf points
 * at one of the members, so the record must not be copied.
 */
class scatter_record
{
public:
    color attenuation; // Color reduction factor (albedo)
    const pdf* direction_pdf =
        nullptr;              // Distribution of scattered directions (diffuse-like materials)
    bool is_specular = false; // true for specular materials: use specular_ray directly
    ray specular_ray;         // The scattered ray when is_specular is true

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
     * @param incoming Incoming ray.
     * @param record Surface hit record (point, normal, uv, etc.).
     * @param scattering Output: attenuation, plus either a pdf to sample the scattered
     *             direction from, or (is_specular) a fixed specular ray.
     * @return True if scattered, false if absorbed.
     */
    virtual bool scatter(const ray& incoming, const hit_record& record,
                         scatter_record& scattering) const
    {
        return false;
    }

    /**
     * @brief Computes emitted light from the material.
     * @param incoming Incoming ray.
     * @param record Surface hit record (front_face decides which side emits).
     * @param u Surface u texture coordinate.
     * @param v Surface v texture coordinate.
     * @param point Hit point position in world space.
     * @return The emitted color (default: black).
     */
    virtual color emitted(const ray& incoming, const hit_record& record, float u, float v,
                          const point3f& point) const
    {
        return color(0, 0, 0);
    }

    /// @brief Whether scatter() or emitted() read the hit's (u, v) texture coordinates.
    /// Objects can skip computing them when not; the default is the safe answer.
    virtual bool needs_uv() const { return true; }

    /// @brief The material's true scattering density for `scattered` (BRDF * cosine).
    virtual float scattering_pdf(const ray& incoming, const hit_record& record,
                                 const ray& scattered) const
    {
        return 0.0f;
    }
};

class lambertian : public material
{
private:
    // Albedo refers to how much light a surface reflects
    shared_ptr<texture> albedo_texture;

public:
    // Construct from a solid color (albedo = base reflectivity)
    lambertian(const color& albedo) : albedo_texture(make_shared<solid_color>(albedo)) {}

    // Construct from an arbitrary texture (e.g. checkerboard, image)
    lambertian(shared_ptr<texture> albedo_texture) : albedo_texture(albedo_texture) {}

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        scattering.attenuation   = albedo_texture->value(record.u, record.v, record.point);
        scattering.cosine        = cosine_pdf(record.normal);
        scattering.direction_pdf = &scattering.cosine;
        scattering.is_specular   = false;
        return true;
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        // cos(theta) without building the normalized vector
        float projection = dot(record.normal, scattered.direction());
        if (projection <= 0.0f)
        {
            return 0.0f;
        }
        return projection / (scattered.direction().length() * PI);
    }

    bool needs_uv() const override { return albedo_texture->needs_uv(); }
};

class metal : public material
{
private:
    color albedo;
    float fuzz;

public:
    metal(const color& albedo, float fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        vec3f reflected = reflect(incoming.direction(), record.normal);
        reflected       = unit_vector(reflected) + (fuzz * random_unit_vector<float, 3>());

        scattering.attenuation   = albedo;
        scattering.direction_pdf = nullptr;
        scattering.is_specular   = true;
        scattering.specular_ray  = ray(record.point, reflected, incoming.time());

        // Absorb fuzzed reflections that end up below the surface
        return dot(reflected, record.normal) > 0;
    }

    bool needs_uv() const override { return false; }
};

class dielectrics : public material
{
private:
    float refraction_index;

    static float reflectance(float cosine, float refraction_index)
    {
        // Use Schlick's approximation for reflectance
        auto normal_reflectance = (1.0f - refraction_index) / (1 + refraction_index);
        normal_reflectance *= normal_reflectance;
        return normal_reflectance + (1.0f - normal_reflectance) * std::pow((1 - cosine), 5);
    }

public:
    dielectrics(float refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        scattering.attenuation   = color(1.0f, 1.0f, 1.0f);
        scattering.direction_pdf = nullptr;
        scattering.is_specular   = true;
        float refraction_ratio   = record.front_face ? (1.0f / refraction_index) : refraction_index;

        vec3f unit_direction = unit_vector(incoming.direction());
        float cos_theta      = std::fmin(dot(-unit_direction, record.normal), 1.0f);
        float sin_theta      = std::sqrt(1.0f - cos_theta * cos_theta);

        bool cannot_refract = (refraction_ratio * sin_theta > 1.0f);
        vec3f direction;

        direction = (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_float())
                        ? reflect(unit_direction, record.normal)
                        : refract(unit_direction, record.normal, refraction_ratio);

        scattering.specular_ray = ray(record.point, direction, incoming.time());
        return true;
    }

    bool needs_uv() const override { return false; }
};

class diffuse_light : public material
{
public:
    diffuse_light(shared_ptr<texture> emission_texture) : emission_texture(emission_texture) {}

    diffuse_light(const color& emission) : emission_texture(make_shared<solid_color>(emission)) {}

    // One-sided: only the front face (the side the surface normal points to) emits.
    color emitted(const ray& incoming, const hit_record& record, float u, float v,
                  const point3f& point) const override
    {
        if (!record.front_face)
        {
            return color(0, 0, 0);
        }
        return emission_texture->value(u, v, point);
    }

    bool needs_uv() const override { return emission_texture->needs_uv(); }

private:
    shared_ptr<texture> emission_texture;
};

class isotropic : public material
{
public:
    isotropic(const color& albedo) : albedo_texture(make_shared<solid_color>(albedo)) {}

    isotropic(shared_ptr<texture> albedo_texture) : albedo_texture(albedo_texture) {}

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        scattering.attenuation   = albedo_texture->value(record.u, record.v, record.point);
        scattering.direction_pdf = &scattering.sphere;
        scattering.is_specular   = false;
        return true;
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        return 1.0f / (4.0f * PI);
    }

    bool needs_uv() const override { return albedo_texture->needs_uv(); }

private:
    shared_ptr<texture> albedo_texture;
};

} // namespace rt
