#include "base/descriptor_set_layout.h"

#include <stdexcept>

namespace RenderThing {
    DescriptorSetLayout::DescriptorSetLayout(
        const DescriptorSetLayoutCreateInfo& create_info,
        const ApiContext& a_ctx
    ) : device(a_ctx.device) {
        VkDescriptorSetLayoutCreateInfo layout_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags = create_info.flags,
            .bindingCount = create_info.binding_count,
            .pBindings = create_info.bindings
        };

        if (vkCreateDescriptorSetLayout(a_ctx.device, &layout_info, nullptr, &layout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }
    }

    DescriptorSetLayout::~DescriptorSetLayout() {
        vkDeviceWaitIdle(device);
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }

    VkDescriptorSetLayout DescriptorSetLayout::get_layout() const { return layout; }
}
