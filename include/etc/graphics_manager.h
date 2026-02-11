#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <GLFW/glfw3.h>
#include <memory>
#include "../uniform_buffer_object.h"
#include "../camera_push_constants.h"
#include "../pixel_push_constants.h"
#include "../base/base.h"
#include "uniform.h"
#include "swap_chain.h"

namespace RenderThing {
    struct GraphicsManagerCreateInfo {
        uint32_t frames_in_flight;
        VkClearValue clear_value;
        GLFWwindow* window;
        const InstanceCreateInfo& instance;
        const SwapChainCreateInfo& swap_chain;
        // const GraphicsPipelineCreateInfo& graphics_pipeline;
    };

    class GraphicsManager {
       private:
        std::unique_ptr<Instance> instance;
        VkPhysicalDevice physical_device;
        VkDevice device;
        VkSurfaceKHR surface;
        GLFWwindow* window;

        std::unique_ptr<SwapChain> swap_chain;
        std::vector<VkSemaphore> image_available_sempahores;
        std::vector<VkSemaphore> render_finished_semaphores;
        std::vector<VkFence> in_flight_fences;
        bool framebuffer_resized;

        VkRenderPass render_pass;
        std::unique_ptr<GraphicsPipeline> pipeline;

        VkDescriptorSetLayout descriptor_set_layout;
        std::unique_ptr<DescriptorPool> descriptor_pool;
        std::vector<std::shared_ptr<Uniform>> uniforms;

        VkQueue graphics_queue;
        VkQueue present_queue;
        VkCommandPool command_pool;
        std::vector<VkCommandBuffer> command_buffers;
        VkClearValue clear_value;

        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateSurface();

        void CreateDescriptorSetLayout();
        void CreateDescriptorPool();

        void CreateRenderPass();
        void CreateSwapChain();
        void CreateGraphicsPipeline();
        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();

        void RecreateSwapChain();

       public:
        GraphicsManager(const GraphicsManagerCreateInfo& create_info);
        ~GraphicsManager();

        void Begin();
        void EndAndPresent();

        std::shared_ptr<Uniform> MakeNewUniform(VkImageView image_view, VkSampler sampler);
        void CmdBindUniform(std::shared_ptr<Uniform> uniform);
        void CmdPushConstants(const void* data, size_t data_size, VkShaderStageFlags shader_stage, uint32_t offset);

        // getters/setters

        VkCommandBuffer get_command_buffer() const;
        VkDevice get_device() const;
        VkPhysicalDevice get_physical_device() const;
        VkClearValue get_clear_value() const;
        VkCommandPool get_command_pool() const;
        VkQueue get_graphics_queue() const;
        VkQueue get_present_queue() const;
        VkExtent2D get_swapchain_extent() const;
        GraphicsContext get_context() const;
        float get_aspect() const;
        void set_clear_value(VkClearValue clear_value);
        void mark_resized();
    };
}
