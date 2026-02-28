#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <GLFW/glfw3.h>
#include <memory>
#include "../base/base.h"
#include "swap_chain.h"
#include "destruction_queue.h"
#include "api_cluster.h"

namespace RenderThing {
    struct GraphicsManagerCreateInfo {
        VkClearValue clear_value;
        GLFWwindow* window;
        std::shared_ptr<ApiCluster> api_cluster;
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
        std::shared_ptr<ApiCluster> api_cluster;
        VkDevice device;

        SwapChainCreateInfo swap_chain_create_info;
        std::unique_ptr<SwapChain> swap_chain;
        std::vector<VkSemaphore> render_finished_semaphores;
        std::vector<FrameData> frame_datas;
        bool framebuffer_resized;

        std::unique_ptr<RenderPass> render_pass;
        std::unique_ptr<GraphicsPipeline> pipeline;

        VkQueue graphics_queue;
        VkQueue present_queue;
        VkCommandPool command_pool;
        VkClearValue clear_value;

        DestructionQueue destruction_queue;

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
