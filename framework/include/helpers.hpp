
#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector> // std::vector

void create_image_view(VkDevice device, VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspect, unsigned mip_levels, unsigned layer_count, VkImageView& image_view);
void create_image(VkPhysicalDevice physical_device, VkDevice device, unsigned image_width, unsigned image_height, unsigned mip_levels, unsigned layers, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkMemoryPropertyFlags desired_memory_properties, VkImage& image, VkDeviceMemory& memory);

void create_buffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize allocation_size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags desired_properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);

void copy_buffer(VkCommandBuffer command_buffer, VkBuffer src, VkDeviceSize src_offset, VkBuffer dst,  VkDeviceSize dst_offset, VkDeviceSize size);

VkDescriptorSetLayoutBinding create_descriptor_set_layout_binding(VkDescriptorType type, VkShaderStageFlags stages, unsigned binding);

std::size_t align_to_device_boundary(VkPhysicalDevice physical_device, std::size_t size);

// TODO: determine access masks from src/dsk pipeline stage
void transition_image(VkCommandBuffer command_buffer, VkImage image, VkImageLayout src, VkImageLayout dst, VkImageSubresourceRange subresource_range, VkAccessFlags src_access_mask, VkPipelineStageFlags src_stage_mask, VkAccessFlags dst_access_mask, VkPipelineStageFlags dst_stage_mask);

#endif // HELPERS_HPP
