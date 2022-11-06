
#include "vulkan/device/types.hpp"
#include "vulkan/loader.hpp"
#include "vulkan/instance.hpp"
#include "vulkan/device.hpp"


namespace vks {
    
    VulkanPhysicalDevice::Properties::Properties(VulkanInstance* instance, VkPhysicalDevice device) {
        // Load required Vulkan functions.
        detail::fp_vk_get_instance_proc_addr vk_get_instance_proc_addr = detail::get_vulkan_symbol_loader();
    
        #if defined(VK_VERSION_1_1) || defined(VK_VERSION_1_2) || defined(VK_VERSION_1_3)
            // vkGetPhysicalDeviceProperties2 is core since Vulkan 1.1.
            fp_vk_get_physical_device_properties = reinterpret_cast<typeof(fp_vk_get_physical_device_properties)>(vk_get_instance_proc_addr(*instance, "vkGetPhysicalDeviceProperties2"));
        #else
            // vkGetPhysicalDeviceProperties2KHR is provided in Vulkan 1.0 with the 'VK_KHR_get_physical_device_properties2' instance extension.
            // TODO: check for extension.
            fp_vk_get_physical_device_properties = reinterpret_cast<typeof(fp_vk_get_physical_device_properties)>(vk_get_instance_proc_addr(*instance, "vkGetPhysicalDeviceProperties2KHR"));
        #endif
    
        std::vector<const char*> device_extensions;
        
        
        
        
        // Construct chain of all valid device property structures.
        #if defined(VK_VERSION_1_2)
            VkPhysicalDeviceDepthStencilResolveProperties depthStencilResolveProperties;
    
            VkPhysicalDeviceDescriptorIndexingProperties descriptorIndexingProperties;
    
            VkPhysicalDeviceDriverProperties driverProperties;
        #endif
        
        #if defined(VK_VERSION_1_1)
            #if defined(VK_KHR_acceleration_structure)
                VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties;
                device_extensions.emplace_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            #endif
            
            // VK_NV_device_generated_commands
            VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV generatedCommandsProperties;
        #endif
        
        #if defined(VK_VERSION_1_0)
            // VK_EXT_blend_operation_advanced
            VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT blendOperationAdvancedProperties;
        
            // VK_EXT_conservative_rasterization
            VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservativeRasterizationProperties;
        
            // VK_NV_cooperative_matrix
            VkPhysicalDeviceCooperativeMatrixPropertiesNV cooperativeMatrixProperties;
        
            // VK_EXT_custom_border_color
            VkPhysicalDeviceCustomBorderColorPropertiesEXT physicalDeviceCustomBorderColorProperties;
            
            // VK_EXT_discard_rectangles
            VkPhysicalDeviceDiscardRectanglePropertiesEXT discardRectangleProperties;
    
            #if !(defined(VK_VERSION_1_2))
                // VkPhysicalDeviceDepthStencilResolveProperties is core since Vulkan 1.2.
                // VK_KHR_depth_stencil_resolve
                VkPhysicalDeviceDepthStencilResolvePropertiesKHR depthStencilResolveProperties;
                
                // VkPhysicalDeviceDescriptorIndexingProperties is core since Vulkan 1.2.
                // VK_EXT_descriptor_indexing
                VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptorIndexingProperties;
                
                // VkPhysicalDeviceDriverProperties is core since Vulkan 1.2.
                // VK_KHR_driver_properties
                VkPhysicalDeviceDriverPropertiesKHR driverProperties;
            #endif
            
            // VK_EXT_physical_device_drm
            VkPhysicalDeviceDrmPropertiesEXT drmProperties;
            
            #if defined(VK_KHR_driver_properties)
            
            #endif
            
            #if defined(VK_EXT_extended_dynamic_state3)
                VkPhysicalDeviceExtendedDynamicState3PropertiesEXT extendedDynamicState3Properties;
            #endif
            
        #endif
        
        VkPhysicalDeviceProperties2 properties {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = nullptr
        };
        
        fp_vk_get_physical_device_properties(device, &properties);
    }

    VulkanPhysicalDevice::Properties::~Properties() {
    }
    
    VulkanPhysicalDevice::Properties::AccelerationStructureProperties::operator VkPhysicalDeviceAccelerationStructurePropertiesKHR() const {
        return VkPhysicalDeviceAccelerationStructurePropertiesKHR {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR,
            .pNext = nullptr,
            .maxGeometryCount = max_geometry_count,
            .maxInstanceCount = max_instance_count,
            .maxPrimitiveCount = max_primitive_count,
            .maxPerStageDescriptorAccelerationStructures = max_per_stage_descriptor_acceleration_structures,
            .maxPerStageDescriptorUpdateAfterBindAccelerationStructures = max_per_stage_descriptor_update_after_bind_acceleration_structures,
            .maxDescriptorSetAccelerationStructures = max_descriptor_set_acceleration_structures,
            .maxDescriptorSetUpdateAfterBindAccelerationStructures = max_descriptor_set_update_after_bind_acceleration_structures,
            .minAccelerationStructureScratchOffsetAlignment = min_acceleration_structure_scratch_offset_alignment
        };
    }
    
}