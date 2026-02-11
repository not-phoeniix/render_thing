#pragma once

#include <vulkan/vulkan.h>
#include "graphics_context.h"

namespace RenderThing {
    struct GraphicsPipelineCreateInfo {
        VkPipelineShaderStageCreateInfo* shader_stages;
        uint32_t shader_stage_count;
        const VkPipelineVertexInputStateCreateInfo* vertex_input;
        const VkPipelineInputAssemblyStateCreateInfo* input_assembly;
        const VkPipelineViewportStateCreateInfo* viewport;
        const VkPipelineRasterizationStateCreateInfo* rasterizer;
        const VkPipelineMultisampleStateCreateInfo* multisample;
        const VkPipelineDepthStencilStateCreateInfo* depth_stencil;
        const VkPipelineColorBlendStateCreateInfo* color_blend;
        const VkPipelineDynamicStateCreateInfo* dynamic_state;
        const VkPipelineLayoutCreateInfo* layout_create_info;
        VkRenderPass render_pass;
        uint32_t subpass_index;
    };

    class GraphicsPipeline {
       private:
        VkDevice device;
        VkPipelineLayout pipeline_layout;
        VkPipeline graphics_pipeline;

       public:
        GraphicsPipeline(const GraphicsPipelineCreateInfo& create_info, const GraphicsContext& ctx);
        ~GraphicsPipeline();

        VkPipelineLayout get_layout() const;
        VkPipeline get_pipeline() const;
    };
}
