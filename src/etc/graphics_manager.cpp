#include "etc/graphics_manager.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstring>
#include <string>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include "../shader_helper.h"
#include "vk_utils.h"

constexpr bool ENABLE_VALIDATION_LAYERS = true;
const std::vector<const char*> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    auto manager = reinterpret_cast<RenderThing::GraphicsManager*>(glfwGetWindowUserPointer(window));
    manager->mark_resized();
}

namespace RenderThing {
    GraphicsManager::GraphicsManager(const GraphicsManagerCreateInfo& create_info)
      : physical_device(nullptr),
        window(create_info.window),
        framebuffer_resized(false),
        clear_value(create_info.clear_value) {
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
        CreateApiObjects(create_info);
        CreateRenderObjects(create_info);
        CreateCommandPool(create_info);
        CreateSyncAndFrameData(create_info);
    }

    GraphicsManager::~GraphicsManager() {
        destruction_queue.Flush();
    }

    void GraphicsManager::CreateApiObjects(const GraphicsManagerCreateInfo& create_info) {
        // create instance
        instance = std::make_unique<Instance>(create_info.instance);
        destruction_queue.QueueDelete([this] { instance.reset(); });

        // create window surface
        if (glfwCreateWindowSurface(instance->get_instance(), window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }
        destruction_queue.QueueDelete([this] {
            vkDestroySurfaceKHR(instance->get_instance(), surface, nullptr);
        });

        // pick physical device
        {
            uint32_t device_count = 0;
            vkEnumeratePhysicalDevices(instance->get_instance(), &device_count, nullptr);

            if (device_count == 0) {
                throw std::runtime_error("Failed to find GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> devices(device_count);
            vkEnumeratePhysicalDevices(instance->get_instance(), &device_count, devices.data());

            for (const auto& device : devices) {
                if (Utils::is_device_suitable(device, surface, DEVICE_EXTENSIONS.data(), DEVICE_EXTENSIONS.size())) {
                    physical_device = device;
                    break;
                }
            }

            if (physical_device == nullptr) {
                throw std::runtime_error("Failed to find a suitable GPU!");
            }
        }

        // create logical device
        {
            QueueFamilyIndices indices = Utils::find_queue_families(physical_device, surface);

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

            VkDeviceCreateInfo device_create_info = {
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
                device_create_info.enabledLayerCount = create_info.instance.validation_layer_count;
                device_create_info.ppEnabledLayerNames = create_info.instance.validation_layers;
            }

            if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create logical device!");
            }

            // aaaaand store the queues from the device we just created :D
            vkGetDeviceQueue(device, indices.graphics.value(), 0, &graphics_queue);
            vkGetDeviceQueue(device, indices.present.value(), 0, &present_queue);
        }
        destruction_queue.QueueDelete([this] { vkDestroyDevice(device, nullptr); });
    }

    void GraphicsManager::CreateRenderObjects(const GraphicsManagerCreateInfo& create_info) {
        // create render pass
        {
            VkAttachmentDescription color_attachment = {
                .format = create_info.swap_chain.surface_format.format,
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
                .format = create_info.swap_chain.depth_format,
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

            RenderPassCreateInfo render_pass_info = {
                .attachments = attachments.data(),
                .attachment_count = static_cast<uint32_t>(attachments.size()),
                .subpasses = &subpass,
                .subpass_count = 1,
                .dependencies = &dependency,
                .dependency_count = 1
            };

            render_pass = std::make_unique<RenderPass>(render_pass_info, get_api_context());
        }
        destruction_queue.QueueDelete([this] { render_pass.reset(); });

        // create swapchain
        {
            swap_chain_create_info = create_info.swap_chain;
            swap_chain_create_info.render_pass = render_pass->get_render_pass();
            swap_chain = std::make_unique<SwapChain>(
                swap_chain_create_info,
                get_graphics_context(),
                get_api_context()
            );
        }
        destruction_queue.QueueDelete([this] { swap_chain.reset(); });

        // create graphics pipeline
        {
            pipeline = std::make_unique<GraphicsPipeline>(create_info.graphics_pipeline, get_api_context());
        }
        destruction_queue.QueueDelete([this] { pipeline.reset(); });
    }

    void GraphicsManager::CreateCommandPool(const GraphicsManagerCreateInfo& create_info) {
        QueueFamilyIndices indices = Utils::find_queue_families(physical_device, surface);

        VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = indices.graphics.value()
        };

        if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }

        destruction_queue.QueueDelete([this] {
            vkDestroyCommandPool(device, command_pool, nullptr);
        });
    }

    void GraphicsManager::CreateSyncAndFrameData(const GraphicsManagerCreateInfo& create_info) {
        VkSemaphoreCreateInfo semaphore_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        // create frame datas
        {
            VkFenceCreateInfo fence_create_info = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                // we create signaled so the very first frame doesn't lock up lol
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
            };

            VkCommandBufferAllocateInfo alloc_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = command_pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1
            };

            frame_datas.resize(create_info.swap_chain.frame_flight_count);
            for (size_t i = 0; i < frame_datas.size(); i++) {
                // ~~~ sync objects ~~~
                VkResult semaphore_result = vkCreateSemaphore(
                    device,
                    &semaphore_create_info,
                    nullptr,
                    &frame_datas[i].image_available_semaphore
                );

                VkResult fence_result = vkCreateFence(
                    device,
                    &fence_create_info,
                    nullptr,
                    &frame_datas[i].in_flight_fence
                );

                if (semaphore_result != VK_SUCCESS || fence_result != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create sync objects for a frame!");
                }

                // ~~~ command buffer ~~~
                if (vkAllocateCommandBuffers(device, &alloc_info, &frame_datas[i].command_buffer) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to allocate command buffer for a frame!");
                }
            }
        }
        destruction_queue.QueueDelete([this] {
            for (auto& data : frame_datas) {
                // only destroy sync objects, command buffers
                //   are automatically freed with command pool
                vkDestroySemaphore(device, data.image_available_semaphore, nullptr);
                vkDestroyFence(device, data.in_flight_fence, nullptr);
            }
        });

        // create render finished semaphores
        {
            // we make one semaphore for every single swap
            //   chain image rather than each frame in flight
            render_finished_semaphores.resize(swap_chain->get_image_count());
            for (size_t i = 0; i < render_finished_semaphores.size(); i++) {
                if (vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create sync objects for a frame!");
                }
            }
        }
        destruction_queue.QueueDelete([this] {
            for (auto& semaphore : render_finished_semaphores) {
                vkDestroySemaphore(device, semaphore, nullptr);
            }
        });
    }

    /*
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

        // viewports/scissors are created dynamically!
        //   as per dynamic states defined above <3

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

        VkDescriptorSetLayout layout = descriptor_set_layout->get_layout();
        VkPipelineLayoutCreateInfo layout_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &layout,
            .pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size()),
            .pPushConstantRanges = push_constant_ranges.data()
        };

        // ~~~ CREATE GRAPHICS PIPELINE ITSELF!!!! ~~~

        GraphicsPipelineCreateInfo pipeline_create_info = {
            .shader_stages = shader_stages,
            .shader_stage_count = 2,
            .vertex_input = &vertex_input_create_info,
            .input_assembly = &input_assembly_create_info,
            .viewport = &viewport_create_info,
            .rasterizer = &rasterizer_create_info,
            .multisample = &multisample_create_info,
            .depth_stencil = &depth_stencil_create_info,
            .color_blend = &color_blend_create_info,
            .dynamic_state = &dynamic_state_create_info,
            .layout_create_info = &layout_create_info,
            .render_pass = render_pass->get_render_pass(),
            .subpass_index = 0
        };

        pipeline = std::make_unique<GraphicsPipeline>(pipeline_create_info, get_api_context());

        // ~~~ clean up shaders ~~~

        vkDestroyShaderModule(device, vert_shader, nullptr);
        vkDestroyShaderModule(device, frag_shader, nullptr);
    }

    */

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

        swap_chain.reset();

        swap_chain_create_info.extent.width = static_cast<uint32_t>(width);
        swap_chain_create_info.extent.height = static_cast<uint32_t>(height);
        swap_chain = std::make_unique<SwapChain>(
            swap_chain_create_info,
            get_graphics_context(),
            get_api_context()
        );
    }

    void GraphicsManager::Begin() {
        // ~~~ resetting things from last frame ~~~

        uint32_t frame_index = swap_chain->get_frame_index();

        VkSemaphore image_available_semaphore = frame_datas[frame_index].image_available_semaphore;
        VkFence in_flight_fence = frame_datas[frame_index].in_flight_fence;

        vkWaitForFences(
            device,
            1,
            &in_flight_fence,
            VK_TRUE,
            UINT64_MAX
        );

        VkResult result = swap_chain->NextImage(image_available_semaphore, nullptr);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire next swapchain image!");
        }

        VkCommandBuffer command_buffer = frame_datas[frame_index].command_buffer;

        // only reset things if we're submitting work
        vkResetFences(device, 1, &in_flight_fence);
        vkResetCommandBuffer(command_buffer, 0);

        // ~~~ recording command buffer <3 ~~~

        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0,
            .pInheritanceInfo = nullptr
        };

        if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin command buffer recording!");
        }

        std::array<VkClearValue, 2> clear_values = {
            clear_value,
            // always clear depth to white (furthest away from camera)
            (VkClearValue) {.depthStencil = {1.0f, 0}}
        };

        VkRenderPassBeginInfo render_pass_begin_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass->get_render_pass(),
            .framebuffer = swap_chain->get_current_framebuffer(),
            .renderArea = {
                .offset = {0, 0},
                .extent = swap_chain->get_extent()
            },
            .clearValueCount = static_cast<uint32_t>(clear_values.size()),
            .pClearValues = clear_values.data()
        };

        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_pipeline());

        // dynamic state things !!! :D

        VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(swap_chain->get_extent().width),
            .height = static_cast<float>(swap_chain->get_extent().height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor = {
            .offset = {0, 0},
            .extent = swap_chain->get_extent()
        };
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    }

    void GraphicsManager::EndAndPresent() {
        // ~~~ end recording ~~~

        uint32_t frame_index = swap_chain->get_frame_index();
        uint32_t image_index = swap_chain->get_image_index();
        VkCommandBuffer command_buffer = frame_datas[frame_index].command_buffer;
        VkSemaphore image_available_semaphore = frame_datas[frame_index].image_available_semaphore;
        VkFence in_flight_fence = frame_datas[frame_index].in_flight_fence;

        vkCmdEndRenderPass(command_buffer);

        if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to end command buffer recording!");
        }

        // ~~~ submitting queue ~~~

        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            // image available semaphores are linked to frame flight .
            //   once it's available we will use swapchain and its image
            //   index with inner systems
            .pWaitSemaphores = &image_available_semaphore,
            .pWaitDstStageMask = wait_stages,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 1,
            // we use more semaphores here! one for each swap
            //  chain image to keep them entirely separate
            .pSignalSemaphores = &render_finished_semaphores[image_index]
        };

        if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fence) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer to graphics queue!");
        }

        // ~~~ presenting itself!!!!!!! ~~~

        VkSwapchainKHR sc = swap_chain->get_swap_chain();
        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_finished_semaphores[image_index],
            .swapchainCount = 1,
            .pSwapchains = &sc,
            .pImageIndices = &image_index,
            .pResults = nullptr
        };

        VkResult result = vkQueuePresentKHR(present_queue, &present_info);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
            framebuffer_resized = false;
            RecreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        swap_chain->NextFrame();
    }

    VkCommandBuffer GraphicsManager::get_command_buffer() const { return frame_datas[swap_chain->get_frame_index()].command_buffer; }
    VkDevice GraphicsManager::get_device() const { return device; }
    VkPhysicalDevice GraphicsManager::get_physical_device() const { return physical_device; }
    VkClearValue GraphicsManager::get_clear_value() const { return clear_value; }
    VkCommandPool GraphicsManager::get_command_pool() const { return command_pool; }
    VkQueue GraphicsManager::get_graphics_queue() const { return graphics_queue; }
    VkQueue GraphicsManager::get_present_queue() const { return present_queue; }
    VkExtent2D GraphicsManager::get_swapchain_extent() const { return swap_chain->get_extent(); }
    ApiContext GraphicsManager::get_api_context() const {
        ApiContext ctx = {
            .instance = instance->get_instance(),
            .device = device,
            .physical_device = physical_device,
            .window = window,
            .surface = surface
        };

        return ctx;
    }
    GraphicsContext GraphicsManager::get_graphics_context() const {
        uint32_t i = 0;
        if (swap_chain != nullptr) {
            i = swap_chain->get_image_index();
        }

        GraphicsContext ctx = {
            .graphics_queue = graphics_queue,
            .command_pool = command_pool,
            .frame_command_buffer = frame_datas[i].command_buffer
        };

        return ctx;
    }
    float GraphicsManager::get_aspect() const {
        VkExtent2D extent = swap_chain->get_extent();
        return extent.width / (float)extent.height;
    }
    void GraphicsManager::set_clear_value(VkClearValue clear_value) { this->clear_value = clear_value; }
    void GraphicsManager::mark_resized() { framebuffer_resized = true; }
}
