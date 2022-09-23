
#include "types/instance.hpp"
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <sstream>

#include <iostream>

namespace vks {

    VulkanInstance::VulkanInstance() : handle(nullptr) {
    }
    
    VulkanInstance::~VulkanInstance() {
        vkDestroyInstance(handle, nullptr);
    }

    VulkanInstance::VulkanInstance(VulkanInstance&& other) noexcept : extensions(std::move(other.extensions)),
                                                                      layers(std::move(other.layers))
                                                                      {
        if (handle) {
            vkDestroyInstance(handle, nullptr);
        }
        handle = other.handle;
        other.handle = nullptr;
    }
    
    VulkanInstance& VulkanInstance::operator=(VulkanInstance&& other) noexcept {
        // Check for self-assignment.
        if (this == &other) {
            return *this;
        }
    
        if (handle) {
            vkDestroyInstance(handle, nullptr);
        }
        handle = other.handle;
        other.handle = nullptr;
    
        extensions = std::move(other.extensions);
        layers = std::move(other.layers);
        
        return *this;
    }
    
    VulkanInstance::Builder::Builder() = default;
    VulkanInstance::Builder::~Builder() = default;

    VulkanInstance VulkanInstance::Builder::build() {
        VulkanInstance instance { };
        
        std::vector<const char*> unsupported_extensions = validate_extensions();
        if (!unsupported_extensions.empty()) {
            std::stringstream builder { };
            builder << "ERROR: Failed to create Vulkan instance - found " << unsupported_extensions.size() << " unsupported extension(s):\n";
            for (const char* extension : unsupported_extensions) {
                builder << "\t- " << extension << '\n';
            }
            throw std::runtime_error(builder.str());
        }
        
        instance.extensions = requested_extensions_;
        
        std::vector<const char*> unsupported_layers = validate_layers();
        if (!unsupported_layers.empty()) {
            std::stringstream builder { };
            builder << "ERROR: Failed to create Vulkan instance - found " << unsupported_layers.size() << " unsupported validation layer(s):\n";
            for (const char* layer : unsupported_layers) {
                builder << "\t- " << layer << '\n';
            }
            throw std::runtime_error(builder.str());
        }
        
        instance.layers = requested_layers_;
        
        // Create Vulkan instance.
        VkApplicationInfo application_info {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Vulkan Samples",
            .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .pEngineName = "vulkan-samples",
            .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .apiVersion = VK_MAKE_API_VERSION(0, 1, 0, 0)
        };

        VkInstanceCreateInfo instance_create_info {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &application_info,
            .enabledLayerCount = static_cast<std::uint32_t>(requested_layers_.size()),
            .ppEnabledLayerNames = requested_layers_.data(),
            .enabledExtensionCount = static_cast<std::uint32_t>(requested_extensions_.size()),
            .ppEnabledExtensionNames = requested_extensions_.data()
        };

        VkResult result = vkCreateInstance(&instance_create_info, nullptr, &instance.handle);
        if (result != VK_SUCCESS) {
            std::stringstream builder;
            builder << "ERROR: Failed to create Vulkan instance - vkCreateInstance exited with code " << result;
            
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    builder << " (VK_ERROR_OUT_OF_HOST_MEMORY).";
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    builder << " (VK_ERROR_OUT_OF_DEVICE_MEMORY).";
                    break;
                case VK_ERROR_INITIALIZATION_FAILED:
                    builder << " (VK_ERROR_INITIALIZATION_FAILED).";
                    break;
                case VK_ERROR_INCOMPATIBLE_DRIVER:
                    builder << " (VK_ERROR_INCOMPATIBLE_DRIVER).";
                    break;
                    
                // Validated above.
                // case VK_ERROR_LAYER_NOT_PRESENT: ...
                // case VK_ERROR_EXTENSION_NOT_PRESENT: ...
            }
            
            throw std::runtime_error(builder.str());
        }

        return instance;
    }

    VulkanInstance::Builder& VulkanInstance::Builder::with_extension(const char* requested) {
        // Ensure requested extension does not already exist.
        bool found = false;

        for (const char* extension : requested_extensions_) {
            if (strcmp(requested, extension) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            requested_extensions_.emplace_back(requested);
        }

        return *this;
    }

    VulkanInstance::Builder& VulkanInstance::Builder::with_layer(const char* requested) {
        // Ensure requested validation layer does not already exist.
        bool found = false;

        for (const char* layer : requested_layers_) {
            if (strcmp(requested, layer) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            requested_layers_.emplace_back(requested);
        }

        return *this;
    }
    
    std::vector<const char*> VulkanInstance::Builder::validate_extensions() const {
        // Enumerate all available extensions.
        std::uint32_t available_extension_count = 0u;
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr);
    
        std::vector<VkExtensionProperties> available_extensions(available_extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, available_extensions.data());
    
        std::vector<const char*> unsupported_extensions { };
        unsupported_extensions.reserve(requested_extensions_.size()); // Maximum number of unsupported extensions.
        
        // Validate all requested extensions are available.
        for (const char* requested : requested_extensions_) {
            bool found = false;

            for (std::uint32_t i = 0u; i < available_extension_count; ++i) {
                const char* available = available_extensions[i].extensionName;
                
                if (strcmp(requested, available) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                unsupported_extensions.emplace_back(requested);
            }
        }
        
        return unsupported_extensions;
    }
    
    std::vector<const char*> VulkanInstance::Builder::validate_layers() const {
        // Enumerate all available validation layers.
        std::uint32_t available_layer_count = 0u;
        vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(available_layer_count);
        vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers.data());

        std::vector<const char*> unsupported_layers { };
        unsupported_layers.reserve(requested_layers_.size()); // Maximum number of unsupported layers.
    
        // Verify that all requested validation layers are available.
        for (const char* requested : requested_layers_) {
            bool found = false;

            for (std::uint32_t i = 0u; i < available_layer_count; ++i) {
                const char* available = available_layers[i].layerName;

                if (strcmp(requested, available) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                unsupported_layers.emplace_back(requested);
            }
        }

        return unsupported_layers;
    }

} // namespace vks