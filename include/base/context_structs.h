#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace RenderThing {
    struct ApiContext {
        VkInstance instance;
        VkDevice device;
        VkPhysicalDevice physical_device;
        GLFWwindow* window;
        VkSurfaceKHR surface;
    };

    struct GraphicsContext {
        VkQueue graphics_queue;
        VkCommandPool command_pool;
        VkCommandBuffer frame_command_buffer;
    };
}
