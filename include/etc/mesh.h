#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include "../base/base.h"

namespace RenderThing {
    struct MeshCreateInfo {
        const void* vertices;
        size_t vertex_size;
        uint32_t num_vertices;
        const void* indices;
        size_t index_size;
        uint32_t num_indices;
    };

    struct Mesh {
       private:
        VkDevice device;

        std::unique_ptr<Buffer> vertex_buffer;
        std::unique_ptr<Buffer> index_buffer;

        uint32_t num_vertices;
        uint32_t num_indices;

       public:
        Mesh(const MeshCreateInfo& create_info, const GraphicsContext& g_ctx, const ApiContext& a_ctx);
        ~Mesh();

        VkBuffer get_vertex_buffer() const;
        VkBuffer get_index_buffer() const;
        uint32_t get_num_vertices() const;
        uint32_t get_num_indices() const;
    };
}
