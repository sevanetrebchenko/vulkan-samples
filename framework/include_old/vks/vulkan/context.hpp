
#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "vks/vulkan/pipeline.hpp"
#include <vulkan/vulkan.h>

namespace vks {

    struct ContextDescription {
        ContextDescription& set_width(unsigned width);
        ContextDescription& set_height(unsigned height);
        ContextDescription& set_extents(unsigned width, unsigned height);

        ContextDescription& set_application_name(const char* name);

        // Use for both instance and device extensions
        ContextDescription& enable_extension(const char* extension);

        ContextDescription& set_enabled_features(VkPhysicalDeviceFeatures features);
        
        unsigned width;
        unsigned height;
    };
    
    struct Context {
        void create_graphics_pipeline(const PipelineDescription& pipeline_description);
        
        VkInstance vulkan_instance;
        VkPhysicalDevice vulkan_physical_device;
        VkPhysicalDeviceProperties vulkan_physical_device_properties;
        VkPhysicalDeviceFeatures vulkan_physical_device_features;
        VkDevice vulkan_device;
        
        VkDebugUtilsMessengerEXT vulkan_debug_messenger;
        
        unsigned width;
        unsigned height;
    
        // Vulkan context should be accessed through Context::instance
    };
    

}

#endif // CONTEXT_HPP
