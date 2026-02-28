#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include "../base/instance.h"
#include "../base/context_structs.h"
#include <GLFW/glfw3.h>

namespace RenderThing {
    struct ApiClusterCreateInfo {
        const InstanceCreateInfo& instance;
        GLFWwindow* window;
    };

    class ApiCluster {
       private:
        std::unique_ptr<Instance> instance;
        VkPhysicalDevice physical_device;
        VkDevice device;
        VkSurfaceKHR surface;
        GLFWwindow* window;

       public:
        ApiCluster(const ApiClusterCreateInfo& create_info);
        ~ApiCluster();

        VkInstance get_instance() const;
        VkPhysicalDevice get_physical_device() const;
        VkDevice get_device() const;
        VkSurfaceKHR get_surface() const;
        GLFWwindow* get_window() const;
        ApiContext get_api_context() const;
        void get_queues(VkQueue* out_graphics_queue, VkQueue* out_present_queue) const;
    };
}
