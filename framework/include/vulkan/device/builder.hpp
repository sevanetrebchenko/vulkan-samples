
#pragma once

#include "vulkan/device.hpp"
#include <vulkan/vulkan.h>
#include <vector>

namespace vks {
    
    class VulkanDevice::Builder {
    
    };
    
    class VulkanPhysicalDevice::Selector {
        public:
            Selector();
            ~Selector();
            
            Selector& with_selector();
            
            Selector& with_extension(const char* extension_name);
            
            NODISCARD VulkanPhysicalDevice select();
            
        private:
            std::vector<const char*> extensions_;
    };
    
}
