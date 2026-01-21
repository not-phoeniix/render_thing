#pragma once

#include <vulkan/vulkan.h>
#include "graphics_context.h"

struct BufferWrapperCreateInfo {
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags properties;
};

class BufferWrapper {
   private:
    VkDevice device;
    VkBuffer buffer;
    void* mapped = nullptr;
    VkDeviceMemory device_memory;
    VkDeviceSize size;
    VkBufferUsageFlags buffer_usage;
    VkMemoryPropertyFlags memory_properties;

   public:
    BufferWrapper(const BufferWrapperCreateInfo& create_info, const GraphicsContext& ctx);
    ~BufferWrapper();

    // maps, copies, and unmaps memory automatically
    void CopyFromHostAuto(const void* data, size_t size);
    void CopyFromHost(const void* data, size_t size);
    void CopyFromBuffer(const BufferWrapper& src, const GraphicsContext& ctx);
    void Map();
    void Unmap();

    VkBuffer get_buffer() const {
        return buffer;
    }
    VkDeviceSize get_size() const { return size; }
    bool get_mapped() const { return (mapped != nullptr); }
};
