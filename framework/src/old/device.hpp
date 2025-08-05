
#ifndef DEVICE_HPP
#define DEVICE_HPP

#include "renderpass.hpp"

#include <vulkan/vulkan.h>


namespace vks {
    
    class Device {
        public:
            class Builder;
            
            RenderPass::Builder create_render_pass();
            
        private:
            VkPhysicalDevice m_device;
            VkDevice m_interface;
    };
    
    class Device::Builder {
        public:
        
        private:
        
    };
    
}


#endif // DEVICE_HPP
