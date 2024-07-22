
#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <vulkan/vulkan.h>
#include <memory> // std::shared_ptr

namespace vks {
    
    class Context {
        public:
            class Builder {
                public:
                    Builder& with_headless(bool headless);
                    
                    std::shared_ptr<Context> build();
                    
                private:
                    std::shared_ptr<Context> handle;
            };
            
            ~Context();
            
            void initialize();
            
        private:
            Context();
            
            VkInstance instance;
    };
    
}

#endif // CONTEXT_HPP
