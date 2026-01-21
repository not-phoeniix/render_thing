#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
#include "buffer_wrapper.h"
#include "uniform_buffer_object.h"
#include <memory>
#include "graphics_context.h"

namespace RenderThing {
    struct UniformWrapperCreateInfo {
        uint32_t frame_flight_count;
        VkDescriptorSetLayout layout;
        VkDescriptorPool pool;
        VkImageView image_view;
        VkSampler sampler;
    };

    class UniformWrapper {
       private:
        VkDevice device;
        uint32_t frame_flight_count;
        uint32_t frame_flight_index;
        std::vector<VkDescriptorSet> descriptor_sets;
        std::vector<std::unique_ptr<BufferWrapper>> uniform_buffers;

        void CreateBuffers(uint32_t count, const GraphicsContext& ctx);
        void CreateDescriptors(uint32_t count, const UniformWrapperCreateInfo& create_info);

       public:
        UniformWrapper(const UniformWrapperCreateInfo& create_info, const GraphicsContext& ctx);
        ~UniformWrapper();

        void NextIndex() {
            frame_flight_index = (frame_flight_index + 1) % frame_flight_count;
        }

        void CopyData(const UniformBufferObject& data);
        VkDescriptorSet get_descriptor_set() { return descriptor_sets[frame_flight_index]; }
    };
}
