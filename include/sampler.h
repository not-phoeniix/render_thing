#pragma once

#include <vulkan/vulkan.h>
#include "graphics_context.h"

namespace RenderThing {
    struct SamplerCreateInfo {
        VkFilter min_filter;
        VkFilter mag_filter;
        VkSamplerAddressMode address_u;
        VkSamplerAddressMode address_v;
        VkSamplerAddressMode address_w;
    };

    class Sampler {
       private:
        VkDevice device;
        VkSampler sampler;

       public:
        Sampler(const SamplerCreateInfo& create_info, const GraphicsContext& ctx);
        ~Sampler();

        VkSampler get_sampler() const;
    };
}
