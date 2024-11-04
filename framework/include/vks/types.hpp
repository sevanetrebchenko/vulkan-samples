
#ifndef TYPES_HPP
#define TYPES_HPP

#include "utils/string.hpp"

#include <vulkan/vulkan.h>
#include <cstdint>

namespace vks {
    
    typedef std::uint8_t u8;
    typedef std::uint16_t u16;
    typedef std::uint32_t u32;
    typedef std::uint64_t u64;
    
    typedef std::int8_t i8;
    typedef std::int16_t i16;
    typedef std::int32_t i32;
    typedef std::int64_t i64;
    
    typedef float f32;
    typedef double f64;
    
}

// Formatters
namespace utils {
    
    // TODO: more sophisticated handling?
    template <>
    struct Formatter<VkResult> : public Formatter<typename std::underlying_type<VkResult>::type> {
    };
    
}

#endif // TYPES_HPP
