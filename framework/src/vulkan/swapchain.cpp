
#include <utility>

#include "vks/vulkan/swapchain.hpp"
#include "vks/sample.hpp"
#include "vks/core.hpp"

namespace vks {
    
    Swapchain::Swapchain(std::shared_ptr<Device> device, VkSurfaceKHR surface, const SampleRequirements& requirements) : m_device(std::move(device)),
                                                                                                                         m_surface(surface),
                                                                                                                         m_swapchain(VK_NULL_HANDLE) {
        recreate(requirements.width, requirements.height);
    }
    
    Swapchain::~Swapchain() {
        VkDevice device = m_device->get_device();
        
        // Destroy swapchain image views
        
        // Swapchain images are controlled by the implementation and are destroyed alongside vkDestroySwapchainKHR
        vkDestroySwapchainKHR(device, m_swapchain, nullptr);
    }
    
    void Swapchain::recreate(std::uint32_t width, std::uint32_t height) {
        // Retrieve surface properties, as these are unique per monitor
        CHECK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, m_device->get_physical_device(), m_surface, &m_surface_properties);
        
        m_surface_format = select_surface_format();
        m_presentation_mode = select_presentation_mode();
        m_extent = get_extent(width, height);
        
        VkSwapchainKHR previous = m_swapchain;
        create_swapchain(previous);
        vkDestroySwapchainKHR(m_device->get_device(), previous, nullptr);
        
        retrieve_swapchain_images();
    }
    
    VkSurfaceFormatKHR Swapchain::select_surface_format() const {
        VkPhysicalDevice gpu = m_device->get_physical_device();
        
        std::uint32_t format_count;
        CHECK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR, gpu, m_surface, &format_count, nullptr);
        
        if (format_count == 0) {
            utils::logging::fatal("GPU does not support any formats for presentation");
        }
        
        std::vector<VkSurfaceFormatKHR> formats(format_count);
        CHECK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR, gpu, m_surface, &format_count, formats.data());
        
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
    
    VkPresentModeKHR Swapchain::select_presentation_mode() const {
        VkPhysicalDevice gpu = m_device->get_physical_device();
        
        // Select Vulkan surface presentation model
        // VK_PRESENT_MODE_IMMEDIATE_KHR - images are transferred to the screen right away (may result in visual tearing if the previous frame is still being drawn as a new one arrives)
        // VK_PRESENT_MODE_FIFO_KHR - display takes an image from the front of a FIFO queue when the display is refreshed, program adds rendered frames to the back but has to wait when the queue is full (vsync)
        // VK_PRESENT_MODE_FIFO_RELAXED_KHR - different only when the program is too slow to present a new frame before the next vertical blank, in which case the image gets transferred immediately upon arrival instead of waiting for a new blank (may result in tearing)
        // VK_PRESENT_MODE_MAILBOX_KHR - triple buffering, older images that are already queued get replaced by newer ones (no tearing, less latency)
        std::uint32_t presentation_mode_count;
        CHECK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR, gpu, m_surface, &presentation_mode_count, nullptr);
    
        std::vector<VkPresentModeKHR> presentation_modes(presentation_mode_count);
        CHECK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR, gpu, m_surface, &presentation_mode_count, presentation_modes.data());
    
        VkPresentModeKHR presentation_mode = VK_PRESENT_MODE_FIFO_KHR; // Standard option, guaranteed to be available
        for (const VkPresentModeKHR& mode : presentation_modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                // Prefer triple buffering
                presentation_mode = mode;
            }
        }
        
        return presentation_mode;
    }
    
    VkExtent2D Swapchain::get_extent(std::uint32_t width, std::uint32_t height) const {
        if (m_surface_properties.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
            // High DPI displays often set currentExtent to UINT32_MAX, which means the extent is dictated by the window manager
            return m_surface_properties.currentExtent;
        }
        
        // Window manager allows us to choose swapchain extent
        // Clamp requested size to supported bounds
        return {
            std::clamp(width, m_surface_properties.minImageExtent.width, m_surface_properties.maxImageExtent.width),
            std::clamp(height, m_surface_properties.minImageExtent.height, m_surface_properties.maxImageExtent.height)
        };
    }
    
    void Swapchain::create_swapchain(VkSwapchainKHR previous) {
        VkSwapchainCreateInfoKHR swapchain_create_info { };
        swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_create_info.surface = m_surface;
        
        // minImageCount represents the minimum number of images required for the swapchain to function with the given present mode
        // Request +1 image to avoid wasting cycles waiting on driver internals to retrieve a new image to render to
        std::uint32_t swapchain_image_count = m_surface_properties.minImageCount + 1;
        
        // maxImageCount must not exceed the maximum supported number of swapchain images
        // Image count of 0 means there is no upper bound
        if (m_surface_properties.maxImageCount > 0 && swapchain_image_count > m_surface_properties.maxImageCount) {
            swapchain_image_count = m_surface_properties.maxImageCount;
        }
        swapchain_create_info.minImageCount = swapchain_image_count;
        
        swapchain_create_info.imageFormat = m_surface_format.format;
        swapchain_create_info.imageColorSpace = m_surface_format.colorSpace;
        swapchain_create_info.imageExtent = m_extent;
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        
        // Allow swapchain images to be used as transfer source / destination
        if (m_surface_properties.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
    		swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    	}
    	if (m_surface_properties.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
    		swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    	}
        
        // Performs global transform to swapchain images before presentation
        VkSurfaceTransformFlagsKHR surface_transform = m_surface_properties.currentTransform;
        if (m_surface_properties.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
            surface_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // Identity (noop)
        }
        swapchain_create_info.preTransform = (VkSurfaceTransformFlagBitsKHR) surface_transform;
        
        swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Do not blend with other windows in the window system
        swapchain_create_info.presentMode = m_presentation_mode;
        swapchain_create_info.clipped = VK_TRUE;
        swapchain_create_info.oldSwapchain = previous; // For reusing resources on swapchain recreation
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Ownership of swapchain images needs to be explicitly transferred between queue families
        
        VkDevice device = m_device->get_device();
        CHECK_CALL(vkCreateSwapchainKHR, device, &swapchain_create_info, nullptr, &m_swapchain);
    }
    
    void Swapchain::retrieve_swapchain_images() {
    
    }
    
}