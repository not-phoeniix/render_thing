#include "ring_buffer.h"
#include <array>
#include <stdexcept>

namespace RenderThing {
    RingBuffer::RingBuffer(const RingBufferCreateInfo& create_info, const GraphicsContext& ctx)
      : device(ctx.device),
        descriptor_type(create_info.descriptor_type),
        max_elements(create_info.max_elements),
        buffer_offset(0),
        descriptor_index(0) {
        // ~~~ create buffer ~~~
        size_t element_size = (create_info.element_size + 255) / 256 * 256;
        BufferCreateInfo buffer_info = {
            .size = element_size * static_cast<size_t>(create_info.max_elements),
            .usage = create_info.usage,
            .properties = create_info.properties,
        };
        buffer = std::make_unique<Buffer>(buffer_info, ctx);

        // ~~~ create pool ~~~
        VkDescriptorPoolSize pool_size = {
            .type = create_info.descriptor_type,
            .descriptorCount = 1 // number of descriptors accessed in an array?
        };
        DescriptorPoolCreateInfo pool_info = {
            .max_sets = create_info.max_elements,
            .flags = 0,
            .pool_sizes = &pool_size,
            .pool_size_count = 1
        };
        pool = std::make_unique<DescriptorPool>(pool_info, ctx);

        // ~~~ allocate descriptors ~~~
        descriptor_sets.resize(max_elements);

        // create a vector of duplicate layouts for all our identical descriptors
        std::vector<VkDescriptorSetLayout> layouts(create_info.max_elements, create_info.layout);
        VkDescriptorSetAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = pool->get_pool(),
            .descriptorSetCount = create_info.max_elements,
            .pSetLayouts = layouts.data()
        };
        if (vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate ring buffer descriptor sets!");
        }
    }

    RingBuffer::~RingBuffer() { }

    VkDescriptorSet RingBuffer::CopyToNextRegion(void* data, size_t size) {
        //~~~ map and copy data into buffer ~~~

        // size needs to be a multiple of 256 for alignment
        size_t reserve_size = (size + 255) / 256 * 256;

        if (buffer_offset + (uint64_t)reserve_size >= (uint64_t)buffer->get_size()) {
            buffer_offset = 0;
        }

        buffer->Map(buffer_offset, size);
        buffer->CopyFromHost(data, size);
        buffer->Unmap();

        // ~~~ write new offset info to descriptor ~~~

        VkDescriptorSet descriptor = descriptor_sets[descriptor_index];
        VkDescriptorBufferInfo buffer_info = {
            .buffer = buffer->get_buffer(),
            .offset = static_cast<VkDeviceSize>(buffer_offset),
            .range = static_cast<VkDeviceSize>(size)
        };

        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = descriptor_type,
            .pImageInfo = nullptr,
            .pBufferInfo = &buffer_info,
            .pTexelBufferView = nullptr,
        };

        vkUpdateDescriptorSets(
            device,
            1,
            &write,
            0,
            nullptr
        );

        // ~~~ update offsets & indices ~~~
        buffer_offset += (uint64_t)reserve_size;
        descriptor_index++;
        descriptor_index %= max_elements;

        return descriptor;
    }
}
