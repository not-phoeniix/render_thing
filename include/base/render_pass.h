#pragma once

#include <vulkan/vulkan.h>
#include "context_structs.h"

namespace RenderThing {
    struct RenderPassCreateInfo {
        const VkAttachmentDescription* attachments;
        uint32_t attachment_count;
        const VkSubpassDescription* subpasses;
        uint32_t subpass_count;
        const VkSubpassDependency* dependencies;
        uint32_t dependency_count;
    };

    class RenderPass {
       private:
        VkDevice device;
        VkRenderPass render_pass;

       public:
        RenderPass(const RenderPassCreateInfo& create_info, const ApiContext& a_ctx);
        ~RenderPass();

        VkRenderPass get_render_pass() const;
    };
}
