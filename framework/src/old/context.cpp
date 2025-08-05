
#include "context.hpp"
#include "window.hpp"
#include "utils/logging.hpp"
#include "utils/exceptions.hpp"

#if defined(PLATFORM_WINDOWS)
    #include <vulkan/vulkan_win32.h>
#endif
#include <GLFW/glfw3.h>

namespace vks {
    
    namespace detail {
        
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
    
            return VK_FALSE; // Whether the Vulkan call that triggered the validation layer message should be aborted
        }
        
    }
    
    // Window is initialized by the Context
    class Window::Builder {
        public:
            Builder(std::shared_ptr<Context> context);
            ~Builder();
            
            std::shared_ptr<Window> build();
            
            Builder& set_width(unsigned width);
            Builder& set_height(unsigned height);
            
            Builder& enable_fullscreen();
            
            Builder& set_name(const char* name);
            
        private:
            [[nodiscard]] bool initialize_glfw();
            [[nodiscard]] bool initialize_surface();
            
            std::shared_ptr<Window> m_handle;
            
            std::shared_ptr<Context> m_context;
            bool m_fullscreen;
    };
    
    
    Window::Builder::Builder(std::shared_ptr<Context> context) : m_handle(std::make_shared<Window>()),
                                                                 m_context(std::move(context)) {
    }
    
    Window::Builder::~Builder() {
    }
    
    [[nodiscard]] std::shared_ptr<Window> Window::Builder::build() {
        if (!glfwInit()) {
            utils::logging::error("Failed to initialize GLFW");
            return nullptr;
        }
        
        // Do not create an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        
        GLFWwindow* window = glfwCreateWindow((int) m_handle->m_width, (int) m_handle->m_height, m_handle->m_name, nullptr, nullptr);
        if (!window) {
            utils::logging::error("Failed to create GLFW window");
            glfwTerminate();
            return nullptr;
        }
        
        // TODO: configure event callbacks
        
        // Initialize surface
        VkResult result = glfwCreateWindowSurface(m_context->instance, window, nullptr, &m_handle->surface);
        if (result != VK_SUCCESS) {
            utils::logging::error("Failed to create Vulkan surface (error code: {})", result);
            return nullptr;
        }
        
        m_context->window = m_handle;
        return std::move(m_handle);
    }
    
    Window::Builder& Window::Builder::set_width(unsigned width) {
        m_handle->m_width = width;
        return *this;
    }
    
    Window::Builder& Window::Builder::set_height(unsigned height) {
        m_handle->m_height = height;
        return *this;
    }
    
    Window::Builder& Window::Builder::enable_fullscreen() {
        m_fullscreen = true;
        return *this;
    }
    
    Window::Builder& Window::Builder::set_name(const char* name) {
        m_handle->m_name = name;
        return *this;
    }
    
    
    Context::Builder::Builder() : m_handle(std::make_shared<Context>()),
                                  m_requested_features({}),
                                  m_headless(false),
                                  m_fullscreen(false),
                                  m_width(1920),
                                  m_height(1080) {
    }
    
    Context::Builder::~Builder() = default;
    
    std::shared_ptr<Context> Context::Builder::build() {
        if (!initialize_vulkan_instance()) {
            return nullptr;
        }
        
        if (!m_headless) {
            // Headless applications do not require any windowing functionality
            if (!initialize_window()) {
                return nullptr;
            }
        }
        
        if (!select_physical_device()) {
            return nullptr;
        }
        
        initialize_logical_device();
        
        return m_handle;
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
    
    bool Context::Builder::initialize_vulkan_instance() {
        // Initialize Vulkan instance
        VkApplicationInfo application_info {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = m_handle->m_name,
            .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .pEngineName = "vulkan-samples",
            .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3
        };
        
        if (!configure_instance_extensions()) {
            return false;
        }
        
        VkInstanceCreateInfo instance_create_info {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &application_info,
            .enabledExtensionCount = (unsigned) m_extensions.size(),
            .ppEnabledExtensionNames = m_extensions.data()
        };
        
        // Query validation layer support
        unsigned validation_layer_count = 0u;
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
        
        VkResult result;
        if (is_validation_supported) {
            // Enable Vulkan validation layers
            instance_create_info.enabledLayerCount = 1;
            instance_create_info.ppEnabledLayerNames = &validation_layer;
            
            // Enable Vulkan debug messenger
            VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .pNext = nullptr,
                .flags = 0,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = detail::process_vulkan_message,
                .pUserData = nullptr
            };
            
            // VkDebugUtilsMessengerCreateInfoEXT struct is passed into the pNext chain of VkInstanceCreateInfo in order to be able to debug instance creation / destruction
            // This debug messenger is attached to the instance and will get cleaned up alongside it
            instance_create_info.pNext = &debug_messenger_create_info;
            
            result = vkCreateInstance(&instance_create_info, nullptr, &m_handle->instance);
            if (result != VK_SUCCESS) {
                throw utils::FormattedError("failed to create Vulkan instance (error code: {})", result);
            }
            
            // Create the actual VkDebugUtilsMessengerEXT to debug all other Vulkan API calls
            // vkCreateDebugUtilsMessenger function is not loaded by default
            static auto vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_handle->instance, "vkCreateDebugUtilsMessengerEXT");
            if (!vkCreateDebugUtilsMessenger) {
                throw std::runtime_error("failed to load VkDebugUtilsMessengerEXT (is the 'VK_EXT_debug_utils' extension enabled?)");
            }
            
            result = vkCreateDebugUtilsMessenger(m_handle->instance, &debug_messenger_create_info, nullptr, &m_handle->m_debug_messenger);
            if (result != VK_SUCCESS) {
                throw utils::FormattedError("failed to create debug messenger (VkDebugUtilsMessengerEXT) (error code: {})", result);
            }
        }
        else {
            utils::logging::warning("'VK_LAYER_KHRONOS_validation' validation layer is not supported, API validation is disabled");
            
            result = vkCreateInstance(&instance_create_info, nullptr, &m_handle->instance);
            if (result != VK_SUCCESS) {
                throw utils::FormattedError("failed to create Vulkan instance (error code: {})", result);
            }
        }
        
        return true;
    }
    
    bool Context::Builder::configure_instance_extensions() {
        // Enable debugging on debug builds
        #ifndef NDEBUG
            m_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #endif
        
        if (!m_headless) {
            // Applications presenting to the screen require the VK_KHR_surface extension for window surface support
            m_extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
        }
        
        // Enable required GLFW extensions
        unsigned glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        for (unsigned i = 0; i < glfw_extension_count; ++i) {
            m_extensions.emplace_back(glfw_extensions[i]);
        }
        
        unsigned extension_count = 0u;
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    
        std::vector<VkExtensionProperties> extensions(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
        
        unsigned unsupported_extension_count = 0;
        
        for (const char* requested : m_extensions) {
            bool is_extension_supported = false;
            for (const VkExtensionProperties& supported : extensions) {
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
            utils::logging::error("Failed to create Vulkan instance - {} requested extension(s) not supported", unsupported_extension_count);
            return false;
        }

        return true;
    }
    
    bool Context::Builder::initialize_window() {
        Window::Builder builder(m_handle);
        
        builder.set_name(m_handle->m_name);
        
        if (m_fullscreen) {
            builder.enable_fullscreen();
        }
        else {
            builder.set_width(m_width)
                   .set_height(m_height);
        }
        
        m_handle->window = builder.build();
        return m_handle->window != nullptr;
    }
    
    bool Context::Builder::select_physical_device() {
        // Enumerate all available physical devices
        unsigned physical_device_count = 0u;
        vkEnumeratePhysicalDevices(m_handle->instance, &physical_device_count, nullptr);
    
        if (physical_device_count == 0) {
            utils::logging::error("Failed to find a GPU with Vulkan support");
            return false;
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
    
        std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_handle->gpu, m_handle->window->surface, &surface_format_count, surface_formats.data());

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