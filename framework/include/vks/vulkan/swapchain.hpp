
#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include "vks/vulkan/device.hpp"
#include "vks/vulkan/image.hpp"
#include <vulkan/vulkan.h>
#include <memory> // std::shared_ptr

namespace vks {
    
    struct SwapchainDescription {
        std::uint32_t width;
        std::uint32_t height;
        
        std::uint32_t swapchain_image_count;
    };
    
    class Swapchain {
        public:
            Swapchain(std::shared_ptr<Device> device, VkSurfaceKHR surface, const SwapchainDescription& description);
            ~Swapchain();
            
            void recreate(std::uint32_t width, std::uint32_t height);
            
        private:
            [[nodiscard]] VkSurfaceFormatKHR select_surface_format() const;
            [[nodiscard]] VkPresentModeKHR select_presentation_mode() const;
            [[nodiscard]] VkExtent2D get_extent(std::uint32_t width, std::uint32_t height) const;
            
            // Returns description of a swapchain image
            ImageDescription create_swapchain(VkSwapchainKHR previous);

            void retrieve_swapchain_images(const ImageDescription& description);
            
            std::shared_ptr<Device> m_device;
            VkSurfaceKHR m_surface;
            VkSurfaceCapabilitiesKHR m_surface_properties;
            
            VkSwapchainKHR m_swapchain;
            std::vector<std::shared_ptr<Image>> m_swapchain_images;
            
            // Configuration
            VkSurfaceFormatKHR m_surface_format;
            VkPresentModeKHR m_presentation_mode;
            VkExtent2D m_extent;
    };
    
}

#endif // SWAPCHAIN_HPP
