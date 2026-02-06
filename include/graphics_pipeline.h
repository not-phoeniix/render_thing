#pragma once

#include <vulkan/vulkan.h>
#include "graphics_context.h"

namespace RenderThing {
    struct GraphicsPipelineCreateInfo {
        VkPipelineShaderStageCreateInfo* shader_stages;
        uint32_t shader_stage_count;
        VkPipelineVertexInputStateCreateInfo* vertex_input;
        VkPipelineInputAssemblyStateCreateInfo* input_assembly;
        VkPipelineViewportStateCreateInfo* viewport;
        VkPipelineRasterizationStateCreateInfo* rasterizer;
        VkPipelineMultisampleStateCreateInfo* multisample;
        VkPipelineDepthStencilStateCreateInfo* depth_stencil;
        VkPipelineColorBlendStateCreateInfo* color_blend;
        VkPipelineDynamicStateCreateInfo* dynamic_state;
        VkPipelineLayoutCreateInfo* layout_create_info;
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
