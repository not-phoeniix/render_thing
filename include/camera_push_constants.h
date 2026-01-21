#pragma once

#include "glm_settings.h"
#include <glm/glm.hpp>

namespace RenderThing {
    struct CameraPushConstants {
        glm::mat4x4 view;
        glm::mat4x4 proj;
    };
}
