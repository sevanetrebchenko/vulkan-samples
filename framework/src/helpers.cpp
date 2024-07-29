
#include "helpers.hpp"
#include <stdexcept> // std::runtime_error

unsigned get_memory_type_index(VkPhysicalDevice physical_device, VkMemoryRequirements memory_requirements, VkMemoryPropertyFlags desired_memory_properties) {
    // Returns the index of the memory
    // TODO: physical device memory properties can be cached to prevent repeat calls to vkGetPhysicalDeviceMemoryProperties
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);
    
    unsigned selected_memory_type = physical_device_memory_properties.memoryTypeCount;
    
    for (unsigned i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i) {
        if (memory_requirements.memoryTypeBits & (1 << i)) {
            // Memory type was requested, check to see if it is supported by the selected physical device
            if ((physical_device_memory_properties.memoryTypes[i].propertyFlags & desired_memory_properties) == desired_memory_properties) {
                // Memory supports all desired features
                selected_memory_type = i;
                break;
            }
        }
    }
    
    if (selected_memory_type == physical_device_memory_properties.memoryTypeCount) {
        throw std::runtime_error("failed to find suitable memory type for resource!");
    }
    
    return selected_memory_type;
}

void create_image_view(VkDevice device, VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspect, unsigned base_mip_level, unsigned num_mip_levels, unsigned layer_count, VkImageView& image_view) {
    VkImageViewCreateInfo image_view_ci { };
    image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_ci.image = image;
    image_view_ci.viewType = type; // TODO: can type be determined from the number of layers?
    image_view_ci.format = format;
    
    image_view_ci.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
    };
    
    image_view_ci.subresourceRange.aspectMask = aspect;
    
    image_view_ci.subresourceRange.baseMipLevel = base_mip_level;
    image_view_ci.subresourceRange.levelCount = num_mip_levels;
    
    // Only one image layer
    image_view_ci.subresourceRange.baseArrayLayer = 0;
    image_view_ci.subresourceRange.layerCount = layer_count;
    
    if (vkCreateImageView(device, &image_view_ci, nullptr, &image_view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }
}

void create_image(VkPhysicalDevice physical_device, VkDevice device, unsigned image_width, unsigned image_height, unsigned mip_levels, unsigned layers, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkMemoryPropertyFlags desired_memory_properties, VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo image_ci { };
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.extent.width = image_width;
    image_ci.extent.height = image_height;
    image_ci.extent.depth = 1; // 2D image must have a depth of 1
    image_ci.mipLevels = mip_levels;
    image_ci.arrayLayers = layers; // No image array
    
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
    image_ci.flags = flags; // TODO: look into sparse 3D textures for something like voxel terrain (allows avoiding allocations for large chunks of 'air' voxels)
    
    if (vkCreateImage(device, &image_ci, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }
    
    // Query image memory requirements
    VkMemoryRequirements image_memory_requirements { };
    vkGetImageMemoryRequirements(device, image, &image_memory_requirements);
    
    VkMemoryAllocateInfo memory_allocate_info { };
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = image_memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = get_memory_type_index(physical_device, image_memory_requirements, desired_memory_properties);
    
    if (vkAllocateMemory(device, &memory_allocate_info, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate memory for image!");
    }
    vkBindImageMemory(device, image, memory, 0); // Associate memory buffer with image
}

void create_buffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize allocation_size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags desired_properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory) {
    VkBufferCreateInfo buffer_create_info { };
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = allocation_size;
    buffer_create_info.usage = buffer_usage; // Indicates the purpose(s) for which the data in the buffer will be used
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &buffer_create_info, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer object!");
    }

    // Query buffer memory requirements
    VkMemoryRequirements memory_requirements { };
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

    // Graphics cards have different types of memory (such as VRAM and RAM swap space) that varies in terms of permitted operations and performance
    // Buffer usage requirements must be considered when finding the right type of memory to use
    VkMemoryAllocateInfo memory_allocate_info { };
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = get_memory_type_index(physical_device, memory_requirements, desired_properties);

    if (vkAllocateMemory(device, &memory_allocate_info, nullptr, &buffer_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate memory for buffer!");
    }
    
    // Associate the memory with the buffer
    vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

VkDescriptorSetLayoutBinding create_descriptor_set_layout_binding(VkDescriptorType type, VkShaderStageFlags stages, unsigned binding, unsigned descriptor_count) {
    VkDescriptorSetLayoutBinding set_binding { };
    
    // Should match with the 'binding' indicator on the buffer
    // layout (binding = 0) uniform ... { ... };
    set_binding.binding = binding;
    set_binding.descriptorType = type;
    
    // Allows for an array of uniform buffer objects
    set_binding.descriptorCount = descriptor_count;
    
    // Describe what stages this buffer is accessed in (can be a bitwise-or of VkShaderStageFlagBits, or VK_SHADER_STAGE_ALL_GRAPHICS)
    set_binding.stageFlags = stages;
    
    set_binding.pImmutableSamplers = nullptr;
    return set_binding;
}

std::size_t align_to_device_boundary(VkPhysicalDevice physical_device, std::size_t size) {
    VkPhysicalDeviceProperties properties { };
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    
	size_t min_alignment = properties.limits.minUniformBufferOffsetAlignment;
	size_t aligned = size;
	if (min_alignment > 0) {
		aligned = (aligned + min_alignment - 1) & ~(min_alignment - 1);
	}
	return aligned;
    
}

void copy_buffer(VkCommandBuffer command_buffer, VkBuffer src, VkDeviceSize src_offset, VkBuffer dst,  VkDeviceSize dst_offset, VkDeviceSize size) {
    // Copy 'size' bytes from the source to the destination buffer.
    VkBufferCopy copy_region { };
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);
}

void copy_buffer_to_image(VkCommandBuffer command_buffer, VkBuffer src, VkDeviceSize src_offset, VkImage dst, int mip_level, int width, int height) {
    VkBufferImageCopy copy_region { };
    copy_region.bufferOffset = src_offset;
    copy_region.bufferRowLength = 0;
    copy_region.bufferImageHeight = 0;
    
    copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.mipLevel = mip_level;
    copy_region.imageSubresource.baseArrayLayer = 0;
    copy_region.imageSubresource.layerCount = 1;
    
    copy_region.imageOffset = { 0, 0, 0 };
    copy_region.imageExtent = { (unsigned) width, (unsigned) height, 1 };
    
    vkCmdCopyBufferToImage(command_buffer, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
}

void copy_image_to_image(VkCommandBuffer command_buffer, VkImage src, VkImage dst, unsigned mip_level, unsigned layer, unsigned layer_count, unsigned width, unsigned height, unsigned depth) {
    VkImageSubresourceLayers src_subresource { };
    src_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    src_subresource.mipLevel = mip_level;
    src_subresource.baseArrayLayer = layer;
    src_subresource.layerCount = layer_count;
    
    VkImageSubresourceLayers dst_subresource = src_subresource;
    
    VkImageCopy copy_region { };
    copy_region.srcSubresource = src_subresource;
    copy_region.dstSubresource = dst_subresource;
    copy_region.extent = { width, height, depth };
    copy_region.srcOffset = { 0, 0, 0 };
    copy_region.dstOffset = { 0, 0, 0 };
    
    vkCmdCopyImage(command_buffer, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
}

void transition_image(VkCommandBuffer command_buffer, VkImage image, VkImageLayout src, VkImageLayout dst, VkImageSubresourceRange subresource_range, VkAccessFlags src_access_mask, VkPipelineStageFlags src_stage_mask, VkAccessFlags dst_access_mask, VkPipelineStageFlags dst_stage_mask) {
    // One of the most common ways to transition layouts for images is using an image memory barrier, which is a type of pipeline barrier
    // Pipeline barriers are used to synchronize access to resources - in this case it is used to transition the layout of the image before any subsequent reads happen from it
    VkImageMemoryBarrier image_memory_barrier { };
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.oldLayout = src;
    image_memory_barrier.newLayout = dst;
    
    // Used for transferring ownership between two different queue families when VK_SHARING_MODE_EXCLUSIVE is enabled on the image (image can only be used by one queue family at a time)
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    
    image_memory_barrier.image = image;
    image_memory_barrier.subresourceRange = subresource_range;
    image_memory_barrier.srcAccessMask = src_access_mask; // Define which operations that involve the image must happen before the barrier
    image_memory_barrier.dstAccessMask = dst_access_mask; // Define which operations that involve the image must wait on the barrier
    
    vkCmdPipelineBarrier(command_buffer,
                         src_stage_mask, // Define the pipeline stage(s) that Vulkan should wait for completion on before proceeding with execution after the barrier
                         dst_stage_mask, // Define the pipeline stage(s) in which operations should occur after the memory barrier
                         0,
                         0, nullptr, // Memory barriers
                         0, nullptr, // Pipeline barriers
                         1, &image_memory_barrier); // Image barriers
}
