#include "etc/mesh.h"

namespace RenderThing {
    Mesh::Mesh(const MeshCreateInfo& create_info, const GraphicsContext& ctx)
      : device(ctx.device),
        num_vertices(create_info.num_vertices),
        num_indices(create_info.num_indices) {

        // vertex buffer
        {
            // intermediary buffer so we don't have to always be using a
            //  buffer that is accessible by CPU host (faster that way)
            BufferCreateInfo vert_create_info = {
                .size = create_info.vertex_size * create_info.num_vertices,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            };

            Buffer staging_buffer(vert_create_info, ctx);
            staging_buffer.CopyFromHostAuto(create_info.vertices, create_info.vertex_size * create_info.num_vertices);

            // actual vertex buffer! can't be accessed directly from CPU
            vert_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            vert_create_info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            vertex_buffer = std::make_unique<Buffer>(vert_create_info, ctx);

            vertex_buffer->CopyFromBuffer(staging_buffer, ctx);
        }

        // index buffer
        {
            // intermediary buffer so we don't have to always be using a
            //  buffer that is accessible by CPU host (faster that way)
            BufferCreateInfo ind_create_info = {
                .size = create_info.index_size * create_info.num_indices,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            };

            Buffer staging_buffer(ind_create_info, ctx);
            staging_buffer.CopyFromHostAuto(create_info.indices, create_info.index_size * create_info.num_indices);

            // actual index buffer! can't be accessed directly from CPU
            ind_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            ind_create_info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            index_buffer = std::make_unique<Buffer>(ind_create_info, ctx);

            index_buffer->CopyFromBuffer(staging_buffer, ctx);
        }
    }

    Mesh::~Mesh() {
    }

    VkBuffer Mesh::get_vertex_buffer() const { return vertex_buffer->get_buffer(); }
    VkBuffer Mesh::get_index_buffer() const { return index_buffer->get_buffer(); }
    uint32_t Mesh::get_num_vertices() const { return num_vertices; }
    uint32_t Mesh::get_num_indices() const { return num_indices; }
}
