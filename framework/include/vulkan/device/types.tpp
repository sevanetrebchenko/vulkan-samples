
#pragma once

namespace vks {
    
    template <typename T>
    const T& VulkanPhysicalDevice::Properties::get() const {
        // TODO: Assert types.
        
        if (std::is_same_v<T, VulkanPhysicalDevice::Properties::AccelerationStructureProperties> || std::is_same_v<T, VkPhysicalDeviceAccelerationStructurePropertiesKHR>) {
            return acceleration_structure_properties;
        }
    }
    
}