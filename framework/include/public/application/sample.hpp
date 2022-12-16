
#pragma once

#include "graphics/window.hpp"
#include "graphics/context.hpp"
#include "graphics/device.hpp"

namespace vks {
    
    class Sample {
        public:
            Sample();
            virtual ~Sample();
            
        protected:
            std::shared_ptr<Window> m_window;
            std::shared_ptr<Context> m_context;
            std::shared_ptr<Device> m_device;
            
        private:
        
    };
    
}