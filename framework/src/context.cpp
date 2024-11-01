
#include "context.hpp"
#include "window.hpp"
#include "utils/logging.hpp"
#include "utils/platform.hpp"


namespace vks {

    VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
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

        return VK_FALSE; // Whether the Vulkan call that triggered the validation layer message should be aborted
    }
    
    // Window is initialized by the Context
    class Window::Builder {
        public:
            Builder(std::shared_ptr<Context> context) : m_handle(std::make_shared<Window>()),
                                                        m_context(std::move(context)) {
            }
            ~Builder() {
            }
            
            [[nodiscard]] std::shared_ptr<Window> build() {
                unsigned width = m_handle->m_width;
                unsigned height = m_handle->m_height;
                GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
                
                if (m_fullscreen) {
                    // In order to avoid flickering when creating a full screen window, the window is first created in a lower resolution (640 x 360) and then updated to match the size of the monitor
                    m_handle->m_window = glfwCreateWindow(640, 360, m_handle->m_name, primary_monitor, nullptr);
                    
                    // Retrieve monitor size (in screen coordinates)
                    const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);
                    glfwSetWindowSize(m_handle->m_window, mode->width, mode->height);
                }
                else {
                    m_handle->m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), m_handle->m_name, primary_monitor, nullptr);
                }
                
                // assert(m_handle->m_window); // TODO: replace
                
                glfwSetWindowUserPointer(m_handle->m_window, m_handle.get()); // Reference the underlying Window pointer
                
                // Initialize window surface
                // Surface needs to be created after creating the vulkan instance (Vulkan surface may affect physical device selection)
//                if (glfwCreateWindowSurface(m_context->instance, m_handle->m_window, nullptr, &m_handle->m_surface) != VK_SUCCESS) {
//                    // TODO: error
//                }
            }
            
            Builder& set_width(unsigned width) {
                m_handle->m_width = width;
                return *this;
            }
            
            Builder& set_height(unsigned height) {
                m_handle->m_height = height;
                return *this;
            }
            
            Builder& enable_fullscreen() {
                m_fullscreen = true;
                return *this;
            }
            
            Builder& set_name(const char* name) {
                m_handle->m_name = name;
                return *this;
            }
            
        private:
            std::shared_ptr<Context> m_context;
            std::shared_ptr<Window> m_handle;
            
            bool m_fullscreen;
    };
    
    
    Context::Builder::Builder() : m_handle(std::make_shared<Context>()),
                                  m_headless(false),
                                  m_fullscreen(false),
                                  m_width(1920u),
                                  m_height(1080u) {
    }
    
    Context::Builder::~Builder() {
    }
    
    std::shared_ptr<Context> Context::Builder::build() {
        initialize_vulkan_instance();
        
        if (!m_headless) {
            // Headless applications do not require any windowing functionality
            initialize_window();
        }
        
        select_physical_device();
        initialize_logical_device();
    }
    
    Context::Builder& Context::Builder::enable_headless_mode() {
        m_headless = true;
        return *this;
    }
    
    Context::Builder& Context::Builder::set_extent(unsigned width, unsigned height) {
        m_width = width;
        m_height = height;
        return *this;
    }
    
    Context::Builder& Context::Builder::enable_fullscreen() {
        m_fullscreen = true;
        return *this;
    }
    
    Context::Builder& Context::Builder::set_application_name(const char* name) {
        m_handle->m_name = name;
        return *this;
    }
    
    Context::Builder& Context::Builder::enable_extension(const char* extension) {
        bool found = false;
        for (const char* enabled : m_extensions) {
            if (strcmp(enabled, extension) == 0) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            m_extensions.emplace_back(extension);
        }
        
        return *this;
    }
    
    void Context::Builder::initialize_vulkan_instance() {
        // Initialize Vulkan instance
        VkApplicationInfo application_info {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = m_handle->m_name,
            .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .pEngineName = "vulkan-samples",
            .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3
        };
        
        // 'VK_LAYER_KHRONOS_validation' validation layer contains all validation functionality
        const char* validation_layer = "VK_LAYER_KHRONOS_validation";
        
        unsigned supported_validation_layer_count = 0u;
        vkEnumerateInstanceLayerProperties(&supported_validation_layer_count, nullptr);
    
        std::vector<VkLayerProperties> supported_validation_layers(supported_validation_layer_count);
        vkEnumerateInstanceLayerProperties(&supported_validation_layer_count, supported_validation_layers.data());
        
        bool validation_supported = false;
        for (const VkLayerProperties& supported : supported_validation_layers) {
            if (strcmp(validation_layer, supported.layerName) == 0) {
                validation_supported = true;
                break;
            }
        }
        if (!validation_supported) {
            // TODO: log message
        }

        // Enable debugging on debug builds
        #ifndef NDEBUG
            m_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #endif
        
        if (!m_headless) {
            // Applications presenting to the screen require the VK_KHR_surface extension for window surface support
            m_extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
        }
        
        unsigned supported_extension_count = 0u;
        vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, nullptr);
    
        std::vector<VkExtensionProperties> supported_extensions(supported_extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, supported_extensions.data());
        
        for (const char* requested : m_extensions) {
            bool found = false;
            for (const VkExtensionProperties& supported : supported_extensions) {
                if (strcmp(requested, supported.extensionName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                // Instance creation will fail
                // TODO: log message
            }
        }
        
        VkInstanceCreateInfo instance_create_info {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &application_info,
            .enabledLayerCount = 1u,
            .ppEnabledLayerNames = &validation_layer,
            .enabledExtensionCount = static_cast<std::uint32_t>(m_extensions.size()),
            .ppEnabledExtensionNames = m_extensions.data()
        };
        
        if (validation_supported) {
            VkDebugUtilsMessengerCreateInfoEXT debug_callback_create_info {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .pNext = nullptr,
                .flags = 0,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = nullptr,
                .pUserData = nullptr
            };
            
            // In order to debug instance creation and destruction, pass VkDebugUtilsMessengerCreateInfoEXT into the pNext chain of VkInstanceCreateInfo
            // This debug messenger is attached to the instance and will get cleaned up alongside it
            instance_create_info.pNext = &debug_callback_create_info;
            if (vkCreateInstance(&instance_create_info, nullptr, &m_handle->instance) != VK_SUCCESS) {
                // TODO: error
            }
            
            // Load vkCreateDebugUtilsMessenger function
            static auto vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_handle->instance, "vkCreateDebugUtilsMessengerEXT");
            if (!vkCreateDebugUtilsMessenger) {
                throw std::runtime_error("failed to load debug messenger create function (is the VK_EXT_debug_utils extension enabled?)");
            }
        
            if (vkCreateDebugUtilsMessenger(m_handle->instance, &debug_callback_create_info, nullptr, &m_handle->m_debug_messenger) != VK_SUCCESS) {
                // TODO: error
            }
        }
        else {
            // Validation layers are not supported, create Vulkan instance without a debug messenger
            if (vkCreateInstance(&instance_create_info, nullptr, &m_handle->instance) != VK_SUCCESS) {
                // TODO: error
            }
        }
    }
    
    void Context::Builder::initialize_window() {
        Window::Builder builder(m_handle);
        builder.set_name(m_handle->m_name);
        
        if (m_fullscreen) {
            builder.enable_fullscreen();
        }
        else {
            builder.set_width(m_width).set_height(m_height);
        }
        
        m_handle->window = builder.build();
    }
    
    void Context::Builder::select_physical_device() {
        // Enumerate all available physical devices
        unsigned physical_device_count = 0u;
        vkEnumeratePhysicalDevices(m_handle->instance, &physical_device_count, nullptr);
    
        if (physical_device_count == 0) {
            // TODO: error
        }
    
        std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
        vkEnumeratePhysicalDevices(m_handle->instance, &physical_device_count, physical_devices.data());
        
        for (VkPhysicalDevice physical_device : physical_devices) {
            // Retrieve physical device properties, features, and memory limits
            VkPhysicalDeviceProperties physical_device_properties { };
            vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

            VkPhysicalDeviceFeatures physical_device_features { };
            vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);
            
            utils::logging::info("Found candidate device: {}", physical_device_properties.deviceName);
            
            if (!verify_requested_feature_support(physical_device_features)) {
                continue;
            }
            
            // TODO: check supported against requested API version (not applicable right now)
            
        }
        
        // For now, use the first device by default
        // TODO: score devices based on queue types, supported features, etc.
        // TODO: check for requested feature support?
        m_handle->gpu = physical_devices[0];
        
        // Retrieve physical device properties, features, and memory limits
        vkGetPhysicalDeviceProperties(m_handle->gpu, &m_handle->m_properties);
        vkGetPhysicalDeviceFeatures(m_handle->gpu, &m_handle->m_features);
        
        // Retrieve surface format, color space, and capabilities
        // Because of this step, the physical device selection must happen after the surface is initialized (surface properties are queried on the device itself)
        unsigned surface_format_count = 0u;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_handle->gpu, m_handle->window->surface, &surface_format_count, nullptr);
    
        if (surface_format_count == 0u) {
            throw std::runtime_error("selected physical device does not support any surface formats");
        }
    
//        std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
//        vkGetPhysicalDeviceSurfaceFormatsKHR(m_handle->gpu, surface, &surface_format_count, surface_formats.data());
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
//        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_handle->gpu, m_handle->window->surface, &surface_capabilities);
    }
    
    void Context::Builder::initialize_logical_device() {
    }
    
    bool Context::Builder::verify_requested_feature_support(const VkPhysicalDeviceFeatures& supported_features) const {
        // While it is possible to treat both VkPhysicalDeviceFeatures structs as arrays of VkBool32s and iterate over them like you would an array,
        // this approach is less readable and requires a mapping between index and feature for things like logging / validation
        // For this reason, a more manual approach of iterating over each member by name is taken here
        bool all_requested_features_supported = true;
        
        if (m_requested_features.robustBufferAccess) {
            if (!supported_features.robustBufferAccess) {
                all_requested_features_supported = false;
            }
            else {
                utils::logging::error("");
            }
        }
        
        if (m_requested_features.fullDrawIndexUint32) {
            if (!supported_features.fullDrawIndexUint32) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.imageCubeArray) {
            if (!supported_features.imageCubeArray) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.independentBlend) {
            if (!supported_features.independentBlend) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.geometryShader) {
            if (!supported_features.geometryShader) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.tessellationShader) {
            if (!supported_features.tessellationShader) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.sampleRateShading) {
            if (!supported_features.sampleRateShading) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.dualSrcBlend) {
            if (!supported_features.dualSrcBlend) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.logicOp) {
            if (!supported_features.logicOp) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.multiDrawIndirect) {
            if (!supported_features.multiDrawIndirect) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.drawIndirectFirstInstance) {
            if (!supported_features.drawIndirectFirstInstance) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.depthClamp) {
            if (!supported_features.depthClamp) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.depthBiasClamp) {
            if (!supported_features.depthBiasClamp) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.fillModeNonSolid) {
            if (!supported_features.fillModeNonSolid) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.depthBounds) {
            if (!supported_features.depthBounds) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.wideLines) {
            if (!supported_features.wideLines) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.largePoints) {
            if (!supported_features.largePoints) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.alphaToOne) {
            if (!supported_features.alphaToOne) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.multiViewport) {
            if (!supported_features.multiViewport) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.samplerAnisotropy) {
            if (!supported_features.samplerAnisotropy) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.textureCompressionETC2) {
            if (!supported_features.textureCompressionETC2) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.textureCompressionASTC_LDR) {
            if (!supported_features.textureCompressionASTC_LDR) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.textureCompressionBC) {
            if (!supported_features.textureCompressionBC) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.occlusionQueryPrecise) {
            if (!supported_features.occlusionQueryPrecise) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.pipelineStatisticsQuery) {
            if (!supported_features.pipelineStatisticsQuery) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.vertexPipelineStoresAndAtomics) {
            if (!supported_features.vertexPipelineStoresAndAtomics) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.fragmentStoresAndAtomics) {
            if (!supported_features.fragmentStoresAndAtomics) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderTessellationAndGeometryPointSize) {
            if (!supported_features.shaderTessellationAndGeometryPointSize) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderImageGatherExtended) {
            if (!supported_features.shaderImageGatherExtended) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderStorageImageExtendedFormats) {
            if (!supported_features.shaderStorageImageExtendedFormats) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderStorageImageMultisample) {
            if (!supported_features.shaderStorageImageMultisample) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderStorageImageReadWithoutFormat) {
            if (!supported_features.shaderStorageImageReadWithoutFormat) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderStorageImageWriteWithoutFormat) {
            if (!supported_features.shaderStorageImageWriteWithoutFormat) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderUniformBufferArrayDynamicIndexing) {
            if (!supported_features.shaderUniformBufferArrayDynamicIndexing) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderSampledImageArrayDynamicIndexing) {
            if (!supported_features.shaderSampledImageArrayDynamicIndexing) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderStorageBufferArrayDynamicIndexing) {
            if (!supported_features.shaderStorageBufferArrayDynamicIndexing) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderStorageImageArrayDynamicIndexing) {
            if (!supported_features.shaderStorageImageArrayDynamicIndexing) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderClipDistance) {
            if (!supported_features.shaderClipDistance) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderCullDistance) {
            if (!supported_features.shaderCullDistance) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderFloat64) {
            if (!supported_features.shaderFloat64) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderInt64) {
            if (!supported_features.shaderInt64) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderInt16) {
            if (!supported_features.shaderInt16) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderResourceResidency) {
            if (!supported_features.shaderResourceResidency) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.shaderResourceMinLod) {
            if (!supported_features.shaderResourceMinLod) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.sparseBinding) {
            if (!supported_features.sparseBinding) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.sparseResidencyBuffer) {
            if (!supported_features.sparseResidencyBuffer) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.sparseResidencyImage2D) {
            if (!supported_features.sparseResidencyImage2D) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.sparseResidencyImage3D) {
            if (!supported_features.sparseResidencyImage3D) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.sparseResidency2Samples) {
            if (!supported_features.sparseResidency2Samples) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.sparseResidency4Samples) {
            if (!supported_features.sparseResidency4Samples) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.sparseResidency8Samples) {
            if (!supported_features.sparseResidency8Samples) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.sparseResidency16Samples) {
            if (!supported_features.sparseResidency16Samples) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.sparseResidencyAliased) {
            if (!supported_features.sparseResidencyAliased) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.variableMultisampleRate) {
            if (!supported_features.variableMultisampleRate) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        if (m_requested_features.inheritedQueries) {
            if (!supported_features.inheritedQueries) {
                all_requested_features_supported = false;
            }
            else {
            }
        }
        
        return all_requested_features_supported;
    }
    
}