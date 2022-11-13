
#include "vulkan/device.hpp"
#include "vulkan/instance.hpp"
#include "core/debug.hpp"

namespace vks {
    
    VulkanDevice::Builder::Builder(const VulkanInstance& instance) : fp_vk_enumerate_physical_devices_(),
                                                                     fp_vk_get_physical_device_properties_(),
                                                                     fp_vk_get_physical_device_features_(),
                                                                     fp_vk_get_physical_device_queue_family_properties_(),
                                                                     fp_vk_enumerate_device_extension_properties_(),
                                                                     fp_vk_enumerate_device_layer_properties_(),
                                                                     fp_vk_create_device(),
                                                                     instance_(instance) {
        // TODO:
        ASSERT(fp_vk_enumerate_physical_devices_ != nullptr, "");
        ASSERT(fp_vk_get_physical_device_properties_ != nullptr, "");
        ASSERT(fp_vk_get_physical_device_features_ != nullptr, "");
        ASSERT(fp_vk_get_physical_device_queue_family_properties_ != nullptr, "");
        ASSERT(fp_vk_enumerate_device_extension_properties_ != nullptr, "");
        ASSERT(fp_vk_enumerate_device_layer_properties_ != nullptr, "");
        ASSERT(fp_vk_create_device != nullptr, "");
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_physical_device_selector_function(const std::function<int(const Properties&, const Features&)>& selector) {
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_features(const Features& features) {
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_dedicated_queue(Operation operation) {
        bool found = false;
        
        for (Operation op : dedicated_queues_) {
            if (op == operation) {
                found = true;
                break;
            }
        }
        
        // Only add a dedicated queue if it doesn't already exist.
        if (!found) {
            dedicated_queues_.emplace_back(operation);
        }
        
        return *this;
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_enabled_extension(const char* extension) {
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_disabled_extension(const char* extension) {
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_enabled_validation_layer(const char* layer) {
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_disabled_validation_layer(const char* layer) {
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_debug_name(const char* name) {
    }
    
}
