
#include "vulkan/instance.hpp"
#include "vulkan/device.hpp"
#include "core/debug.hpp"
#include <cstring>
#include <cstdint>

namespace vks {
    
    VulkanInstance::VulkanInstance(VkInstance handle) : handle_(handle),
                                                        fp_vk_destroy_instance(reinterpret_cast<typeof(fp_vk_destroy_instance)>(detail::vk_get_instance_proc_addr(handle, "vkDestroyInstance"))) {
        ASSERT(fp_vk_destroy_instance != nullptr, "");
    }
    
    VulkanInstance::~VulkanInstance() {
        fp_vk_destroy_instance(handle_, nullptr);
    }
    
    VulkanInstance::VulkanInstance(VulkanInstance&& other) noexcept {
        if (handle_) {
            // 'this' is already initialized.
            // fp_vk_destroy_instance(handle_, nullptr);
        }
        
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    
    VulkanInstance& VulkanInstance::operator=(VulkanInstance&& other) noexcept {
        // Check for self-assignment.
        if (this == &other) {
            return *this;
        }
        
        if (handle_) {
            // fp_vk_destroy_instance(handle_, nullptr);
        }
        handle_ = other.handle_;
        other.handle_ = nullptr;
        
        return *this;
    }
    
    VulkanInstance::operator VkInstance() const {
        return handle_;
    }

    VulkanDevice::Builder VulkanInstance::create_device() {
        VulkanDevice::Builder builder(*this);
        
        return builder;
    }
    
    VulkanWindow::Builder VulkanInstance::create_window() {
        VulkanWindow::Builder builder(*this);
        
        return builder;
    }
    
    VulkanInstance::Builder::Builder() : application_name_("My Application"),
                                         application_version_({
                                                                      .major = 1u,
                                                                      .minor = 0u,
                                                                      .patch = 0u
                                                              }),
                                         engine_name_("vulkan-samples"),
                                         engine_version_({
                                                                 .major = 1u,
                                                                 .minor = 0u,
                                                                 .patch = 0u
                                                         }),
            // Use Vulkan 1.0 by default.
                                         api_version_({
                                                              .major = 1u,
                                                              .minor = 0u,
                                                              .patch = 0u
                                                      })
    {
        // Load global-level Vulkan functions.
        fp_vk_create_instance = reinterpret_cast<typeof(fp_vk_create_instance)>(detail::vk_get_instance_proc_addr(VK_NULL_HANDLE, "vkCreateInstance"));
        fp_vk_enumerate_instance_version = reinterpret_cast<typeof(fp_vk_enumerate_instance_version)>(detail::vk_get_instance_proc_addr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
        fp_vk_enumerate_instance_extension_properties = reinterpret_cast<typeof(fp_vk_enumerate_instance_extension_properties)>(detail::vk_get_instance_proc_addr(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties"));
        fp_vk_enumerate_instance_layer_properties = reinterpret_cast<typeof(fp_vk_enumerate_instance_layer_properties)>(detail::vk_get_instance_proc_addr(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties"));
        
        // TODO: messages
        ASSERT(fp_vk_create_instance != nullptr, "");
        ASSERT(fp_vk_enumerate_instance_extension_properties != nullptr, "");
        ASSERT(fp_vk_enumerate_instance_layer_properties != nullptr, "");
        
        // Enable default presentation extensions.
        with_headless_mode(false);
        
        // Enable / disable default Vulkan validation features.
        with_enabled_validation_layer("VK_LAYER_KHRONOS_validation");
    }
    
    VulkanInstance::Builder::~Builder() {
    }
    
    VulkanInstance VulkanInstance::Builder::build() const {
        // Validate requested validation layers first (instance extensions may depend on extensions provided by validation layers).
        std::uint32_t available_layer_count = 0u;
        fp_vk_enumerate_instance_layer_properties(&available_layer_count, nullptr);
        
        std::vector<VkLayerProperties> available_layers(available_layer_count);
        fp_vk_enumerate_instance_layer_properties(&available_layer_count, available_layers.data());
        
        std::vector<const char*> unsupported_layers;
        unsupported_layers.reserve(validation_layers_.size()); // Maximum number of unsupported layers.
        
        // Verify that all requested validation layers are available.
        for (const char* requested : validation_layers_) {
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
        
        if (!unsupported_layers.empty()) {
            // TODO: specify which layers are unsupported.
            throw VulkanException("ERROR: Failed to create Vulkan instance - found %zu unsupported validation layer(s).");
        }
        
        // Validate requested instance extensions.
        
        // Determine total number of supported instance extensions.
        // Include instance extensions that are implicitly enabled / provided by the Vulkan implementation.
        std::uint32_t available_extension_count = 0u;
        fp_vk_enumerate_instance_extension_properties(nullptr, &available_extension_count, nullptr);
        
        std::vector<VkExtensionProperties> available_extensions(available_extension_count);
        fp_vk_enumerate_instance_extension_properties(nullptr, &available_extension_count, available_extensions.data());
        
        // Include instance extensions provided by enabled validation layers.
        for (const char* layer : validation_layers_) {
            std::uint32_t layer_extension_count = 0u;
            fp_vk_enumerate_instance_extension_properties(layer, &layer_extension_count, nullptr);
            
            std::vector<VkExtensionProperties> layer_extensions(layer_extension_count);
            fp_vk_enumerate_instance_extension_properties(layer, &layer_extension_count, layer_extensions.data());
            
            available_extension_count += layer_extension_count;
            available_extensions.insert(available_extensions.end(), layer_extensions.begin(), layer_extensions.end());
        }
        
        std::vector<const char*> unsupported_extensions;
        unsupported_extensions.reserve(extensions_.size()); // Maximum number of unsupported extensions.
        
        // Verify all requested extensions are available.
        for (const char* requested : extensions_) {
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
        
        if (!unsupported_extensions.empty()) {
            // TODO: specify which instance extensions are unsupported.
            throw VulkanException("ERROR: Failed to create Vulkan instance - found %zu unsupported extension(s).", unsupported_extensions.size());
        }
        
        // Determine application version.
        std::uint32_t available_instance_version;
        fp_vk_enumerate_instance_version(&available_instance_version);
        
        if (api_version_ > available_instance_version) {
            throw VulkanException("ERROR: Failed to create Vulkan instance - target instance Vulkan version (%i.%i.%i.0) is unsupported (highest available Vulkan version: %i.%i.%i.%i).",
                                  api_version_.major, api_version_.minor, api_version_.patch,
                                  VK_API_VERSION_MAJOR(available_instance_version), VK_API_VERSION_MINOR(available_instance_version), VK_API_VERSION_PATCH(available_instance_version), VK_API_VERSION_VARIANT(available_instance_version));
        }
        
        // Create Vulkan instance.
        VkApplicationInfo application_info {
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pNext = nullptr,
                .pApplicationName = application_name_,
                .applicationVersion = application_version_,
                .pEngineName = engine_name_,
                .engineVersion = engine_version_,
                .apiVersion = api_version_
        };
        
        VkInstanceCreateInfo instance_create_info {
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pNext = nullptr,
                .pApplicationInfo = &application_info,
                .enabledLayerCount = static_cast<std::uint32_t>(validation_layers_.size()),
                .ppEnabledLayerNames = validation_layers_.data(),
                .enabledExtensionCount = static_cast<std::uint32_t>(extensions_.size()),
                .ppEnabledExtensionNames = extensions_.data()
        };
        
        VkInstance handle = nullptr;
        VkResult result = fp_vk_create_instance(&instance_create_info, nullptr, &handle);
        if (result != VK_SUCCESS) {
            throw VulkanException("ERROR: Failed to create Vulkan instance - vkCreateInstance exited with code %i.", result);
        }
        
        return { handle };
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_application_name(const char* name) {
        if (strlen(name) > 0u) {
            application_name_ = name;
        }
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_application_version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch) {
        application_version_ = {
            .major = major,
            .minor = minor,
            .patch = patch
        };
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_engine_name(const char* name) {
        if (strlen(name) > 0u) {
            engine_name_ = name;
        }
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_engine_version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch) {
        engine_version_ = {
            .major = major,
            .minor = minor,
            .patch = patch
        };
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_target_api_version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch) {
        api_version_ = {
            .major = major,
            .minor = minor,
            .patch = patch
        };
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_enabled_extension(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
        
        // Ensure requested extension does not already exist.
        bool found = false;
        
        for (const char* extension : extensions_) {
            if (strcmp(requested, extension) == 0) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            // Only add the first instance of the extension.
            extensions_.emplace_back(requested);
        }
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_disabled_extension(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
        
        for (std::size_t i = 0u; i < extensions_.size(); ++i) {
            const char* extension = extensions_[i];
            
            if (strcmp(requested, extension) == 0) {
                // Found extension.
                std::swap(extensions_[i], extensions_[extensions_.size() - 1u]); // Swap idiom.
                extensions_.pop_back();
                
                // Extension is guaranteed to be present in the list only once.
                break;
            }
        }
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_enabled_validation_layer(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
        
        // Ensure requested validation layer does not already exist.
        bool found = false;
        
        for (const char* layer : validation_layers_) {
            if (strcmp(requested, layer) == 0) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            validation_layers_.emplace_back(requested);
        }
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_disabled_validation_layer(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
        
        for (std::size_t i = 0u; i < validation_layers_.size(); ++i) {
            const char* layer = validation_layers_[i];
            
            if (strcmp(requested, layer) == 0) {
                // Found validation layer.
                std::swap(validation_layers_[i], validation_layers_[validation_layers_.size() - 1u]); // Swap idiom.
                validation_layers_.pop_back();
                
                // Validation layer is guaranteed to be present in the list only once.
                break;
            }
        }
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_headless_mode(bool headless) {
        if (headless) {
            #if defined VKS_PLATFORM_WINDOWS
            with_disabled_extension("VK_KHR_win32_surface");
            #elif defined VKS_PLATFORM_LINUX
            with_disabled_extension("VK_KHR_xcb_surface");
                // with_disabled_extension("VK_KHR_xlib_surface");
                // with_disabled_extension("VK_KHR_wayland_surface");
            #elif defined VKS_PLATFORM_ANDROID
                with_disabled_extension("VK_KHR_android_surface");
            #elif defined VKS_PLATFORM_APPLE
                with_disabled_extension("VK_EXT_metal_surface");
            #endif
            
        }
        else {
            #if defined VKS_PLATFORM_WINDOWS
            with_enabled_extension("VK_KHR_win32_surface");
            #elif defined VKS_PLATFORM_LINUX
            with_enabled_extension("VK_KHR_xcb_surface");
                // with_enabled_extension("VK_KHR_xlib_surface");
                // with_enabled_extension("VK_KHR_wayland_surface");
            #elif defined VKS_PLATFORM_ANDROID
                with_enabled_extension("VK_KHR_android_surface");
            #elif defined VKS_PLATFORM_APPLE
                with_enabled_extension("VK_EXT_metal_surface");
            #endif
        }
        
        with_enabled_extension("VK_KHR_surface");
        return *this;
    }
    
} // namespace vks