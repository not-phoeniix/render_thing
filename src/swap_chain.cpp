#include "swap_chain.h"
#include "vk_helpers.h"
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <array>

#pragma region // helpers

#pragma endregion

namespace RenderThing {
    void SwapChain::CreateSwapChain(const SwapChainCreateInfo& create_info, const GraphicsContext& ctx) {
        SwapChainSupportDetails details = query_swap_chain_support(ctx.physical_device, ctx.surface);

        // VkSurfaceFormatKHR surface_format = choose_swap_surface_format(details.formats);
        // VkPresentModeKHR present_mode = choose_swap_present_mode(details.present_modes);
        // VkExtent2D extent = choose_swap_extent(details.capabilities, ctx.window);

        uint32_t image_count = details.capabilities.minImageCount + (create_info.frame_flight_count - 1);
        if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount) {
            image_count = details.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swap_create_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = ctx.surface,
            // image info
            .minImageCount = image_count,
            .imageFormat = create_info.surface_format.format,
            .imageColorSpace = create_info.surface_format.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            // misc info about behavior
            .preTransform = details.capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = create_info.present_mode,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr
        };

        // set up queue families for swapchain
        QueueFamilyIndices indices = find_queue_families(ctx.physical_device, ctx.surface);
        std::array<uint32_t, 2> queue_family_indices = {
            indices.graphics.value(),
            indices.present.value()
        };

        // decide how sharing of images work
        //   (it'll be different if we have multiple queue families)
        if (indices.graphics != indices.present) {
            swap_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swap_create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size());
            swap_create_info.pQueueFamilyIndices = queue_family_indices.data();
        } else {
            swap_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swap_create_info.queueFamilyIndexCount = 0;
            swap_create_info.pQueueFamilyIndices = nullptr;
        }

        // create swapchain itself!!!!
        if (vkCreateSwapchainKHR(ctx.device, &swap_create_info, nullptr, &swap_chain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swap chain!");
        }

        // then grab images from the swapchain we just created
        vkGetSwapchainImagesKHR(ctx.device, swap_chain, &image_count, nullptr);
        images.resize(image_count);
        vkGetSwapchainImagesKHR(ctx.device, swap_chain, &image_count, images.data());

        image_format = create_info.surface_format.format;
        this->extent = extent;
    }

    void SwapChain::CreateImageViews(const GraphicsContext& ctx) {
        image_views.resize(images.size());

        for (size_t i = 0; i < images.size(); i++) {
            VkImageViewCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = image_format,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            if (vkCreateImageView(ctx.device, &create_info, nullptr, &image_views[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create swap chain image views!");
            }
        }
    }

    void SwapChain::CreateDepthImage(const SwapChainCreateInfo& create_info, const GraphicsContext& ctx) {
        ImageCreateInfo image_create_info = {
            .width = extent.width,
            .height = extent.height,
            .format = create_info.depth_format,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .image_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .view_aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT
        };
        depth_image = std::make_unique<Image>(image_create_info, ctx);
        depth_image->TransitionToLayout(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, ctx);
    }

    void SwapChain::CreateFrameBuffers(const SwapChainCreateInfo& create_info, const GraphicsContext& ctx) {
        framebuffers.resize(image_views.size());

        for (size_t i = 0; i < image_views.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                image_views[i],
                depth_image->get_view()
            };

            VkFramebufferCreateInfo fb_create_info = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = create_info.render_pass,
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments = attachments.data(),
                .width = extent.width,
                .height = extent.height,
                .layers = 1
            };

            if (vkCreateFramebuffer(ctx.device, &fb_create_info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }
    }

    SwapChain::SwapChain(const SwapChainCreateInfo& create_info, const GraphicsContext& ctx)
      : device(ctx.device),
        extent(create_info.extent),
        frame_flight_count(create_info.frame_flight_count),
        image_index(0),
        frame_flight_index(0) {
        if (frame_flight_count == 0) {
            throw std::runtime_error("Cannot create swap chain with a frame flight count of zero!");
        }

        CreateSwapChain(create_info, ctx);
        CreateImageViews(ctx);
        CreateDepthImage(create_info, ctx);
        CreateFrameBuffers(create_info, ctx);
    }

    SwapChain::~SwapChain() {
        vkDeviceWaitIdle(device);

        depth_image.reset();

        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (auto image_view : image_views) {
            vkDestroyImageView(device, image_view, nullptr);
        }

        vkDestroySwapchainKHR(device, swap_chain, nullptr);
    }

    VkResult SwapChain::NextImage(VkSemaphore semaphore, VkFence fence) {
        return vkAcquireNextImageKHR(
            device,
            swap_chain,
            UINT64_MAX,
            semaphore,
            fence,
            &image_index
        );
    }

    void SwapChain::NextFrame() {
        frame_flight_index = (frame_flight_index + 1) % frame_flight_count;
    }

    uint32_t SwapChain::get_frame_index() const { return frame_flight_index; }
    uint32_t SwapChain::get_image_index() const { return image_index; }
    VkExtent2D SwapChain::get_extent() const { return extent; }
    VkFormat SwapChain::get_image_format() const { return image_format; }
    VkFormat SwapChain::get_depth_format() const { return depth_image->get_format(); }
    uint32_t SwapChain::get_image_count() const { return static_cast<uint32_t>(images.size()); }
    VkSwapchainKHR SwapChain::get_swap_chain() const { return swap_chain; }
    VkFramebuffer SwapChain::get_current_framebuffer() const { return framebuffers[image_index]; }
}
