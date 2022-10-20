
#pragma once

#include "core/defines.hpp"
#include "vulkan/types.hpp"
#include <vulkan/vulkan.h>
#include <functional>
#include <cstdint>

namespace vks {
    
    class VulkanInstance;
    
    // A VulkanDevice encapsulates both the physical hardware and a logical interface with the Vulkan API.
    class VulkanDevice {
        public:
            class Builder;
            
            enum class Type : char {
                OTHER = 0,
                INTEGRATED_GPU = 1,
                DISCRETE_GPU = 2,
                VIRTUAL_GPU = 3,
                CPU = 4
            };
            
            ~VulkanDevice();
        
            VulkanDevice(VulkanDevice&& other) noexcept;
            VulkanDevice& operator=(VulkanDevice&& other) noexcept;
        
            // Devices should not be copied.
            VulkanDevice(const VulkanDevice& other) = delete;
            VulkanDevice& operator=(const VulkanDevice& other) = delete;
        
        private:
            struct DispatchTable {
            
            } dispatch_;
            
            VulkanDevice(VulkanInstance* instance);
    };
    
    class VulkanDevice::Builder {
        public:
            Builder(VulkanInstance* instance);
            ~Builder();

            VulkanDevice build();
            
            Builder& with_debug_name(const char* debug_name);

        private:
            struct DispatchTable {
                VkResult (VKAPI_PTR *fp_vk_enumerate_physical_devices)(VkInstance, std::uint32_t*, VkPhysicalDevice*);
                void (VKAPI_PTR *fp_vk_get_physical_device_properties)(VkPhysicalDevice, VkPhysicalDeviceProperties*);
                void (VKAPI_PTR *fp_vk_get_physical_device_features)(VkPhysicalDevice, VkPhysicalDeviceFeatures*);
                void (VKAPI_PTR *fp_vk_get_physical_device_queue_family_properties)(VkPhysicalDevice, std::uint32_t*, VkQueueFamilyProperties*);
                VkResult (VKAPI_PTR *fp_vk_enumerate_device_extension_properties)(VkPhysicalDevice, const char*, std::uint32_t*, VkExtensionProperties*);
            } dispatch_;
            
            VulkanInstance* instance_;
    };
    
}
