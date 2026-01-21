#include "shader_helper.h"

#include <fstream>
#include <stdexcept>
#include <iostream>

std::vector<char> shaders_read_file(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}

VkShaderModule shaders_create_module(const std::vector<char>& byte_code, VkDevice device) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = byte_code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(byte_code.data())
    };

    VkShaderModule shader = nullptr;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shader;
}

VkShaderModule shaders_create_module_from_file(const std::string& path, VkDevice device) {
    std::vector<char> byte_code = shaders_read_file(path);
    return shaders_create_module(byte_code, device);
}
