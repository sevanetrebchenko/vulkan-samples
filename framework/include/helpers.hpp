
#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector> // std::vector

void create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, unsigned mip_levels, VkImageView& image_view);
void create_image(VkPhysicalDevice physical_device, VkDevice device, unsigned image_width, unsigned image_height, unsigned mip_levels, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags desired_memory_properties, VkImage& image, VkDeviceMemory& memory);

void create_buffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize allocation_size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags desired_properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);

struct Model {
    struct Vertex {
        Vertex(glm::vec3 position, glm::vec3 normal, glm::vec2 uv);
        Vertex(glm::vec3 position, glm::vec2 uv);
        Vertex(glm::vec3 position);
        
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };
    
    std::vector<Vertex> vertices;
    std::vector<unsigned> indices;
};

Model load_model(const char* filepath);

#endif // HELPERS_HPP
