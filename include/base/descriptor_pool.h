#pragma once

#include <vulkan/vulkan.h>
#include "graphics_context.h"

namespace RenderThing {
    struct DescriptorPoolCreateInfo {
        uint32_t max_sets;
        VkDescriptorPoolCreateFlags flags;
        const VkDescriptorPoolSize* pool_sizes;
        uint32_t pool_size_count;
    };

    class DescriptorPool {
       private:
        VkDevice device;
        VkDescriptorPool pool;

       public:
        DescriptorPool(const DescriptorPoolCreateInfo& create_info, const GraphicsContext& ctx);
        ~DescriptorPool();

        VkDescriptorPool get_pool();
    };
}
