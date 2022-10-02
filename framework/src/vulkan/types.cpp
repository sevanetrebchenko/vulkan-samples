
#include "vulkan/types.hpp"
#include <vulkan/vulkan.h>

namespace vks {
    
    Version::operator std::uint32_t() const {
        return VK_MAKE_API_VERSION(variant, major, minor, patch);
    }
    
}