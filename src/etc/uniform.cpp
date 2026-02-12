#include "etc/uniform.h"

#include <stdexcept>

namespace RenderThing {
    void Uniform::CreateBuffers(uint32_t count, const ApiContext& a_ctx) {
        VkDeviceSize size = sizeof(UniformBufferObject);
        uniform_buffers.resize(count);

        for (uint32_t i = 0; i < count; i++) {
            BufferCreateInfo create_info = {
                .size = size,
                .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            };

            uniform_buffers[i] = std::make_unique<Buffer>(create_info, a_ctx);
            uniform_buffers[i]->Map();
        }
    }

    void Uniform::CreateDescriptors(uint32_t count, const UniformCreateInfo& create_info) {
        std::vector<VkDescriptorSetLayout> layouts(count, create_info.layout);
        VkDescriptorSetAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = create_info.pool,
            .descriptorSetCount = count,
            .pSetLayouts = layouts.data()
        };

        descriptor_sets.resize(count);
        if (vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor sets!");
        }

        for (uint32_t i = 0; i < count; i++) {
            VkDescriptorBufferInfo buffer_info = {
                .buffer = uniform_buffers[i]->get_buffer(),
                .offset = 0,
                .range = sizeof(UniformBufferObject)
            };

            VkDescriptorImageInfo image_info = {
                .sampler = create_info.sampler,
                .imageView = create_info.image_view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            std::array<VkWriteDescriptorSet, 2> writes = {
                (VkWriteDescriptorSet) {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = descriptor_sets[i],
                    .dstBinding = 0,
                    //! descriptors can also be arrays! come back to here when
                    //!   you are using instanced rendering for the boids
                    //! (this is the starting index to write to)
                    // TODO: change this later as WELL so we can pass in a ton of data
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pImageInfo = nullptr,
                    .pBufferInfo = &buffer_info,
                    .pTexelBufferView = nullptr
                },
                (VkWriteDescriptorSet) {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = descriptor_sets[i],
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &image_info,
                    .pBufferInfo = nullptr,
                    .pTexelBufferView = nullptr
                }
            };

            vkUpdateDescriptorSets(
                device,
                static_cast<uint32_t>(writes.size()),
                writes.data(),
                0,
                nullptr
            );
        }
    }

    Uniform::Uniform(
        const UniformCreateInfo& create_info,
        const ApiContext& a_ctx
    ) : device(a_ctx.device),
        frame_flight_count(create_info.frame_flight_count),
        frame_flight_index(0) {
        CreateBuffers(create_info.frame_flight_count, a_ctx);
        CreateDescriptors(create_info.frame_flight_count, create_info);
    }

    Uniform::~Uniform() {
        uniform_buffers.clear();
        descriptor_sets.clear();
    }

    void Uniform::CopyData(const UniformBufferObject& data) {
        uniform_buffers[frame_flight_index]->CopyFromHost(
            &data,
            sizeof(UniformBufferObject)
        );
    }

    void Uniform::NextIndex() {
        frame_flight_index = (frame_flight_index + 1) % frame_flight_count;
    }

    VkDescriptorSet Uniform::get_descriptor_set() {
        return descriptor_sets[frame_flight_index];
    }
}
