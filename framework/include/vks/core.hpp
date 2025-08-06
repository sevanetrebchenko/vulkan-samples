
#ifndef CORE_HPP
#define CORE_HPP

#define NOMINMAX

#include <utils/logging.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <memory> // std::enable_shared_from_this

#define CHECK_CALL(FUNCTION, ...) { \
    VkResult result = FUNCTION(__VA_ARGS__); \
    if (result != VK_SUCCESS) { \
        utils::logging::fatal("{} failed with code {} ({})", #FUNCTION, static_cast<std::underlying_type_t<VkResult>>(result), string_VkResult(result)); \
    } \
}

namespace vks {
    
    template <typename T>
    struct ManagedObject : public std::enable_shared_from_this<T> {
        virtual ~ManagedObject() = default;
    };
    
}

#endif // CORE_HPP
