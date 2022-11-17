
#pragma once

#include "core/debug.hpp"

#if defined(VKS_PLATFORM_WINDOWS)
    #include <windows.h>
    #include <vulkan/vulkan_win32.h>
#endif

namespace vks {
    
    // Forward declaration.
    class VulkanInstance;
    
    class VulkanWindow {
        public:
            class Builder;
            
            class Surface {
                public:
                
                private:
                
            };
            
            ~VulkanWindow();
            
        private:
            VulkanWindow();
            
    };
    
    class VulkanWindow::Builder {
        public:
            Builder(const VulkanInstance& instance);
            ~Builder();
            
            VulkanWindow& build() const;
            
            Builder& with_width(std::uint32_t width);
            Builder& with_height(std::uint32_t height);
            
        private:
            const VulkanInstance& instance_;
        
            #if defined(VKS_PLATFORM_WINDOWS)
                
                VkResult (VKAPI_PTR* fp_vk_create_win32_surface_)(VkInstance, const VkWin32SurfaceCreateInfoKHR*, const VkAllocationCallbacks*, VkSurfaceKHR*);
                
            #elif defined(VKS_PLATFORM_LINUX)
            
            
            
            #elif defined(VKS_PLATFORM_ANDROID)
            
            
            
            #elif defined(VKS_PLATFORM_APPLE)
            
            
            
            #endif
    };
    
}