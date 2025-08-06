
#include "vks/vulkan/device.hpp"
#include "vks/core.hpp"
#include "vks/sample.hpp"
#include <utils/logging.hpp>

namespace vks {

    Device::Device(VkInstance instance, VkSurfaceKHR surface, const SampleRequirements& requirements) {
        collect_requirements(requirements);
        select_physical_device(instance, surface);
        create_device();
        m_queues.retrieve_queue_handles(m_device);
    }
    
    Device::~Device() {
        vkDestroyDevice(m_device, nullptr);
    }
    
    VkSurfaceFormatKHR Device::get_surface_format(VkSurfaceKHR surface) const {
        std::uint32_t format_count;
        CHECK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR, m_gpu, surface, &format_count, nullptr);
        
        if (format_count == 0) {
            utils::logging::fatal("Selected GPU does not support any formats for presentation");
        }
        
        std::vector<VkSurfaceFormatKHR> formats(format_count);
        CHECK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR, m_gpu, surface, &format_count, formats.data());
        
        // Surface format is for presentation
        // In an ideal case, intermediate shader calculations should be performed in linear space (accurate for lighting calculations) but converted to sRGB (accurate for displaying) when presenting
        
        // Prefer sRGB for final presentation
        for (const VkSurfaceFormatKHR& format : formats) {
            // VK_FORMAT_B8G8R8A8_SRGB (sRGB) results in more accurate perceived colors in the final image, as it is a non-linear format that more accurately matches how humans perceive light
            // If this format is supported, the hardware will automatically apply the sRGB gamma curve during presentation (no manual gamma correction needed)
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        
        // Fallback to UNORM + sRGB color space
        for (const VkSurfaceFormatKHR& format : formats) {
            // VK_FORMAT_B8G8R8A8_UNORM is a linear format, which does not have automatic sRGB conversion
            // However, the VK_COLOR_SPACE_SRGB_NONLINEAR_KHR color space tells the monitor to expect sRGB values
            // If this format is selected, a manual gamma correction step is required before presenting to the screen
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        
        // Return the first format as a last resort
        return formats[0];
    }
    
    VkQueue Device::get_graphics_queue() const {
        return m_queues.graphics;
    }
    
    VkQueue Device::get_compute_queue() const {
        return m_queues.compute;
    }
    
    VkQueue Device::get_transfer_queue() const {
        return m_queues.transfer;
    }
    
    void Device::collect_requirements(const SampleRequirements& requirements) {
        m_enabled_features = requirements.enabled_features;
        
        // Check each feature bit individually
        if (test(m_enabled_features, FeatureFlags::Raytracing)) {
            m_extensions.emplace_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            m_extensions.emplace_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            m_extensions.emplace_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
            m_extensions.emplace_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        }
        if (test(m_enabled_features, FeatureFlags::MeshShaders)) {
            m_extensions.emplace_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
        }
        if (test(m_enabled_features, FeatureFlags::VariableRateShading)) {
            m_extensions.emplace_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
        }
    }
    
    void Device::select_physical_device(VkInstance instance, VkSurfaceKHR surface) {
        std::uint32_t device_count;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        if (device_count == 0) {
            utils::logging::fatal("Failed to find GPU with Vulkan support");
        }
    
        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
        
        std::uint32_t best_score = 0;
        for (VkPhysicalDevice gpu : devices) {
            if (!validate_extension_support(gpu)) {
                continue;
            }
            
            // All required extensions are supported, but features may not be
            if (!validate_feature_support(gpu)) {
                continue;
            }
            
            DeviceQueues queues = select_queue_families(surface, gpu);
            std::uint32_t score = calculate_device_score(gpu, queues);
            
            if (score > best_score) {
                best_score = score;
                m_queues = queues;
                m_gpu = gpu;
            }
        }
    }
    
    bool Device::validate_extension_support(VkPhysicalDevice gpu) const {
        std::uint32_t extension_count;
        CHECK_CALL(vkEnumerateDeviceExtensionProperties, gpu, nullptr, &extension_count, nullptr);
        
        std::vector<VkExtensionProperties> extensions(extension_count);
        CHECK_CALL(vkEnumerateDeviceExtensionProperties, gpu, nullptr, &extension_count, extensions.data());
        
        bool result = true;
        for (const char* required : m_extensions) {
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
    
    bool Device::validate_feature_support(VkPhysicalDevice gpu) const {
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
    
    Device::DeviceQueues Device::select_queue_families(VkSurfaceKHR surface, VkPhysicalDevice gpu) const {
        std::uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, nullptr);
        
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_families.data());
        
        std::vector<std::uint32_t> graphics_queue_families;
        std::vector<std::uint32_t> compute_queue_families;
        std::vector<std::uint32_t> transfer_queue_families;
        
        bool is_headless = surface == VK_NULL_HANDLE;
        
        // Enumerate all queue families by supported operation
        for (std::uint32_t i = 0; i < queue_family_count; ++i) {
            VkQueueFlags flags = queue_families[i].queueFlags;
            
            bool supports_graphics = flags & VK_QUEUE_GRAPHICS_BIT;
            bool supports_compute = flags & VK_QUEUE_COMPUTE_BIT;
            bool supports_transfer = supports_graphics || supports_compute || (flags & VK_QUEUE_TRANSFER_BIT); // Graphics and compute queues implicitly support transfer operations
            
            VkBool32 supports_presentation = false;
            if (!is_headless) {
                CHECK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR, gpu, i, surface, &supports_presentation);
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
        
        DeviceQueues best_config { };
        std::uint32_t best_score = 0;

        for (std::uint32_t graphics : graphics_queue_families) {
            for (std::uint32_t compute : compute_queue_families) {
                for (std::uint32_t transfer : transfer_queue_families) {
                    bool supports_async_compute = compute != graphics && compute != VK_QUEUE_FAMILY_IGNORED;
                    bool supports_async_transfer = transfer != graphics && transfer != VK_QUEUE_FAMILY_IGNORED;
                    
                    DeviceQueues config {
                        // Graphics family is guaranteed to be valid
                        .graphics_family_index = graphics,
                        .compute_family_index = supports_async_compute ? compute : VK_QUEUE_FAMILY_IGNORED,
                        .transfer_family_index = supports_async_transfer ? transfer : VK_QUEUE_FAMILY_IGNORED,
                        
                        .graphics_family_flags = queue_families[graphics].queueFlags,
                        .compute_family_flags = supports_async_compute ? queue_families[compute].queueFlags : 0,
                        .transfer_family_flags = supports_async_transfer ? queue_families[transfer].queueFlags : 0
                    };
                    
                    std::uint32_t current_score = config.calculate_score();
                    
                    if (current_score > best_score) {
                        best_score = current_score;
                        best_config = config;
                    }
                }
            }
        }
        
        return best_config;
    }
    
    std::uint32_t Device::calculate_device_score(VkPhysicalDevice gpu, const Device::DeviceQueues& queue_families) const {
        VkPhysicalDeviceProperties gpu_properties;
        vkGetPhysicalDeviceProperties(gpu, &gpu_properties);
        
        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(gpu, &memory_properties);
        
        std::uint32_t score = 0;
        
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
            default:
                break;
        }
        
        // VRAM amount, in MB
        std::uint64_t vram = 0;
        for (std::uint32_t i = 0; i < memory_properties.memoryHeapCount; ++i) {
            if (memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                vram += memory_properties.memoryHeaps[i].size; // Size, in bytes
            }
        }
        
        score += std::min(static_cast<int>(vram / (1024 * 1024)), 5000); // Cap at 5GB
        
        // Device properties should dominate over queue properties when scoring a device
        // This prevents a worse tier GPU with a better queue selection winning over a more powerful GPU
        return score * 100 + queue_families.calculate_score();
    }
    
    void Device::create_device() {
        VkDeviceCreateInfo device_create_info { };
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        
        // Enable requested features
        // This (unfortunately) must be done inline to remain in scope for device creation
        VkPhysicalDeviceFeatures2 features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracing_features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
        VkPhysicalDeviceAccelerationStructureFeaturesKHR raytracing_acceleration_features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
        VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR };
        VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
        VkPhysicalDeviceFragmentShadingRateFeaturesKHR vrs_features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR };
        
        if (test(m_enabled_features, FeatureFlags::Raytracing)) {
            ray_query_features.pNext = features.pNext;
            features.pNext = &ray_query_features;
            
            raytracing_acceleration_features.pNext = features.pNext;
            features.pNext = &raytracing_acceleration_features;
            
            raytracing_features.pNext = features.pNext;
            features.pNext = &raytracing_features;
            
            raytracing_features.rayTracingPipeline = VK_TRUE;
            raytracing_acceleration_features.accelerationStructure = VK_TRUE;
            ray_query_features.rayQuery = VK_TRUE;
        }
        if (test(m_enabled_features, FeatureFlags::MeshShaders)) {
            mesh_shader_features.pNext = features.pNext;
            features.pNext = &mesh_shader_features;
            
            mesh_shader_features.meshShader = VK_TRUE;
            mesh_shader_features.taskShader = VK_TRUE;
        }
        if (test(m_enabled_features, FeatureFlags::VariableRateShading)) {
            vrs_features.pNext = features.pNext;
            features.pNext = &vrs_features;
            
            vrs_features.pipelineFragmentShadingRate = VK_TRUE;
            vrs_features.primitiveFragmentShadingRate = VK_TRUE;
            vrs_features.attachmentFragmentShadingRate = VK_TRUE;
        }
        if (test(m_enabled_features, FeatureFlags::GeometryShaders)) {
            features.features.geometryShader = VK_TRUE;
        }
        if (test(m_enabled_features, FeatureFlags::TesselationShaders)) {
            features.features.tessellationShader = VK_TRUE;
        }
        
        // Device extensions
        device_create_info.enabledExtensionCount = m_extensions.size();
        device_create_info.ppEnabledExtensionNames = m_extensions.data();
        
        // Device queues
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos = m_queues.to_create_infos();
        device_create_info.queueCreateInfoCount = queue_create_infos.size();
        device_create_info.pQueueCreateInfos = queue_create_infos.data();
        
        CHECK_CALL(vkCreateDevice, m_gpu, &device_create_info, nullptr, &m_device);
    }
    
    std::uint32_t Device::DeviceQueues::calculate_score() const {
        std::uint32_t score = 0;
        
        // Prefer graphics family that supports compute operations
        // Graphics family inherently supports transfer operations
        if (graphics_family_flags & VK_QUEUE_COMPUTE_BIT) {
            score |= 1 << 9;
        }
        
        bool compute_supported = compute_family_index != VK_QUEUE_FAMILY_IGNORED;
        if (compute_supported) {
            score |= 1 << 8;
        }
        
        // Prefer device that has support for an async compute queue
        bool async_compute = compute_supported && compute_family_index != graphics_family_index;
        if (async_compute) {
            score |= 1 << 7;
        }
    
        bool transfer_supported = transfer_family_index != VK_QUEUE_FAMILY_IGNORED;
        if (transfer_supported) {
            score |= 1 << 6;
        }
        
        // Prefer device that has support for an async transfer queue
        bool async_transfer = transfer_supported && transfer_family_index != graphics_family_index;
        if (async_transfer) {
            score |= 1 << 5;
        }
        
        // Efficiency bonus: async compute and async transfer queue come from the same queue family
        if (async_compute && async_transfer && compute_family_index == transfer_family_index) {
            score |= 1 << 4;
        }
        
        // In the case of a tie-breaker, prefer lower queue family indices
        score = score * 100 - graphics_family_index;
        if (compute_supported) {
            score -= compute_family_index;
        }
        if (transfer_supported) {
            score -= transfer_family_index;
        }
        
        return score;
    }
    
    std::vector<VkDeviceQueueCreateInfo> Device::DeviceQueues::to_create_infos() const {
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::unordered_set<std::uint32_t> queue_families;

        // Create a queue per unique queue family
        queue_families.insert(graphics_family_index); // Graphics is always supported
        if (compute_family_index != VK_QUEUE_FAMILY_IGNORED) {
            queue_families.insert(compute_family_index);
        }
        if (transfer_family_index != VK_QUEUE_FAMILY_IGNORED) {
            queue_families.insert(transfer_family_index);
        }
        
        float queue_priority = 1.0f;
        for (std::uint32_t family : queue_families) {
            VkDeviceQueueCreateInfo queue_create_info { };
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = family;
            queue_create_info.queueCount = 1; // No need to create more than one queue per family
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }
        
        return queue_create_infos;
    }
    
    void Device::DeviceQueues::retrieve_queue_handles(VkDevice device) {
        vkGetDeviceQueue(device, graphics_family_index, 0, &graphics);
        
        if (compute_family_index != VK_QUEUE_FAMILY_IGNORED) {
            vkGetDeviceQueue(device, compute_family_index, 0, &compute);
        }
        else {
            compute = VK_NULL_HANDLE;
        }
        
        if (transfer_family_index != VK_QUEUE_FAMILY_IGNORED) {
            vkGetDeviceQueue(device, transfer_family_index, 0, &transfer);
        }
        else {
            transfer = VK_NULL_HANDLE;
        }
    }
    
}