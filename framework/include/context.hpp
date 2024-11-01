
#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "core.hpp"
#include "window.hpp"
#include "renderpass.hpp"
#include <vulkan/vulkan.h>
#include <memory> // std::shared_ptr
#include <vector> // std::vector

namespace vks {
    
    class Context : public ManagedObject<Context> {
        public:
            class Builder;
            
            // Context instances should be created using Context::Builder
            Context();
            ~Context();
            
            VkInstance instance;
            VkPhysicalDevice gpu;
            VkDevice device;
            
            // nullptr for headless contexts
            std::shared_ptr<Window> window;
            
        private:
            VkDebugUtilsMessengerEXT m_debug_messenger;
            
            VkPhysicalDeviceProperties m_properties;
            VkPhysicalDeviceFeatures m_features;
            
            const char* m_name;
    };
    
    class Context::Builder {
        public:
            Builder();
            ~Builder();

            [[nodiscard]] std::shared_ptr<Context> build();

            Builder& enable_headless_mode();

            Builder& set_extent(unsigned width, unsigned height);
            Builder& enable_fullscreen();

            Builder& set_application_name(const char* name);
            
            // Use for both instance and device extensions
            Builder& enable_extension(const char* extension);

            // Logical or
            Builder& enable_features(VkPhysicalDeviceFeatures features);
            
        private:
            void initialize_vulkan_instance();
            
            void initialize_window();
            
            void select_physical_device();
            bool verify_requested_feature_support(const VkPhysicalDeviceFeatures& supported_features) const;
            
            void initialize_logical_device();
            
            std::shared_ptr<Context> m_handle;
            
            std::vector<const char*> m_extensions;
            VkPhysicalDeviceFeatures m_requested_features;
            
            bool m_headless;
            
            bool m_fullscreen;
            unsigned m_width;
            unsigned m_height;
    };
    
}


#endif // CONTEXT_HPP
