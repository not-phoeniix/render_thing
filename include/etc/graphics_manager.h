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
#include "destruction_queue.h"

namespace RenderThing {
    struct GraphicsManagerCreateInfo {
        VkClearValue clear_value;
        GLFWwindow* window;
        const InstanceCreateInfo& instance;
        const SwapChainCreateInfo& swap_chain;
        const GraphicsPipelineCreateInfo& graphics_pipeline;
    };

    struct FrameData {
        VkSemaphore image_available_semaphore;
        VkFence in_flight_fence;
        VkCommandBuffer command_buffer;
    };

    class GraphicsManager {
       private:
        std::unique_ptr<Instance> instance;
        VkPhysicalDevice physical_device;
        VkDevice device;
        VkSurfaceKHR surface;
        GLFWwindow* window;

        SwapChainCreateInfo swap_chain_create_info;
        std::unique_ptr<SwapChain> swap_chain;
        std::vector<VkSemaphore> render_finished_semaphores;
        std::vector<FrameData> frame_datas;
        bool framebuffer_resized;

        std::unique_ptr<RenderPass> render_pass;
        std::unique_ptr<GraphicsPipeline> pipeline;

        std::unique_ptr<DescriptorSetLayout> descriptor_set_layout;
        std::unique_ptr<DescriptorPool> descriptor_pool;

        VkQueue graphics_queue;
        VkQueue present_queue;
        VkCommandPool command_pool;
        VkClearValue clear_value;

        DestructionQueue destruction_queue;

        void CreateApiObjects(const GraphicsManagerCreateInfo& create_info);
        void CreateDescriptors(const GraphicsManagerCreateInfo& create_info);
        void CreateRenderObjects(const GraphicsManagerCreateInfo& create_info);
        void CreateCommandPool(const GraphicsManagerCreateInfo& create_info);
        void CreateSyncAndFrameData(const GraphicsManagerCreateInfo& create_info);

        void RecreateSwapChain();

       public:
        GraphicsManager(const GraphicsManagerCreateInfo& create_info);
        ~GraphicsManager();

        void Begin();
        void EndAndPresent();

        // getters/setters

        VkCommandBuffer get_command_buffer() const;
        VkDevice get_device() const;
        VkPhysicalDevice get_physical_device() const;
        VkClearValue get_clear_value() const;
        VkCommandPool get_command_pool() const;
        VkQueue get_graphics_queue() const;
        VkQueue get_present_queue() const;
        VkExtent2D get_swapchain_extent() const;
        ApiContext get_api_context() const;
        GraphicsContext get_graphics_context() const;
        float get_aspect() const;
        void set_clear_value(VkClearValue clear_value);
        void mark_resized();
    };
}
