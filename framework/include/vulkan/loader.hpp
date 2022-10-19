
#pragma once

#include "core/defines.hpp"
#include <vulkan/vulkan.h>

namespace vks {
    namespace detail {
    
        // https://cdecl.org/
        // vkGetInstanceProcAddr(VkInstance, const char*) returns a pointer to a function returning void.
        typedef void (VKAPI_PTR *(VKAPI_PTR *fp_vk_get_instance_proc_addr)(VkInstance, const char*))(void);
        NODISCARD fp_vk_get_instance_proc_addr get_vulkan_symbol_loader();
    
    }
}