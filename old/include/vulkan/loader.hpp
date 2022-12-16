
#pragma once

#include "core/defines.hpp"
#include <vulkan/vulkan.h>

namespace vks {
    namespace detail {
    
        typedef void (*void_fp)(void);
    
        // Functions need to be cast to the desired type.
        NODISCARD void_fp vk_get_instance_proc_addr(VkInstance instance, const char* name);
    
    }
}