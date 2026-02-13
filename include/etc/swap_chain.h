#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "../base/base.h"

namespace RenderThing {
    struct SwapChainCreateInfo {
        VkFormat depth_format;
        VkSurfaceFormatKHR surface_format;
        VkPresentModeKHR present_mode;
        uint32_t frame_flight_count;
        VkExtent2D extent;
        VkRenderPass render_pass;
    };

    class SwapChain {
       private:
        VkDevice device;
        VkSwapchainKHR swap_chain;
        std::vector<VkImage> images;
        std::unique_ptr<Image> depth_image;
        VkFormat image_format;
        VkExtent2D extent;
        std::vector<VkImageView> image_views;
        std::vector<VkFramebuffer> framebuffers;
        const uint32_t frame_flight_count;
        uint32_t image_index;
        uint32_t frame_flight_index;

        void CreateSwapChain(const SwapChainCreateInfo& create_info, const ApiContext& a_ctx);
        void CreateImageViews(const ApiContext& a_ctx);
        void CreateDepthImage(const SwapChainCreateInfo& create_info, const GraphicsContext& g_ctx, const ApiContext& a_ctx);
        void CreateFrameBuffers(const SwapChainCreateInfo& create_info, const ApiContext& a_ctx);

       public:
        SwapChain(const SwapChainCreateInfo& create_info, const GraphicsContext& g_ctx, const ApiContext& a_ctx);
        ~SwapChain();

        VkResult NextImage(VkSemaphore semaphore, VkFence fence);
        void NextFrame();

        uint32_t get_frame_index() const;
        uint32_t get_image_index() const;
        VkExtent2D get_extent() const;
        VkFormat get_image_format() const;
        VkFormat get_depth_format() const;
        uint32_t get_image_count() const;
        uint32_t get_frame_flight_count() const;
        VkSwapchainKHR get_swap_chain() const;
        VkFramebuffer get_current_framebuffer() const;
    };
}
