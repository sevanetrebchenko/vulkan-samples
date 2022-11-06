
#pragma once

#include "core/defines.hpp"
#include "device.hpp"
#include "vulkan/types.hpp"
#include "vulkan/loader.hpp"
#include <vector>
#include <vulkan/vulkan.h>
#include <cstdint>

namespace vks {
    
    class VulkanInstance {
        public:
            class Builder;
            
            ~VulkanInstance();
        
            VulkanInstance(VulkanInstance&& other) noexcept;
            VulkanInstance& operator=(VulkanInstance&& other) noexcept;
        
            // Instances should not be copied.
            VulkanInstance(const VulkanInstance& other) = delete;
            VulkanInstance& operator=(const VulkanInstance& other) = delete;
            
            operator VkInstance() const;
            NODISCARD VulkanPhysicalDevice::Selector select_physical_device()  {
                return VulkanPhysicalDevice::Selector(this);
            }
        
            NODISCARD bool is_extension_enabled(const char* extension) const;
            
            Version api_version;
            std::vector<const char*> extensions;
    
        private:
            // Vulkan function dispatch.
            void (VKAPI_PTR *fp_vk_destroy_instance)(VkInstance, const VkAllocationCallbacks*);
            
            VulkanInstance(VkInstance handle);
            VkInstance handle_;
    };
    
    class VulkanInstance::Builder {
        public:
            enum VulkanValidationFeature : std::uint32_t {
                CORE_VALIDATION       = (1 << 0),
                GPU_VALIDATION        = (1 << 1),
                THREAD_SAFETY         = (1 << 2),
                STATELESS_VALIDATION  = (1 << 3),
                OBJECT_LIFETIMES      = (1 << 4),
                UNIQUE_HANDLES        = (1 << 5),
                SYNCHRONIZATION       = (1 << 6),
                BEST_PRACTICES        = (1 << 7),
                DEBUG_PRINTING        = (1 << 8)
            };
            
            Builder();
            ~Builder();
            
            NODISCARD VulkanInstance build();
            
            Builder& with_application_name(const char* name);
            
            Builder& with_application_version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch = 0u);
        
            Builder& with_engine_name(const char* name);
            
            Builder& with_engine_version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch = 0u);
        
            // Specify the Vulkan API version that the source code of the application expects to target.
            // Note: specifying unsupported / invalid versions will throw an exception.
            Builder& with_target_api_version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch = 0u); // TODO: no variant for now.
        
            Builder& with_enabled_extension(const char* extension);
            Builder& with_disabled_extension(const char* extension);
            
            Builder& with_enabled_validation_layer(const char* layer);
            Builder& with_disabled_validation_layer(const char* layer);
            
            // Enable / disable features checked by validation layers.
            Builder& with_enabled_validation_feature(VulkanValidationFeature feature);
            Builder& with_disabled_validation_feature(VulkanValidationFeature feature);
        
            // Headless mode does not load extensions required for presentation.
            Builder& with_headless_mode(bool headless = true);
            
        private:
            // Vulkan function dispatch.
            VkResult (VKAPI_PTR *fp_vk_create_instance)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*);
            VkResult (VKAPI_PTR *fp_vk_enumerate_instance_version)(std::uint32_t*);
            VkResult (VKAPI_PTR *fp_vk_enumerate_instance_extension_properties)(const char*, std::uint32_t*, VkExtensionProperties*);
            VkResult (VKAPI_PTR *fp_vk_enumerate_instance_layer_properties)(std::uint32_t*, VkLayerProperties*);
            
            void add_validation_feature(VulkanValidationFeature feature);
            NODISCARD bool has_validation_feature(VulkanValidationFeature feature) const;
            void remove_validation_feature(VulkanValidationFeature feature);
            
            const char* application_name_;
            Version application_version_;
            
            const char* engine_name_;
            Version engine_version_;
        
            Version api_version_;
            
            std::vector<const char*> extensions_;
            std::vector<const char*> validation_layers_;
            
            std::vector<VulkanValidationFeature> validation_features_;
    };
    
    VulkanInstance::Builder::VulkanValidationFeature operator|(VulkanInstance::Builder::VulkanValidationFeature first, VulkanInstance::Builder::VulkanValidationFeature second);
    VulkanInstance::Builder::VulkanValidationFeature operator&(VulkanInstance::Builder::VulkanValidationFeature first, VulkanInstance::Builder::VulkanValidationFeature second);
    
} // namespace vks