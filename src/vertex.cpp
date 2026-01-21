#include "vertex.h"

namespace RenderThing {
    // describes how to input the data (size and per-vertex data vs per-instance data)
    VkVertexInputBindingDescription vertex_get_binding_desc() {
        VkVertexInputBindingDescription desc = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };

        return desc;
    }

    // describes all members of vertex struct <3
    std::array<VkVertexInputAttributeDescription, 3> vertex_get_attribute_descs() {
        std::array<VkVertexInputAttributeDescription, 3> descs = {
            (VkVertexInputAttributeDescription) {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, pos)
            },
            (VkVertexInputAttributeDescription) {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, normal)
            },
            (VkVertexInputAttributeDescription) {
                .location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, uv)
            },
        };

        return descs;
    }
}
