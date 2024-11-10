
#include "vks/vulkan/device.hpp"
#include "vks/vulkan/utility.hpp"

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

    struct ShaderIncluder final : public shaderc::CompileOptions::IncluderInterface {
        struct Include {
            std::filesystem::path filepath;
            std::string contents;
        };

        ~ShaderIncluder() override;

        shaderc_include_result* GetInclude(const char* file, shaderc_include_type type, const char* source, std::size_t depth) override;
        void ReleaseInclude(shaderc_include_result* data) override;

        // Contents of included files must remain valid from when GetInclude is invoked to retrieve the include contents to when ReleaseInclude is invoked to release them
        std::vector<Include> include_data;
        std::vector<shaderc_include_result> include_results;
    };

    struct ShaderModule {
        VkShaderModule handle { };
        std::filesystem::file_time_type last_modified_time;
        SpvReflectShaderModule reflection_data { };
    };

    std::unordered_map<std::filesystem::path, ShaderModule> shader_cache;

    std::string load_shader(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::in);
        if (!file.is_open()) {
            throw std::runtime_error("");
        }

        // Get the length of the file
        file.seekg(0, std::ifstream::end);
        std::streamsize length = file.tellg();
        file.seekg(0, std::ifstream::beg);

        // Reading the file in line by line is slower, but avoids <bad token> errors later with preprocessing
        std::string source;
        source.reserve(length);

        std::string line;
        while (std::getline(file, line)) {
            source += line;
            source += '\n';
        }

        return std::move(source);
    }

    const ShaderModule& compile_shader(VkDevice device, const ShaderStageDescription& stage_description) {
        // Configure compile options
        shaderc::CompileOptions options { };

        #ifndef NDEBUG
            // Optimizing for performance interferes with shader reflection
            // options.SetOptimizationLevel(shaderc_optimization_level_performance);
        #else
            options.SetWarningsAsErrors();
        #endif

        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        options.SetIncluder(std::make_unique<ShaderIncluder>());

        // Add preprocessor definitions
        for (const auto& [name, value] : stage_description.preprocessor_definitions) {
            options.AddMacroDefinition(name, value);
        }

        // Using Vulkan GLSL
        shaderc_shader_kind type;
        switch (stage_description.stage) {
            case ShaderStage::Vertex:
                type = shaderc_glsl_vertex_shader;
                break;
            case ShaderStage::TesselationControl:
                type = shaderc_glsl_tess_control_shader;
                break;
            case ShaderStage::TesselationEvaluation:
                type = shaderc_glsl_tess_evaluation_shader;
                break;
            case ShaderStage::Geometry:
                type = shaderc_glsl_geometry_shader;
                break;
            case ShaderStage::Fragment:
                type = shaderc_glsl_fragment_shader;
                break;
            case ShaderStage::Compute:
                type = shaderc_glsl_compute_shader;
                break;
            case ShaderStage::Mesh:
                type = shaderc_glsl_mesh_shader;
                break;
            case ShaderStage::Task:
                type = shaderc_glsl_task_shader;
                break;
        }

        shaderc::Compiler compiler { };

        // Shader modules should only be compiled again if they do not yet exist or if the shader source has been modified since being compiled the first time
        bool recompile = true;

        std::filesystem::file_time_type last_modified_time = std::filesystem::last_write_time(stage_description.path);
        auto iter = shader_cache.find(stage_description.path);

        if (iter != shader_cache.end()) {
            const ShaderModule& cached = iter->second;

            if (cached.last_modified_time == last_modified_time) {
                // Shader source has not been modified since the original time it was cached and is still valid
                recompile = false;
            }
        }

        if (!recompile) {
            // Shader module is guaranteed to be current
            return iter->second;
        }

        // Register new shader module
        ShaderModule& module = shader_cache[stage_description.path];
        module.last_modified_time = last_modified_time;

        std::string source = load_shader(stage_description.path);

        // Replace preprocessor definitions and resolve includes
        #if defined(PLATFORM_WINDOWS)
            // Convert std::filesystem::path::c_str() to a const char* (returns const wchar_t* on Windows)
            std::string path = stage_description.path.string();
            shaderc::PreprocessedSourceCompilationResult preprocess_result = compiler.PreprocessGlsl(source.c_str(), source.size(), type, path.c_str(), options);
        #else
            const char* path = stage_description.path.c_str();
            shaderc::PreprocessedSourceCompilationResult preprocess_result = compiler.PreprocessGlsl(source.c_str(), source.size(), type, path, options);
        #endif

        shaderc_compilation_status preprocess_status = preprocess_result.GetCompilationStatus();
        if (preprocess_status != shaderc_compilation_status_success) {
            std::string error = utils::format("Shader compilation failed with error code {} - {}", std::underlying_type<shaderc_compilation_status>::type(preprocess_status), preprocess_result.GetErrorMessage());
            utils::logging::error(error);
            throw std::runtime_error(error);
        }

        source = std::string(preprocess_result.begin(), preprocess_result.end());

        // Compile to SPIR-V bytecode

        // Assume shader entry point is 'main'
        #if defined(PLATFORM_WINDOWS)
            shaderc::SpvCompilationResult compile_result = compiler.CompileGlslToSpv(source, type, path.c_str(), options);
        #else
            shaderc::SpvCompilationResult compile_result = compiler.CompileGlslToSpv(source, type, path, options);
        #endif

        shaderc_compilation_status compile_status = compile_result.GetCompilationStatus();
        if (compile_status != shaderc_compilation_status_success) {
            std::string error = utils::format("Shader compilation failed with error code {} - {}", std::underlying_type<shaderc_compilation_status>::type(compile_status), compile_result.GetErrorMessage());
            utils::logging::error(error);
            throw std::runtime_error(error);
        }

        std::vector<unsigned> spirv { compile_result.cbegin(), compile_result.cend() };
        std::size_t size = spirv.size() * sizeof(unsigned); // Size in bytes

        // Create shader module
        VkShaderModuleCreateInfo shader_module_create_info { };
        shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_create_info.codeSize = size;
        shader_module_create_info.pCode = spirv.data();

//            VkResult result = vkCreateShaderModule(device, &shader_module_create_info, nullptr, &module.handle);
//            if (result != VK_SUCCESS) {
//                std::string error = utils::format("vkCreateShaderModule failed with error code {}", result);
//                utils::logging::error(error);
//                throw std::runtime_error(error);
//            }

        // Generate reflection data using SPIR-V bytecode
        spvReflectCreateShaderModule(size, spirv.data(), &module.reflection_data);
        return module;
    }

    ShaderIncluder::~ShaderIncluder() = default;

    shaderc_include_result* ShaderIncluder::GetInclude(const char* file, shaderc_include_type type, const char* source, std::size_t depth) {
        // Determine the global filepath of the included file
        std::filesystem::path filepath(file);
        shaderc_include_result& result = include_results.emplace_back();

        if (std::filesystem::exists(filepath)) {
            // Ensure that a shader is included only once
            bool found = false;

            for (const Include& include : include_data) {
                if (include.filepath == filepath) {
                    found = true;
                    break;
                }
            }

            std::size_t length = strlen(file);

            if (found) {
                // File has already been included and should not be duplicated
                result.source_name = file;
                result.source_name_length = length;
                result.content = nullptr;
                result.content_length = 0;
            }
            else {
                // Register new include
                Include& include = include_data.emplace_back();
                include.filepath = filepath;
                include.contents = load_shader(filepath);

                result.source_name = file;
                result.source_name_length = length;
                result.content = include.contents.c_str();
                result.content_length = include.contents.length();
            }
        }
        else {
            // For a failed inclusion, source_name should be empty
            result.source_name = nullptr;
            result.source_name_length = 0;

            // For a failed inclusion, content contains the error message
            result.content = "";
            result.content_length = strlen(result.content);
        }

        return &result;
    }

    void ShaderIncluder::ReleaseInclude(shaderc_include_result* data) {
        // Nothing to do here
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
    }

    void Device::shutdown() {
    }

    struct VertexAttribute {
        unsigned location;
        VkFormat format;
    };
    
    std::shared_ptr<GraphicsPipeline> Device::create_graphics_pipeline(const GraphicsPipelineDescription& pipeline_description) {
        // Perform pipeline validation steps
        // The only required shader stage of a graphics pipeline is the vertex shader
        const ShaderStageDescription& vertex_stage_description = pipeline_description.shader_stages[to_pipeline_index(ShaderStage::Vertex)];
        if (vertex_stage_description.stage == ShaderStage::None) {
            std::string error = "Failed to create graphics pipeline - vertex shader stage is required";
            utils::logging::error(error);
            throw std::runtime_error(error);
        }

        const ShaderModule& vertex_module = compile_shader(vulkan_device, vertex_stage_description);
        
        // Use std::string_view instead of const char* to hash (compare) on the string contents, not just the address of the pointer
        std::unordered_map<std::string_view, VertexAttribute> vertex_attribute_map;
        
        for (unsigned i = 0; i < vertex_module.reflection_data.input_variable_count; ++i) {
            SpvReflectInterfaceVariable* input_variable = vertex_module.reflection_data.input_variables[i];
            vertex_attribute_map[input_variable->name] = VertexAttribute {
                .location = input_variable->location,
                .format = (VkFormat) input_variable->format
            };
        }
        
        // All vertex attributes specified in the shader source must have a corresponding registration in the vertex input of the pipeline description
        // The opposite is not required to be true - there can be more vertex attributes specified in the pipeline description than what is used in the shader
        
        for (const auto& [name, attribute] : vertex_attribute_map) {
            bool found = false;
            
            for (const VertexBinding& binding : pipeline_description.vertex_input_description.bindings) {
                for (const char* attribute_name : binding.attribute_names) {
                    if (name == std::string_view(attribute_name) == 0) {
                        found = true;
                        break;
                    }
                }
            }
            
            if (!found) {
                std::string error = utils::format("Failed to create graphics pipeline - encountered vertex attribute '{}' with no registered binding (location: {})", name, attribute.location);
                utils::logging::error(error);
                throw std::runtime_error(error);
            }
        }
        
        // Generate vertex bindings and vertex attributes
        
        // Vertex bindings specify the layout of the data used as input to the vertex shader
        std::size_t vertex_binding_count = pipeline_description.vertex_input_description.bindings.size();
        std::vector<VkVertexInputBindingDescription> vertex_bindings(vertex_binding_count);

        for (unsigned i = 0; i < vertex_binding_count; ++i) {
            unsigned stride = 0;
            const VertexBinding& vertex_binding = pipeline_description.vertex_input_description.bindings[i];
            
            if (vertex_binding.stride == (unsigned) -1) {
                // The stride of this binding is not directly specified, calculate it using the attributes referenced in the pipeline description
                for (const char* attribute_name : vertex_binding.attribute_names) {
                    auto iter = vertex_attribute_map.find(attribute_name);
                    
                    if (iter == vertex_attribute_map.end()) {
                        std::string error = utils::format("Failed to create graphics pipeline - unable to determine stride of vertex input binding {} because type of attribute '{}' is not known", vertex_binding.binding, attribute_name);
                        utils::logging::error(error);
                        throw std::runtime_error(error);
                    }
                    
                    const VertexAttribute& vertex_attribute = iter->second;
                    stride += get_format_size(vertex_attribute.format);
                }
            }
            else {
                stride = vertex_binding.stride;
            }
            
            // Register new vertex binding
            VkVertexInputBindingDescription& binding_description = vertex_bindings[i];
            binding_description.binding = vertex_binding.binding;
            binding_description.stride = stride;
            binding_description.inputRate = (VkVertexInputRate) vertex_binding.rate;
        }
        
        // Create the vertex attribute layout as specified by the shader reflection data as this matches the expected layout of the data in the vertex buffer
        // Note: it is possible to have more vertex attributes in the vertex buffer than what is used by the shader, just as long as the attribute offsets and binding stride is configured correctly
        std::size_t vertex_attribute_count = vertex_attribute_map.size();
        
        // Vertex attributes describe how individual vertex attributes (ex. position, normal, uv, tangent) are extracted from the buffer bound at the corresponding binding point (described above)
        std::vector<VkVertexInputAttributeDescription> vertex_attributes(vertex_attribute_count);
        
        unsigned vertex_attribute_index = 0;
        for (const VertexBinding& vertex_binding : pipeline_description.vertex_input_description.bindings) {
            unsigned offset = 0;
            
            for (const char* attribute_name : vertex_binding.attribute_names) {
                auto iter = vertex_attribute_map.find(attribute_name);
                if (iter == vertex_attribute_map.end()) {
                    // Vertex attribute is not referenced in the shader source
                    // This is not an error, since for the purposes of pipeline creation we only care about vertex attributes that the shader uses
                    continue;
                }
                
                const VertexAttribute& vertex_attribute = iter->second;
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
    }


}