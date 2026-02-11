#pragma once

#include "../base/graphics_context.h"
#include "../base/base.h"
#include <memory>
#include <vector>

namespace RenderThing {
    struct RingBufferCreateInfo {
        size_t element_size;
        uint32_t max_elements;
        VkBufferUsageFlags usage;
        VkMemoryPropertyFlags properties;
        VkDescriptorType descriptor_type;
        VkDescriptorSetLayout layout;
    };

    class RingBuffer {
       private:
        VkDevice device;
        std::unique_ptr<Buffer> buffer;
        std::unique_ptr<DescriptorPool> pool;
        std::vector<VkDescriptorSet> descriptor_sets;

        VkDescriptorType descriptor_type;
        uint32_t max_elements;
        uint64_t buffer_offset;
        uint32_t descriptor_index;

       public:
        RingBuffer(const RingBufferCreateInfo& create_info, const GraphicsContext& ctx);
        ~RingBuffer();

        VkDescriptorSet CopyToNextRegion(void* data, size_t size);
    };
}
