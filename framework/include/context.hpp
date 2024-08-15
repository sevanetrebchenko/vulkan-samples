
#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "core.hpp"
#include "window.hpp"
#include "renderpass.hpp"
#include <vulkan/vulkan.h>
#include <memory> // std::shared_ptr
#include <vector> // std::vector

namespace vks {
    
    class Context : ManagedObject<Context> {
        public:
            class Builder;
            
            // Context instances should be created using Context::Builder
            Context();
            ~Context();
            
        private:
            // Access to: m_instance
            friend class Window;
            
            // Instance
            VkInstance m_instance;
            VkDebugUtilsMessengerEXT m_debug_messenger;
            
            // Physical device
            VkPhysicalDevice m_gpu;
            
            // Logical device
            VkDevice m_device;
            
            const char* m_name;
            
            std::shared_ptr<Window> m_window;
    };
    
    class Context::Builder {
        public:
            Builder();
            ~Builder();

            [[nodiscard]] std::shared_ptr<Context> build();

            // Enabling headless mode
            Builder& enable_headless_mode();

            Builder& set_extent(unsigned width, unsigned height);
            Builder& enable_fullscreen();

            Builder& set_application_name(const char* name);
            
            // Use for both instance and device extensions
            Builder& enable_extension(const char* extension);

            Builder& enable_features(VkPhysicalDeviceFeatures features);
            
        private:
            void initialize_vulkan_instance();
            
            void initialize_window();
            void initialize_surface();
            
            void select_physical_device();
            void initialize_logical_device();
            
            std::shared_ptr<Context> m_handle;
            
            std::vector<const char*> m_extensions;
            
            bool m_headless;
            
            bool m_fullscreen;
            unsigned m_width;
            unsigned m_height;
    };
    
}


#endif // CONTEXT_HPP
