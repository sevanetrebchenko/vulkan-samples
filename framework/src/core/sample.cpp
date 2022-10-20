
#include "core/sample.hpp"
#include <vulkan/vulkan.h>

#include <windows.h>



#include <iostream>

namespace vks {

    Sample::Sample() : instance_(VulkanInstance::Builder().build()) {
        instance_ = VulkanInstance::Builder()
                .with_target_api_version(1, 3)
                .with_enabled_extension("")
                .with_application_name("")
                .build();
    }

    Sample::~Sample() {
    }

} // namespace vks