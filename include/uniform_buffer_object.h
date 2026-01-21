#pragma once

#include "glm_settings.h"
#include <glm/glm.hpp>

namespace RenderThing {
    struct UniformBufferObject {
        glm::mat4x4 world;
    };
}
