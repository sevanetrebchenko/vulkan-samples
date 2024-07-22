
#ifndef VULKAN_HPP
#define VULKAN_HPP

#include <vulkan/vulkan.h>
#include <cstdint>

namespace vks {
    
    enum class MemoryType : std::uint8_t {
        GPU = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        CPU = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };
    
    // UNORM - unsigned normalized floating point, in the range [0.0, 1.0f]
    // SNORM - signed normalized floating point, in the range [-1.0f, 1.0f]
    // USCALED - unsigned floating point
    // SSCALED - signed floating point
    // UINT - unsigned integer
    // SINT - signed integer
    enum class Format : std::uint8_t {
        RGBA8_UNORM,
    };

    enum class BufferUsage : std::uint8_t {
        TransferSrc,
        TransferDst,
        
        Uniform,
        Storage,
        Index,
        Vertex
    };
    
    
    enum class ImageAspect : std::uint8_t {
        Color,
        Depth,
        Stencil
    };
    
    enum class ImageUsage : std::uint8_t {
        TransferSrc,
        TransferDst,
        
        Sampled,
        Storage,
        
        ColorAttachment,
        DepthStencilAttachment
    };
    
    enum class ImageLayout : std::uint8_t {
        Undefined = VK_IMAGE_LAYOUT_UNDEFINED,
        
        General = VK_IMAGE_LAYOUT_GENERAL,
        ColorAttachment = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        DepthStencilAttachment = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        
        ShaderRead = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        DepthStencilRead = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        
        TransferSrc = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        TransferDst = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        ReadOnly,
        Present
    };
    
}

#endif // VULKAN_HPP
