
#include "helpers.hpp"
#include <stdexcept> // std::runtime_error

void create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, unsigned mip_levels, VkImageView& image_view) {
    VkImageViewCreateInfo image_view_create_info { };
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format;
    
    image_view_create_info.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
    };
    
    image_view_create_info.subresourceRange.aspectMask = aspect;
    
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = mip_levels;
    
    // Only one image layer
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(device, &image_view_create_info, nullptr, &image_view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }
}