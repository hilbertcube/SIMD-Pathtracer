#pragma once

#include <cmath>
#include <memory>

// Macros are not scoped by namespaces, so this one stays global
#define FORCE_INLINE inline __attribute__((always_inline))

namespace rt
{

using std::floor;
using std::make_shared;
using std::shared_ptr;
using std::sqrt;
using std::unique_ptr;
using std::weak_ptr;

} // namespace rt
