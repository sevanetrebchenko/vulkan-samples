
#include "vulkan/device.hpp"
#include "vulkan/instance.hpp"

namespace vks {
    
    VulkanDevice::Builder::Builder(VulkanInstance* instance) : instance_(instance) {
        // Load instance-level Vulkan functions.
        detail::fp_vk_get_instance_proc_addr vk_get_instance_proc_addr = detail::get_vulkan_symbol_loader();
        dispatch_ = DispatchTable {
            .fp_vk_enumerate_physical_devices = reinterpret_cast<typeof(DispatchTable::fp_vk_enumerate_physical_devices)>(vk_get_instance_proc_addr((VkInstance) instance, "vkEnumeratePhysicalDevices")),
            .fp_vk_get_physical_device_properties = reinterpret_cast<typeof(DispatchTable::fp_vk_get_physical_device_properties)>(vk_get_instance_proc_addr((VkInstance) instance, "vkGetPhysicalDeviceProperties")),
            .fp_vk_get_physical_device_features = reinterpret_cast<typeof(DispatchTable::fp_vk_get_physical_device_features)>(vk_get_instance_proc_addr((VkInstance) instance, "vkGetPhysicalDeviceFeatures")),
            .fp_vk_get_physical_device_queue_family_properties = reinterpret_cast<typeof(DispatchTable::fp_vk_get_physical_device_queue_family_properties)>(vk_get_instance_proc_addr((VkInstance) instance, "vkGetPhysicalDeviceQueueFamilyProperties")),
            .fp_vk_enumerate_device_extension_properties = reinterpret_cast<typeof(DispatchTable::fp_vk_enumerate_device_extension_properties)>(vk_get_instance_proc_addr((VkInstance) instance, "vkEnumerateDeviceExtensionProperties"))
        };
        
    }
    
    VulkanDevice::Builder::~Builder() {
    }
    
    VulkanDevice VulkanDevice::Builder::build() {
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_debug_name(const char* debug_name) {
        return *this;
    }
}