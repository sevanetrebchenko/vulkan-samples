
#ifndef UTILITY_HPP
#define UTILITY_HPP

#include "utils/string.hpp"
#include <vulkan/vulkan.h>
#include <cstddef>  // std::size_t

namespace vks {
    
    // Returns the size of the given format, in bytes
    unsigned get_format_size(VkFormat format);

    template <typename T>
    void hash_combine(std::size_t& seed, const T& value);
    
    void hash_combine(std::size_t& seed, const char* value);
    void hash_combine(std::size_t& seed, const char* value, std::size_t length);
    
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

#include "utility.tpp"

#endif // UTILITY_HPP
