#pragma once

#include <vulkan/vulkan.h>
#include "graphics_context.h"

namespace RenderThing {
    struct ImageWrapperCreateInfo {
        uint32_t width;
        uint32_t height;
        VkFormat format;
        VkImageTiling tiling;
        VkImageUsageFlags image_usage;
        VkMemoryPropertyFlags memory_properties;
        VkImageAspectFlags view_aspect_flags;
    };

    class ImageWrapper {
       private:
        VkDevice device;
        VkImage image;
        VkImageView view;
        VkDeviceMemory memory;
        VkDeviceSize size;
        VkFormat image_format;
        VkImageLayout image_layout;
        uint32_t width;
        uint32_t height;

        void CreateImage(const ImageWrapperCreateInfo& create_info, const GraphicsContext& ctx);
        void CreateImageView(const ImageWrapperCreateInfo& create_info, const GraphicsContext& ctx);

       public:
        ImageWrapper(const ImageWrapperCreateInfo& create_info, const GraphicsContext& ctx);
        ~ImageWrapper();

        void CopyData(const void* data, const GraphicsContext& ctx);
        void TransitionToLayout(VkImageLayout layout, const GraphicsContext& ctx);

        VkImage get_image() const { return image; }
        VkImageView get_view() const { return view; }
        uint32_t get_width() const { return width; }
        uint32_t get_height() const { return height; }
    };
}
