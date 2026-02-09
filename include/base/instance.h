#pragma once

#include <vulkan/vulkan.h>

namespace RenderThing {
    struct InstanceCreateInfo {
        const char* app_name;
        uint32_t app_version;
        uint32_t api_version;
        const char* const* validation_layers;
        uint32_t validation_layer_count;
    };

    class Instance {
       private:
        VkInstance instance;

       public:
        Instance(const InstanceCreateInfo& create_info);
        ~Instance();

        VkInstance get_instance() const;
    };
}
