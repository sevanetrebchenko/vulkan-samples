
#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <vulkan/vulkan.h>
#include <memory> // std::shared_ptr

namespace vks {
    
    class Device;
    
    class ImageView {
        public:
            class Builder;
            
            
        private:
        
    };
    
    class ImageView::Builder {
        public:
            
        
        private:
    };
    
    struct ImageDescription {
        ImageDescription& set_name(const char* name);
        
        ImageDescription& set_width(unsigned width);
        ImageDescription& set_height(unsigned height);
        ImageDescription& set_depth(unsigned depth);
        ImageDescription& set_extents(unsigned width, unsigned height, unsigned depth = 1);

        ImageDescription& set_mip_level_count(unsigned count);
        ImageDescription& set_layer_count(unsigned count);
        ImageDescription& set_sample_count(unsigned count);
        
        ImageDescription& set_usage_flags(VkImageUsageFlags flags);
        ImageDescription& set_create_flags(VkImageCreateFlags flags);
        
        ImageDescription& set_format(VkFormat format);
    };
    
    class Image {
        public:
            class Builder;
            ~Image();
            
            ImageView::Builder create_image_view();
            
            void generate_mipmaps();
            
        private:
            Image();
            
            unsigned m_width;
            unsigned m_height;
            unsigned m_depth;
            unsigned m_mip_levels;
            unsigned m_layer_count;
            VkFormat m_format;
            VkImageUsageFlags m_usage_flags;
            VkSampleCountFlagBits m_sample_count;
            VkImageType m_type;
            VkImageCreateFlags m_create_flags;
            VkImageTiling m_tiling;
            VkImage m_image;
            VkDeviceMemory m_memory;
            
            std::weak_ptr<Device> m_device;
    };
    
    class Image::Builder {
        public:
            Builder(std::shared_ptr<Device> device);
            ~Builder();
            
            operator std::shared_ptr<Image>();
            std::shared_ptr<Image> build();
            
            Builder& set_name(const char* name);
            
            Builder& set_width(unsigned width);
            Builder& set_height(unsigned height);
            Builder& set_depth(unsigned depth);
            Builder& set_extents(unsigned width, unsigned height, unsigned depth = 1);
    
            Builder& set_mip_level_count(unsigned count);
            Builder& set_layer_count(unsigned count);
            Builder& set_sample_count(unsigned count);
            
            Builder& set_usage_flags(VkImageUsageFlags flags);
            Builder& set_create_flags(VkImageCreateFlags flags);
            
            Builder& set_format(VkFormat format);
            
        private:
        
    };

    
}

#endif // IMAGE_HPP
