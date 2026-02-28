#include "etc/api_cluster.h"

#include <vector>
#include <set>
#include "vk_utils.h"

const std::vector<const char*> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

namespace RenderThing {
    ApiCluster::ApiCluster(const ApiClusterCreateInfo& create_info) {
        // create instance
        instance = std::make_unique<Instance>(create_info.instance);

        // create window surface
        if (glfwCreateWindowSurface(instance->get_instance(), window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }

        // pick physical device
        {
            uint32_t device_count = 0;
            vkEnumeratePhysicalDevices(instance->get_instance(), &device_count, nullptr);

            if (device_count == 0) {
                throw std::runtime_error("Failed to find GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> devices(device_count);
            vkEnumeratePhysicalDevices(instance->get_instance(), &device_count, devices.data());

            for (const auto& device : devices) {
                if (Utils::is_device_suitable(device, surface, DEVICE_EXTENSIONS.data(), DEVICE_EXTENSIONS.size())) {
                    physical_device = device;
                    break;
                }
            }

            if (physical_device == nullptr) {
                throw std::runtime_error("Failed to find a suitable GPU!");
            }
        }

        // create logical device
        {
            QueueFamilyIndices indices = Utils::find_queue_families(physical_device, surface);

            std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
            // we use a set so we dont have duplicate indices <3
            std::set<uint32_t> unique_queue_families = {
                indices.graphics.value(),
                indices.present.value()
            };

            // priorities are used to influence scheduling order, inchresting
            float queue_priority = 1.0f;
            for (uint32_t family_index : unique_queue_families) {
                VkDeviceQueueCreateInfo queue_create_info = {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex = family_index,
                    .queueCount = 1,
                    .pQueuePriorities = &queue_priority
                };

                queue_create_infos.push_back(queue_create_info);
            }

            VkPhysicalDeviceFeatures device_features = {
                .samplerAnisotropy = VK_TRUE
            };

            VkDeviceCreateInfo device_create_info = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
                .pQueueCreateInfos = queue_create_infos.data(),
                .enabledLayerCount = 0,
                .enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size()),
                .ppEnabledExtensionNames = DEVICE_EXTENSIONS.data(),
                .pEnabledFeatures = &device_features,
            };

            // we don't rlly need this with newer versions of vulkan since
            //   instance validation layers kinda replaced everything
            //   else but we keep it just to be compatible for old versions
            device_create_info.enabledLayerCount = create_info.instance.validation_layer_count;
            device_create_info.ppEnabledLayerNames = create_info.instance.validation_layers;

            if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create logical device!");
            }
        }
    }

    ApiCluster::~ApiCluster() {
        vkDeviceWaitIdle(device);

        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance->get_instance(), surface, nullptr);
        instance.reset();
    }

    VkInstance ApiCluster::get_instance() const { return instance->get_instance(); }
    VkPhysicalDevice ApiCluster::get_physical_device() const { return physical_device; }
    VkDevice ApiCluster::get_device() const { return device; }
    VkSurfaceKHR ApiCluster::get_surface() const { return surface; }
    GLFWwindow* ApiCluster::get_window() const { return window; }
    ApiContext ApiCluster::get_api_context() const {
        return (ApiContext) {
            .instance = instance->get_instance(),
            .device = device,
            .physical_device = physical_device,
            .window = window,
            .surface = surface
        };
    }
    void ApiCluster::get_queues(VkQueue* out_graphics_queue, VkQueue* out_present_queue) const {
        QueueFamilyIndices indices = Utils::find_queue_families(physical_device, surface);
        vkGetDeviceQueue(device, indices.graphics.value(), 0, out_graphics_queue);
        vkGetDeviceQueue(device, indices.present.value(), 0, out_present_queue);
    }
}
