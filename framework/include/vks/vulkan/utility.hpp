
#ifndef UTILITY_HPP
#define UTILITY_HPP

#include "utils/string.hpp"
#include <vulkan/vulkan.h>

namespace vks {

    // Returns the size of the given format, in bytes
    unsigned get_format_size(VkFormat format);
    
}

// Formatters
namespace utils {
    
    // TODO: more sophisticated handling?
    template <>
    struct Formatter<VkResult> : public Formatter<typename std::underlying_type<VkResult>::type> {
    };
    
    template <>
    struct Formatter<VkFormat> : public Formatter<typename std::underlying_type<VkFormat>::type> {
    };
    
}

#endif // UTILITY_HPP
