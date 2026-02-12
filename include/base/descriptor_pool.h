#pragma once

#include <vulkan/vulkan.h>
#include "context_structs.h"

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
        DescriptorPool(const DescriptorPoolCreateInfo& create_info, const ApiContext& a_ctx);
        ~DescriptorPool();

        VkDescriptorPool get_pool();
    };
}
