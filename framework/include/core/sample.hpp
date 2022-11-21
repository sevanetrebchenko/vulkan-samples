
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
        
//            EventListener<Sample> event_listener;
        
            bool test(float e) {
                std::cout << "hello from test" << std::endl;
                return true;
            }
            
            int selector(const VulkanDevice::Properties& p, const VulkanDevice::Features& f);
    
        private:
    };
    
}// namespace vks

