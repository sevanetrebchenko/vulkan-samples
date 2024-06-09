
#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <vulkan/vulkan.h>

void create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, unsigned mip_levels, VkImageView& image_view);

#endif // HELPERS_HPP
