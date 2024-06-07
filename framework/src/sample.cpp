
#include "sample.hpp"
#include <stdexcept> // std::runtime_error
#include <iostream> // std::cout, std::endl;

Sample::Sample(const char* name) : name(name) {
    #ifndef NDEBUG
        settings.debug = true;
    #else
        settings.debug = false;
    #endif
}

Sample::~Sample() {
}

void Sample::initialize_window(int w, int h) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Create a surface for Vulkan

    window = glfwCreateWindow(w, h, name, nullptr, nullptr);
    assert(window);
    
    width = w;
    height = h;
    
    glfwSetWindowUserPointer(window, this);
    
    // Initialize window callbacks
    glfwSetKeyCallback(window, [](GLFWwindow* win, int key, int, int action, int) {
        Sample* sample = reinterpret_cast<Sample*>(glfwGetWindowUserPointer(win));
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            sample->on_key_press(key);
        }
    });
    
    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int) {
        Sample* sample = reinterpret_cast<Sample*>(glfwGetWindowUserPointer(win));
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            sample->on_mouse_button_press(button);
        }
    });
    
    glfwSetScrollCallback(window, [](GLFWwindow* win, double x, double y) {
        Sample* sample = reinterpret_cast<Sample*>(glfwGetWindowUserPointer(win));
        sample->on_mouse_scroll(y);
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y) {
        Sample* sample = reinterpret_cast<Sample*>(glfwGetWindowUserPointer(win));
        sample->on_mouse_move(x, y);
    });

    // glfwSetFramebufferSizeCallback(window, framebuffer_resized_callback);
}

void Sample::initialize_vulkan() {
    create_vulkan_instance();

    if (settings.debug) {
        // Enable validation layers for debug builds
        initialize_debugging();
    }

    // Enable requested extensions
    // Establish a connection to the physical device (GPU)
}

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            std::cout << "[TRACE]";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            std::cout << "[INFO]";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            // Behavior that is not necessarily an error, but very likely a bug
            std::cout << "[WARNING]";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            // Behavior that is invalid and may cause crashes
            std::cout << "[ERROR]";
            break;
        default:
            break;
    }

    switch (type) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            // General, unrelated to specification or performance
            std::cout << "[GENERAL] - ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            // Violation of the specification, indicates possible mistakes
            std::cout << "[VALIDATION] - ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            // Performance, non-optimal use of Vulkan
            std::cout << "[PERFORMANCE] - ";
            break;
        default:
            break;
    }

    std::cout << callback_data->pMessage << std::endl;
    return VK_FALSE; // Whether the Vulkan call that triggered the validation layer message should be aborted
}

void Sample::create_vulkan_instance() {
    // A Vulkan instance represents the connection between the Vulkan API context and the application
    VkApplicationInfo application_info { };
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "vulkan-samples";
    application_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    application_info.pEngineName = "";
    application_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    application_info.apiVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);

    // Tells the Vulkan driver which global extensions and validation layers are in use.
    VkInstanceCreateInfo instance_create_info { };
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &application_info;

    if (vkCreateInstance(&instance_create_info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }
    
    // VK_LAYER_KHRONOS_validation contains all validation functionality
    // Check to see if it is supported
    const char* validation_layer = "VK_LAYER_KHRONOS_validation";
    
    unsigned validation_layer_count = 0u;
    vkEnumerateInstanceLayerProperties(&validation_layer_count, nullptr);

    std::vector<VkLayerProperties> available_validation_layers(validation_layer_count);
    vkEnumerateInstanceLayerProperties(&validation_layer_count, available_validation_layers.data());

    bool found = false;
    for (const VkLayerProperties& available : available_validation_layers) {
        if (strcmp(validation_layer, available.layerName) == 0) {
            found = true;
            
            instance_create_info.ppEnabledLayerNames = &validation_layer;
            instance_create_info.enabledLayerCount = 1u;
            
            break;
        }
    }
    if (!found) {
        std::cerr << "validation layer VK_LAYER_KHRONOS_validation is not supported, debug validation is explicitly disabled" << std::endl;
        settings.debug = false;
    }
    
    std::vector<const char*> extensions {
        VK_KHR_SURFACE_EXTENSION_NAME,
    };
    if (settings.debug) {
        extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    // Append GLFW instance extensions.
    unsigned glfw_extension_count = 0u;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    for (unsigned i = 0u; i < glfw_extension_count; ++i) {
        extensions.emplace_back(glfw_extensions[i]);
    }

    // Verify extension support.
    unsigned extension_count = 0u;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data());
    
    for (const char* requested : extensions) {
        found = false;
        for (const VkExtensionProperties& available : available_extensions) {
            if (strcmp(requested, available.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            // Instance creation will fail
            std::cerr << "extension " << requested << " is not supported" << std::endl;
        }
    }

    instance_create_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    instance_create_info.ppEnabledExtensionNames = extensions.data();
    
    // Create Vulkan instance
    if (vkCreateInstance(&instance_create_info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Vulkan instance");
    }
}

void Sample::initialize_debugging() {
    // Configure Vulkan debug callback.
    VkDebugUtilsMessengerCreateInfoEXT debug_callback_create_info { };
    debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_callback_create_info.messageSeverity = /* VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | */ VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_callback_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_callback_create_info.pfnUserCallback = vulkan_debug_callback;
    debug_callback_create_info.pUserData = nullptr;

    // vkCreateDebugUtilsMessengerEXT function needs to be loaded in
    static auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (!vkCreateDebugUtilsMessengerEXT) {
        throw std::runtime_error("failed to load debug messenger create function");
    }

    if (vkCreateDebugUtilsMessengerEXT(instance, &debug_callback_create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to create debug messenger");
    }
}
