
#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "vks/core.hpp"
#include <vulkan/vulkan.h>
#include <cstdint> // std::uint32_t
#include <unordered_map> // std::unordered_map
#include <memory> // std::shared_ptr
#include <utility> // std::hash

namespace vks {
    
    // Forward declarations
    class Device;
    
    struct ImageDescription {
        [[nodiscard]] bool operator==(const ImageDescription& other) const;

        std::uint32_t width = 1;
        std::uint32_t height = 1;
        std::uint32_t depth = 1; // For 3D textures
        
        VkFormat format = VK_FORMAT_UNDEFINED;
        
        std::uint32_t mip_levels = 1;
        std::uint32_t layers = 1;
        
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        
        // Optional (will be automatically detected if not set)
        VkImageCreateFlags flags = 0;
        VkImageUsageFlags usage = 0;
    };
    
    struct ImageViewDescription {
        [[nodiscard]] bool operator==(const ImageViewDescription& other) const;
        
        std::uint32_t base_mip_level = 0;
        std::uint32_t mip_count = VK_REMAINING_MIP_LEVELS;
        
        std::uint32_t base_layer = 0;
        std::uint32_t layer_count = VK_REMAINING_ARRAY_LAYERS;

        VkComponentMapping swizzle = {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A
        };
        
        // Optional (will be automatically detected from image properties)
        VkImageViewCreateFlags flags = 0;
        VkImageViewType type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_NONE;
    };
    
    class Image {
        public:
            Image(std::shared_ptr<Device> device);
            ~Image();
            
            operator VkImage() const;
            
            // Update image description
            template <typename Fn>
            void configure_description(Fn&& fn);
            void set_description(const ImageDescription& description);
            
            [[nodiscard]] bool dirty() const;
            
            void build();
            void reset();
            
            // Image view creation
            VkImageView create_view(); // Entire image
            VkImageView create_view(const ImageViewDescription& description);
            
            [[nodiscard]] std::uint32_t get_width() const;
            [[nodiscard]] std::uint32_t get_height() const;
            
            [[nodiscard]] VkFormat get_format() const;
            
        private:
            // For configuring images managed externally by the swapchain
            friend class Swapchain;
            void configure(VkImage image, const ImageDescription& description);
            
            struct ImageViewHash {
                [[nodiscard]] std::size_t operator()(const ImageViewDescription& description) const;
            };
            
            [[nodiscard]] VkImageType get_image_type() const;
            [[nodiscard]] VkImageViewType get_default_view_type() const;
            [[nodiscard]] VkImageAspectFlags get_default_aspect() const;
            
            std::shared_ptr<Device> m_device;
            ImageDescription m_description;

            VkImage m_image;
            VkImageType m_type;
            
            std::unordered_map<ImageViewDescription, VkImageView, ImageViewHash> m_image_views;
            
            bool m_dirty;
            bool m_managed;
    };
    
}


#include "vks/vulkan/image.tpp"

#endif // IMAGE_HPP
