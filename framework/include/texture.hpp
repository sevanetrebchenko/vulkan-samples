
#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <vulkan/vulkan.h>

struct Texture {
    Texture();
    ~Texture();
    
    // Mipmaps are not automatically generated
    void generate_mipmaps();
    
    VkImage resource;
    VkDeviceMemory memory;
    VkImageView image_view;
    VkFormat format;
    
    VkSampler sampler;

    VkImageUsageFlags usage;
    
    unsigned width;
    unsigned height;
    unsigned mipmap_levels;
    unsigned layers;
};

struct TextureCreateInfo {
};

Texture load_texture(const char* filepath);

#endif // TEXTURE_HPP
