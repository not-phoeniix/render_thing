#pragma once

#include <vulkan/vulkan.h>
#include "graphics_context.h"

namespace RenderThing {
    struct BufferCreateInfo {
        VkDeviceSize size;
        VkBufferUsageFlags usage;
        VkMemoryPropertyFlags properties;
    };

    class Buffer {
       private:
        VkDevice device;
        VkBuffer buffer;
        void* mapped = nullptr;
        VkDeviceMemory device_memory;
        VkDeviceSize size;
        VkBufferUsageFlags buffer_usage;
        VkMemoryPropertyFlags memory_properties;

       public:
        Buffer(const BufferCreateInfo& create_info, const GraphicsContext& ctx);
        ~Buffer();

        // maps, copies, and unmaps memory automatically
        void CopyFromHostAuto(const void* data, size_t size);
        void CopyFromHost(const void* data, size_t size);
        void CopyFromBuffer(const Buffer& src, const GraphicsContext& ctx);
        void Map();
        void Unmap();

        VkBuffer get_buffer() const;
        VkDeviceSize get_size() const;
        bool get_mapped() const;
    };
}
