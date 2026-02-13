#include "base/render_pass.h"

#include <stdexcept>

namespace RenderThing {
    RenderPass::RenderPass(const RenderPassCreateInfo& create_info, const ApiContext& a_ctx)
      : device(a_ctx.device) {
        VkRenderPassCreateInfo subpass_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = create_info.attachment_count,
            .pAttachments = create_info.attachments,
            .subpassCount = create_info.subpass_count,
            .pSubpasses = create_info.subpasses,
            .dependencyCount = create_info.dependency_count,
            .pDependencies = create_info.dependencies
        };

        if (vkCreateRenderPass(device, &subpass_info, nullptr, &render_pass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass!");
        }
    }

    RenderPass::~RenderPass() {
        vkDeviceWaitIdle(device);
        vkDestroyRenderPass(device, render_pass, nullptr);
    }

    VkRenderPass RenderPass::get_render_pass() const { return render_pass; }
}
