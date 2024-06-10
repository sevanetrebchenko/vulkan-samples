
#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <vulkan/vulkan.h>

void create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, unsigned mip_levels, VkImageView& image_view);
void create_image(VkPhysicalDevice physical_device, VkDevice device, unsigned image_width, unsigned image_height, unsigned mip_levels, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags desired_memory_properties, VkImage& image, VkDeviceMemory& memory);

#endif // HELPERS_HPP
