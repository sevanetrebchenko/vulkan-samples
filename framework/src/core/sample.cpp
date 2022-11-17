
#include "core/sample.hpp"
#include <vulkan/vulkan.h>

#include <windows.h>



#include <iostream>

namespace vks {

    Sample::Sample() : instance_(VulkanInstance::Builder()
                                         .with_target_api_version(1, 0)
                                         .with_enabled_extension("")
                                         .with_application_name("")
                                         .build()) {
    
//        auto h = detail::vk_get_instance_proc_addr(instance_, "vkGetPhysicalDeviceProperties2KHR");
//        if (!h) {
//            throw std::runtime_error("");
//        }
        
        instance_.select_physical_device().build();
    }

    Sample::~Sample() {
    }

} // namespace vks