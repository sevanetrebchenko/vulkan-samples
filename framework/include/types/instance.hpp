
#pragma once

#include "core/defines.hpp"
#include <vector>
#include <vulkan/vulkan.h>

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
            
            VkInstance handle;
            std::vector<const char*> extensions;
            std::vector<const char*> layers;
        
        private:
            VulkanInstance();
    };
    
    class VulkanInstance::Builder {
        public:
            Builder();
            ~Builder();
            
            NODISCARD VulkanInstance build();
            
            NODISCARD Builder& with_extension(const char* extension);
            NODISCARD Builder& with_layer(const char* layer);
        
        private:
            // Returns a vector of unsupported extension names, if any.
            NODISCARD std::vector<const char*> validate_extensions() const;
            
            // Returns a vector of unsupported validation layers, if any.
            NODISCARD std::vector<const char*> validate_layers() const;
            
            std::vector<const char*> requested_extensions_;
            std::vector<const char*> requested_layers_;
    };
    
} // namespace vks