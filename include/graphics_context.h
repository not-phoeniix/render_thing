#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

struct GraphicsContext {
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkExtent2D swapchain_extent;
    VkPipelineLayout pipeline_layout;
    VkQueue graphics_queue;
    VkQueue present_queue;
    GLFWwindow* window;
};
