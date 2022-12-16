
#pragma once

#include "graphics/context.hpp"

#include <vulkan/vulkan.h>
#include <vector>

namespace vks {
    
    class VulkanContext final : public Context {
        public:
            VulkanContext();
            ~VulkanContext() override;
            
        private:
            VkInstance m_handle;
        
            const char* m_application_name;
            // Version m_application_version;
        
            const char* m_engine_name;
            // Version m_engine_version;
        
            // Version m_runtime_version;
        
            bool m_headless;
        
            std::vector<const char*> m_extensions;
            std::vector<const char*> m_validation_layers;
    };
    
}
