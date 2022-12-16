
#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "graphics/context.hpp"
#include "private/core/version.hpp"

namespace vks {
    
    class VulkanContext final : public Context {
        public:
            VulkanContext();
            ~VulkanContext() override;
        
            operator VkInstance() const;
    
        private:
            // vkDestroyInstance
            void (VKAPI_PTR* m_fp_vk_destroy_instance)(VkInstance, const VkAllocationCallbacks*);
            
            VkInstance m_handle;
        
            const char* m_application_name;
            Version m_application_version;
        
            const char* m_engine_name;
            Version m_engine_version;
        
            Version m_runtime_version;
        
            bool m_headless;
        
            std::vector<const char*> m_extensions;
            std::vector<const char*> m_validation_layers;
    };
    
}
