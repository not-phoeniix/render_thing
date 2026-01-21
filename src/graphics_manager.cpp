#include "graphics_manager.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstring>
#include <string>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include "shader_helper.h"
#include "vertex.h"
#include "vk_helpers.h"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr uint32_t MAX_NUM_UNIFORMS = 20000;
constexpr bool ENABLE_VALIDATION_LAYERS = true;
const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool is_complete() {
        return graphics.has_value() && present.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

#pragma region // Helper functions!

static QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    // find the indices of the returned families and store !!!
    for (uint32_t i = 0; i < queue_family_count; i++) {
        // check for graphics support at this queue index, save index if so
        if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            indices.graphics = i;
        }

        // check for present support at this queue index, save index if so
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
        if (present_support) {
            indices.present = i;
        }

        if (indices.is_complete()) {
            break;
        }
    }

    return indices;
}

static SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    if (format_count > 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
    }

    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
    if (present_mode_count > 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device,
            surface,
            &present_mode_count,
            details.present_modes.data()
        );
    }

    return details;
}

static bool check_device_extension_support(VkPhysicalDevice device) {
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    // we use string so equality comparisons work in the set
    std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

static bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices = find_queue_families(device, surface);

    bool extensions_supported = check_device_extension_support(device);

    bool swap_chain_adequate = false;
    if (extensions_supported) {
        SwapChainSupportDetails details = query_swap_chain_support(device, surface);
        swap_chain_adequate = !details.formats.empty() && !details.present_modes.empty();
    }

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    return indices.is_complete() &&
           extensions_supported &&
           swap_chain_adequate &&
           features.samplerAnisotropy;
}

static VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (const auto& format : formats) {
        if (
            format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        ) {
            return format;
        }
    }

    return formats[0];
}

static VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& present_modes) {
    for (const auto& present_mode : present_modes) {
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return present_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width;
        int height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actual_extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actual_extent.width = std::clamp(
            actual_extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width
        );
        actual_extent.height = std::clamp(
            actual_extent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height
        );

        return actual_extent;
    }
}

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    auto manager = reinterpret_cast<GraphicsManager*>(glfwGetWindowUserPointer(window));
    manager->mark_resized();
}

static VkFormat find_depth_format(VkPhysicalDevice physical_device) {
    return find_supported_format(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        physical_device
    );
}

#pragma endregion

#pragma region // Class definitions <3

GraphicsManager::GraphicsManager(GLFWwindow* window) : window(window) {
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

    CreateInstance();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();

    CreateDescriptorSetLayout();
    CreateDescriptorPool();

    CreateCommandPool();
    CreateCommandBuffers();

    CreateDepthResources();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateSyncObjects();
}

GraphicsManager::~GraphicsManager() {
    CleanupSwapChain();

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, image_available_sempahores[i], nullptr);
        vkDestroyFence(device, in_flight_fences[i], nullptr);
    }

    for (size_t i = 0; i < render_finished_semaphores.size(); i++) {
        vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
    }

    uniforms.clear();
    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);

    depth_image.reset();
    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroyCommandPool(device, command_pool, nullptr);

    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void GraphicsManager::CreateInstance() {
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Triangle 3 <3",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine :O",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4
    };

    // ~~~ vulkan validation layers ~~~

    std::vector<const char*> layers_to_use;
    if (ENABLE_VALIDATION_LAYERS) {
        uint32_t layer_count = 0;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        // make sure that each validation layer we wannna use is
        //   actually in the supported layer properties list
        for (const char* layer_name : VALIDATION_LAYERS) {
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

        layers_to_use.assign(VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
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

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(layers_to_use.size()),
        .ppEnabledLayerNames = layers_to_use.data(),
        .enabledExtensionCount = glfw_extension_count,
        .ppEnabledExtensionNames = glfw_extensions
    };

    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instance!");
    }
}

void GraphicsManager::PickPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    for (const auto& device : devices) {
        if (is_device_suitable(device, surface)) {
            physical_device = device;
            break;
        }
    }

    if (physical_device == nullptr) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

void GraphicsManager::CreateLogicalDevice() {
    QueueFamilyIndices indices = find_queue_families(physical_device, surface);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    // we use a set so we dont have duplicate indices <3
    std::set<uint32_t> unique_queue_families = {
        indices.graphics.value(),
        indices.present.value()
    };

    // priorities are used to influence scheduling order, inchresting
    float queue_priority = 1.0f;
    for (uint32_t family_index : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = family_index,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };

        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features = {
        .samplerAnisotropy = VK_TRUE
    };

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledLayerCount = 0,
        .enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size()),
        .ppEnabledExtensionNames = DEVICE_EXTENSIONS.data(),
        .pEnabledFeatures = &device_features,
    };

    // we don't rlly need this with newer versions of vulkan since
    //   instance validation layers kinda replaced everything
    //   else but we keep it just to be compatible for old versions
    if (ENABLE_VALIDATION_LAYERS) {
        create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    }

    if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }

    // aaaaand store the queues from the device we just created :D
    vkGetDeviceQueue(device, indices.graphics.value(), 0, &graphics_queue);
    vkGetDeviceQueue(device, indices.present.value(), 0, &present_queue);
}

void GraphicsManager::CreateSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void GraphicsManager::CreateSwapChain() {
    SwapChainSupportDetails details = query_swap_chain_support(physical_device, surface);

    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(details.formats);
    VkPresentModeKHR present_mode = choose_swap_present_mode(details.present_modes);
    VkExtent2D extent = choose_swap_extent(details.capabilities, window);

    uint32_t image_count = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount) {
        image_count = details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        // image info
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        // misc info about behavior
        .preTransform = details.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = nullptr
    };

    // set up queue families for swapchain
    QueueFamilyIndices indices = find_queue_families(physical_device, surface);
    uint32_t queue_family_indices[] = {indices.graphics.value(), indices.present.value()};

    // decide how sharing of images work
    //   (it'll be different if we have multiple queue families)
    if (indices.graphics != indices.present) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    // create swapchain itself!!!!
    if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    // then grab images from the swapchain we just created
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());

    swap_chain_image_format = surface_format.format;
    swap_chain_extent = extent;
}

void GraphicsManager::CreateImageViews() {
    swap_chain_image_views.resize(swap_chain_images.size());

    for (size_t i = 0; i < swap_chain_images.size(); i++) {
        VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swap_chain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swap_chain_image_format,
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

        if (vkCreateImageView(device, &create_info, nullptr, &swap_chain_image_views[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swap chain image views!");
        }
    }
}

void GraphicsManager::CreateDepthResources() {
    GraphicsContext ctx = get_context();

    VkFormat depth_format = find_depth_format(physical_device);

    ImageWrapperCreateInfo create_info = {
        .width = swap_chain_extent.width,
        .height = swap_chain_extent.height,
        .format = depth_format,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .image_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .view_aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT
    };
    depth_image = std::make_unique<ImageWrapper>(create_info, ctx);
    depth_image->TransitionToLayout(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, ctx);
}

void GraphicsManager::CreateRenderPass() {
    VkAttachmentDescription color_attachment = {
        .format = swap_chain_image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depth_attachment = {
        .format = find_depth_format(physical_device),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
        .pDepthStencilAttachment = &depth_attachment_ref
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    std::array<VkAttachmentDescription, 2> attachments = {
        color_attachment,
        depth_attachment
    };

    VkRenderPassCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    if (vkCreateRenderPass(device, &create_info, nullptr, &render_pass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void GraphicsManager::CreateGraphicsPipeline() {
    // ~~~ shader stages ~~~

    VkShaderModule vert_shader = shaders_create_module_from_file("shaders/vertex.spv", device);
    VkShaderModule frag_shader = shaders_create_module_from_file("shaders/pixel.spv", device);

    VkPipelineShaderStageCreateInfo vert_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_shader,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo frag_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_shader,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_state_create_info, frag_state_create_info};

    // ~~~ dynamic states ~~~

    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data()
    };

    // ~~~ vertex input ~~~

    auto binding_desc = vertex_get_binding_desc();
    auto attribute_descs = vertex_get_attribute_descs();

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_desc,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descs.size()),
        .pVertexAttributeDescriptions = attribute_descs.data()
    };

    // ~~~ input assembly ~~~

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // ~~~ viewports and scissors ~~~

    //* viewports/scissors are created dynamically!
    //*   as per dynamic states defined above <3

    // these will be set later before drawing!
    VkPipelineViewportStateCreateInfo viewport_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    // ~~~ rasterizer ~~~

    VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    // ~~~ multisampling ~~~

    VkPipelineMultisampleStateCreateInfo multisample_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    // ~~~ depth/stencil buffers ~~~

    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        // optional depth bounds testing (disabled here)
        .depthBoundsTestEnable = VK_FALSE,
        // optional stencil testing (disabled here)
        .stencilTestEnable = VK_FALSE,
        // parameters for above mentioned testing
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f
    };

    // ~~~ color blending ~~~

    // alpha blending!
    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = VK_FALSE,

        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,

        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo color_blend_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment
    };

    // ~~~ pipeline layout (UNIFORMS!!! :D) ~~~

    std::array<VkPushConstantRange, 2> push_constant_ranges = {
        (VkPushConstantRange) {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(CameraPushConstants)
        },
        (VkPushConstantRange) {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = (sizeof(CameraPushConstants) / 16) * 16,
            .size = sizeof(PixelPushConstants)
        }
    };

    VkPipelineLayoutCreateInfo layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout,
        .pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size()),
        .pPushConstantRanges = push_constant_ranges.data()
    };

    if (vkCreatePipelineLayout(device, &layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // ~~~ CREATE GRAPHICS PIPELINE ITSELF!!!! ~~~

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_create_info,
        .pInputAssemblyState = &input_assembly_create_info,
        .pViewportState = &viewport_create_info,
        .pRasterizationState = &rasterizer_create_info,
        .pMultisampleState = &multisample_create_info,
        .pDepthStencilState = &depth_stencil_create_info,
        .pColorBlendState = &color_blend_create_info,
        .pDynamicState = &dynamic_state_create_info,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0
    };

    if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_create_info, nullptr, &graphics_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    // ~~~ clean up shaders ~~~

    vkDestroyShaderModule(device, vert_shader, nullptr);
    vkDestroyShaderModule(device, frag_shader, nullptr);
}

void GraphicsManager::CreateFramebuffers() {
    swap_chain_framebuffers.resize(swap_chain_image_views.size());

    for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swap_chain_image_views[i],
            depth_image->get_view()
        };

        VkFramebufferCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = swap_chain_extent.width,
            .height = swap_chain_extent.height,
            .layers = 1
        };

        if (vkCreateFramebuffer(device, &create_info, nullptr, &swap_chain_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void GraphicsManager::CreateCommandPool() {
    QueueFamilyIndices indices = find_queue_families(physical_device, surface);

    VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = indices.graphics.value()
    };

    if (vkCreateCommandPool(device, &create_info, nullptr, &command_pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void GraphicsManager::CreateCommandBuffers() {
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void GraphicsManager::CreateSyncObjects() {
    image_available_sempahores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        // we create signaled so the very first frame doesn't lock up lol
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (
            vkCreateSemaphore(device, &semaphore_create_info, nullptr, &image_available_sempahores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fence_create_info, nullptr, &in_flight_fences[i]) != VK_SUCCESS
        ) {
            throw std::runtime_error("Failed to create sync objects for a frame!");
        }
    }

    // we make one semaphore for every single swap
    //   chain image rather than each frame in flight
    render_finished_semaphores.resize(swap_chain_images.size());
    for (size_t i = 0; i < render_finished_semaphores.size(); i++) {
        if (vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create sync objects for a frame!");
        }
    }
}

void GraphicsManager::CreateDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> pool_sizes = {
        (VkDescriptorPoolSize) {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            // TODO: change the descriptor count to less than this
            //   see what happens if it doesn't match the maxSets below !
            .descriptorCount = MAX_NUM_UNIFORMS * MAX_FRAMES_IN_FLIGHT
        },
        (VkDescriptorPoolSize) {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = MAX_NUM_UNIFORMS * MAX_FRAMES_IN_FLIGHT
        },
    };

    VkDescriptorPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = MAX_NUM_UNIFORMS * MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };

    VkResult result = vkCreateDescriptorPool(device, &create_info, nullptr, &descriptor_pool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create descriptor pool! result: " +
            std::to_string(static_cast<int32_t>(result))
        );
    }
}

void GraphicsManager::CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding ubo_layout_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        // THIS REPRESENTS NUMBER OF VALUES IN A POSSIBLE UBO ARRAY...
        //   TODO: change this later so we can pass in a ton of matrix data
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };

    VkDescriptorSetLayoutBinding sampler_layout_binding = {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    };

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
        ubo_layout_binding,
        sampler_layout_binding
    };

    VkDescriptorSetLayoutCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    if (vkCreateDescriptorSetLayout(device, &create_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void GraphicsManager::CleanupSwapChain() {
    depth_image.reset();

    for (auto framebuffer : swap_chain_framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto image_view : swap_chain_image_views) {
        vkDestroyImageView(device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(device, swap_chain, nullptr);
}

void GraphicsManager::RecreateSwapChain() {
    // handle minimization (when dimensions are zero)
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    CleanupSwapChain();

    CreateSwapChain();
    CreateImageViews();
    CreateDepthResources();
    CreateFramebuffers();
}

void GraphicsManager::Begin() {
    // ~~~ resetting things from last frame ~~~

    vkWaitForFences(device, 1, &in_flight_fences[frame_flight_index], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(
        device,
        swap_chain,
        UINT64_MAX,
        image_available_sempahores[frame_flight_index],
        nullptr,
        &swap_chain_image_index
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire next swapchain image!");
    }

    // only reset things if we're submitting work
    vkResetFences(device, 1, &in_flight_fences[frame_flight_index]);
    vkResetCommandBuffer(command_buffers[frame_flight_index], 0);

    // ~~~ recording command buffer <3 ~~~

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    if (vkBeginCommandBuffer(command_buffers[frame_flight_index], &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer recording!");
    }

    std::array<VkClearValue, 2> clear_values = {
        clear_value,
        // always clear depth to white (furthest away from camera)
        (VkClearValue) {.depthStencil = {1.0f, 0}}
    };

    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = swap_chain_framebuffers[swap_chain_image_index],
        .renderArea = {
            .offset = {0, 0},
            .extent = swap_chain_extent
        },
        .clearValueCount = static_cast<uint32_t>(clear_values.size()),
        .pClearValues = clear_values.data()
    };

    vkCmdBeginRenderPass(command_buffers[frame_flight_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffers[frame_flight_index], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

    // dynamic state things !!! :D

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swap_chain_extent.width),
        .height = static_cast<float>(swap_chain_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffers[frame_flight_index], 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = swap_chain_extent
    };
    vkCmdSetScissor(command_buffers[frame_flight_index], 0, 1, &scissor);
}

void GraphicsManager::EndAndPresent() {
    // ~~~ end recording ~~~

    vkCmdEndRenderPass(command_buffers[frame_flight_index]);

    if (vkEndCommandBuffer(command_buffers[frame_flight_index]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer recording!");
    }

    // ~~~ submitting queue ~~~

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image_available_sempahores[frame_flight_index],
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffers[frame_flight_index],
        .signalSemaphoreCount = 1,
        // we use more semaphores here! one for each swap
        //  chain image to keep them entirely separate
        .pSignalSemaphores = &render_finished_semaphores[swap_chain_image_index]
    };

    if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[frame_flight_index]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer to graphics queue!");
    }

    // ~~~ presenting itself!!!!!!! ~~~

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &render_finished_semaphores[swap_chain_image_index],
        .swapchainCount = 1,
        .pSwapchains = &swap_chain,
        .pImageIndices = &swap_chain_image_index,
        .pResults = nullptr
    };

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
        framebuffer_resized = false;
        RecreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    frame_flight_index = (frame_flight_index + 1) % MAX_FRAMES_IN_FLIGHT;
    for (size_t i = 0; i < uniforms.size(); i++) {
        uniforms[i]->NextIndex();
    }
}

std::shared_ptr<UniformWrapper> GraphicsManager::MakeNewUniform(VkImageView image_view, VkSampler sampler) {
    if (uniforms.size() >= MAX_NUM_UNIFORMS) {
        return nullptr;
    }

    UniformWrapperCreateInfo create_info = {
        .frame_flight_count = MAX_FRAMES_IN_FLIGHT,
        .layout = descriptor_set_layout,
        .pool = descriptor_pool,
        .image_view = image_view,
        .sampler = sampler
    };

    auto uniform = std::make_shared<UniformWrapper>(
        create_info,
        get_context()
    );

    uniforms.push_back(uniform);

    return uniform;
}

void GraphicsManager::CmdBindUniform(std::shared_ptr<UniformWrapper> uniform) {
    VkDescriptorSet set = uniform->get_descriptor_set();
    vkCmdBindDescriptorSets(
        command_buffers[frame_flight_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout,
        0,
        1,
        &set,
        0,
        nullptr
    );
}

void GraphicsManager::CmdPushConstants(const void* data, size_t data_size, VkShaderStageFlags shader_stage, uint32_t offset) {
    vkCmdPushConstants(
        command_buffers[frame_flight_index],
        pipeline_layout,
        shader_stage,
        offset,
        data_size,
        data
    );
}

#pragma endregion
