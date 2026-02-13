#pragma once

#include <vulkan/vulkan.h>
#include "context_structs.h"

namespace RenderThing {
    struct DescriptorSetLayoutCreateInfo {
        VkDescriptorSetLayoutCreateFlags flags;
        const VkDescriptorSetLayoutBinding* bindings;
        uint32_t binding_count;
    };

    class DescriptorSetLayout {
       private:
        VkDevice device;
        VkDescriptorSetLayout layout;

       public:
        DescriptorSetLayout(const DescriptorSetLayoutCreateInfo& create_info, const ApiContext& a_ctx);
        ~DescriptorSetLayout();

        VkDescriptorSetLayout get_layout() const;
    };
}
