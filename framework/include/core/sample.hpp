
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
            struct SampleInfo {
                bool enable_debugging;
            };
            
        private:
            struct Data {
                int test;
            } data;
            
            VulkanInstance instance_;
            Window window_;
    };
    
}// namespace vks

