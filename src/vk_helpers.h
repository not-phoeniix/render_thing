#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "graphics_context.h"

VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physical_device);
uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties, VkPhysicalDevice physical_device);
VkCommandBuffer begin_single_use_commands(const RenderThing::GraphicsContext& ctx);
void end_single_use_commands(VkCommandBuffer command_buffer, const RenderThing::GraphicsContext& ctx);
void transition_image_layout(VkImage image, VkFormat format, VkImageLayout prev_layout, VkImageLayout new_layout, const RenderThing::GraphicsContext& ctx);
void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, const RenderThing::GraphicsContext& ctx);
