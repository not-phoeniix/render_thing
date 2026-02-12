#include "base/graphics_pipeline.h"
#include <stdexcept>

namespace RenderThing {
    GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineCreateInfo& create_info, const ApiContext& a_ctx)
      : device(a_ctx.device) {
        if (vkCreatePipelineLayout(a_ctx.device, create_info.layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipeline_create_info = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = create_info.shader_stage_count,
            .pStages = create_info.shader_stages,
            .pVertexInputState = create_info.vertex_input,
            .pInputAssemblyState = create_info.input_assembly,
            .pViewportState = create_info.viewport,
            .pRasterizationState = create_info.rasterizer,
            .pMultisampleState = create_info.multisample,
            .pDepthStencilState = create_info.depth_stencil,
            .pColorBlendState = create_info.color_blend,
            .pDynamicState = create_info.dynamic_state,
            .layout = pipeline_layout,
            .renderPass = create_info.render_pass,
            .subpass = create_info.subpass_index
        };

        if (vkCreateGraphicsPipelines(a_ctx.device, nullptr, 1, &pipeline_create_info, nullptr, &graphics_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }
    }

    GraphicsPipeline::~GraphicsPipeline() {
        vkDeviceWaitIdle(device);

        vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        vkDestroyPipeline(device, graphics_pipeline, nullptr);
    }

    VkPipelineLayout GraphicsPipeline::get_layout() const { return pipeline_layout; }
    VkPipeline GraphicsPipeline::get_pipeline() const { return graphics_pipeline; }
}
