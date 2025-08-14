
#include "vks/vulkan/render_context.hpp"
#include "vks/vulkan/device.hpp"
#include "vks/core.hpp"
#include "vks/sample.hpp"
#include <utils/logging.hpp>
#include <algorithm> // std::min

#include "vks/vulkan/shader.hpp"

namespace vks {
    
    void RenderContext::initialize(const Window& window, const SampleRequirements& requirements) {
        collect_requirements(requirements);
        create_vulkan_instance();
        create_surface(window);
        
        DeviceRequirements device_requirements {
            .enabled_features = requirements.enabled_features
        };
        
        m_device = std::make_shared<Device>(m_instance, m_surface, device_requirements);
    }
    
    void RenderContext::shutdown() {
        // TODO: wait for idle
        m_device.reset();
    }
    
    void RenderContext::begin_frame() {
        ShaderCompiler compiler { m_device };
        auto module = compiler.compile({"shaders/sample.frag"});
        
        return;
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
        
        bool extensions_supported = validate_extension_support();
        if (extensions_supported) {
            instance_create_info.ppEnabledExtensionNames = m_extensions.data();
            instance_create_info.enabledExtensionCount = static_cast<std::uint32_t>(m_extensions.size());
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
    
    bool RenderContext::validate_extension_support() const {
        std::uint32_t extension_count;
        CHECK_CALL(vkEnumerateInstanceExtensionProperties, nullptr, &extension_count, nullptr);
    
        std::vector<VkExtensionProperties> extensions(extension_count);
        CHECK_CALL(vkEnumerateInstanceExtensionProperties, nullptr, &extension_count, extensions.data());
        
        bool result = true;
        
        for (const char* requested : m_extensions) {
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
    
    void RenderContext::collect_requirements(const SampleRequirements& requirements) {
        #ifndef NDEBUG
            // Enable debugging on debug builds
            m_layers.emplace_back("VK_LAYER_KHRONOS_validation");
            m_extensions.emplace_back("VK_EXT_debug_utils");
        #endif
        
        // Applications that use a window require windowing extension support
        if (requirements.display_mode != DisplayMode::None) {
            // Typically includes VK_KHR_surface and platform-specific windowing system extension
            std::uint32_t glfw_extension_count;
            const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
            m_extensions.insert(m_extensions.end(), glfw_extensions, glfw_extensions + glfw_extension_count);
        }
        
        if (test(requirements.enabled_features, FeatureFlags::Raytracing) ||
            test(requirements.enabled_features, FeatureFlags::MeshShaders) ||
            test(requirements.enabled_features, FeatureFlags::VariableRateShading)) {
            m_extensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }
    }
    
    void RenderContext::create_surface(const Window& window) {
        m_surface = window.create_surface(m_instance);
    }
    
}