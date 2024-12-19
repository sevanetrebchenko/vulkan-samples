
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
        DeviceDescription& enable_instance_extension(const char* name);
        DeviceDescription& enable_device_extension(const char* name);
        DeviceDescription& enable_features(VkPhysicalDeviceFeatures features);
        
        DeviceDescription& set_application_name(const char* name);
        
        unsigned width;
        unsigned height;
        const char* name;
        VkPhysicalDeviceFeatures enabled_features;
        
        std::vector<const char*> instance_extensions;
        std::vector<const char*> device_extensions;
    };
    
    struct Device {
        static std::shared_ptr<Device> instance();
        
        // Device should be accessed through Device::instance()
        Device();
        ~Device();
        
        void initialize(const DeviceDescription& description);
        void shutdown();
        
        std::shared_ptr<GraphicsPipeline> create_graphics_pipeline(GraphicsPipelineDescription pipeline_description);
        
        VmaAllocator vk_allocator;
        
        VkInstance vulkan_instance;
        VkPhysicalDevice vulkan_physical_device;
        VkPhysicalDeviceProperties vulkan_physical_device_properties;
        VkPhysicalDeviceFeatures vulkan_physical_device_features;
        VkDevice vulkan_device;
        
        VkDebugUtilsMessengerEXT vulkan_debug_messenger;
        
        unsigned width;
        unsigned height;
        
    };
    
}

#endif // DEVICE_HPP
