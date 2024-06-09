
#include "sample.hpp"
#include "helpers.hpp"
#include <stdexcept> // std::runtime_error
#include <iostream> // std::cout, std::endl;

Sample::Settings::Settings() : fullscreen(false),
                               headless(false),
                               #ifndef NDEBUG
                                   debug(true)
                               #else
                                   debug(false)
                               #endif
                               {
}

Sample::Sample(const char* name) : instance(nullptr),
                                   enabled_instance_extensions(),
                                   physical_device(nullptr),
                                   enabled_device_extensions(),
                                   physical_device_properties({ }),
                                   physical_device_memory_properties({ }),
                                   physical_device_features({ }),
                                   enabled_physical_device_features({ }),
                                   surface(nullptr),
                                   surface_capabilities({ }),
                                   surface_format({ }),
                                   device(nullptr),
                                   queue(nullptr),
                                   width(1920),
                                   height(1080),
                                   window(nullptr),
                                   name(name),
                                   settings(),
                                   debug_messenger(nullptr),
                                   initialized(false),
                                   running(false) {
}

Sample::~Sample() {
}

void Sample::set_dimensions(int w, int h) {
    on_window_resize(w, h);
}

void Sample::initialize() {
    if (initialized) {
        return;
    }
    
    if (!settings.headless) {
        initialize_glfw();
        initialize_window();
    }
    
    create_vulkan_instance();

    if (settings.debug) {
        initialize_debugging();
    }
    
    create_surface();
    select_physical_device();
    create_logical_device();
    
    initialize_swapchain();
    
    initialize_resources();
    
    initialized = true;
    
    running = true;
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

bool Sample::active() const {
    return running && !glfwWindowShouldClose(window);
}

void Sample::initialize_glfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Create a surface for Vulkan
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

    // Tells the Vulkan driver which global extensions and validation layers are in use
    VkInstanceCreateInfo instance_create_info { };
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &application_info;

    // 'VK_LAYER_KHRONOS_validation' validation layer contains all validation functionality
    const char* validation_layer = "VK_LAYER_KHRONOS_validation";
    
    unsigned supported_validation_layer_count = 0u;
    vkEnumerateInstanceLayerProperties(&supported_validation_layer_count, nullptr);

    std::vector<VkLayerProperties> supported_validation_layers(supported_validation_layer_count);
    vkEnumerateInstanceLayerProperties(&supported_validation_layer_count, supported_validation_layers.data());

    bool found = false;
    for (const VkLayerProperties& supported : supported_validation_layers) {
        if (strcmp(validation_layer, supported.layerName) == 0) {
            found = true;
            
            instance_create_info.ppEnabledLayerNames = &validation_layer;
            instance_create_info.enabledLayerCount = 1u;
            
            break;
        }
    }
    if (!found) {
        std::cerr << "validation layer 'VK_LAYER_KHRONOS_validation' is not supported" << std::endl;
        settings.debug = false;
    }
    
    std::vector<const char*> extensions { };
    
    if (!settings.headless) {
        // Surface extension VK_KHR_SURFACE_EXTENSION_NAME is also guaranteed to be contained within extensions returned by glfwGetRequiredInstanceExtensions
        extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

        // Append GLFW instance extensions.
        unsigned glfw_extension_count = 0u;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        for (unsigned i = 0u; i < glfw_extension_count; ++i) {
            extensions.emplace_back(glfw_extensions[i]);
        }
    }
    
    if (settings.debug) {
        extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Append any sample-specific extensions
    std::vector<const char*> requested_extensions = request_instance_extensions();
    extensions.insert(extensions.end(), requested_extensions.begin(), requested_extensions.end());
    
    // Verify extension support.
    unsigned supported_extension_count = 0u;
    vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, nullptr);

    std::vector<VkExtensionProperties> supported_extensions(supported_extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, supported_extensions.data());
    
    for (const char* requested : extensions) {
        found = false;
        for (const VkExtensionProperties& supported : supported_extensions) {
            if (strcmp(requested, supported.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            // Instance creation will fail
            std::cerr << "extension '" << requested << "' is not supported" << std::endl;
        }
    }

    instance_create_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    instance_create_info.ppEnabledExtensionNames = extensions.data();
    
    // Create Vulkan instance
    if (vkCreateInstance(&instance_create_info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Vulkan instance!");
    }
}

void Sample::initialize_debugging() {
    // Configure Vulkan debug callback
    VkDebugUtilsMessengerCreateInfoEXT debug_callback_create_info { };
    debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_callback_create_info.messageSeverity = /* VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | */ VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_callback_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_callback_create_info.pfnUserCallback = vulkan_debug_callback;
    debug_callback_create_info.pUserData = nullptr;

    // Load vkCreateDebugUtilsMessengerEXT function
    static auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (!vkCreateDebugUtilsMessengerEXT) {
        throw std::runtime_error("failed to load debug messenger create function");
    }

    if (vkCreateDebugUtilsMessengerEXT(instance, &debug_callback_create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to create debug messenger");
    }
}

void Sample::create_surface() {
    // Surface needs to be created after creating the vulkan instance (Vulkan surface may affect physical device selection)
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan surface.");
    }

    // Other fields will be set when physical device is selected
}


void Sample::select_physical_device() {
    // Enumerate all available physical devices
    unsigned physical_device_count = 0u;
    vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);

    if (physical_device_count == 0) {
        throw std::runtime_error("failed to find a GPU that supports Vulkan!");
    }

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());
    
    // For now, use the first device by default
    // TODO: score devices based on queue types, supported features, etc.
    // TODO: check for requested feature support?
    physical_device = physical_devices[0];
    
    // Retrieve physical device properties, features, and memory limits
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
	vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);
	vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);
    
    // Retrieve surface format, color space, and capabilities
    // Because of this step, the physical device selection must happen after the surface is initialized (surface properties are queried on the device itself)
    unsigned surface_format_count = 0u;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr);

    if (surface_format_count == 0u) {
        throw std::runtime_error("selected physical device does not support any surface formats");
    }

    std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats.data());

    surface_format = surface_formats[0]; // Use the first provided format as a default
    for (const VkSurfaceFormatKHR& format : surface_formats) {
        // sRGB color space (VK_FORMAT_B8G8R8A8_SRGB) results in more accurate perceived colors in the final image, as it is a non-linear format that more accurately matches how humans perceive light
        // However, prefer VK_FORMAT_B8G8R8A8_UNORM as this is better suited for intermediate render targets and textures, especially for applications that handle gamma corrections manually (HDR, PBR, etc.)
        // TODO: convert between VK_FORMAT_B8G8R8A8_UNORM and VK_FORMAT_B8G8R8A8_SRGB for final images?
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM) {
            surface_format = format;
            break;
        }
    }
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
}

void Sample::create_logical_device() {
    VkDeviceCreateInfo device_create_info = { };
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    
    // Select enabled device features
    VkPhysicalDeviceFeatures enabled_features = request_device_features();
    device_create_info.pEnabledFeatures = &enabled_features;
    
    // Device extensions
    std::vector<const char*> extensions;
    
    if (!settings.headless) {
        // VK_KHR_swapchain is the extension for swapchain support
        // The swapchain is primarily used for presentation operations and is not required for headless applications
        extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    
    std::vector<const char*> requested_extesions = request_device_extensions();
    extensions.insert(extensions.end(), requested_extesions.begin(), requested_extesions.end());
    
    device_create_info.enabledExtensionCount = static_cast<unsigned>(extensions.size());
    device_create_info.ppEnabledExtensionNames = extensions.data();
    
    // Initialize queue requirements and retrieve queue family indices
    VkQueueFlags requested_queue_types = request_device_queues();
    bool graphics_support_requested = !settings.headless || requested_queue_types & VK_QUEUE_GRAPHICS_BIT;
    bool compute_support_requested = requested_queue_types & VK_QUEUE_COMPUTE_BIT;
    bool transfer_support_requested = requested_queue_types & VK_QUEUE_TRANSFER_BIT;
    bool presentation_support_requested = !settings.headless; // Headless applications do not need presentation support
    
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos { };
    
    // TODO: more intricate device queue selection
    // For now, select ONE queue family that supports graphics, presentation (if required), compute (if requested), and transfer (if requested) operations (assuming there exists such a queue)
    // TODO: multiple queues are not supported
    unsigned queue_family_count = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
    
    unsigned queue_family_index = queue_family_count; // Invalid index
    
    for (unsigned i = 0u; i < queue_family_count; ++i) {
        bool has_graphics_support = queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
        bool has_compute_support = queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
        
        VkBool32 has_presentation_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &has_presentation_support);
        
        // Queues that support both graphics and compute operations also implicitly support transfer operations
        bool has_transfer_support = (has_graphics_support && has_compute_support) || (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT);
        
        bool valid = true;
        if (graphics_support_requested && !has_graphics_support) {
            valid = false;
        }
        if (compute_support_requested && !has_compute_support) {
            valid = false;
        }
        if (presentation_support_requested && !static_cast<bool>(has_presentation_support)) {
            valid = false;
        }
        if (transfer_support_requested && !has_transfer_support) {
            valid = false;
        }
        if (valid) {
            queue_family_index = i;
            
            VkDeviceQueueCreateInfo& queue_create_info = queue_create_infos.emplace_back();
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family_index;
            queue_create_info.queueCount = 1;
    
            // Priority influences scheduling command buffer execution for this queue family
            // Since this framework only uses one queue, this value does not matter
            float queuePriority = 1.0f;
            queue_create_info.pQueuePriorities = &queuePriority;
            
            break;
        }
    }

    if (queue_family_index == queue_family_count) {
        throw std::runtime_error("unable to find queue family that satisfies application requirements");
    }
    
    device_create_info.queueCreateInfoCount = static_cast<unsigned>(queue_create_infos.size());
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    
    if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    
    // Retrieve device queues
    vkGetDeviceQueue(device, queue_family_index, 0u, &queue);
    
    // Command pools are used to allocate / store command buffers
    VkCommandPoolCreateInfo command_pool_create_info { };
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.queueFamilyIndex = queue_family_index; // Each command pool can only allocate command buffers that are submitted on a single type of queue

    // Two options for command allocation strategy:
    //  - VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: command buffers allocated from this pool are short-lived and will be reset/freed in a short amount of time
    //  - VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: command buffers allocated from this pool have longer lifetimes and can be individually reset by calling vkResetCommandBuffer or vkBeginCommandBuffer
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &command_pool_create_info, nullptr, &command_pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command pool");
    }
    
    //
}

void Sample::initialize_window() {
    window = glfwCreateWindow(width, height, name, nullptr, nullptr);
    assert(window);
    
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

void Sample::on_window_resize(int w, int h) {
    width = w;
    height = h;
    
    if (!initialized) {
        // Initializing Vulkan will create the necessary resources at the specified window resolution
        return;
    }
    
    on_window_resized(w, h);
}

void Sample::on_key_press(int key) {
    if (key == GLFW_KEY_ESCAPE) {
        running = false;
    }
    
    on_key_pressed(key);
}

void Sample::on_mouse_button_press(int button) {
    on_mouse_button_pressed(button);
}

void Sample::on_mouse_move(double x, double y) {
    on_mouse_moved(glm::vec2(x, y));
}

void Sample::on_mouse_scroll(double distance) {
    on_mouse_scrolled(distance);
}

void Sample::initialize_resources() {
}

std::vector<const char*> Sample::request_instance_extensions() const {
    return { };
}

std::vector<const char*> Sample::request_device_extensions() const {
    return { };
}

VkPhysicalDeviceFeatures Sample::request_device_features() const {
    return { };
}

VkQueueFlags Sample::request_device_queues() const {
    return { };
}

void Sample::on_window_resized(int w, int h) {
}

void Sample::on_key_pressed(int key) {
}

void Sample::on_mouse_button_pressed(int button) {
}

void Sample::on_mouse_moved(glm::vec2 position) {
}

void Sample::on_mouse_scrolled(double distance) {
}

void Sample::run() {
    glfwPollEvents();
}

void Sample::initialize_swapchain() {
    VkSwapchainCreateInfoKHR swapchain_create_info { };
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = surface;
    
    // Surface format is initialized when physical device is selected
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageArrayLayers = 1; // The number of layers an image consists of (more than 1 for stereoscopic 3D applications)
    
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Do not blend with other windows in the window system
    
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Color attachments (render targets)
    // TODO: transfer src/dst if supported?
//    if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
//		swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
//	}
//	if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
//		swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
//	}
    
    // VK_SHARING_MODE_EXCLUSIVE — ownership has to be explicitly transferred between queue families (has the highest performance)
    // VK_SHARING_MODE_CONCURRENT — images can be used across multiple families and do not have to be explicitly transferred
    // Since this application is build with only one queue family in mind, this is not (yet) supported
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // This function is called on initialization where there is no existing swapchain defined
    // TODO: use old swapchain on resize to reuse resources and still being able to present already acquired images
    swapchain_create_info.oldSwapchain = nullptr;

    // minImageCount represents the minimum number of images required for the swapchain to function with the given present mode
    // Request +1 image to avoid wasting cycles waiting on driver internals to retrieve a new image to render to
    // Note that the number of requested images must not exceed the maximum supported number of swapchain images
    unsigned swapchain_image_count = surface_capabilities.minImageCount + 1;

    // Image count of 0 means there is no maximum
    if (surface_capabilities.maxImageCount > 0 && swapchain_image_count > surface_capabilities.maxImageCount) {
        swapchain_image_count = surface_capabilities.maxImageCount;
    }
    swapchain_create_info.minImageCount = swapchain_image_count;
    
    // Performs global transform to swapchain images before presentation
    VkSurfaceTransformFlagsKHR surface_transform = surface_capabilities.currentTransform;
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        // Prefer an identity transform (noop)
        surface_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    swapchain_create_info.preTransform = (VkSurfaceTransformFlagBitsKHR)surface_transform;
    
    // Vulkan wants the swap extent to match the resolution of the window
    // However, high density displays have screen resolutions that don't correspond to pixel data
    // Some window managers set the currentExtent value to (u32)-1 to indicate that the application window size is determined by the swap chain size
    if (surface_capabilities.currentExtent.width == std::numeric_limits<unsigned>::max() || surface_capabilities.currentExtent.height == std::numeric_limits<unsigned>::max()) {
        // Need to pick a resolution that best matches the window within minImageExtent and maxImageExtent bounds
        // Note: resolution must be specified in pixels, not GLFW screen coordinates
        int w;
        int h;
        glfwGetFramebufferSize(window, &w, &h);

        swapchain_extent.width = glm::clamp(static_cast<unsigned>(w), surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
        swapchain_extent.height = glm::clamp(static_cast<unsigned>(h), surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
    }
    else {
        swapchain_extent = surface_capabilities.currentExtent;
    }
    swapchain_create_info.imageExtent = swapchain_extent;
    
    // Select Vulkan surface presentation model
    // VK_PRESENT_MODE_IMMEDIATE_KHR - images are transferred to the screen right away (may result in visual tearing if the previous frame is still being drawn as a new one arrives)
    // VK_PRESENT_MODE_FIFO_KHR - display takes an image from the front of a FIFO queue when the display is refreshed, program adds rendered frames to the back (vsync, guaranteed to be available)
    // VK_PRESENT_MODE_FIFO_RELAXED_KHR - different only when the program is too slow to present a new frame before the next vertical blank, in which case the image gets transferred immediately upon arrival instead of waiting for a new blank (may result in tearing)
    // VK_PRESENT_MODE_MAILBOX_KHR - triple buffering, older images that are already queued get replaced by newer ones (no tearing, less latency)
    unsigned supported_presentation_mode_count = 0u;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &supported_presentation_mode_count, nullptr);

    std::vector<VkPresentModeKHR> supported_presentation_modes(supported_presentation_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &supported_presentation_mode_count, supported_presentation_modes.data());

    swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR; // Standard option, guaranteed to be available
    for (const VkPresentModeKHR& presentation_mode : supported_presentation_modes) {
        if (presentation_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            // Prefer triple buffering
            swapchain_present_mode = presentation_mode;
        }
    }
    swapchain_create_info.presentMode = swapchain_present_mode;
    
    if (vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swapchain!");
    }
    
    // TODO: if an existing (old) swapchain exists, it must be cleaned up here
    
    // Retrieve handles to swapchain images
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);

    swapchain_images.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());

    // Image views describe how to access the image and which part of the image to access
    swapchain_image_views.resize(swapchain_image_count);

    for (unsigned i = 0u; i < swapchain_image_count; ++i) {
        create_image_view(device, swapchain_images[i], surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, swapchain_image_views[i]);
    }
}
