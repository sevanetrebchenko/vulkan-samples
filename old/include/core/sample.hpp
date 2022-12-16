
#pragma once

#include "core/defines.hpp"

#include "vulkan/instance.hpp"
#include "core/events.hpp"

#include <iostream>

namespace vks {
    
    // Sample base class.
    
    class Sample {
        public:
            Sample();
            ~Sample();
    

            
        protected:
            std::shared_ptr<VulkanInstance> m_instance;
            std::shared_ptr<VulkanDevice> m_device;
    
        private:
    };
    
}// namespace vks

