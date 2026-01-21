#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <string>

std::vector<char> shaders_read_file(const std::string& path);
VkShaderModule shaders_create_module(const std::vector<char>& byte_code, VkDevice device);
VkShaderModule shaders_create_module_from_file(const std::string& path, VkDevice device);
