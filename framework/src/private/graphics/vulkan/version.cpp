
#include <vulkan/vulkan.h>

#include "private/core/version.hpp"

namespace vks {
    
    Version::operator std::uint32_t() const {
        return VK_MAKE_API_VERSION(0u, major, minor, patch);
    }
    
}