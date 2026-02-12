#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "base/context_structs.h"
#include <optional>

namespace RenderThing {
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;

        bool is_complete() {
            return graphics.has_value() && present.has_value();
        }
    };

    namespace Utils {
        VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physical_device);
        VkFormat find_depth_format(VkPhysicalDevice physical_device);
        uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties, VkPhysicalDevice physical_device);
        VkCommandBuffer begin_single_use_commands(const GraphicsContext& g_ctx, const ApiContext& a_ctx);
        void end_single_use_commands(VkCommandBuffer command_buffer, const GraphicsContext& g_ctx, const ApiContext& a_ctx);
        void transition_image_layout(VkImage image, VkFormat format, VkImageLayout prev_layout, VkImageLayout new_layout, const GraphicsContext& g_ctx, const ApiContext& a_ctx);
        void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, const GraphicsContext& g_ctx, const ApiContext& a_ctx);
        SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface);
        QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);
        bool check_device_extension_support(VkPhysicalDevice device, const char* const* extensions, uint32_t extension_count);
        bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface, const char* const* extensions, uint32_t extension_count);
        VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& present_modes);
        VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    }
}
