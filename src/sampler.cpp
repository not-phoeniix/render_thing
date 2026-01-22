#include "sampler.h"
#include <stdexcept>

namespace RenderThing {
    Sampler::Sampler(const SamplerCreateInfo& create_info, const GraphicsContext& ctx)
      : device(ctx.device) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(ctx.physical_device, &properties);

        VkSamplerCreateInfo sampler_create_info = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,

            .magFilter = create_info.mag_filter,
            .minFilter = create_info.min_filter,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,

            .addressModeU = create_info.address_u,
            .addressModeV = create_info.address_v,
            .addressModeW = create_info.address_w,

            .mipLodBias = 0.0f,

            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = properties.limits.maxSamplerAnisotropy,

            .compareEnable = VK_TRUE,
            .compareOp = VK_COMPARE_OP_ALWAYS,

            .minLod = 0.0f,
            .maxLod = 0.0f,

            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };

        if (vkCreateSampler(ctx.device, &sampler_create_info, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create sampler!");
        }
    }

    Sampler::~Sampler() {
        vkDeviceWaitIdle(device);
        vkDestroySampler(device, sampler, nullptr);
    }

    VkSampler Sampler::get_sampler() const { return sampler; }
}
