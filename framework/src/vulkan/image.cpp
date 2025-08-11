
#include "vks/vulkan/image.hpp"
#include "vks/vulkan/device.hpp"
#include <utils/hash.hpp>

namespace vks {
    
    bool ImageDescription::operator==(const ImageDescription& other) const {
        return width == other.width &&
               height == other.height &&
               depth == other.depth &&
               format == other.format &&
               mip_levels == other.mip_levels &&
               layers == other.layers &&
               samples == other.samples &&
               flags == other.flags &&
               usage == other.usage;
    }
    
    bool ImageViewDescription::operator==(const ImageViewDescription& other) const {
        return base_mip_level == other.base_mip_level &&
               mip_count == other.mip_count &&
               base_layer == other.base_layer &&
               layer_count == other.layer_count &&
               swizzle.r == other.swizzle.r &&
               swizzle.g == other.swizzle.g &&
               swizzle.b == other.swizzle.b &&
               swizzle.a == other.swizzle.a &&
               flags == other.flags &&
               type == other.type &&
               aspect == other.aspect;
    }
    
    Image::Image(std::shared_ptr<Device> device) : m_device(std::move(device)),
                                                   m_description(),
                                                   m_image(VK_NULL_HANDLE),
                                                   m_type(VK_IMAGE_TYPE_MAX_ENUM),
                                                   m_dirty(true), // Defer image initialization
                                                   m_managed(false) {
    }
    
    Image::~Image() {
        reset();
    }
    
    Image::operator VkImage() const {
        return m_image;
    }
    
    void Image::set_description(const ImageDescription& description) {
        m_dirty = m_description != description;
        m_description = description;
    }
    
    bool Image::dirty() const {
        return m_dirty;
    }
    
    void Image::build() {
        if (!m_dirty) {
            return;
        }
        
        m_type = get_image_type();
        
        // Images that are managed externally should not be recreated / destroyed
        if (!m_managed) {
            VkImageCreateInfo image_create_info {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .flags = m_description.flags,
                .imageType = m_type,
                .format = m_description.format,
                .extent = { m_description.width, m_description.height, m_description.depth },
                .mipLevels = m_description.mip_levels,
                .arrayLayers = m_description.layers,
                .samples = m_description.samples,
                .tiling = VK_IMAGE_TILING_OPTIMAL, // TODO: support LINEAR?
                .usage = m_description.usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // No sharing between queue families
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
            };
            
            CHECK_CALL(vkCreateImage, *m_device, &image_create_info, nullptr, &m_image);
        }
        
        m_dirty = false;
    }
    
    void Image::reset() {
        // Automatic recreation of image views is too fragile due to compatibility issues
        // Better to invalidate everything and recreate lazily
        for (const auto& [description, view] : m_image_views) {
            vkDestroyImageView(*m_device, view, nullptr);
        }
        m_image_views.clear();
        
        // Images that are managed externally should not be recreated / destroyed
        if (!m_managed) {
            if (m_image) {
                vkDestroyImage(*m_device, m_image, nullptr);
            }
        }
        
        m_dirty = true;
    }

    VkImageView Image::create_view() {
        return create_view({});
    }
    
    VkImageView Image::create_view(const ImageViewDescription& in) {
        ImageViewDescription description = in;
        
        // Configure image view properties to reasonable defaults (if not set)
        if (description.type == VK_IMAGE_VIEW_TYPE_MAX_ENUM) {
            description.type = get_default_view_type();
        }
        if (description.aspect == VK_IMAGE_ASPECT_NONE) {
            description.aspect = get_default_aspect();
        }
        
        auto iter = m_image_views.find(description);
        if (iter != m_image_views.end()) {
            return iter->second;
        }
        
        // Create new image view
        VkImageViewCreateInfo image_view_create_info {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = m_image,
            .viewType = description.type,
            .format = m_description.format,
            .components = description.swizzle,
            .subresourceRange = {
                .aspectMask = description.aspect,
                .baseMipLevel = description.base_mip_level,
                .levelCount = description.mip_count,
                .baseArrayLayer = description.base_layer,
                .layerCount = description.layer_count
            }
        };
        
        VkImageView image_view;
        CHECK_CALL(vkCreateImageView, *m_device, &image_view_create_info, nullptr, &image_view);
        
        return image_view;
    }
    
    void Image::configure(VkImage image, const ImageDescription& description) {
        m_image = image;
        m_description = description;
        m_type = get_image_type();
        
        m_dirty = true;
    }
    
    std::size_t Image::ImageViewHash::operator()(const ImageViewDescription& description) const {
        std::size_t hash = 0;
        
        utils::hash_combine(hash, description.base_mip_level);
        utils::hash_combine(hash, description.mip_count);
        
        utils::hash_combine(hash, description.base_layer);
        utils::hash_combine(hash, description.layer_count);
        
        utils::hash_combine(hash, description.swizzle.r);
        utils::hash_combine(hash, description.swizzle.g);
        utils::hash_combine(hash, description.swizzle.b);
        utils::hash_combine(hash, description.swizzle.a);
        
        utils::hash_combine(hash, description.type);
        
        return hash;
    }
    
    VkImageViewType Image::get_default_view_type() const {
        if (m_type == VK_IMAGE_TYPE_1D) {
            return (m_description.layers > 1) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        }
        else if (m_type == VK_IMAGE_TYPE_2D) {
            bool is_cube_compatible = (m_description.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) != 0;
            
            if (is_cube_compatible && m_description.layers == 6) {
                return VK_IMAGE_VIEW_TYPE_CUBE;
            }
            if (is_cube_compatible && m_description.layers % 6 == 0 && m_description.layers > 6) {
                return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            }
            if (m_description.layers > 1) {
                return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            }
            
            return VK_IMAGE_VIEW_TYPE_2D;
        }
        else { // if (m_type == VK_IMAGE_TYPE_3D)
            return VK_IMAGE_VIEW_TYPE_3D;
        }
    }
    
    VkImageAspectFlags Image::get_default_aspect() const {
        switch (m_description.format) {
            // Depth-only formats
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D32_SFLOAT:
                return VK_IMAGE_ASPECT_DEPTH_BIT;
                
            // Stencil-only formats
            case VK_FORMAT_S8_UINT:
                return VK_IMAGE_ASPECT_STENCIL_BIT;
                
            // Depth-stencil formats
            case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                
            // All other formats are color
            default:
                return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }
    
    VkImageType Image::get_image_type() const {
        // Auto-detect image type
        // Image type depends on resource dimensions
        if (m_description.depth > 1) {
            // 3D image
            return VK_IMAGE_TYPE_3D;
        }
        else if (m_description.height > 1) {
            // 2D image
            return VK_IMAGE_TYPE_2D;
        }
        else {
            // 1D image
            return VK_IMAGE_TYPE_1D;
        }
    }
    
}