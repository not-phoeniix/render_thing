#pragma once

#include <vulkan/vulkan.h>
#include "graphics_context.h"

struct SamplerWrapperCreateInfo {
    VkFilter min_filter;
    VkFilter mag_filter;
    VkSamplerAddressMode address_u;
    VkSamplerAddressMode address_v;
    VkSamplerAddressMode address_w;
};

class SamplerWrapper {
   private:
    VkDevice device;
    VkSampler sampler;

   public:
    SamplerWrapper(const SamplerWrapperCreateInfo& create_info, const GraphicsContext& ctx);
    ~SamplerWrapper();

    VkSampler get_sampler() const { return sampler; }
};
