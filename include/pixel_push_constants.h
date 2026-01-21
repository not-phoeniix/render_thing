#pragma once

#include "glm_settings.h"
#include <glm/glm.hpp>

struct PixelPushConstants {
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 ambient;
};
