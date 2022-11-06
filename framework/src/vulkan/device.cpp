
#include "vulkan/device.hpp"
#include "vulkan/instance.hpp"
#include "core/debug.hpp"

namespace vks {

    VulkanPhysicalDevice::Selector::Selector(VulkanInstance* instance) : instance_(instance) {
        // Load Vulkan functions.
        #if defined(VK_VERSION_1_0)
            fp_vk_enumerate_physical_devices_ = reinterpret_cast<typeof(fp_vk_enumerate_physical_devices_)>(detail::vk_get_instance_proc_addr(*instance, "vkEnumeratePhysicalDevices"));
            fp_vk_get_physical_device_properties_ = reinterpret_cast<typeof(fp_vk_get_physical_device_properties_)>(detail::vk_get_instance_proc_addr(*instance, "vkGetPhysicalDeviceProperties"));
            fp_vk_get_physical_device_features_ = reinterpret_cast<typeof(fp_vk_get_physical_device_features_)>(detail::vk_get_instance_proc_addr(*instance, "vkGetPhysicalDeviceFeatures"));
            fp_vk_get_physical_device_queue_family_properties_ = reinterpret_cast<typeof(fp_vk_get_physical_device_queue_family_properties_)>(detail::vk_get_instance_proc_addr(*instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
            fp_vk_enumerate_device_extension_properties_ = reinterpret_cast<typeof(fp_vk_enumerate_device_extension_properties_)>(detail::vk_get_instance_proc_addr(*instance, "vkEnumerateDeviceExtensionProperties"));
            fp_vk_enumerate_device_layer_properties_ = reinterpret_cast<typeof(fp_vk_enumerate_device_layer_properties_)>(detail::vk_get_instance_proc_addr(*instance, "vkEnumerateDeviceLayerProperties"));
    
            #if defined(VK_VERSION_1_1)
                fp_vk_get_physical_device_properties_2_ = reinterpret_cast<typeof(fp_vk_get_physical_device_properties_2_)>(detail::vk_get_instance_proc_addr(*instance, "vkGetPhysicalDeviceProperties2"));
            #endif
            
            #if defined(VK_KHR_get_display_properties2)
                fp_vk_get_physical_device_properties_2_khr_ = reinterpret_cast<typeof(fp_vk_get_physical_device_properties_2_khr_)>(detail::vk_get_instance_proc_addr(*instance, "vkGetPhysicalDeviceProperties2KHR"));
            #endif
        #endif
    }
    
    VulkanPhysicalDevice::Selector::~Selector() {
    }
    
    VulkanPhysicalDevice::Selector& VulkanPhysicalDevice::Selector::with_selector_function(const std::function<int(const Properties&, const Features&)>& selector) {
    }
    
    VulkanPhysicalDevice::Selector& VulkanPhysicalDevice::Selector::with_debug_name(const char* name) {
    }
    
    VulkanPhysicalDevice VulkanPhysicalDevice::Selector::select() {
        #if defined(VK_VERSION_1_0)
            // Enumerate physical devices.
            std::uint32_t physical_device_count = 0u;
            VkResult result = fp_vk_enumerate_physical_devices_(*instance_, &physical_device_count, nullptr);
    
            if (physical_device_count == 0u) {
                throw VulkanException("ERROR: Failed to find GPU with Vulkan support.");
            }
    
            VulkanPhysicalDevice handle { };
            
            std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
            fp_vk_enumerate_physical_devices_(*instance_, &physical_device_count, physical_devices.data());
    
            #undef VK_VERSION_1_1
            // #undef VK_KHR_get_display_properties2
            
            for (const VkPhysicalDevice& physical_device : physical_devices) {
                
                VkPhysicalDeviceProperties2KHR physical_device_properties { };
                
                
                
                
                fp_vk_get_physical_device_properties_2_khr_(physical_device, &physical_device_properties);
                
    
//                VkPhysicalDeviceFeatures physical_device_features { };
//                fp_vk_get_physical_device_features(physical_device, &physical_device_features);
            }
        #endif
    }

//    VulkanDevice::Builder::~Builder() {
//    }
//
//    VulkanDevice VulkanDevice::Builder::build() {
//        // Enumerate physical devices.
//        std::uint32_t physical_device_count = 0u;
//        fp_vk_enumerate_physical_devices((VkInstance) instance_, &physical_device_count, nullptr);
//
//        if (physical_device_count == 0u) {
//            throw VulkanException("ERROR: Failed to find GPU with Vulkan support.");
//        }
//
//        std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
//        fp_vk_enumerate_physical_devices((VkInstance) instance_, &physical_device_count, physical_devices.data());
//
//        for (const VkPhysicalDevice& physical_device : physical_devices) {
//            // TODO: promoted to core in 1.1 (use vkGetPhysicalDeviceProperties2), needs VK_KHR_get_physical_device_properties2 extension in 1.0 (use vkGetPhysicalDeviceProperties2KHR)
//            VkPhysicalDeviceProperties2 physical_device_properties { };
//            fp_vk_get_physical_device_properties(physical_device, &physical_device_properties);
//
//            VkPhysicalDeviceFeatures physical_device_features { };
//            fp_vk_get_physical_device_features(physical_device, &physical_device_features);
//        }
//    }
//
//    VulkanDevice::Builder& VulkanDevice::Builder::with_debug_name(const char* debug_name) {
//        return *this;
//    }

}