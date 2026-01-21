#include "buffer_wrapper.h"
#include <stdexcept>
#include <cstring>
#include "vk_helpers.h"

BufferWrapper::BufferWrapper(const BufferWrapperCreateInfo& create_info, const GraphicsContext& ctx)
  : device(ctx.device),
    size(create_info.size),
    buffer_usage(create_info.usage),
    memory_properties(create_info.properties) {
    VkBufferCreateInfo buffer_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .flags = 0,
        .size = create_info.size,
        .usage = create_info.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateBuffer(ctx.device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(ctx.device, buffer, &mem_req);

    VkMemoryAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = find_memory_type(
            mem_req.memoryTypeBits,
            create_info.properties,
            ctx.physical_device
        )
    };

    if (vkAllocateMemory(ctx.device, &alloc_info, nullptr, &device_memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate GPU buffer memory!");
    }

    vkBindBufferMemory(ctx.device, buffer, device_memory, 0);
}

BufferWrapper::~BufferWrapper() {
    vkDeviceWaitIdle(device);

    Unmap();

    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, device_memory, nullptr);
}

void BufferWrapper::CopyFromHostAuto(const void* data, size_t size) {
    Map();
    CopyFromHost(data, size);
    Unmap();
}

void BufferWrapper::CopyFromHost(const void* data, size_t size) {
    if (
        (memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0 ||
        (memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0
    ) {
        throw std::runtime_error("Cannot copy data into a buffer whose memory properties don't include VK_MEMORY_PROPERTY_HOST_COHERENT_BIT!");
    }

    if (mapped == nullptr) {
        throw std::runtime_error("Failed to copy data, buffer was never mapped!");
    }

    memcpy(mapped, data, size);
}

void BufferWrapper::CopyFromBuffer(const BufferWrapper& src, const GraphicsContext& ctx) {
    if ((src.buffer_usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0) {
        throw std::runtime_error("Cannot copy data from a buffer whose usage doesn't include VK_BUFFER_USAGE_TRANSFER_SRC_BIT!");
    }

    if ((buffer_usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0) {
        throw std::runtime_error("Cannot copy data into a buffer whose usage doesn't include VK_BUFFER_USAGE_TRANSFER_DST_BIT!");
    }

    VkCommandBuffer command_buffer = begin_single_use_commands(ctx);

    VkDeviceSize copy_size = src.size;
    if (copy_size > size) copy_size = size;
    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = copy_size
    };
    vkCmdCopyBuffer(command_buffer, src.buffer, buffer, 1, &copy_region);

    end_single_use_commands(command_buffer, ctx);
}

void BufferWrapper::Map() {
    if (mapped == nullptr) {
        vkMapMemory(device, device_memory, 0, size, 0, &mapped);
    }
}

void BufferWrapper::Unmap() {
    if (mapped != nullptr) {
        vkUnmapMemory(device, device_memory);
        mapped = nullptr;
    }
}
