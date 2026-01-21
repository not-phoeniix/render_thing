#pragma once

#include "glm_settings.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <array>

namespace RenderThing {
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    VkVertexInputBindingDescription vertex_get_binding_desc();
    std::array<VkVertexInputAttributeDescription, 3> vertex_get_attribute_descs();
}
