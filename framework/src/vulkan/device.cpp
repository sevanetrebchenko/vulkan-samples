
#include "vks/vulkan/device.hpp"
#include "vks/vulkan/descriptor_set.hpp"
#include "vks/vulkan/utility.hpp"
#include "vks/detail/shader_compiler.hpp"

#include "utils/logging.hpp"
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>

#include <fstream> // std::ifstream

namespace vks {

    VKAPI_ATTR VkBool32 VKAPI_CALL process_vulkan_message(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
        switch (severity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                // Behavior that is not necessarily an error, but very likely a bug
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                // Behavior that is invalid and may cause crashes
                break;
            default:
                break;
        }

        switch (type) {
            case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
                // General, unrelated to specification or performance
                break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
                // Violation of the specification, indicates possible mistakes
                break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
                // Performance, non-optimal use of Vulkan
                break;
            default:
                break;
        }

        // utils::logging::info(callback_data->pMessage);

        return VK_FALSE; // Whether the Vulkan call that triggered the validation layer message should be aborted
    }

    DeviceDescription::DeviceDescription() : width(1920),
                                             height(1080),
                                             name("My Vulkan Application"),
                                             enabled_features(),
                                             instance_extensions(),
                                             device_extensions() {
    }

    DeviceDescription::~DeviceDescription() {
    }

    DeviceDescription& DeviceDescription::set_width(unsigned int _width) {
        width = _width;
        return *this;
    }

    DeviceDescription& DeviceDescription::set_height(unsigned int _height) {
        height = _height;
        return *this;
    }

    DeviceDescription& DeviceDescription::set_extent(unsigned int _width, unsigned int _height) {
        width = _width;
        height = _height;
        return *this;
    }

    DeviceDescription& DeviceDescription::enable_instance_extension(const char* _name) {
        bool found = false;

        for (const char* extension : instance_extensions) {
            if (strcmp(extension, _name) == 0) {
                found = true;
                break;
            }
        }

        // Only keep unique extension names
        if (!found) {
            instance_extensions.emplace_back(_name);
        }

        return *this;
    }

    DeviceDescription& DeviceDescription::enable_device_extension(const char* _name) {
        bool found = false;

        for (const char* extension : device_extensions) {
            if (strcmp(extension, _name) == 0) {
                found = true;
                break;
            }
        }

        // Only keep unique extension names
        if (!found) {
            device_extensions.emplace_back(_name);
        }

        return *this;
    }

    DeviceDescription& DeviceDescription::enable_features(VkPhysicalDeviceFeatures features) {
        enabled_features = features;
        return *this;
    }

    DeviceDescription& DeviceDescription::set_application_name(const char* _name) {
        name = _name;
        return *this;
    }

    // Device implementation

    std::shared_ptr<Device> Device::instance() {
        static std::shared_ptr<Device> device = std::make_shared<Device>();
        return device;
    }

    Device::Device() : vulkan_instance(VK_NULL_HANDLE) {
    }

    Device::~Device() {
    }

    void Device::initialize(const DeviceDescription& device_description) {
        // Query for validation layer support

        unsigned validation_layer_count;
        vkEnumerateInstanceLayerProperties(&validation_layer_count, nullptr);

        std::vector<VkLayerProperties> validation_layers(validation_layer_count);
        vkEnumerateInstanceLayerProperties(&validation_layer_count, validation_layers.data());

        // 'VK_LAYER_KHRONOS_validation' validation layer contains all validation functionality
        const char* validation_layer = "VK_LAYER_KHRONOS_validation";
        bool is_validation_supported = false;

        for (const VkLayerProperties& current : validation_layers) {
            if (std::strcmp(validation_layer, current.layerName) == 0) {
                is_validation_supported = true;
                break;
            }
        }

        // Query all supported instance extensions

        unsigned global_instance_extension_count;
        vkEnumerateInstanceExtensionProperties(nullptr, &global_instance_extension_count, nullptr);

        // Take into account extensions exposed by Vulkan validation layers
        unsigned validation_layer_instance_extension_count = 0;
        if (is_validation_supported) {
            vkEnumerateInstanceExtensionProperties(validation_layer, &validation_layer_instance_extension_count, nullptr);
        }

        std::vector<VkExtensionProperties> supported_instance_extensions(global_instance_extension_count + validation_layer_instance_extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &global_instance_extension_count, &supported_instance_extensions[0]);

        if (is_validation_supported) {
            vkEnumerateInstanceExtensionProperties(validation_layer, &validation_layer_instance_extension_count, &supported_instance_extensions[global_instance_extension_count]);
        }

        std::vector<const char*> instance_extensions = device_description.instance_extensions;

        // Enable debugging on debug builds
        #ifndef NDEBUG
            instance_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #endif

        // Applications presenting to the screen require the VK_KHR_surface extension for window surface support
        instance_extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

        // Enable required GLFW extensions
        unsigned glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        for (unsigned i = 0; i < glfw_extension_count; ++i) {
            instance_extensions.emplace_back(glfw_extensions[i]);
        }

        unsigned unsupported_extension_count = 0;

        for (const char* requested : device_description.instance_extensions) {
            bool is_extension_supported = false;

            for (const VkExtensionProperties& supported : supported_instance_extensions) {
                if (strcmp(requested, supported.extensionName) == 0) {
                    is_extension_supported = true;
                    break;
                }
            }

            if (!is_extension_supported) {
                utils::logging::error("'{}' instance extension not found", requested);
                ++unsupported_extension_count;
            }
        }

        if (unsupported_extension_count) {
            // Instance creation will fail if any requested extensions are not supported
            std::string error = utils::format("Failed to create Vulkan instance - {} requested extension(s) not supported", unsupported_extension_count);
            utils::logging::error(error);
            throw std::runtime_error(error);
        }

        // Create Vulkan instance

        VkApplicationInfo application_info {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "device_description.name",
            .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .pEngineName = "vks",
            .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3
        };

        VkInstanceCreateInfo instance_create_info {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &application_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = (unsigned) instance_extensions.size(),
            .ppEnabledExtensionNames = instance_extensions.data()
        };

        VkResult result;
        if (is_validation_supported) {
            // Enable Vulkan validation layers
            instance_create_info.enabledLayerCount = 1;
            instance_create_info.ppEnabledLayerNames = &validation_layer;

            // Initialize Vulkan debug messenger
            VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .pNext = nullptr,
                .flags = 0,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = process_vulkan_message,
                .pUserData = nullptr
            };

            // VkDebugUtilsMessengerCreateInfoEXT struct is passed into the pNext chain of VkInstanceCreateInfo in order to be able to debug instance creation / destruction
            // This debug messenger is attached to the instance and will get cleaned up alongside it
            instance_create_info.pNext = &debug_messenger_create_info;

            result = vkCreateInstance(&instance_create_info, nullptr, &vulkan_instance);
            if (result != VK_SUCCESS) {
                std::string error = utils::format("Failed to create Vulkan instance (error code: {})", result);
                utils::logging::error(error);
                throw std::runtime_error("aa");
            }

            // Create the actual VkDebugUtilsMessengerEXT to debug all other Vulkan API calls
            // vkCreateDebugUtilsMessenger function is not loaded by default
            static PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vulkan_instance, "vkCreateDebugUtilsMessengerEXT");
            if (!vkCreateDebugUtilsMessenger) {
                std::string error = "Failed to load VkDebugUtilsMessengerEXT (is the 'VK_EXT_debug_utils' instance extension enabled?)";
                utils::logging::error(error);
                throw std::runtime_error(error);
            }

            result = vkCreateDebugUtilsMessenger(vulkan_instance, &debug_messenger_create_info, nullptr, &vulkan_debug_messenger);
            if (result != VK_SUCCESS) {
                std::string error = utils::format("Failed to create debug messenger (VkDebugUtilsMessengerEXT) (error code: {})", result);
                utils::logging::error(error);
                throw std::runtime_error(error);
            }
        }
        else {
             utils::logging::warning("'VK_LAYER_KHRONOS_validation' validation layer is not supported, API validation is disabled");

            result = vkCreateInstance(&instance_create_info, nullptr, &vulkan_instance);
            if (result != VK_SUCCESS) {
                std::string error = utils::format("Failed to create Vulkan instance (error code: {})", result);
                utils::logging::error(error);
                throw std::runtime_error(error);
            }
        }
        
        // Create Vulkan device
        
        
    }

    void Device::shutdown() {
    }


    
    Uniform reflect_member(const SpvReflectBlockVariable& var) {
        Uniform uniform {
            .type = UniformType::Integer,
            .name = var.name,
            .offset = var.absolute_offset,
            .size = var.size
        };
        
        unsigned member_count = var.member_count;
        if (member_count) {
            // Uniform is a struct type
            uniform.type = UniformType::Struct;
            
            uniform.members.resize(member_count);
            for (unsigned i = 0; i < member_count; ++i) {
                uniform.members[i] = reflect_member(var.members[i]);
            }
        }
        else if (var.type_description->traits.array.dims_count) {
            // Uniform is an array type
            unsigned dimensions_count = var.type_description->traits.array.dims_count;
            
            uniform.dimensions = dimensions_count;
            uniform.stride = var.type_description->traits.array.stride;
            
            uniform.capacity.resize(dimensions_count);
            for (unsigned i = 0; i < dimensions_count; ++i) {
                uniform.capacity[i] = var.type_description->traits.array.dims[i];
            }
        }
        
        return std::move(uniform);
    }
    
    std::shared_ptr<GraphicsPipeline> Device::create_graphics_pipeline(GraphicsPipelineDescription pipeline_description) {
        // A Vulkan graphics pipeline can have up to 5 shader stages
        ShaderModule shader_modules[5] = { };
        for (unsigned stage = 0; stage < 5; ++stage) {
            const ShaderStageDescription& shader_stage = pipeline_description.shader_stages[stage];
            if (shader_stage.stage == ShaderStage::None) {
                continue;
            }
            
            shader_modules[stage] = compile_shader(vulkan_device, shader_stage);
        }

        // The vertex shader is the only required stage
        unsigned vertex_stage_index = to_pipeline_index(ShaderStage::Vertex);
        bool has_vertex_shader = pipeline_description.shader_stages[vertex_stage_index].stage != ShaderStage::None;
        
        if (!has_vertex_shader) {
            std::string error = "Failed to create graphics pipeline - vertex shader stage is required";
            utils::logging::error(error);
            throw std::runtime_error(error);
        }
        
        const ShaderModule& vertex_shader_module = shader_modules[vertex_stage_index];
        for (unsigned i = 0; i < vertex_shader_module.spv_module.input_variable_count; ++i) {
            SpvReflectInterfaceVariable* input_variable = vertex_shader_module.spv_module.input_variables[i];
            const char* name = input_variable->name;
            unsigned location = input_variable->location;
            
            bool found = false;
            
            for (VertexBinding& vertex_binding : pipeline_description.vertex_input_description.bindings) {
                for (VertexAttribute& vertex_attribute : vertex_binding.attributes) {
                    if (strcmp(vertex_attribute.name, name) == 0) {
                        vertex_attribute.location = location;
                        vertex_attribute.format = (VkFormat) input_variable->format;
                        found = true;
                    }
                }
            }
            
            if (!found) {
                // All vertex attributes specified in the shader source must have a corresponding registration in the vertex input of the pipeline description
                // The opposite is not required to be true - there can be more vertex attributes specified in the pipeline description than what is used in the shader
                std::string error = utils::format("Failed to create graphics pipeline - encountered vertex attribute '{}' with no registered binding (location: {})", name, location);
                utils::logging::error(error);
                throw std::runtime_error(error);
            }
        }
        
        // Generate vertex bindings and vertex attributes
        
        // Vertex bindings specify the layout of the data used as input to the vertex shader
        std::size_t vertex_binding_count = pipeline_description.vertex_input_description.bindings.size();
        std::vector<VkVertexInputBindingDescription> vertex_bindings(vertex_binding_count);

        for (unsigned i = 0; i < vertex_binding_count; ++i) {
            const VertexBinding& vertex_binding = pipeline_description.vertex_input_description.bindings[i];
            unsigned stride = 0;
            
            if (vertex_binding.stride == (unsigned) -1) {
                // The stride of this binding is not directly specified, calculate it using the attributes referenced in the pipeline description
                for (const VertexAttribute& vertex_attribute : vertex_binding.attributes) {
                    if (vertex_attribute.location == (unsigned) -1) {
                        std::string error = utils::format("Failed to create graphics pipeline - unable to determine stride of vertex input binding {} because type of attribute '{}' is not known", vertex_binding.binding, vertex_attribute.name);
                        utils::logging::error(error);
                        throw std::runtime_error(error);
                    }
                    
                    // If the location was set, that means the vertex attribute was successfully reflected
                    stride += get_format_size(vertex_attribute.format);
                }
            }
            else {
                stride = vertex_binding.stride;
            }
            
            // Register new VkVertexInputBindingDescription
            VkVertexInputBindingDescription& binding_description = vertex_bindings[i];
            binding_description.binding = vertex_binding.binding;
            binding_description.stride = stride;
            binding_description.inputRate = (VkVertexInputRate) vertex_binding.rate;
        }
        
        // Create the vertex attribute layout as specified by the shader reflection data as this matches the expected layout of the data in the vertex buffer
        // Note: it is possible to have more vertex attributes in the vertex buffer than what is used by the shader, just as long as the attribute offsets and binding stride is configured correctly
        std::size_t vertex_attribute_count = 0;
        for (const VertexBinding& vertex_binding : pipeline_description.vertex_input_description.bindings) {
            vertex_attribute_count += vertex_binding.attributes.size();
        }

        // Vertex attributes describe how individual vertex attributes (ex. position, normal, uv, tangent) are extracted from the buffer bound at the corresponding binding point (described above)
        std::vector<VkVertexInputAttributeDescription> vertex_attributes(vertex_attribute_count);

        unsigned vertex_attribute_index = 0;
        for (const VertexBinding& vertex_binding : pipeline_description.vertex_input_description.bindings) {
            unsigned offset = 0;

            for (const VertexAttribute& vertex_attribute : vertex_binding.attributes) {
                if (vertex_attribute.location == (unsigned) -1) {
                    // Vertex attribute is not referenced in the shader source
                    // This is not an error, since for the purposes of pipeline creation we only care about vertex attributes that the shader uses
                    continue;
                }
                
                VkVertexInputAttributeDescription& vertex_attribute_description = vertex_attributes[vertex_attribute_index++];
                vertex_attribute_description.location = vertex_attribute.location;
                vertex_attribute_description.binding = vertex_binding.binding;
                vertex_attribute_description.format = vertex_attribute.format;
                vertex_attribute_description.offset = offset;

                offset += get_format_size(vertex_attribute.format);
            }
        }

        VkPipelineVertexInputStateCreateInfo vertex_input_state {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = (unsigned) vertex_bindings.size(),
            .pVertexBindingDescriptions = &vertex_bindings[0],
            .vertexAttributeDescriptionCount = (unsigned) vertex_attributes.size(),
            .pVertexAttributeDescriptions = &vertex_attributes[0]
        };

        struct DescriptorSet {
            unsigned index;
            const SpvReflectDescriptorSet* set;
            VkShaderStageFlags stages;
        };
        
        // Retrieve the reflected descriptor set information
        std::vector<DescriptorSet> descriptor_sets;
        
//        for (unsigned stage = 0; stage < 5; ++stage) {
//            const ShaderModule* shader_module = shader_modules[stage];
//            if (!shader_module) {
//                continue;
//            }
//
//            for (unsigned i = 0; i < shader_module->reflection_data.descriptor_set_count; ++i) {
//                const SpvReflectDescriptorSet& descriptor_set = shader_module->reflection_data.descriptor_sets[i];
//                unsigned index = descriptor_set.set;
//
//                // Only add unique set indices
//                bool found = false;
//                for (const DescriptorSet& current : descriptor_sets) {
//                    if (current.index == index) {
//                        found = true;
//                        break;
//                    }
//                }
//
//                if (!found) {
//                    DescriptorSet set {
//                        .index = index,
//                        .set = &descriptor_set,
//                        .stages = (VkShaderStageFlags) shader_module->reflection_data.shader_stage
//                    };
//                    descriptor_sets.emplace_back(set);
//                }
//            }
//        }

        unsigned descriptor_set_count = descriptor_sets.size();
        
        // Generate descriptor set bindings
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts(descriptor_set_count);
        
        for (unsigned i = 0; i < descriptor_set_count; ++i) {
            const DescriptorSet& descriptor_set = descriptor_sets[i];

            // Create descriptor set layout
            unsigned descriptor_binding_count = descriptor_set.set->binding_count;
            std::vector<VkDescriptorSetLayoutBinding> descriptor_bindings(descriptor_binding_count);

            for (unsigned j = 0; j < descriptor_binding_count; ++j) {
                const SpvReflectDescriptorBinding* descriptor_binding = descriptor_set.set->bindings[j];
                
                VkDescriptorType type = (VkDescriptorType) descriptor_binding->descriptor_type;
                
                // Create VkDescriptorSetLayoutBinding for descriptor set layout creation
                descriptor_bindings[j].binding = descriptor_binding->binding;
                descriptor_bindings[j].descriptorType = type;
                descriptor_bindings[j].descriptorCount = descriptor_binding->count;
                descriptor_bindings[j].stageFlags = descriptor_set.stages;
                
                // Immutable samplers are bound directly to the descriptor set layout and do not change
                // Only applicable to descriptor bindings of type SAMPLER or COMBINED_SAMPLER
                descriptor_bindings[j].pImmutableSamplers = nullptr;
                
                // Reflect uniform data
                std::vector<Uniform> uniforms;
                
                if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
                    unsigned uniform_count = descriptor_binding->block.member_count;
                    uniforms.resize(uniform_count);
                    
                    for (unsigned k = 0; k < uniform_count; ++k) {
                        uniforms[k] = reflect_member(descriptor_binding->block.members[k]);
                    }
                }
            }
            
            VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .bindingCount = descriptor_binding_count,
                .pBindings = &descriptor_bindings[0]
            };
            
//            VkResult result = vkCreateDescriptorSetLayout(vulkan_device, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layouts[i]);
//            if (result != VK_SUCCESS) {
//                // TODO: throw;
//            }
        }
        
//        VkPipelineLayout pipeline_layout { };
//        result = vkCreatePipelineLayout(vulkan_device, &pipeline_layout_create_info, nullptr, &pipeline_layout);
//        if (result != VK_SUCCESS) {
//        }
        
        VkGraphicsPipelineCreateInfo pipeline_create_info {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 0,
            .pStages = nullptr,
//            .pVertexInputState = &vertex_input_state,
            .pInputAssemblyState = nullptr,
            .pTessellationState = nullptr,
            .pViewportState = nullptr,
            .pRasterizationState = nullptr,
            .pMultisampleState = nullptr,
            .pDepthStencilState = nullptr,
            .pColorBlendState = nullptr,
            .pDynamicState = nullptr,
            .layout = { },
            .renderPass = { },
            .subpass = 0,
            .basePipelineHandle = nullptr,
            .basePipelineIndex = 0
        };
        
//        VkPipeline pipeline { };
//        result = vkCreateGraphicsPipelines(vulkan_device, nullptr, 1, &pipeline_create_info, nullptr, &pipeline);
//        if (result != VK_SUCCESS) {
//        }
        
    }


}