
#pragma once

#include "core/defines.hpp"
#include "vulkan/device.hpp"
#include "vulkan/loader.hpp"
#include <vector>
#include <vulkan/vulkan.h>

namespace vks {
    
    class VulkanInstance {
        public:
            class Builder;
            
            ~VulkanInstance();
        
            VulkanInstance(VulkanInstance&& other) noexcept;
            VulkanInstance& operator=(VulkanInstance&& other) noexcept;
            
            explicit operator VkInstance() const;
            NODISCARD VulkanDevice::Builder create_device() const;
        
            // Instances should not be copied.
            VulkanInstance(const VulkanInstance& other) = delete;
            VulkanInstance& operator=(const VulkanInstance& other) = delete;
            
        private:
            static struct DispatchTable {
                VkResult (VKAPI_PTR *fp_vk_create_instance)(const VkInstanceCreateInfo* create_info, const VkAllocationCallbacks* allocator, VkInstance* instance);
                void (VKAPI_PTR *fp_vk_destroy_instance)(VkInstance instance, const VkAllocationCallbacks* allocator);
    
                VkResult (VKAPI_PTR *fp_vk_enumerate_instance_version)(std::uint32_t* version);
    
                VkResult (VKAPI_PTR *fp_vk_enumerate_instance_extension_properties)(const char* layer_name, std::uint32_t* property_count, VkExtensionProperties* properties);
                VkResult (VKAPI_PTR *fp_vk_enumerate_instance_layer_properties)(std::uint32_t* property_count, VkLayerProperties* properties);
            } dispatch;
            
            VulkanInstance();
            VkInstance handle;
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