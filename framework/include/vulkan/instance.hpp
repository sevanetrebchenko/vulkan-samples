
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
            
            ~VulkanInstance() override;
            
            operator VkInstance() const;
        
            NODISCARD VulkanWindow::Builder create_window();
            NODISCARD VulkanDevice::Builder create_device();
            
            NODISCARD bool is_headless() const;
            NODISCARD bool is_extension_enabled(const char* extension) const;
            
            NODISCARD Version get_runtime_version() const;
            
        private:
            class Data;
            
            // Instances should only be created using VulkanInstance::Builder.
            VulkanInstance();
    };
    
    class VulkanInstance::Builder {
        public:
            Builder();
            ~Builder();
            
            // Returns nullptr on failure.
            NODISCARD std::shared_ptr<VulkanInstance> build();
            
            Builder& with_application_name(const char* name);
            
            Builder& with_application_version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch = 0u);
            
            Builder& with_engine_name(const char* name);
            
            Builder& with_engine_version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch = 0u);
            
            // Specify the Vulkan API version that the source code of the application expects to target.
            Builder& with_target_runtime_version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch = 0u);
            
            Builder& with_enabled_extension(const char* extension);
            Builder& with_disabled_extension(const char* extension);
            
            Builder& with_enabled_validation_layer(const char* layer);
            Builder& with_disabled_validation_layer(const char* layer);
            
            // Headless mode does not load extensions required for presentation.
            Builder& with_headless_mode(bool headless = true);
            
            // TODO: validation features?
        
        private:
            std::shared_ptr<VulkanInstance> m_instance;
    };
    
} // namespace vks