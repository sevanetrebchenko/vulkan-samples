
#include "context.hpp"

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
        Context& context = *m_handle;
        
        // Initialize Vulkan instance
        VkApplicationInfo application_info {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = context.m_name,
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
            if (vkCreateInstance(&instance_create_info, nullptr, &context.m_instance) != VK_SUCCESS) {
                // TODO: error
            }
            
            // Load vkCreateDebugUtilsMessenger function
            static auto vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(context.m_instance, "vkCreateDebugUtilsMessengerEXT");
            if (!vkCreateDebugUtilsMessenger) {
                throw std::runtime_error("failed to load debug messenger create function (is the VK_EXT_debug_utils extension enabled?)");
            }
        
            if (vkCreateDebugUtilsMessenger(context.m_instance, &debug_callback_create_info, nullptr, &context.m_debug_messenger) != VK_SUCCESS) {
                // TODO: error
            }
        }
        else {
            // Validation layers are not supported, create Vulkan instance without a debug messenger
            if (vkCreateInstance(&instance_create_info, nullptr, &context.m_instance) != VK_SUCCESS) {
                // TODO: error
            }
        }
    }
    
    void Context::Builder::initialize_window() {
        Context& context = *m_handle;
        
        // Initialize GLFW
        glfwInit();
        
        // In OpenGL, the window and rendering context (instance) are coupled together
        // In Vulkan, the instance is created by the API itself and context creation should be disabled using GLFW_NO_API
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        
        unsigned width = m_width;
        unsigned height = m_height;
        
        if (m_fullscreen) {
        
        }
        
        context.m_window = Window::Builder(m_handle).set_width(m_width)
                                                    .set_height(m_height)
                                                    .set_name(context.m_name)
                                                    .build();
        
        // Initialize window

    }
    
    
}