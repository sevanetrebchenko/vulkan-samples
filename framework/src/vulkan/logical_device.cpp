
#include "vulkan/logical_device.hpp"
#include "vulkan/instance.hpp"
#include "vulkan/loader.hpp"
#include "core/debug.hpp"
#include "vulkan/device.hpp"


namespace vks {
    
    VulkanDevice VulkanDevice::Builder::build() const {
        // Create device queues.
    }
    
    VulkanDevice::Builder::Builder(const VulkanInstance& instance, const VulkanPhysicalDevice& device) : device_(device),
                                                                                                         instance_(instance),
                                                                                                         fp_vk_create_device(reinterpret_cast<typeof(fp_vk_create_device)>(detail::vk_get_instance_proc_addr(instance_, "vkCreateDevice")))
                                                                                                         {
        ASSERT(fp_vk_create_device != nullptr, "");
    }
    
    VulkanDevice::Builder::~Builder() {
    }
    
    VulkanDevice::Builder& VulkanDevice::Builder::with_features(const VulkanPhysicalDevice::Features& features) const {
        
    }
    

}