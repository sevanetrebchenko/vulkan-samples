
#include "sample.hpp"
#include "helpers.hpp"
#include <shaderc/shaderc.hpp>
#include <stdexcept> // std::runtime_error
#include <filesystem> // std::filesystem
#include <iostream> // std::cout, std::endl;
#include <fstream> // std::ifstream

Sample::Settings::Settings() : fullscreen(false),
                               headless(false),
                               #ifndef NDEBUG
                                   debug(true),
                               #else
                                   debug(false),
                               #endif
                               use_depth_buffer(true)
                               {
}

Sample::Sample(const char* name) : instance(nullptr),
                                   enabled_instance_extensions(),
                                   physical_device(nullptr),
                                   enabled_device_extensions(),
                                   physical_device_properties({ }),
                                   physical_device_features({ }),
                                   enabled_physical_device_features({ }),
                                   device(nullptr),
                                   command_pool(nullptr),
                                   command_buffers({ }),
                                   queue_family_index(-1),
                                   queue(nullptr),
                                   render_pass(nullptr),
                                   width(1920),
                                   height(1080),
                                   window(nullptr),
                                   name(name),
                                   surface(nullptr),
                                   surface_capabilities({ }),
                                   surface_format({ }),
                                   swapchain(nullptr),
                                   swapchain_images({ }),
                                   swapchain_image_views({ }),
                                   swapchain_extent(),
                                   swapchain_present_mode(),
                                   depth_buffer(nullptr),
                                   depth_buffer_format(),
                                   depth_buffer_view(),
                                   depth_buffer_memory(),
                                   frame_index(0u),
                                   framebuffers({ }),
                                   is_presentation_complete({ }),
                                   is_rendering_complete({ }),
                                   is_frame_in_flight({ }),
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

void Sample::set_headless(bool headless) {
    settings.headless = headless;
}

void Sample::set_debug_mode(bool enabled) {
    settings.debug = enabled;
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
    create_surface();
    
    select_physical_device();
    create_logical_device();
    
    initialize_swapchain();
    
    create_synchronization_objects();
    
    create_command_pools();
    allocate_command_buffers();
    
    // Some samples do not require the use of the depth buffer
    if (settings.use_depth_buffer) {
        create_depth_buffer();
    }
    
    // Initialize resources needed for the sample
    initialize_render_passes();
    initialize_framebuffers();
    initialize_pipelines();
    
    initialize_resources();
    
    initialized = true;
    running = true;
}

void Sample::run() {
    glfwPollEvents();
    
    // Show FPS in window title
    double current_frame_time = glfwGetTime();
    dt = current_frame_time - last_frame_time;
    last_frame_time = current_frame_time;

    frame_time_accumulator += dt;
    ++frame_count;

    if (frame_time_accumulator > 1.0) {
        char title[256u] = { '\0' };
        snprintf(title, sizeof(title) / sizeof(title[0]) - 1, "%s - %zu fps", name, frame_count);
        glfwSetWindowTitle(window, title);

        frame_count = 0u;
        frame_time_accumulator -= 1.0;
    }
    
    // Fences are used when the CPU needs to know when the GPU has finished executing some work
    // We can optionally attach a fence to work submitted to the GPU that gets signaled when the work is completed
    // Command buffers are re-recorded at the start of every frame, and it is important to not overwrite the current contents of the command buffer while it is still in use for rendering on the GPU
    
    vkWaitForFences(device, 1, &is_frame_in_flight[frame_index], VK_TRUE, std::numeric_limits<std::uint64_t>::max()); // Blocks CPU execution (fence is created in the signaled state so that the first pass through this function doesn't block execution indefinitely)
    vkResetFences(device, 1, &is_frame_in_flight[frame_index]);
    
    render();
    
    // Present to the screen
    VkPresentInfoKHR present_info { };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &is_rendering_complete[frame_index]; // Presenting the image must wait for rendering to complete
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &frame_index;

    // Presentation queue family is mixed in with graphics / compute / transfer families
    vkQueuePresentKHR(queue, &present_info);
    
    // Advance frame
    frame_index = (frame_index + 1) % NUM_FRAMES_IN_FLIGHT;
}

void Sample::shutdown() {
    if (!initialized) {
        return;
    }
    
    // Wait until the logical device finishes all operations currently in flight before shutting down
    vkDeviceWaitIdle(device);
    
    // Destroy any sample-specific resources
    destroy_resources();
    
    // Main pipeline, render pass, and framebuffer resources are owned by the base Sample
    destroy_pipelines();
    destroy_framebuffers();
    destroy_render_passes();
    
    // Some samples do not require the use of the depth buffer
    if (settings.use_depth_buffer) {
        destroy_depth_buffer();
    }
    
    // deallocate_command_buffers();
    destroy_command_pool(); // Command buffers are automatically deallocated with the destruction of the command pool they were allocated from
    
    destroy_synchronization_objects();
    
    destroy_swapchain();
    
    destroy_logical_device();
    destroy_physical_device();
    
    destroy_surface();
    destroy_vulkan_instance();
    
    if (!settings.headless) {
        destroy_window();
        shutdown_glfw();
    }
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

void Sample::shutdown_glfw() {
    glfwTerminate();
}

void Sample::create_vulkan_instance() {
    VkInstanceCreateInfo instance_create_info { };
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    
    // A Vulkan instance represents the connection between the Vulkan API context and the application
    VkApplicationInfo application_info { };
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "vulkan-samples";
    application_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    application_info.pEngineName = "";
    application_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    application_info.apiVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    instance_create_info.pApplicationInfo = &application_info;

    // Tell the Vulkan driver which global extensions and validation layers are in use
    
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
    
    if (settings.debug) {
        VkDebugUtilsMessengerCreateInfoEXT debug_callback_create_info { };
        debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_callback_create_info.messageSeverity =  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_callback_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_callback_create_info.pfnUserCallback = vulkan_debug_callback;
        debug_callback_create_info.pUserData = nullptr;
        
        // In order to debug instance creation and destruction, pass VkDebugUtilsMessengerCreateInfoEXT into the pNext chain of VkInstanceCreateInfo
        // This debug messenger is attached to the instance and will get cleaned up alongside it
        instance_create_info.pNext = &debug_callback_create_info;
        
        // Create Vulkan instance
        if (vkCreateInstance(&instance_create_info, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create Vulkan instance!");
        }
        
        // Creating debug messenger requires a valid instance
        
        // Load vkCreateDebugUtilsMessengerEXT function
        static auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (!vkCreateDebugUtilsMessengerEXT) {
            throw std::runtime_error("failed to load debug messenger create function (is the VK_EXT_debug_utils extension enabled?)");
        }
    
        if (vkCreateDebugUtilsMessengerEXT(instance, &debug_callback_create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to create debug messenger!");
        }
    }
    else {
        // Create Vulkan instance
        if (vkCreateInstance(&instance_create_info, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create Vulkan instance!");
        }
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
    VkDeviceCreateInfo device_ci = { };
    device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    
    // Select enabled device features
    VkPhysicalDeviceFeatures enabled_features = request_device_features();
    device_ci.pEnabledFeatures = &enabled_features;
    
    // Device extensions
    std::vector<const char*> extensions;
    
    if (!settings.headless) {
        // VK_KHR_swapchain is the extension for swapchain support
        // The swapchain is primarily used for presentation operations and is not required for headless applications
        extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    
    std::vector<const char*> requested_extesions = request_device_extensions();
    extensions.insert(extensions.end(), requested_extesions.begin(), requested_extesions.end());
    
    device_ci.enabledExtensionCount = static_cast<unsigned>(extensions.size());
    device_ci.ppEnabledExtensionNames = extensions.data();
    
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
    
    queue_family_index = queue_family_count; // Invalid index
    
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
    
    device_ci.queueCreateInfoCount = static_cast<unsigned>(queue_create_infos.size());
    device_ci.pQueueCreateInfos = queue_create_infos.data();
    
    if (vkCreateDevice(physical_device, &device_ci, nullptr, &device) != VK_SUCCESS) {
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
}

void Sample::initialize_swapchain() {
    VkSwapchainCreateInfoKHR swapchain_ci { };
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.surface = surface;
    
    // Surface format is initialized when physical device is selected
    swapchain_ci.imageFormat = surface_format.format;
    swapchain_ci.imageColorSpace = surface_format.colorSpace;
    swapchain_ci.imageArrayLayers = 1; // The number of layers an image consists of (more than 1 for stereoscopic 3D applications)
    
    swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Do not blend with other windows in the window system
    
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Color attachments (render targets)
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
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // This function is called on initialization where there is no existing swapchain defined
    // TODO: use old swapchain on resize to reuse resources and still being able to present already acquired images
    swapchain_ci.oldSwapchain = nullptr;

    // minImageCount represents the minimum number of images required for the swapchain to function with the given present mode
    // Request +1 image to avoid wasting cycles waiting on driver internals to retrieve a new image to render to
    // Note that the number of requested images must not exceed the maximum supported number of swapchain images
    unsigned swapchain_image_count = surface_capabilities.minImageCount + 1;

    // Image count of 0 means there is no maximum
    if (surface_capabilities.maxImageCount > 0 && swapchain_image_count > surface_capabilities.maxImageCount) {
        swapchain_image_count = surface_capabilities.maxImageCount;
    }
    swapchain_ci.minImageCount = swapchain_image_count;
    
    // Performs global transform to swapchain images before presentation
    VkSurfaceTransformFlagsKHR surface_transform = surface_capabilities.currentTransform;
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        // Prefer an identity transform (noop)
        surface_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    swapchain_ci.preTransform = (VkSurfaceTransformFlagBitsKHR)surface_transform;
    
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
    swapchain_ci.imageExtent = swapchain_extent;
    
    // Select Vulkan surface presentation model
    // VK_PRESENT_MODE_IMMEDIATE_KHR - images are transferred to the screen right away (may result in visual tearing if the previous frame is still being drawn as a new one arrives)
    // VK_PRESENT_MODE_FIFO_KHR - display takes an image from the front of a FIFO queue when the display is refreshed, program adds rendered frames to the back but has to wait when the queue is full (vsync)
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
    swapchain_ci.presentMode = swapchain_present_mode;
    
    if (vkCreateSwapchainKHR(device, &swapchain_ci, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swapchain!");
    }
    
    // TODO: if an existing (old) swapchain exists, it must be cleaned up here
    
    // Retrieve handles to swapchain images
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());

    // Retrieve swapchain image views
    // Image views describe how to access the image and which part of the image to access
    for (unsigned i = 0u; i < swapchain_image_count; ++i) {
        create_image_view(device, swapchain_images[i], surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, swapchain_image_views[i]);
    }
}

void Sample::create_synchronization_objects() {
    // Semaphore: CPU - GPU synchronization (notify the GPU to kick off the next queued operation)
    // A semaphore is used to add explicit ordering between queue operations. Queue operations refer to work submitted to queues, either through command buffers or from function calls
    // Ordering queue operations involves signaling the semaphore in one queue operation and waiting on it in another queue operation. The first operation will signal the semaphore when it finishes, which will kick off the next operation
    // Signal semaphores: once queue operation A on the GPU is completed, it signals the semaphore to begin executing queue operation B that was waiting on the semaphore
    // Wait semaphore: queue operation B waits on the semaphore while queue operation A gets executed

    // Fence: CPU - CPU synchronization (notify the host when the GPU has finished executing a command)
    // Most Vulkan operations are asynchronous, and a fence is used to add a guarantee that whatever work was enqueued before the fence will be completed when the fence is signaled.
    // Fences must be manually reset to unsignal them.

    for (int i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i) {
        VkSemaphoreCreateInfo semaphore_create_info { };
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(device, &semaphore_create_info, nullptr, &is_presentation_complete[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphore (is_presentation_complete)");
        }

        if (vkCreateSemaphore(device, &semaphore_create_info, nullptr, &is_rendering_complete[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphore (is_rendering_complete)");
        }

        VkFenceCreateInfo fence_create_info { };
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Create the fence in the signaled state so that waiting on the fence during the first frame returns immediately

        if (vkCreateFence(device, &fence_create_info, nullptr, &is_frame_in_flight[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fence");
        }
    }
}

//void Sample::create_render_pass() {
//    // https://registry.khronos.org/vulkan/specs/1.2-khr-extensions/html/chap8.html#renderpass-objects
//    // A render pass specifies how attachments should be handled at various stages of the render pass (load, store, and layout transitions)
//    // Render passes minimize the need for explicit synchronization and memory barriers by using subpass dependencies and attachment descriptions
//    // A render pass is (mainly) composed of attachment description(s), subpasses, and subpass dependencies
//
//    // An attachment description describes:
//    //   - The format of the attachment (color, depth, stencil)
//    //   - The number of samples, used for multisampling
//    //   - Load + store operations, what should happen to the attachment at the beginning (load) and end (store) of the render pass
//    //   - The initial (before the render pass begins) and final (after the render pass ends) layouts of the attachment
//
//    std::vector<VkAttachmentDescription> attachment_descriptions;
//
//    VkAttachmentDescription color_attachment_description { };
//
//        // Format: color attachment (format of a swapchain image)
//        color_attachment_description.format = surface_format.format;
//
//        // Number of samples
//        color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: expose for samples to set
//
//        // What to do with the attachment data before the render pass:
//        //   - VK_ATTACHMENT_LOAD_OP_LOAD - preserve the existing contents of the attachment
//        //   - VK_ATTACHMENT_LOAD_OP_CLEAR - clear the values to a constant at the start
//        //   - VK_ATTACHMENT_LOAD_OP_DONT_CARE - existing contents are undefined
//        color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//
//        // What to do with the attachment data after the render pass:
//        //   - VK_ATTACHMENT_STORE_OP_STORE - rendered contents will be stored in memory and can be read later
//        //   - VK_ATTACHMENT_STORE_OP_DONT_CARE - contents of the framebuffer will be undefined after the rendering operation
//        color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//
//        // TODO: stencil buffer load / store configurations
//        color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//        color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//
//        // Render pass images need to be transitioned to specific layouts that are suitable for the next operation
//        //   - VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL - images used as color attachment
//        //   - VK_IMAGE_LAYOUT_PRESENT_SRC_KHR - images to be presented in the swap chain
//        //   - VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL - images to be used as destination for a memory copy operation
//
//        // Layout the image before the start of the render pass
//        color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//
//        // Layout of the image transitioned to the end of the render pass
//        color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//
//    attachment_descriptions.emplace_back(color_attachment_description);
//
//    // Depth attachment
//    VkAttachmentDescription depth_attachment_description { };
//
//        depth_attachment_description.format = depth_buffer_format;
//        depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: multisampled depth buffer for multisampling support
//        depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//        depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Depth information will not be used after drawing is finished
//        depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//        depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//        depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//        depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//    attachment_descriptions.emplace_back(depth_attachment_description);
//
//    // TODO: attachment for resolving multisampled images
////    VkAttachmentDescription color_resolve_attachment_description { };
//    //    color_resolve_attachment_description.format = surface_format.format;
//    //    color_resolve_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT; // Final image must be 1 spp.
//    //    color_resolve_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//    //    color_resolve_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//    //    color_resolve_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//    //    color_resolve_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//    //    color_resolve_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//    //    color_resolve_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
////    attachment_descriptions.emplace_back(color_resolve_attachment_description);
//
//    // A subpass describes:
//    //   - Which attachments are being read from within the subpass, typically assigned as inputs to fragment shaders (such as for deferred rendering, where the output from one subpass is used as input for another)
//    //   - Attachments that will be used as color outputs
//    //   - Attachments that will be used to resolve multisampled color attachments (only used when multisampling is enabled)
//    //   - Attachments that will store depth / stencil information
//    //   - Attachments that are not used but preserved throughout the pass
//
//    std::vector<VkAttachmentReference> color_attachments;
//
//    // Color attachment
//    VkAttachmentReference color_attachment { };
//
//        // The 'attachment' index is referenced by the layout(location = 0) out vec4 outColor line in the fragment shader
//        // Note: this must also align with the first element in the attachment_descriptions array initialized above (order of attachment descriptions and attachment references matters), as this specifies which attachment is being referenced
//        color_attachment.attachment = 0;
//
//        // Layout of the attachment when used during the subpass
//        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//    color_attachments.emplace_back(color_attachment);
//
//    // Depth attachment
//    VkAttachmentReference depth_attachment { };
//
//        depth_attachment.attachment = 1;
//        depth_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//    // TODO: attachment reference for resolving multisampled attachments
////    VkAttachmentReference color_resolve_attachment { };
////    color_resolve_attachment.attachment = 2;
////    color_resolve_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//    // Define subpass
//    VkSubpassDescription subpass { };
//    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//    subpass.colorAttachmentCount = color_attachments.size();
//    subpass.pColorAttachments = color_attachments.data();
//    subpass.pDepthStencilAttachment = &depth_attachment; // Attachments to perform depth testing on (can only have 1 depth attachment bound at one time)
//    subpass.pResolveAttachments = nullptr;
//    subpass.inputAttachmentCount = 0;
//    subpass.pInputAttachments = nullptr;
//    subpass.preserveAttachmentCount = 0;
//    subpass.pPreserveAttachments = nullptr;
//
//    // TODO: understand subpass dependencies
////    // Define subpass dependencies
////    // Images in a render pass need to be transitioned to the proper image layout
////    // This is automatically handled by subpasses through the use of subpass depedencies, which specify memory and execution dependencies and ensure correct ordering and access synchronization of memory reads / writes between subpasses
////    // Even with a single subpass, operations right before and right after count as subpasses implicitly, and need to be transitioned accordingly
////    std::vector<VkSubpassDependency> subpass_dependencies;
////
////    // Subpass dependency for transitioning the color attachment
////
////    VkSubpassDependency subpass_dependency { };
////
////    // Depth subpass dependency
////    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Subpass
////    subpass_dependency.dstSubpass = 0; //
//
//    // https://registry.khronos.org/vulkan/specs/1.2-khr-extensions/html/chap8.html#renderpass-store-operations
//    // LOAD operations define the initial values of an attachment at the start of a render pass
//    //   For attachments with a depth/stencil format, LOAD operations execute during the VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT pipeline stage
//    //   For attachments with a color format, LOAD operations execute during the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT pipeline stage
//
//    // LOAD operations that can be used for a render pass:
//    //   - VK_ATTACHMENT_LOAD_OP_LOAD      - previous attachment contents will be preserved as initial values
//    //                                     - memory access type: VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT (depth/stencil), VK_ACCESS_COLOR_ATTACHMENT_READ_BIT (color)
//    //   - VK_ATTACHMENT_LOAD_OP_CLEAR     - previous attachment contents will be cleared to a uniform value
//    //                                     - memory access type: VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT (depth/stencil), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT (color)
//    //   - VK_ATTACHMENT_LOAD_OP_DONT_CARE - previous attachment contents will not be preserved or cleared (undefined)
//    //                                     - memory access type: VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT (depth/stencil), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT (color)
//    //   - VK_ATTACHMENT_LOAD_OP_NONE_KHR  - previous attachment is not used
//    //                                     - memory access type: N/A
//
//    // https://registry.khronos.org/vulkan/specs/1.2-khr-extensions/html/chap8.html#renderpass-store-operations
//    // STORE operations define how the values written to an attachment during a render pass are stored in memory
//    //   For attachments with a depth/stencil format, STORE operations execute during the VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT pipeline stage
//    //   For attachments with a color format, STORE operations execute during the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT pipeline stage
//
//    // STORE operations that can be used for a render pass:
//    //   - VK_ATTACHMENT_STORE_OP_STORE - attachment contents generated during the render pass are written to memory
//    //                                  - memory access type: VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT (depth/stencil), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT (color)
//    //   - VK_ATTACHMENT_STORE_OP_DONT_CARE - attachment contents are not needed after the render pass, and may be discarded (undefined)
//    //                                      - memory access type: VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT (depth/stencil), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT (color)
//    //   - VK_ATTACHMENT_STORE_OP_NONE - attachment contents are not referenced by the STORE operation as long as no values are written (*otherwise same as VK_ATTACHMENT_STORE_OP_DONT_CARE)
//    //                                 - memory access type: *N/A
//
//
//
//
//    // https://registry.khronos.org/vulkan/specs/1.2-khr-extensions/html/chap7.html#VkPipelineStageFlagBits
//    // https://registry.khronos.org/vulkan/specs/1.2-khr-extensions/html/chap7.html#VkAccessFlagBits
//
////    // Subpass automatically transitions image to the desired format
////
////    auto LOAD = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
////    auto STORE = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
////
////    // Subpass dependency for ordering writes to the depth buffer
////    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
////    subpass_dependency.dstSubpass = 0;
////
////    // Depth buffer will be read from to see if a fragment is visible during VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT and written to when a new fragment is drawn during VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
////
////    // Wait for both early and late fragment testing to complete before proceeding with clearing the depth buffer
////    subpass_dependency.srcStageMask = LOAD | STORE;
////    subpass_dependency.dstStageMask = LOAD | STORE;
////
////    subpass_dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
////	subpass_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
////
////
////    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
////    subpass_dependency.dstSubpass = 0;
////
////    // srcStageMask specifies that pipeline stages that must be completed before
////    // Wait on the output from the color attachment itself (wait for the swapchain to finish reading from the source image before accessing it).
////
////    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT ;
////    subpass_dependency.srcAccessMask = 0;
////
////    // Wait on the color attachment stage to perform color writing operations to the attachment.
////    // Wait on the depth attachment stage to perform depth write operations to the depth attachment.
////    // This ensures the image is transitioned only when the render pass is ready to write color/depth data to it.
////    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
////    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//
//    VkRenderPassCreateInfo render_pass_create_info { };
//    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//    render_pass_create_info.attachmentCount = attachment_descriptions.size();
//    render_pass_create_info.pAttachments = attachment_descriptions.data();
//    render_pass_create_info.subpassCount = 1;
//    render_pass_create_info.pSubpasses = &subpass;
//    render_pass_create_info.dependencyCount = 0;
//    render_pass_create_info.pDependencies = nullptr;
//
//    if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create render pass!");
//    }
//}

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
    // TODO: this needs to account for swapchain limits
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

void Sample::create_depth_buffer() {
    // Formats that contain a depth component, ordered from best to worst
    std::vector<VkFormat> formats { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL; // Must be one of VK_IMAGE_TILING_OPTIMAL or VK_IMAGE_TILING_LINEAR
    VkFormatFeatureFlags depth_image_features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT; // Want the depth texture to act as an attachment
    
    VkFormat image_format = VK_FORMAT_UNDEFINED;
    
    // Ensure that the selected physical device supports the requested depth format
    for (VkFormat format : formats) {
        VkFormatProperties physical_device_format_properties { };
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &physical_device_format_properties);
        
        if (tiling == VK_IMAGE_TILING_OPTIMAL) {
            if ((physical_device_format_properties.optimalTilingFeatures & depth_image_features) == depth_image_features) {
                image_format = format;
            }
        }
        else if (tiling == VK_IMAGE_TILING_LINEAR) {
            if ((physical_device_format_properties.linearTilingFeatures & depth_image_features) == depth_image_features) {
                image_format = format;
            }
        }
    }
    
    if (image_format == VK_FORMAT_UNDEFINED) {
        throw std::runtime_error("physical device does not support desired depth format!");
    }
    
    depth_buffer_format = image_format;
    unsigned depth_mip_levels = 1;
    
    create_image(physical_device, device,
                 swapchain_extent.width, swapchain_extent.height, // Depth image needs to be the same size as any other framebuffer attachment
                 depth_mip_levels,
                 VK_SAMPLE_COUNT_1_BIT, // TODO: this assumes 1 sample per pixel, which is not the case for multisampling (this method should be virtual)
                 // Depth buffers do not require a separate resolve step and can be used directly in render passes for resolving to a single sample / presentation
                 depth_buffer_format,
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // The most optimal memory type for GPU reads is VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT (meant for device read, not accessible by the CPU)
                 depth_buffer, depth_buffer_memory);
    vkBindImageMemory(device, depth_buffer, depth_buffer_memory, 0); // Associate memory buffer with image
    
    create_image_view(device, depth_buffer, image_format, VK_IMAGE_ASPECT_DEPTH_BIT, depth_mip_levels, depth_buffer_view);
}

void Sample::create_command_pools() {
    // This function must be called after queue families are selected, since commands are submitted to a specific type of queue
    
    // Command pools are used to allocate / store command buffers
    VkCommandPoolCreateInfo command_pool_create_info { };
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.queueFamilyIndex = queue_family_index;

    // Two options for command buffer allocation strategy:
    //  - VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: command buffers allocated from this pool are short-lived and will be reset/freed in a short amount of time
    //  - VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: command buffers allocated from this pool have longer lifetimes and can be individually reset by calling vkResetCommandBuffer or vkBeginCommandBuffer
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &command_pool_create_info, nullptr, &command_pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate main command pool!");
    }

    // Allocate a command pool for transient command buffers
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.queueFamilyIndex = queue_family_index;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    if (vkCreateCommandPool(device, &command_pool_create_info, nullptr, &transient_command_pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate transient command pool!");
    }
}

void Sample::allocate_command_buffers() {
    // This function must be called after the command pool is initialized
    
    VkCommandBufferAllocateInfo command_buffer_ai { };
    command_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_ai.commandPool = command_pool;
    
    // Two options for command buffer level:
    //   - VK_COMMAND_BUFFER_LEVEL_PRIMARY: command buffers of this type are used for the main rendering or compute operations, and can be submitted to a queue for execution
    //   - VK_COMMAND_BUFFER_LEVEL_SECONDARY: command buffers of this type are used to bundle reusable sets of rendering / compute commands that can be included into one or more primary command buffers using vkCmdExecuteCommands
    command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    
    // Allocate a command buffer per swapchain image
    command_buffer_ai.commandBufferCount = NUM_FRAMES_IN_FLIGHT;
    
    if (vkAllocateCommandBuffers(device, &command_buffer_ai, command_buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void Sample::deallocate_command_buffers() {
    // vkFreeCommandBuffers(device, command_pool, NUM_FRAMES_IN_FLIGHT, command_buffers.data());
}

VkShaderModule Sample::load_shader(const char* filepath) {
    // Read shader into memory
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open shader: " + std::string(filepath));
    }

    std::streamsize file_size = file.tellg();
    std::string source;
    source.resize(file_size);

    // Read data
    file.seekg(0);
    file.read(source.data(), file_size);
    file.close();

    shaderc::CompileOptions options { };
    
    // TODO: configure shader defines

    // TODO: support multiple shader languages
    shaderc_shader_kind type;
    std::filesystem::path path = std::filesystem::path(filepath);
    std::filesystem::path extension = path.extension();
    if (extension == ".vert") {
        type = shaderc_glsl_vertex_shader;
    }
    else if (extension == ".frag") {
        type = shaderc_glsl_fragment_shader;
    }
    else {
        throw std::runtime_error("unknown shader type!");
    }
    
    shaderc::Compiler compiler { };
    std::string filename = path.stem().u8string(); // Convert from wchar_t
    
    // Function assumes entry point is 'main'
    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, type, filename.c_str());
    
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error("failed to compile shader: " + result.GetErrorMessage());
    }
    
    std::vector<unsigned> spirv = { result.cbegin(), result.cend() };
    VkShaderModuleCreateInfo shader_module_create_info { };
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = spirv.size() * sizeof(unsigned); // Bytes
    shader_module_create_info.pCode = spirv.data();
    
    VkShaderModule shader_module { };
    if (vkCreateShaderModule(device, &shader_module_create_info, nullptr, &shader_module) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    
    return shader_module;
}

void Sample::destroy_vulkan_instance() {
    if (settings.debug) {
        static auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (vkDestroyDebugUtilsMessengerEXT) {
            vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
        }
    }
    
    vkDestroyInstance(instance, nullptr);
}

void Sample::destroy_window() {
    glfwDestroyWindow(window);
}

void Sample::destroy_physical_device() {
    // Physical device gets shutdown when the Vulkan instance is destroyed
}

void Sample::destroy_logical_device() {
    // Device queues get automatically cleaned up when the logical device is destroyed
    vkDestroyDevice(device, nullptr);
}

void Sample::destroy_surface() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
}

void Sample::destroy_swapchain() {
    // Destroy swapchain image views
    for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyImageView(device, swapchain_image_views[i], nullptr);
    }

    // Swapchain images are owned by the swapchain
    // Validation layer message : vkDestroyImage(): VkImage is a presentable image and it is controlled by the implementation and is destroyed with vkDestroySwapchainKHR
    
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Sample::destroy_command_pool() {
    vkDestroyCommandPool(device, transient_command_pool, nullptr);
    vkDestroyCommandPool(device, command_pool, nullptr);
}

void Sample::destroy_synchronization_objects() {
    for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(device, is_rendering_complete[i], nullptr);
    }
    
    for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(device, is_presentation_complete[i], nullptr);
    }
    
    for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyFence(device, is_frame_in_flight[i], nullptr);
    }
}

void Sample::destroy_depth_buffer() {
    // Depth buffer consists of the image, image view, and allocated device memory
    vkDestroyImageView(device, depth_buffer_view, nullptr);
    vkFreeMemory(device, depth_buffer_memory, nullptr);
    vkDestroyImage(device, depth_buffer, nullptr);
}

void Sample::destroy_framebuffers() {
    for (std::size_t i = 0u; i < NUM_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
    }
}

VkCommandBuffer Sample::allocate_transient_command_buffer() {
    VkCommandBufferAllocateInfo command_buffer_allocate_info { };
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandPool = transient_command_pool;
    command_buffer_allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    if (vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate transient command buffer!");
    }
    
    return command_buffer;
}

void Sample::deallocate_transient_command_buffer(VkCommandBuffer command_buffer) {
    vkFreeCommandBuffers(device, transient_command_pool, 1, &command_buffer);
}
