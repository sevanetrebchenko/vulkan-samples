
#include "helpers.hpp"
#include <stdexcept> // std::runtime_error

void create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, unsigned mip_levels, VkImageView& image_view) {
    VkImageViewCreateInfo image_view_ci { };
    image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_ci.image = image;
    image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_ci.format = format;
    
    image_view_ci.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
    };
    
    image_view_ci.subresourceRange.aspectMask = aspect;
    
    image_view_ci.subresourceRange.baseMipLevel = 0;
    image_view_ci.subresourceRange.levelCount = mip_levels;
    
    // Only one image layer
    image_view_ci.subresourceRange.baseArrayLayer = 0;
    image_view_ci.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(device, &image_view_ci, nullptr, &image_view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }
}

void create_image(VkPhysicalDevice physical_device, VkDevice device, unsigned image_width, unsigned image_height, unsigned mip_levels, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags desired_memory_properties, VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo image_ci { };
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.extent.width = image_width;
    image_ci.extent.height = image_height;
    image_ci.extent.depth = 1; // 2D image must have a depth of 1
    image_ci.mipLevels = mip_levels;
    image_ci.arrayLayers = 1; // No image array
    
    image_ci.format = format; // TODO: format must be supported by the graphics hardware (VkSurfaceFormatKHR) (check?)
    
    // Tiling mode specifies the tiling arrangement of texels in an image
    image_ci.tiling = tiling; // VK_IMAGE_TILING_LINEAR: texels are laid out in row-major order to support direct access of texels in the image memory
                                             // VK_IMAGE_TILING_OPTIMAL: texels are laid out in an implementation-dependent order for optimal access from shaders
    
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#resources-image-layouts
    // The initial layout of an image specifies the layout the image is in before the start of the renderpass it is used in. Applications can transform image resources from one layout to another based on the specific usage of the image memory
    //   - VK_IMAGE_LAYOUT_PREINITIALIZED: image data ca be pre-initialized by the host, and transitioning to another layout will preserve that data
    //   - VK_IMAGE_LAYOUT_UNDEFINED: image data is undefined (don't care) and preservation of the data when transitioning to another layout is not guaranteed
    // Note: All image resources are required to be transitioned to another layout (than the ones above) before accessed by the GPU
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_ci.usage = usage;
    
    image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.samples = samples;
    image_ci.flags = 0; // TODO: look into sparse 3D textures for something like voxel terrain (allows avoiding allocations for large chunks of 'air' voxels)
    
    if (vkCreateImage(device, &image_ci, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image.");
    }
    
    // Query image memory requirements
    VkMemoryRequirements image_memory_requirements { };
    vkGetImageMemoryRequirements(device, image, &image_memory_requirements);
    
    VkMemoryAllocateInfo memory_allocate_info { };
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = image_memory_requirements.size;
    
    // TODO: physical device memory properties can be cached to prevent repeat calls to vkGetPhysicalDeviceMemoryProperties
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);
    
    unsigned selected_memory_type = physical_device_memory_properties.memoryTypeCount;
    
    for (unsigned i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i) {
        if (image_memory_requirements.memoryTypeBits & (1 << i)) {
            // Memory type was requested, check to see if it is supported by the selected physical device
            if ((physical_device_memory_properties.memoryTypes[i].propertyFlags & desired_memory_properties) == desired_memory_properties) {
                // Memory supports all desired features
                selected_memory_type = i;
                break;
            }
        }
    }
    
    if (selected_memory_type == physical_device_memory_properties.memoryTypeCount) {
        throw std::runtime_error("failed to find suitable memory type for image!");
    }
    
    memory_allocate_info.memoryTypeIndex = selected_memory_type;
    
    if (vkAllocateMemory(device, &memory_allocate_info, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate memory for image!");
    }
}