
#ifndef DEVICE_HPP
#define DEVICE_HPP

#include "vks/vulkan/pipeline.hpp"
#include "vks/vulkan/descriptor_set.hpp"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <vector> // std::vector
#include <memory> // std::shared_ptr

namespace vks {
    
    struct DeviceDescription {
        DeviceDescription();
        ~DeviceDescription();
        
        DeviceDescription& set_width(unsigned width);
        DeviceDescription& set_height(unsigned height);
        DeviceDescription& set_extent(unsigned width, unsigned height);
        
        // TODO: find a way to unify extension interface
        DeviceDescription& enable_extension(const char* name);
        DeviceDescription& enable_features(VkPhysicalDeviceFeatures features);
        
        DeviceDescription& set_application_name(const char* name);
        
        VkPhysicalDeviceFeatures enabled_features;
        
        unsigned width;
        unsigned height;
        const char* name;
        
        std::vector<const char*> extensions;
    };
    
    struct Device {
        static std::shared_ptr<Device> get_instance();
        
        // Device should be accessed through Device::get_instance()
        Device();
        ~Device();
        
        void initialize(const DeviceDescription& description);
        void shutdown();
        
        std::shared_ptr<GraphicsPipeline> create_graphics_pipeline(GraphicsPipelineDescription pipeline_description);
        
        VmaAllocator allocator;
        
        VkInstance instance;
        
        VkPhysicalDevice gpu;
        VkPhysicalDeviceProperties gpu_properties;
        VkPhysicalDeviceFeatures gpu_features;
        
        VkDevice device;
        
        VkDebugUtilsMessengerEXT messenger;
        
        unsigned width;
        unsigned height;
    };
    
}

#endif // DEVICE_HPP
