
#pragma once

#include "core/debug.hpp"

#include <cstdint>

#if defined(VKS_PLATFORM_WINDOWS)
    #include <windows.h>
    #include <vulkan/vulkan_win32.h>
#endif

namespace vks {
    
    // Forward declarations.
    class VulkanInstance;
    class VulkanDevice;
    
    class VulkanWindow {
        public:
            class Builder;
            
            class Surface {
                public:
                
                private:
                
            };
            
            ~VulkanWindow();
            
            NODISCARD bool has_presentation_support(std::shared_ptr<VulkanQueue> queue, u32 queue_family_index);
        
        private:
            class Data;
            
            VulkanWindow();
    };
    
    class VulkanWindow::Builder {
        public:
            Builder(std::shared_ptr<VulkanInstance> instance);
            ~Builder();
            
            NODISCARD std::shared_ptr<VulkanWindow>& build() const;
            
            Builder& with_debug_name(const char* name);
            
            // Dimensions.
            Builder& with_width(std::uint32_t width);
            Builder& with_height(std::uint32_t height);
            
        private:
            std::shared_ptr<VulkanWindow> m_window;
            
            
            std::shared_ptr<VulkanInstance> m_instance;
        
            #if defined(VKS_PLATFORM_WINDOWS)
                
                // vkCreateWin32SurfaceKHR
                VkResult (VKAPI_PTR* fp_vk_create_win32_surface_)(VkInstance, const VkWin32SurfaceCreateInfoKHR*, const VkAllocationCallbacks*, VkSurfaceKHR*);
                
                HINSTANCE instance_handle_;
                HWND window_handle_;
                
            #elif defined(VKS_PLATFORM_LINUX)
            
            
            
            #elif defined(VKS_PLATFORM_ANDROID)
            
            
            
            #elif defined(VKS_PLATFORM_APPLE)
            
            
            
            #endif
            
            const char* name_;
            std::uint32_t width_;
            std::uint32_t height_;
    };
    
}