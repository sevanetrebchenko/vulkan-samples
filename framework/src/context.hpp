
#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <vulkan/vulkan.h>

namespace vks {
    
    class Context {
        public:
            class Builder;
            
        private:
            VkInstance m_instance;
    };
    
    class Context::Builder {
        public:
            Builder();
            ~Builder();
            
            void build();
            
        private:
        
    };
    
}


#endif // CONTEXT_HPP
