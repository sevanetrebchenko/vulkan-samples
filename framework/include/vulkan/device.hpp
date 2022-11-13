
#pragma once

#include "core/defines.hpp"

#include <functional>
#include <vulkan/vulkan.h>
#include <cstdint>

namespace vks {
    
    // Forward declarations.
    class VulkanInstance;
    
    class VulkanDevice {
        public:
            class Builder;

            class Properties {
            
            } properties;
            
            class Features {
            
            } features;

            enum class Operation {
                GRAPHICS = 1u << 0u,
                COMPUTE  = 1u << 1u,
                TRANSFER = 1u << 2u
            };
            
            ~VulkanDevice();
            
        private:
            VulkanDevice();
            
            
    };
    
    class VulkanDevice::Builder {
        public:
            typedef std::function<int (const Properties&, const Features&)> SelectorFn;
            
            Builder(const VulkanInstance& instance);
            ~Builder();
            
            NODISCARD VulkanDevice build() const;
            
            // Provides a read-only view into the capabilities of an available physical device on the system.
            Builder& with_physical_device_selector_function(const SelectorFn& selector);
            
            // Enable / disable specific device features.
            // Note: any enabled features must be supported by the device.
            Builder& with_features(const Features& features);
            
            // Request a device with a queue (solely) dedicated to a specific type of operation.
            // Note: presentation is automatically assigned to the graphics queue.
            Builder& with_dedicated_queue(Operation operation);

            // Device extensions.
            Builder& with_enabled_extension(const char* extension);
            Builder& with_disabled_extension(const char* extension);
            
            // Device validation layers.
            Builder& with_enabled_validation_layer(const char* layer);
            Builder& with_disabled_validation_layer(const char* layer);
            
            Builder& with_debug_name(const char* name);
            
        private:
            
            // SECTION: Physical device Vulkan functions.
            
            // vkEnumeratePhysicalDevices
            VkResult (VKAPI_PTR *fp_vk_enumerate_physical_devices_)(VkInstance, std::uint32_t*, VkPhysicalDevice*);
        
            // vkGetPhysicalDeviceProperties
            void (VKAPI_PTR *fp_vk_get_physical_device_properties_)(VkPhysicalDevice, VkPhysicalDeviceProperties*);
        
            // vkGetPhysicalDeviceFeatures
            void (VKAPI_PTR *fp_vk_get_physical_device_features_)(VkPhysicalDevice, VkPhysicalDeviceFeatures*);
        
            // vkGetPhysicalDeviceQueueFamilyProperties
            void (VKAPI_PTR *fp_vk_get_physical_device_queue_family_properties_)(VkPhysicalDevice, std::uint32_t*, VkQueueFamilyProperties*);
        
            // vkEnumerateDeviceExtensionProperties
            VkResult (VKAPI_PTR *fp_vk_enumerate_device_extension_properties_)(VkPhysicalDevice, const char*, std::uint32_t*, VkExtensionProperties*);
        
            // vkEnumerateDeviceLayerProperties
            VkResult (VKAPI_PTR *fp_vk_enumerate_device_layer_properties_)(VkPhysicalDevice, std::uint32_t*, VkLayerProperties*);
        
            
            // SECTION: Logical device Vulkan functions.
            
            // vkCreateDevice
            VkResult (VKAPI_PTR *fp_vk_create_device)(VkPhysicalDevice, const VkDeviceCreateInfo*, const VkAllocationCallbacks*, VkDevice*);
        
            const VulkanInstance& instance_;
        
            SelectorFn selector_;
            Features features_;
            std::vector<Operation> dedicated_queues_;
            std::vector<const char*> extensions_;
            std::vector<const char*> validation_layers_;
            const char* name_;
    };
    
}
