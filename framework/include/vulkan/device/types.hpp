
#pragma once

#include "vulkan/device.hpp"
#include <vulkan/vulkan.h>
#include <cstdint>
#include <type_traits>

namespace vks {

    class VulkanInstance;
    
    class VulkanPhysicalDevice::Properties {
        public:
            Properties(VulkanInstance* instance, VulkanPhysicalDevice* device);
            ~Properties();
            
            template <typename T>
            const T& get() const;
        
            #if defined(VK_VERSION_1_1) && defined(VK_KHR_acceleration_structure)
                // https://registry.khronos.org/vulkan/specs/1.3-khr-extensions/html/vkspec.html#VkPhysicalDeviceAccelerationStructurePropertiesKHR
                struct AccelerationStructureProperties {
                    operator VkPhysicalDeviceAccelerationStructurePropertiesKHR() const;
        
                    std::uint64_t max_geometry_count;
                    std::uint64_t max_instance_count;
                    std::uint64_t max_primitive_count;
        
                    std::uint32_t max_per_stage_descriptor_acceleration_structures;
                    std::uint32_t max_per_stage_descriptor_update_after_bind_acceleration_structures;
        
                    std::uint32_t max_descriptor_set_acceleration_structures;
                    std::uint32_t max_descriptor_set_update_after_bind_acceleration_structures;
        
                    std::uint32_t min_acceleration_structure_scratch_offset_alignment;
                } acceleration_structure_properties;
            #endif
            
        private:
            void (VKAPI_PTR *fp_vk_get_physical_device_properties)(VkPhysicalDevice, VkPhysicalDeviceProperties2*);
    };
    
}

#include "vulkan/device/types.tpp"