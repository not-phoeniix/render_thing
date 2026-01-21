#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.h>
#include <vector>
#include <GLFW/glfw3.h>
#include "buffer_wrapper.h"
#include <memory>
#include "uniform_buffer_object.h"
#include "uniform_wrapper.h"
#include "graphics_context.h"
#include "image_wrapper.h"
#include "camera_push_constants.h"
#include "pixel_push_constants.h"

class GraphicsManager {
   private:
    VkInstance instance;
    VkPhysicalDevice physical_device = nullptr;
    VkDevice device;
    VkSurfaceKHR surface;
    GLFWwindow* window;

    VkSwapchainKHR swap_chain;
    std::vector<VkImage> swap_chain_images;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    std::vector<VkImageView> swap_chain_image_views;
    std::vector<VkFramebuffer> swap_chain_framebuffers;
    uint32_t swap_chain_image_index = 0;
    uint32_t frame_flight_index = 0;
    bool framebuffer_resized = false;

    std::unique_ptr<ImageWrapper> depth_image;
    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    std::vector<std::shared_ptr<UniformWrapper>> uniforms;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;
    VkClearValue clear_value = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    std::vector<VkSemaphore> image_available_sempahores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;

    void CreateInstance();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSurface();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateDepthResources();

    void CreateDescriptorSetLayout();
    void CreateDescriptorPool();

    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();

    void CleanupSwapChain();
    void RecreateSwapChain();

   public:
    GraphicsManager(GLFWwindow* window);
    ~GraphicsManager();

    void Begin();
    void EndAndPresent();

    std::shared_ptr<UniformWrapper> MakeNewUniform(VkImageView image_view, VkSampler sampler);
    void CmdBindUniform(std::shared_ptr<UniformWrapper> uniform);
    void CmdPushConstants(const void* data, size_t data_size, VkShaderStageFlags shader_stage, uint32_t offset);

    // getters/setters

    VkCommandBuffer get_command_buffer() const { return command_buffers[frame_flight_index]; }
    VkDevice get_device() const { return device; }
    VkPhysicalDevice get_physical_device() const { return physical_device; }
    VkClearValue get_clear_value() const { return clear_value; }
    VkCommandPool get_command_pool() const { return command_pool; }
    VkQueue get_graphics_queue() const { return graphics_queue; }
    VkQueue get_present_queue() const { return present_queue; }
    VkExtent2D get_swapchain_extent() const { return swap_chain_extent; }
    GraphicsContext get_context() const {
        return (GraphicsContext) {
            .instance = instance,
            .device = device,
            .physical_device = physical_device,
            .command_pool = command_pool,
            .command_buffer = command_buffers[frame_flight_index],
            .swapchain_extent = swap_chain_extent,
            .pipeline_layout = pipeline_layout,
            .graphics_queue = graphics_queue,
            .present_queue = present_queue,
            .window = window
        };
    }
    float get_aspect() const { return swap_chain_extent.width / (float)swap_chain_extent.height; }
    void set_clear_value(VkClearValue clear_value) { this->clear_value = clear_value; }
    void mark_resized() { framebuffer_resized = true; }
};
