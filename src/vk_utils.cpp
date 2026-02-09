#include "vk_utils.h"

#include <stdexcept>
#include <set>
#include <limits>
#include <algorithm>

namespace RenderThing::Utils {
    VkFormat find_supported_format(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features,
        VkPhysicalDevice physical_device
    ) {
        for (auto format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("Failed to find supported format!");
    }

    VkFormat find_depth_format(VkPhysicalDevice physical_device) {
        return find_supported_format(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
            physical_device
        );
    }

    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties, VkPhysicalDevice physical_device) {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
            if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find any suitable memory type!");
    }

    VkCommandBuffer begin_single_use_commands(const RenderThing::GraphicsContext& ctx) {
        VkCommandBufferAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = ctx.command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(ctx.device, &alloc_info, &command_buffer);

        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        vkBeginCommandBuffer(command_buffer, &begin_info);

        return command_buffer;
    }

    void end_single_use_commands(
        VkCommandBuffer command_buffer,
        const RenderThing::GraphicsContext& ctx
    ) {
        vkEndCommandBuffer(command_buffer);

        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer
        };

        vkQueueSubmit(ctx.graphics_queue, 1, &submit_info, nullptr);

        vkQueueWaitIdle(ctx.graphics_queue);

        vkFreeCommandBuffers(ctx.device, ctx.command_pool, 1, &command_buffer);
    }

    void transition_image_layout(
        VkImage image,
        VkFormat format,
        VkImageLayout prev_layout,
        VkImageLayout new_layout,
        const RenderThing::GraphicsContext& ctx
    ) {
        VkCommandBuffer command_buffer = begin_single_use_commands(ctx);

        // use a barrier to transition layouts :D
        //   (these are typically used for synchronization stuff
        //   but they can also be used to change image layouts <3)
        VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            // specify which async operations must be awaited for
            //   the barrier transition to take place
            //   (these are filled out down there v)
            .srcAccessMask = 0,
            .dstAccessMask = 0,
            // actual layout info
            .oldLayout = prev_layout,
            .newLayout = new_layout,
            // we could use a barrier to transfer queue family ownership
            //   but things are not exclusive right now so we can ignore
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                // we'll specify aspect mask in a second...
                .aspectMask = 0,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
        };

        // find stage flags
        VkPipelineStageFlags src_stage = 0;
        VkPipelineStageFlags dst_stage = 0;

        bool success = true;

        switch (prev_layout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                barrier.srcAccessMask = 0;
                src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            default:
                success = false;
                break;
        }

        switch (new_layout) {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                // if there is a stencil component, specify stencil buffer mask too !
                if (
                    format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                    format == VK_FORMAT_D24_UNORM_S8_UINT
                ) {
                    barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
                break;

            default:
                success = false;
                break;
        }

        if (!success) {
            throw std::runtime_error("Unsupported layout transition combination!");
        }

        vkCmdPipelineBarrier(
            command_buffer,
            src_stage,
            dst_stage,
            0,
            0, nullptr, // regular memory barriers
            0, nullptr, // buffer memory barriers
            1, &barrier // IMAGE memory barriers :]
        );

        end_single_use_commands(command_buffer, ctx);
    }

    void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, const RenderThing::GraphicsContext& ctx) {
        VkCommandBuffer command_buffer = begin_single_use_commands(ctx);

        VkBufferImageCopy region = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset = {0, 0},
            .imageExtent = {width, height, 1}
        };

        vkCmdCopyBufferToImage(
            command_buffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        end_single_use_commands(command_buffer, ctx);
    }

    SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
        if (format_count > 0) {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
        }

        uint32_t present_mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
        if (present_mode_count > 0) {
            details.present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device,
                surface,
                &present_mode_count,
                details.present_modes.data()
            );
        }

        return details;
    }

    QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
        QueueFamilyIndices indices;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        // find the indices of the returned families and store !!!
        for (uint32_t i = 0; i < queue_family_count; i++) {
            // check for graphics support at this queue index, save index if so
            if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                indices.graphics = i;
            }

            // check for present support at this queue index, save index if so
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
            if (present_support) {
                indices.present = i;
            }

            if (indices.is_complete()) {
                break;
            }
        }

        return indices;
    }

    bool check_device_extension_support(VkPhysicalDevice device, const char* const* extensions, uint32_t in_ext_size) {
        uint32_t extension_count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

        // we use string so equality comparisons work in the set
        std::set<std::string> required_extensions;
        for (uint32_t i = 0; i < in_ext_size; i++) {
            required_extensions.insert(std::string(extensions[i]));
        }

        for (const auto& extension : available_extensions) {
            required_extensions.erase(extension.extensionName);
        }

        return required_extensions.empty();
    }

    bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface, const char* const* extensions, uint32_t extension_count) {
        RenderThing::QueueFamilyIndices indices = RenderThing::Utils::find_queue_families(device, surface);

        bool extensions_supported = check_device_extension_support(device, extensions, extension_count);

        bool swap_chain_adequate = false;
        if (extensions_supported) {
            RenderThing::SwapChainSupportDetails details = RenderThing::Utils::query_swap_chain_support(device, surface);
            swap_chain_adequate = !details.formats.empty() && !details.present_modes.empty();
        }

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

        return indices.is_complete() &&
               extensions_supported &&
               swap_chain_adequate &&
               features.samplerAnisotropy;
    }

    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& formats) {
        for (const auto& format : formats) {
            if (
                format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            ) {
                return format;
            }
        }

        return formats[0];
    }

    VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& present_modes) {
        for (const auto& present_mode : present_modes) {
            if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return present_mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width;
            int height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actual_extent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actual_extent.width = std::clamp(
                actual_extent.width,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width
            );
            actual_extent.height = std::clamp(
                actual_extent.height,
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height
            );

            return actual_extent;
        }
    }

}
