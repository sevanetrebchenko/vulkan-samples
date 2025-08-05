
#include "vks/vulkan/render_context.hpp"
#include "vks/sample.hpp"
#include <utils/logging.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <algorithm> // std::min

#define CHECK_CALL(FUNCTION, ...) { \
    VkResult result = FUNCTION(__VA_ARGS__); \
    if (result != VK_SUCCESS) { \
        utils::logging::fatal("{} failed with code {} ({})", #FUNCTION, static_cast<std::underlying_type_t<VkResult>>(result), string_VkResult(result)); \
    } \
}

namespace vks {
    
    void RenderContext::initialize(const Window& window, const SampleRequirements& requirements) {
        collect_instance_requirements(requirements);
        create_vulkan_instance();
        create_surface(window);
        
        collect_device_requirements(requirements);
        select_physical_device();
        create_device();
    }
    
    void RenderContext::begin_frame() {
    
    }
    
    void RenderContext::end_frame() {
    
    }
    
    void RenderContext::create_vulkan_instance() {
        VkInstanceCreateInfo instance_create_info { };
        instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    
        // A Vulkan instance represents the connection between the Vulkan API context and the application
        VkApplicationInfo application_info { };
        application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        application_info.pApplicationName = "vulkan-samples";
        application_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        application_info.pEngineName = "";
        application_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        application_info.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
        instance_create_info.pApplicationInfo = &application_info;

        #ifndef NDEBUG
            // Validation layers are disabled on non-debug builds
            bool validation_supported = validate_layer_support();
            if (validation_supported) {
                instance_create_info.ppEnabledLayerNames = m_layers.data();
                instance_create_info.enabledLayerCount = static_cast<std::uint32_t>(m_layers.size());
            }
        #else
            bool validation_supported = false;
        #endif
        
        bool extensions_supported = validate_instance_extension_support();
        if (extensions_supported) {
            instance_create_info.ppEnabledExtensionNames = m_instance_extensions.data();
            instance_create_info.enabledExtensionCount = static_cast<std::uint32_t>(m_instance_extensions.size());
        }
    
        if (validation_supported) {
            VkDebugUtilsMessengerCreateInfoEXT debug_callback_create_info { };
            debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debug_callback_create_info.messageSeverity =  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debug_callback_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debug_callback_create_info.pfnUserCallback = nullptr;
            debug_callback_create_info.pUserData = nullptr;
    
            // In order to debug instance creation and destruction, pass VkDebugUtilsMessengerCreateInfoEXT into the pNext chain of VkInstanceCreateInfo
            // This debug messenger is attached to the instance and will get cleaned up alongside it
//            instance_create_info.pNext = &debug_callback_create_info;
    
            // Creating debug messenger requires a valid instance
            CHECK_CALL(vkCreateInstance, &instance_create_info, nullptr, &m_instance);
    
            // Load vkCreateDebugUtilsMessengerEXT function
            static auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
            if (!vkCreateDebugUtilsMessengerEXT) {
                utils::logging::fatal("Failed to load vkCreateDebugUtilsMessengerEXT (is the VK_EXT_debug_utils extension enabled?)");
            }
    
            // CHECK_CALL(vkCreateDebugUtilsMessengerEXT, m_instance, &debug_callback_create_info, nullptr, &m_debug_messenger);
        }
        else {
            CHECK_CALL(vkCreateInstance, &instance_create_info, nullptr, &m_instance);
        }
    }
    
    void RenderContext::select_physical_device() {
        std::uint32_t device_count;
        vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
        if (device_count == 0) {
            utils::logging::fatal("Failed to find GPU with Vulkan support");
        }
    
        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());
        
        int best_score = -1;
        for (VkPhysicalDevice gpu : devices) {
            if (!validate_device_extension_support(gpu)) {
                continue;
            }
            
            // All required extensions are supported, but features may not be
            if (!validate_feature_support(gpu)) {
                continue;
            }
            
            QueueFamilies queue_families = select_queue_families(gpu);
            
            // Overall device score should dominate device selection
            // This prevents a worse tier GPU with a better queue selection winning over a more powerful GPU
            int score = compute_device_score(gpu) * 100 + queue_families.compute_score();
            
            if (score > best_score) {
                best_score = score;
                m_queue_families = queue_families;
                m_gpu = gpu;
            }
        }
        
        
        
//        // Retrieve physical device properties, features, and memory limits
//        vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
//        vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);
//
//        // Retrieve surface format, color space, and capabilities
//        // Because of this step, the physical device selection must happen after the surface is initialized (surface properties are queried on the device itself)
//        unsigned surface_format_count = 0u;
//        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr);
//
//        if (surface_format_count == 0u) {
//            throw std::runtime_error("selected physical device does not support any surface formats");
//        }
//
//        std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
//        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats.data());
//
//        surface_format = surface_formats[0]; // Use the first provided format as a default
//        for (const VkSurfaceFormatKHR& format : surface_formats) {
//            // sRGB color space (VK_FORMAT_B8G8R8A8_SRGB) results in more accurate perceived colors in the final image, as it is a non-linear format that more accurately matches how humans perceive light
//            // However, prefer VK_FORMAT_B8G8R8A8_UNORM as this is better suited for intermediate render targets and textures, especially for applications that handle gamma corrections manually (HDR, PBR, etc.)
//            // TODO: convert between VK_FORMAT_B8G8R8A8_UNORM and VK_FORMAT_B8G8R8A8_SRGB for final images?
//            if (format.format == VK_FORMAT_B8G8R8A8_UNORM) {
//                surface_format = format;
//                break;
//            }
//        }
//
//        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    }
    
    void RenderContext::create_device() {
    
    }
    
    bool RenderContext::validate_layer_support() const {
        std::uint32_t layer_count;
        CHECK_CALL(vkEnumerateInstanceLayerProperties, &layer_count, nullptr);
        
        std::vector<VkLayerProperties> layers(layer_count);
        CHECK_CALL(vkEnumerateInstanceLayerProperties, &layer_count, layers.data());
        
        bool result = true;
        
        for (const char* required : m_layers) {
            bool supported = std::any_of(layers.begin(), layers.end(), [required](const VkLayerProperties& layer) {
                return strcmp(layer.layerName, required) == 0;
            });
            
            if (!supported) {
                utils::logging::debug("Validation layer '{}' is not supported", required);
                result = false;
            }
        }
        
        return result;
    }
    
    bool RenderContext::validate_instance_extension_support() const {
        std::uint32_t extension_count;
        CHECK_CALL(vkEnumerateInstanceExtensionProperties, nullptr, &extension_count, nullptr);
    
        std::vector<VkExtensionProperties> extensions(extension_count);
        CHECK_CALL(vkEnumerateInstanceExtensionProperties, nullptr, &extension_count, extensions.data());
        
        bool result = true;
        
        for (const char* requested : m_instance_extensions) {
            bool supported = std::any_of(extensions.begin(), extensions.end(), [requested](const VkExtensionProperties& extension) {
                return strcmp(extension.extensionName, requested) == 0;
            });
            
            if (!supported) {
                utils::logging::debug("Instance extension '{}' is not supported", requested);
                result = false;
            }
        }
        
        return result;
    }
    
    void RenderContext::collect_instance_requirements(const SampleRequirements& requirements) {
        #ifndef NDEBUG
            // Enable debugging on debug builds
            m_layers.emplace_back("VK_LAYER_KHRONOS_validation");
            m_instance_extensions.emplace_back("VK_EXT_debug_utils");
        #endif
        
        // Applications that use a window require windowing extension support
        if (requirements.display_mode != DisplayMode::None) {
            // Typically includes VK_KHR_surface and platform-specific windowing system extension
            std::uint32_t glfw_extension_count;
            const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
            m_instance_extensions.insert(m_instance_extensions.end(), glfw_extensions, glfw_extensions + glfw_extension_count);
        }
        
        if (test(requirements.enabled_features, FeatureFlags::Raytracing)) {
            // Raytracing device extension requires instance extension
            m_instance_extensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }
    }
    
    void RenderContext::create_surface(const Window& window) {
        m_surface = window.create_surface(m_instance);
    }
    
    void RenderContext::collect_device_requirements(const SampleRequirements& requirements) {
        m_enabled_features = requirements.enabled_features;
        
        switch (requirements.enabled_features) {
            case FeatureFlags::Raytracing:
                m_device_extensions.emplace_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
                m_device_extensions.emplace_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
                m_device_extensions.emplace_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
                m_device_extensions.emplace_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
                break;
            case FeatureFlags::MeshShaders:
                m_device_extensions.emplace_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
                break;
            case FeatureFlags::VariableRateShading:
                m_device_extensions.emplace_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
                break;
            case FeatureFlags::GeometryShaders:
            case FeatureFlags::TesselationShaders:
                // Supported in core Vulkan, no extensions needed
                break;
        }
    }
    
    bool RenderContext::validate_device_extension_support(VkPhysicalDevice gpu) const {
        std::uint32_t extension_count;
        CHECK_CALL(vkEnumerateDeviceExtensionProperties, gpu, nullptr, &extension_count, nullptr);
        
        std::vector<VkExtensionProperties> extensions(extension_count);
        CHECK_CALL(vkEnumerateDeviceExtensionProperties, gpu, nullptr, &extension_count, extensions.data());
        
        // Include extensions from enabled validation layers
        for (const char* layer : m_layers) {
            CHECK_CALL(vkEnumerateDeviceExtensionProperties, gpu, layer, &extension_count, nullptr);
            
            if (extension_count > 0) {
                std::size_t size = extensions.size();
                extensions.resize(size + extension_count);
                vkEnumerateDeviceExtensionProperties(gpu, layer, &extension_count, extensions.data() + size);
            }
        }
        
        bool result = true;
        for (const char* required : m_device_extensions) {
            bool supported = std::any_of(extensions.begin(), extensions.end(), [required](const VkExtensionProperties& extension) {
                return strcmp(extension.extensionName, required) == 0;
            });
            
            if (!supported) {
                utils::logging::debug("Device extension '{}' is not supported", required);
                result = false;
            }
        }
        
        return result;
    }
    
    bool RenderContext::validate_feature_support(VkPhysicalDevice gpu) const {
        VkPhysicalDeviceFeatures2 features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracing_features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
        VkPhysicalDeviceAccelerationStructureFeaturesKHR raytracing_acceleration_features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
        VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR };
        VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
        VkPhysicalDeviceFragmentShadingRateFeaturesKHR vrs_features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR };
        
        // Build feature struct chain
        if (test(m_enabled_features, FeatureFlags::Raytracing)) {
            ray_query_features.pNext = features.pNext;
            features.pNext = &ray_query_features;
            
            raytracing_acceleration_features.pNext = features.pNext;
            features.pNext = &raytracing_acceleration_features;
            
            raytracing_features.pNext = features.pNext;
            features.pNext = &raytracing_features;
        }
        
        if (test(m_enabled_features, FeatureFlags::MeshShaders)) {
            mesh_shader_features.pNext = features.pNext;
            features.pNext = &mesh_shader_features;
        }
        
        if (test(m_enabled_features, FeatureFlags::VariableRateShading)) {
            vrs_features.pNext = features.pNext;
            features.pNext = &vrs_features;
        }
        
        vkGetPhysicalDeviceFeatures2(gpu, &features);
        bool result = true;
        
        // Check for raytracing support
        if (test(m_enabled_features, FeatureFlags::Raytracing)) {
            if (!raytracing_features.rayTracingPipeline) {
                utils::logging::error("Device does not support ray tracing pipeline functionality");
                result = false;
            }
            
            if (!raytracing_acceleration_features.accelerationStructure) {
                utils::logging::error("Device does not support acceleration structure functionality");
                result = false;
            }
            
            if (!ray_query_features.rayQuery) {
                utils::logging::error("Device does not support ray query functionality");
                result = false;
            }
        }
        
        // Check for mesh shader support
        if (test(m_enabled_features, FeatureFlags::MeshShaders)) {
            if (!mesh_shader_features.meshShader) {
                utils::logging::error("Device does not support mesh shaders");
                result = false;
            }
            
            if (!mesh_shader_features.taskShader) {
                utils::logging::error("Device does not support task shaders");
                result = false;
            }
        }
        
        // Check for variable rate shading support
        if (test(m_enabled_features, FeatureFlags::VariableRateShading)) {
            if (!vrs_features.pipelineFragmentShadingRate) {
                utils::logging::error("Device does not variable fragment shading functionality");
                result = false;
            }
        }
        
        // Check for geometry shader support
        if (test(m_enabled_features, FeatureFlags::GeometryShaders) && !features.features.geometryShader) {
            utils::logging::error("Device does not support geometry shaders");
            result = false;
        }
        
        // Check for tesselation shader support
        if (test(m_enabled_features, FeatureFlags::TesselationShaders) && !features.features.tessellationShader) {
            utils::logging::error("Device does not support tesselation shaders");
            result = false;
        }
        
        // All features supported
        return result;
    }
    
    int RenderContext::QueueFamilies::compute_score() const {
        int score = 0;
        
        // Prefer graphics family that supports compute operations
        // Graphics family inherently supports transfer operations
        if (graphics.flags & VK_QUEUE_COMPUTE_BIT) {
            score |= 1 << 9;
        }
        
        bool compute_supported = compute.index != VK_QUEUE_FAMILY_IGNORED;
        if (compute_supported) {
            score |= 1 << 8;
        }
        
        // Prefer device that has support for an async compute queue
        bool async_compute = compute_supported && compute.index != graphics.index;
        if (async_compute) {
            score |= 1 << 7;
        }
    
        bool transfer_supported = transfer.index != VK_QUEUE_FAMILY_IGNORED;
        if (transfer_supported) {
            score |= 1 << 6;
        }
        
        // Prefer device that has support for an async transfer queue
        bool async_transfer = transfer_supported && transfer.index != graphics.index;
        if (async_transfer) {
            score |= 1 << 5;
        }
        
        // Efficiency bonus: async compute and async transfer queue come from the same queue family
        if (async_compute && async_transfer && compute.index == transfer.index) {
            score |= 1 << 4;
        }
        
        // In the case of a tie-breaker, prefer lower queue family indices
        score = score * 100 - graphics.index;
        if (compute_supported) {
            score -= compute.index;
        }
        if (transfer_supported) {
            score -= transfer.index;
        }
        
        return score;
    }
    
    RenderContext::QueueFamilies RenderContext::select_queue_families(VkPhysicalDevice gpu) const {
        std::uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, nullptr);
        
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_families.data());
        
        std::vector<std::uint32_t> graphics_queue_families;
        std::vector<std::uint32_t> compute_queue_families;
        std::vector<std::uint32_t> transfer_queue_families;
        
        bool is_headless = m_surface == VK_NULL_HANDLE;
        
        // Enumerate all queue families by supported operation
        for (std::uint32_t i = 0; i < queue_family_count; ++i) {
            VkQueueFlags flags = queue_families[i].queueFlags;
            
            bool supports_graphics = flags & VK_QUEUE_GRAPHICS_BIT;
            bool supports_compute = flags & VK_QUEUE_COMPUTE_BIT;
            bool supports_transfer = supports_graphics || supports_compute || (flags & VK_QUEUE_TRANSFER_BIT); // Graphics and compute queues implicitly support transfer operations
            
            VkBool32 supports_presentation = false;
            if (!is_headless) {
                CHECK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR, gpu, i, m_surface, &supports_presentation);
            }
            
            if (supports_graphics) {
                if (is_headless || supports_presentation) {
                    graphics_queue_families.emplace_back(i);
                }
            }
            
            if (supports_compute) {
                compute_queue_families.emplace_back(i);
            }
            
            if (supports_transfer) {
                transfer_queue_families.emplace_back(i);
            }
        }
        
        if (compute_queue_families.empty()) {
            compute_queue_families.emplace_back(VK_QUEUE_FAMILY_IGNORED);
        }
        
        if (transfer_queue_families.empty()) {
            transfer_queue_families.emplace_back(VK_QUEUE_FAMILY_IGNORED);
        }
        
        QueueFamilies best_config { };
        int best_score = -1;

        for (std::uint32_t graphics : graphics_queue_families) {
            for (std::uint32_t compute : compute_queue_families) {
                for (std::uint32_t transfer : transfer_queue_families) {
                    bool supports_async_compute = compute != graphics && compute != VK_QUEUE_FAMILY_IGNORED;
                    bool supports_async_transfer = transfer != graphics && transfer != VK_QUEUE_FAMILY_IGNORED;
                    
                    QueueFamilies config {
                        // Graphics family is guaranteed to be valid
                        .graphics = {
                            .index = graphics,
                            .flags = queue_families[graphics].queueFlags
                        },
                        .compute = {
                            .index = supports_async_compute ? compute : VK_QUEUE_FAMILY_IGNORED,
                            .flags = supports_async_compute ? queue_families[compute].queueFlags : 0
                        },
                        .transfer = {
                            .index = supports_async_transfer ? transfer : VK_QUEUE_FAMILY_IGNORED,
                            .flags = supports_async_transfer ? queue_families[transfer].queueFlags : 0
                        }
                    };
                    
                    int current_score = config.compute_score();
                    
                    if (current_score > best_score) {
                        best_score = current_score;
                        best_config = config;
                    }
                }
            }
        }
        
        return best_config;
    }
    
    int RenderContext::compute_device_score(VkPhysicalDevice gpu) const {
        VkPhysicalDeviceProperties gpu_properties;
        vkGetPhysicalDeviceProperties(gpu, &gpu_properties);
        
        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(gpu, &memory_properties);
        
        int score = 0;
        
        // GPU type
        switch (gpu_properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score += 10000;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score += 1000;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                score += 100;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                score += 10;
                break;
            default: break;
        }
        
        // VRAM amount, in MB
        std::uint64_t vram = 0;
        for (std::uint32_t i = 0; i < memory_properties.memoryHeapCount; ++i) {
            if (memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                vram += memory_properties.memoryHeaps[i].size; // Size, in bytes
            }
        }
        
        score += std::min(static_cast<int>(vram / (1024 * 1024)), 5000); // Cap at 5GB bonus
        return score;
    }
    
}