
#pragma once

#include "core/defines.hpp"
#include "core/window.hpp"
#include "types/instance.hpp"

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
            VulkanInstance instance_;
            Window window_;
    };
    
}// namespace vks

