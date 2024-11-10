
#include "vks/vulkan/utility.hpp"
#include <stdexcept> // std::runtime_error

namespace vks {
    
    unsigned get_format_size(VkFormat format) {
        // A non-exhaustive list of common formats
        switch (format) {
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_SNORM:
            case VK_FORMAT_R8_UINT:
            case VK_FORMAT_R8_SINT:
                return 1;
    
            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_SNORM:
            case VK_FORMAT_R8G8_UINT:
            case VK_FORMAT_R8G8_SINT:
                return 2;
    
            case VK_FORMAT_R8G8B8_UNORM:
            case VK_FORMAT_R8G8B8_SNORM:
            case VK_FORMAT_R8G8B8_UINT:
            case VK_FORMAT_R8G8B8_SINT:
                return 3;
    
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SNORM:
            case VK_FORMAT_R8G8B8A8_UINT:
            case VK_FORMAT_R8G8B8A8_SINT:
            case VK_FORMAT_B8G8R8A8_UNORM:
                return 4;
    
            case VK_FORMAT_R16_UNORM:
            case VK_FORMAT_R16_SNORM:
            case VK_FORMAT_R16_UINT:
            case VK_FORMAT_R16_SINT:
            case VK_FORMAT_R16_SFLOAT:
                return 2;
    
            case VK_FORMAT_R16G16_UNORM:
            case VK_FORMAT_R16G16_SNORM:
            case VK_FORMAT_R16G16_UINT:
            case VK_FORMAT_R16G16_SINT:
            case VK_FORMAT_R16G16_SFLOAT:
                return 4;
    
            case VK_FORMAT_R16G16B16_UNORM:
            case VK_FORMAT_R16G16B16_SNORM:
            case VK_FORMAT_R16G16B16_UINT:
            case VK_FORMAT_R16G16B16_SINT:
            case VK_FORMAT_R16G16B16_SFLOAT:
                return 6;
    
            case VK_FORMAT_R16G16B16A16_UNORM:
            case VK_FORMAT_R16G16B16A16_SNORM:
            case VK_FORMAT_R16G16B16A16_UINT:
            case VK_FORMAT_R16G16B16A16_SINT:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return 8;
    
            case VK_FORMAT_R32_UINT:
            case VK_FORMAT_R32_SINT:
            case VK_FORMAT_R32_SFLOAT:
                return 4;
    
            case VK_FORMAT_R32G32_UINT:
            case VK_FORMAT_R32G32_SINT:
            case VK_FORMAT_R32G32_SFLOAT:
                return 8;
    
            case VK_FORMAT_R32G32B32_UINT:
            case VK_FORMAT_R32G32B32_SINT:
            case VK_FORMAT_R32G32B32_SFLOAT:
                return 12;
    
            case VK_FORMAT_R32G32B32A32_UINT:
            case VK_FORMAT_R32G32B32A32_SINT:
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return 16;
    
            case VK_FORMAT_R64_UINT:
            case VK_FORMAT_R64_SINT:
            case VK_FORMAT_R64_SFLOAT:
                return 8;
    
            case VK_FORMAT_R64G64_UINT:
            case VK_FORMAT_R64G64_SINT:
            case VK_FORMAT_R64G64_SFLOAT:
                return 16;
    
            case VK_FORMAT_R64G64B64_UINT:
            case VK_FORMAT_R64G64B64_SINT:
            case VK_FORMAT_R64G64B64_SFLOAT:
                return 24;
    
            case VK_FORMAT_R64G64B64A64_UINT:
            case VK_FORMAT_R64G64B64A64_SINT:
            case VK_FORMAT_R64G64B64A64_SFLOAT:
                return 32;
    
            case VK_FORMAT_D16_UNORM:
                return 2;
    
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_X8_D24_UNORM_PACK32:
                return 4;
    
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return 4;
    
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return 8;
    
            default:
                throw std::runtime_error(utils::format("Unknown VkFormat format provided to get_format_size ({})", format));
                return 0;
        }
    }
    
}