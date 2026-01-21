#pragma once

#include <vulkan/vulkan.h>
#include "vertex.h"
#include "buffer_wrapper.h"
#include <memory>
#include "graphics_context.h"

struct MeshCreateInfo {
    const Vertex* vertices;
    uint32_t num_vertices;
    const uint32_t* indices;
    uint32_t num_indices;
};

struct Mesh {
   private:
    VkDevice device;

    std::unique_ptr<BufferWrapper> vertex_buffer;
    std::unique_ptr<BufferWrapper> index_buffer;

    uint32_t num_vertices;
    uint32_t num_indices;

   public:
    Mesh(const MeshCreateInfo& create_info, const GraphicsContext& ctx);
    ~Mesh() = default;

    VkBuffer get_vertex_buffer() const { return vertex_buffer->get_buffer(); }
    VkBuffer get_index_buffer() const { return index_buffer->get_buffer(); }
    uint32_t get_num_vertices() const { return num_vertices; }
    uint32_t get_num_indices() const { return num_indices; }
};
