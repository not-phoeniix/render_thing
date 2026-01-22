#include "image.h"

#include <stdexcept>
#include "vk_helpers.h"
#include "buffer.h"

namespace RenderThing {
    Image::Image(const ImageCreateInfo& create_info, const GraphicsContext& ctx)
      : device(ctx.device),
        image_format(create_info.format),
        width(create_info.width),
        height(create_info.height) {
        CreateImage(create_info, ctx);
        CreateImageView(create_info, ctx);
    }

    Image::~Image() {
        vkDeviceWaitIdle(device);

        vkDestroyImage(device, image, nullptr);
        vkFreeMemory(device, memory, nullptr);
        vkDestroyImageView(device, view, nullptr);
    }

    void Image::CreateImage(const ImageCreateInfo& create_info, const GraphicsContext& ctx) {
        VkImageCreateInfo image_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = create_info.format,
            .extent = {
                .width = create_info.width,
                .height = create_info.height,
                .depth = 1
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = create_info.tiling,
            .usage = create_info.image_usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        image_layout = image_create_info.initialLayout;

        if (vkCreateImage(ctx.device, &image_create_info, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image!");
        }

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(ctx.device, image, &mem_requirements);
        size = mem_requirements.size;

        VkMemoryAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = mem_requirements.size,
            .memoryTypeIndex = find_memory_type(
                mem_requirements.memoryTypeBits,
                create_info.memory_properties,
                ctx.physical_device
            )
        };

        if (vkAllocateMemory(device, &alloc_info, nullptr, &memory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, memory, 0);
    }

    void Image::CreateImageView(const ImageCreateInfo& create_info, const GraphicsContext& ctx) {
        VkImageViewCreateInfo view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = image_format,
            .subresourceRange = {
                .aspectMask = create_info.view_aspect_flags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
        };

        if (vkCreateImageView(ctx.device, &view_create_info, nullptr, &view) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view!");
        }
    }

    void Image::CopyData(const void* data, const GraphicsContext& ctx) {
        BufferCreateInfo staging_create_info = {
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };
        Buffer staging_buffer(staging_create_info, ctx);
        staging_buffer.CopyFromHostAuto(data, static_cast<size_t>(size));

        TransitionToLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ctx);

        copy_buffer_to_image(staging_buffer.get_buffer(), image, width, height, ctx);

        TransitionToLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ctx);
    }

    void Image::TransitionToLayout(VkImageLayout layout, const GraphicsContext& ctx) {
        transition_image_layout(
            image,
            image_format,
            image_layout,
            layout,
            ctx
        );
        image_layout = layout;
    }
}
