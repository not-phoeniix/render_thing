#include "base/instance.h"

#include <vector>
#include <cstring>
#include <stdexcept>
#include <GLFW/glfw3.h>

RenderThing::Instance::Instance(const InstanceCreateInfo& create_info) {
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = create_info.app_name,
        .applicationVersion = create_info.app_version,
        .pEngineName = "render_thing",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = create_info.api_version
    };

    // ~~~ vulkan validation layers ~~~

    std::vector<const char*> layers_to_use;
    if (create_info.validation_layer_count > 0) {
        uint32_t layer_count = 0;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        // make sure that each validation layer we wannna use is
        //   actually in the supported layer properties list
        for (uint32_t i = 0; i < create_info.validation_layer_count; i++) {
            const char* layer_name = create_info.validation_layers[i];
            bool found = false;

            for (const auto& layer_properties : available_layers) {
                if (strcmp(layer_name, layer_properties.layerName) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                throw std::runtime_error("Validation layer requested but not available: " + std::string(layer_name));
            }
        }

        for (uint32_t i = 0; i < create_info.validation_layer_count; i++) {
            layers_to_use.push_back(create_info.validation_layers[i]);
        }
    }

    // ~~~ GLFW instance extensions ~~~

    // get GLFW extension
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    // make sure extensions are supported
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    for (uint32_t i = 0; i < glfw_extension_count; i++) {
        bool found = false;
        for (const auto& extension : extensions) {
            if (strcmp(glfw_extensions[i], extension.extensionName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfw_extensions[i]));
        }
    }

    // ~~~ create instance itself ~~~

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(layers_to_use.size()),
        .ppEnabledLayerNames = layers_to_use.data(),
        .enabledExtensionCount = glfw_extension_count,
        .ppEnabledExtensionNames = glfw_extensions
    };

    if (vkCreateInstance(&instance_create_info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instance!");
    }
}

RenderThing::Instance::~Instance() {
    vkDestroyInstance(instance, nullptr);
}

VkInstance RenderThing::Instance::get_instance() const { return instance; }
