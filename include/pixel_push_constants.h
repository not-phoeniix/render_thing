#pragma once

#include "glm_settings.h"
#include <glm/glm.hpp>

namespace RenderThing {
    struct PixelPushConstants {
        alignas(16) glm::vec3 color;
        alignas(16) glm::vec3 ambient;
    };
}
