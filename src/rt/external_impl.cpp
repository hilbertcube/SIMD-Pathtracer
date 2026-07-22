// stb and tinyobjloader are single-header libraries: exactly one source file defines their
// IMPLEMENTATION macro to compile their code, and every other file only sees the declarations.

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "external/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb/stb_image_write.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"
