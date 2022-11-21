
#pragma once

#include "core/defines.hpp"
#include "vulkan/device.hpp"
#include "vulkan/types.hpp"
#include "vulkan/loader.hpp"
#include "vulkan/window.hpp"

#include <vector>
#include <vulkan/vulkan.h>
#include <cstdint>

#include <memory>

namespace vks {
    
    class VulkanInstance : public ManagedObject<VulkanInstance> {
        public:
            class Builder;
            
            ~VulkanInstance();
            
            VulkanInstance(VulkanInstance&& other) noexcept;
            VulkanInstance& operator=(VulkanInstance&& other) noexcept;
            
            // Instances should not be copied.
            VulkanInstance(const VulkanInstance& other) = delete;
            VulkanInstance& operator=(const VulkanInstance& other) = delete;
            
            operator VkInstance() const;
            
            NODISCARD VulkanDevice::Builder create_device();
            NODISCARD VulkanWindow::Builder create_window();
            
            NODISCARD bool is_extension_enabled(const char* extension) const;
            
            Version runtime;
            std::vector<const char*> extensions;
        
        private:
            // vkDestroyInstance
            void (VKAPI_PTR *fp_vk_destroy_instance)(VkInstance, const VkAllocationCallbacks*);
            
            VulkanInstance(VkInstance handle);
            VkInstance handle_;
    };
    
    class VulkanInstance::Builder {
        public:
            Builder();
            ~Builder();
            
            NODISCARD VulkanInstance build() const;
            
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
            
            // Headless mode does not load extensions required for presentation.
            Builder& with_headless_mode(bool headless = true);
            
            // TODO: validation features?
        
        private:
            // vkCreateInstance
            VkResult (VKAPI_PTR *fp_vk_create_instance)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*);
            
            // vkEnumerateInstanceVersion
            VkResult (VKAPI_PTR *fp_vk_enumerate_instance_version)(std::uint32_t*);
            
            // vkEnumerateInstanceExtensionProperties
            VkResult (VKAPI_PTR *fp_vk_enumerate_instance_extension_properties)(const char*, std::uint32_t*, VkExtensionProperties*);
            
            // vkEnumerateInstanceLayerProperties
            VkResult (VKAPI_PTR *fp_vk_enumerate_instance_layer_properties)(std::uint32_t*, VkLayerProperties*);
            
            const char* application_name_;
            Version application_version_;
            
            const char* engine_name_;
            Version engine_version_;
            
            Version api_version_;
            
            std::vector<const char*> extensions_;
            std::vector<const char*> validation_layers_;
    };
    
} // namespace vks