#include "base/descriptor_pool.h"

#include <stdexcept>

namespace RenderThing {
    DescriptorPool::DescriptorPool(const DescriptorPoolCreateInfo& create_info, const ApiContext& a_ctx)
      : device(a_ctx.device) {
        VkDescriptorPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = create_info.flags,
            .maxSets = create_info.max_sets,
            .poolSizeCount = create_info.pool_size_count,
            .pPoolSizes = create_info.pool_sizes,
        };

        VkResult result = vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create descriptor pool! result: " +
                std::to_string(static_cast<int32_t>(result))
            );
        }
    }

    DescriptorPool::~DescriptorPool() {
        vkDeviceWaitIdle(device);

        vkDestroyDescriptorPool(device, pool, nullptr);
    }

    VkDescriptorPool DescriptorPool::get_pool() { return pool; }
}
