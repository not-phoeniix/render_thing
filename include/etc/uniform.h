#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>
#include "../base/base.h"
#include "../uniform_buffer_object.h"

namespace RenderThing {
    struct UniformCreateInfo {
        uint32_t frame_flight_count;
        VkDescriptorSetLayout layout;
        VkDescriptorPool pool;
        VkImageView image_view;
        VkSampler sampler;
    };

    class Uniform {
       private:
        VkDevice device;
        uint32_t frame_flight_count;
        uint32_t frame_flight_index;
        std::vector<VkDescriptorSet> descriptor_sets;
        std::vector<std::unique_ptr<Buffer>> uniform_buffers;

        void CreateBuffers(uint32_t count, const GraphicsContext& ctx);
        void CreateDescriptors(uint32_t count, const UniformCreateInfo& create_info);

       public:
        Uniform(const UniformCreateInfo& create_info, const GraphicsContext& ctx);
        ~Uniform();

        void NextIndex();
        void CopyData(const UniformBufferObject& data);

        VkDescriptorSet get_descriptor_set();
    };
}
