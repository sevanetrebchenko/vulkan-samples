
#pragma once

#ifndef VULKAN_SAMPLES_LOGICAL_DEVICE_HPP
#define VULKAN_SAMPLES_LOGICAL_DEVICE_HPP

#include "vulkan/types.hpp"
#include "core/defines.hpp"
#include "vulkan/physical_device.hpp"

namespace vks {
    
    // Forward declarations.
    class VulkanInstance;
    
    class VulkanDevice {
        public:
            class Builder;
        
            class Queue {
                public:
                    Queue(VulkanPhysicalDevice::QueueFamily::Operation);
                    ~Queue();
                    
                    operator VkQueue() const;
                    
                private:
                    VkQueue handle_;
                    VulkanPhysicalDevice::QueueFamily::Operation supported_operations_;
            };
            
        private:

            // vkDestroyDevice
            void (VKAPI_PTR *fp_vk_destroy_device)(VkDevice, const VkAllocationCallbacks*);
        
            VkDevice handle_;
            std::vector<Queue> queues_;
    };
    
    class VulkanDevice::Builder {
        public:
            Builder(const VulkanInstance& instance, const VulkanPhysicalDevice& device);
            ~Builder();
            
            NODISCARD VulkanDevice build() const;
            
            Builder& with_features(const VulkanPhysicalDevice::Features& features) const;
        
        private:
            const VulkanPhysicalDevice& device_;
            const VulkanInstance& instance_;
        

            
    };
    
}

#endif //VULKAN_SAMPLES_LOGICAL_DEVICE_HPP
