#pragma once

#include <vulkan/vulkan.h>
#include "context_structs.h"

namespace RenderThing {
    struct ImageCreateInfo {
        uint32_t width;
        uint32_t height;
        VkFormat format;
        VkImageTiling tiling;
        VkImageUsageFlags image_usage;
        VkMemoryPropertyFlags memory_properties;
        VkImageAspectFlags view_aspect_flags;
    };

    class Image {
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

        void CreateImage(const ImageCreateInfo& create_info, const ApiContext& a_ctx);
        void CreateImageView(const ImageCreateInfo& create_info, const ApiContext& a_ctx);

       public:
        Image(const ImageCreateInfo& create_info, const ApiContext& a_ctx);
        ~Image();

        void CopyData(const void* data, const GraphicsContext& g_ctx, const ApiContext& a_ctx);
        void TransitionToLayout(VkImageLayout layout, const GraphicsContext& g_ctx, const ApiContext& a_ctx);

        VkImage get_image() const;
        VkImageView get_view() const;
        uint32_t get_width() const;
        uint32_t get_height() const;
        VkFormat get_format() const;
    };
}
