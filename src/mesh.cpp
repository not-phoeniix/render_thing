#include "mesh.h"

Mesh::Mesh(const MeshCreateInfo& create_info, const GraphicsContext& ctx)
  : device(ctx.device),
    num_vertices(create_info.num_vertices),
    num_indices(create_info.num_indices) {

    // vertex buffer
    {
        // intermediary buffer so we don't have to always be using a
        //  buffer that is accessible by CPU host (faster that way)
        BufferWrapperCreateInfo vert_create_info = {
            .size = sizeof(Vertex) * create_info.num_vertices,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        BufferWrapper staging_buffer(vert_create_info, ctx);
        staging_buffer.CopyFromHostAuto(create_info.vertices, sizeof(Vertex) * create_info.num_vertices);

        // actual vertex buffer! can't be accessed directly from CPU
        vert_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        vert_create_info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        vertex_buffer = std::make_unique<BufferWrapper>(vert_create_info, ctx);

        vertex_buffer->CopyFromBuffer(staging_buffer, ctx);
    }

    // index buffer
    {
        // intermediary buffer so we don't have to always be using a
        //  buffer that is accessible by CPU host (faster that way)
        BufferWrapperCreateInfo ind_create_info = {
            .size = sizeof(uint32_t) * create_info.num_indices,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        BufferWrapper staging_buffer(ind_create_info, ctx);
        staging_buffer.CopyFromHostAuto(create_info.indices, sizeof(uint32_t) * create_info.num_indices);

        // actual index buffer! can't be accessed directly from CPU
        ind_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        ind_create_info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        index_buffer = std::make_unique<BufferWrapper>(ind_create_info, ctx);

        index_buffer->CopyFromBuffer(staging_buffer, ctx);
    }
}
