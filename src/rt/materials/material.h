#pragma once

#include <cstddef>
#include <new>
#include <utility>

#include "rt/geometry/hittable.h"
#include "rt/materials/textures/texture.h"
#include "rt/materials/common/pdf.h"
#include "rt/math/vector.h"

namespace rt
{

// Result of scattering. direction_pdf points at one of the members below (no heap allocation
// per bounce), so a record must not be copied.
class scatter_record
{
public:
    color attenuation;
    const pdf* direction_pdf = nullptr; // null when is_specular
    bool is_specular         = false;
    ray specular_ray; // used when is_specular

    // Storage for the pdfs materials can pick from
    cosine_pdf cosine;
    sphere_pdf sphere;

    /*
     * Space for the less common pdfs (rough metals, anisotropic media). It is left uninitialized,
     * so it costs nothing on the common paths: a material that needs one builds it here with
     * emplace and points direction_pdf at the result.
     */
    template <typename Pdf, typename... Arguments>
    const Pdf* emplace(Arguments&&... arguments)
    {
        static_assert(sizeof(Pdf) <= sizeof(rare_pdf_storage), "grow rare_pdf_storage");
        static_assert(alignof(Pdf) <= alignof(std::max_align_t), "pdf is over-aligned");
        return new (rare_pdf_storage) Pdf(std::forward<Arguments>(arguments)...);
    }

    scatter_record()                                 = default;
    scatter_record(const scatter_record&)            = delete;
    scatter_record& operator=(const scatter_record&) = delete;

private:
    alignas(std::max_align_t) unsigned char rare_pdf_storage[96];
};

class material
{
public:
    material()          = default;
    virtual ~material() = default;

    /*
     * Decides how `incoming` scatters at `record`: fills `scattering` with the attenuation and
     * either a pdf to sample the new direction from or (is_specular) a fixed ray. Returns false
     * if the ray is absorbed.
     */
    virtual bool scatter(const ray& incoming, const hit_record& record,
                         scatter_record& scattering) const
    {
        return false;
    }

    // Light the surface emits at the hit point; black by default.
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

} // namespace rt
