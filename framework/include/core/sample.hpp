
#pragma once

#include "core/defines.hpp"
#include "core/window.hpp"
#include "vulkan/instance.hpp"

namespace vks {
    
    // Sample base class.
    
    class Sample {
        public:
            Sample();
            ~Sample();
    
        protected:
            VulkanInstance instance_;
    
        private:
            Window window_;
    };
    
}// namespace vks

