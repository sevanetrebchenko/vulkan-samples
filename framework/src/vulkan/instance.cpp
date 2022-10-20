
#include "vulkan/instance.hpp"
#include "vulkan/device.hpp"
#include "core/debug.hpp"
#include <cstring>
#include <cstdint>

#include "vulkan/dispatch.hpp"

namespace vks {
    
    VulkanInstance::VulkanInstance(VkInstance handle) : handle_(handle) {
        // Instance-level functions need to be loaded with a valid handle.
        detail::fp_vk_get_instance_proc_addr vk_get_instance_proc_addr = detail::get_vulkan_symbol_loader();
        dispatch_ = DispatchTable {
            .fp_vk_destroy_instance = reinterpret_cast<typeof(DispatchTable::fp_vk_destroy_instance)>(vk_get_instance_proc_addr(handle, "vkDestroyInstance"))
        };
    }
    
    VulkanInstance::~VulkanInstance() {
        dispatch_.fp_vk_destroy_instance(handle_, nullptr);
    }

    VulkanInstance::VulkanInstance(VulkanInstance&& other) noexcept {
        if (handle_) {
            // 'this' is already initialized.
            dispatch_.fp_vk_destroy_instance(handle_, nullptr);
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
            dispatch_.fp_vk_destroy_instance(handle_, nullptr);
        }
        handle_ = other.handle_;
        other.handle_ = nullptr;
        
        return *this;
    }
    
    VulkanInstance::operator VkInstance() const {
        return handle_;
    }
    
    VulkanDevice::Builder VulkanInstance::create_device() const {
//        VulkanDevice::Builder builder { };
//
//        builder.instance_ = handle;
//
//        // Initialize physical device.
//        unsigned physical_device_count = 0u;
//        vkEnumeratePhysicalDevices(nullptr, &physical_device_count, nullptr);
//
//        std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
//        vkEnumeratePhysicalDevices(nullptr, &physical_device_count, physical_devices.data());
//
//        builder.with_selector([](const Device::Properties& device_properties) -> int {
//            int score = 0;
//
//
//
//            return score;
//        }).with_debug_name()
    }
    
    
    // https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-part-1.html
    
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
        detail::fp_vk_get_instance_proc_addr vk_get_instance_proc_addr = detail::get_vulkan_symbol_loader();
        dispatch_ = DispatchTable {
            .fp_vk_create_instance = reinterpret_cast<typeof(DispatchTable::fp_vk_create_instance)>(vk_get_instance_proc_addr(VK_NULL_HANDLE, "vkCreateInstance")),
            .fp_vk_enumerate_instance_version = reinterpret_cast<typeof(DispatchTable::fp_vk_enumerate_instance_version)>(vk_get_instance_proc_addr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion")),
            .fp_vk_enumerate_instance_extension_properties = reinterpret_cast<typeof(DispatchTable::fp_vk_enumerate_instance_extension_properties)>(vk_get_instance_proc_addr(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties")),
            .fp_vk_enumerate_instance_layer_properties = reinterpret_cast<typeof(DispatchTable::fp_vk_enumerate_instance_layer_properties)>(vk_get_instance_proc_addr(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties"))
        };
    
        // TODO: messages
        ASSERT(dispatch_.fp_vk_create_instance != nullptr, "");
        ASSERT(dispatch_.fp_vk_enumerate_instance_extension_properties != nullptr, "");
        ASSERT(dispatch_.fp_vk_enumerate_instance_layer_properties != nullptr, "");
        
        // Enable default presentation extensions.
        with_headless_mode(false);
        
        // Enable / disable default Vulkan validation features.
        with_enabled_validation_layer("VK_LAYER_KHRONOS_validation");
        with_enabled_validation_feature(GPU_VALIDATION | CORE_VALIDATION | THREAD_SAFETY | OBJECT_LIFETIMES | UNIQUE_HANDLES);
    }
    
    VulkanInstance::Builder::~Builder() {
    }

    VulkanInstance VulkanInstance::Builder::build() {
        // Validate requested validation layers first (instance extensions may depend on extensions provided by validation layers).
        std::uint32_t available_layer_count = 0u;
        dispatch_.fp_vk_enumerate_instance_layer_properties(&available_layer_count, nullptr);
    
        std::vector<VkLayerProperties> available_layers(available_layer_count);
        dispatch_.fp_vk_enumerate_instance_layer_properties(&available_layer_count, available_layers.data());
    
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
        dispatch_.fp_vk_enumerate_instance_extension_properties(nullptr, &available_extension_count, nullptr);
    
        std::vector<VkExtensionProperties> available_extensions(available_extension_count);
        dispatch_.fp_vk_enumerate_instance_extension_properties(nullptr, &available_extension_count, available_extensions.data());
    
        // Include instance extensions provided by enabled validation layers.
        for (const char* layer : validation_layers_) {
            std::uint32_t layer_extension_count = 0u;
            dispatch_.fp_vk_enumerate_instance_extension_properties(layer, &layer_extension_count, nullptr);
            
            std::vector<VkExtensionProperties> layer_extensions(layer_extension_count);
            dispatch_.fp_vk_enumerate_instance_extension_properties(layer, &layer_extension_count, layer_extensions.data());
    
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
    
        // Configure enabled / disabled instance validation features.
        
        // If CORE_VALIDATION is disabled, GPU_VALIDATION must also be disabled.
        if (!has_validation_feature(VulkanValidationFeature::CORE_VALIDATION)) {
            remove_validation_feature(VulkanValidationFeature::GPU_VALIDATION);
        }
        
        // GPU_VALIDATION cannot be enabled alongside DEBUG_PRINTING. Builder respects the most recently enabled validation feature.
        for (auto iter = validation_features_.rbegin(); iter != validation_features_.rend(); ++iter) {
            if (*iter == VulkanValidationFeature::GPU_VALIDATION) {
                // Found GPU_VALIDATION first, discard DEBUG_PRINTING (if applicable).
                remove_validation_feature(DEBUG_PRINTING);
                break;
            }
    
            if (*iter == VulkanValidationFeature::DEBUG_PRINTING) {
                // Found DEBUG_PRINTING, discard GPU_VALIDATION (if applicable).
                remove_validation_feature(VulkanValidationFeature::GPU_VALIDATION);
                break;
            }
        }
    
        std::vector<VkValidationFeatureEnableEXT> enabled_validation_features;
        std::vector<VkValidationFeatureDisableEXT> disabled_validation_features;
        
        // Features below are enabled by default - if feature is present in the requested validation feature list, disable it.
        if (!has_validation_feature(VulkanValidationFeature::CORE_VALIDATION)) {
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_CORE_CHECKS_EXT);
        }
    
        if (!has_validation_feature(VulkanValidationFeature::THREAD_SAFETY)) {
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_THREAD_SAFETY_EXT);
        }
    
        if (!has_validation_feature(VulkanValidationFeature::STATELESS_VALIDATION)) {
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_API_PARAMETERS_EXT);
        }
    
        if (!has_validation_feature(VulkanValidationFeature::OBJECT_LIFETIMES)) {
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_OBJECT_LIFETIMES_EXT);
        }
    
        if (!has_validation_feature(VulkanValidationFeature::UNIQUE_HANDLES)) {
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES_EXT);
        }
        
        // Features below are disabled by default - if feature is present in the requested validation feature list, enable it.
        if (has_validation_feature(VulkanValidationFeature::SYNCHRONIZATION)) {
            enabled_validation_features.emplace_back(VkValidationFeatureEnableEXT::VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
        }
    
        if (has_validation_feature(VulkanValidationFeature::BEST_PRACTICES)) {
            enabled_validation_features.emplace_back(VkValidationFeatureEnableEXT::VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
        }
    
        if (has_validation_feature(VulkanValidationFeature::DEBUG_PRINTING)) {
            enabled_validation_features.emplace_back(VkValidationFeatureEnableEXT::VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
        }
        
        // GPU_VALIDATION controls a combination of shader and GPU validation features.
        if (has_validation_feature(VulkanValidationFeature::GPU_VALIDATION)) {
            enabled_validation_features.emplace_back(VkValidationFeatureEnableEXT::VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
            enabled_validation_features.emplace_back(VkValidationFeatureEnableEXT::VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT);
    
            // Enabled by default (no action necessary):
            //      - VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT
            //      - VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT
        }
        else {
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT);
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT);
    
            // Disabled by default (no action necessary):
            //      - VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT
            //      - VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT
        }
    
        VkValidationFeaturesEXT validation_features {
            .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
            .pNext = nullptr,
            .enabledValidationFeatureCount = static_cast<std::uint32_t>(enabled_validation_features.size()),
            .pEnabledValidationFeatures = enabled_validation_features.data(),
            .disabledValidationFeatureCount = static_cast<std::uint32_t>(disabled_validation_features.size()),
            .pDisabledValidationFeatures = disabled_validation_features.data()
        };
        
        // Determine application version.
        std::uint32_t available_instance_version;
        dispatch_.fp_vk_enumerate_instance_version(&available_instance_version);
        
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
            .pNext = &validation_features,
            .pApplicationInfo = &application_info,
            .enabledLayerCount = static_cast<std::uint32_t>(validation_layers_.size()),
            .ppEnabledLayerNames = validation_layers_.data(),
            .enabledExtensionCount = static_cast<std::uint32_t>(extensions_.size()),
            .ppEnabledExtensionNames = extensions_.data()
        };
    
        VkInstance handle = nullptr;
        VkResult result = dispatch_.fp_vk_create_instance(&instance_create_info, nullptr, &handle);
        if (result != VK_SUCCESS) {
            throw VulkanException("ERROR: Failed to create Vulkan instance - vkCreateInstance exited with code %i.", result);
        }

        return VulkanInstance { handle };
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
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_enabled_validation_feature(VulkanValidationFeature feature) {
        if (feature == 0u) {
            // No requested validation features.
            return *this;
        }
        
        // Enable extensions for supporting validation features.
        with_enabled_validation_layer("VK_LAYER_KHRONOS_validation");
        with_enabled_extension("VK_EXT_validation_features");
    
        // Note: 'feature' is a bit field and may have several requested validation features to enable.
        if (feature & VulkanValidationFeature::CORE_VALIDATION) {
            add_validation_feature(VulkanValidationFeature::CORE_VALIDATION);
        }
        
        if (feature & VulkanValidationFeature::GPU_VALIDATION) {
            add_validation_feature(VulkanValidationFeature::GPU_VALIDATION);
        }
    
        if (feature & VulkanValidationFeature::THREAD_SAFETY) {
            add_validation_feature(VulkanValidationFeature::THREAD_SAFETY);
        }
    
        if (feature & VulkanValidationFeature::STATELESS_VALIDATION) {
            add_validation_feature(VulkanValidationFeature::STATELESS_VALIDATION);
        }
    
        if (feature & VulkanValidationFeature::OBJECT_LIFETIMES) {
            add_validation_feature(VulkanValidationFeature::OBJECT_LIFETIMES);
        }
    
        if (feature & VulkanValidationFeature::UNIQUE_HANDLES) {
            add_validation_feature(VulkanValidationFeature::UNIQUE_HANDLES);
        }
    
        if (feature & VulkanValidationFeature::SYNCHRONIZATION) {
            add_validation_feature(VulkanValidationFeature::SYNCHRONIZATION);
        }
    
        if (feature & VulkanValidationFeature::BEST_PRACTICES) {
            add_validation_feature(VulkanValidationFeature::BEST_PRACTICES);
        }
    
        if (feature & VulkanValidationFeature::DEBUG_PRINTING) {
            add_validation_feature(VulkanValidationFeature::DEBUG_PRINTING);
        }
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_disabled_validation_feature(VulkanValidationFeature feature) {
        if (!feature) {
            return *this;
        }
    
        // Enable extension for supporting validation features if not enabled already.
        // Includes all functionality provided in the (deprecated) 'VK_EXT_validation_flags' extension.
        with_enabled_extension(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    
        // Note: 'feature' is a bit field and may have several validation features requested.
        if (feature & VulkanValidationFeature::CORE_VALIDATION) {
            remove_validation_feature(VulkanValidationFeature::CORE_VALIDATION);
        }
    
        if (feature & VulkanValidationFeature::GPU_VALIDATION) {
            remove_validation_feature(VulkanValidationFeature::GPU_VALIDATION);
        }
    
        if (feature & VulkanValidationFeature::THREAD_SAFETY) {
            remove_validation_feature(VulkanValidationFeature::THREAD_SAFETY);
        }
    
        if (feature & VulkanValidationFeature::STATELESS_VALIDATION) {
            remove_validation_feature(VulkanValidationFeature::STATELESS_VALIDATION);
        }
    
        if (feature & VulkanValidationFeature::OBJECT_LIFETIMES) {
            remove_validation_feature(VulkanValidationFeature::OBJECT_LIFETIMES);
        }
    
        if (feature & VulkanValidationFeature::UNIQUE_HANDLES) {
            remove_validation_feature(VulkanValidationFeature::UNIQUE_HANDLES);
        }
    
        if (feature & VulkanValidationFeature::SYNCHRONIZATION) {
            remove_validation_feature(VulkanValidationFeature::SYNCHRONIZATION);
        }
    
        if (feature & VulkanValidationFeature::BEST_PRACTICES) {
            remove_validation_feature(VulkanValidationFeature::BEST_PRACTICES);
        }
    
        if (feature & VulkanValidationFeature::DEBUG_PRINTING) {
            remove_validation_feature(VulkanValidationFeature::DEBUG_PRINTING);
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
        
        return *this;
    }
    
    void VulkanInstance::Builder::add_validation_feature(VulkanValidationFeature requested) {
        ASSERT(requested && !(requested & (requested - 1u)), "VulkanValidationFeature provided to 'enable_validation_feature' must be singular.");
        if (!has_validation_feature(requested)) {
            validation_features_.emplace_back(requested);
        }
    }
    
    bool VulkanInstance::Builder::has_validation_feature(VulkanValidationFeature requested) const {
        bool found = false;
        
        for (VulkanValidationFeature feature : validation_features_) {
            if (feature == requested) {
                found = true;
                break;
            }
        }
        
        return found;
    }
    
    void VulkanInstance::Builder::remove_validation_feature(VulkanValidationFeature requested) {
        ASSERT(requested && !(requested & (requested - 1u)), "VulkanValidationFeature provided to 'disable_validation_feature' must be singular.");
        
        // Note: cannot use swap paradigm as the validation feature insert order matters.
        for (auto iter = validation_features_.begin(); iter != validation_features_.end(); ++iter) {
            if (*iter == requested) {
                validation_features_.erase(iter);
                break;
            }
        }
    }
    
    VulkanInstance::Builder::VulkanValidationFeature operator|(VulkanInstance::Builder::VulkanValidationFeature first, VulkanInstance::Builder::VulkanValidationFeature second) {
        using T = std::underlying_type<VulkanInstance::Builder::VulkanValidationFeature>::type;
        return static_cast<VulkanInstance::Builder::VulkanValidationFeature>(static_cast<T>(first) | static_cast<T>(second));
    }
    
    VulkanInstance::Builder::VulkanValidationFeature operator&(VulkanInstance::Builder::VulkanValidationFeature first, VulkanInstance::Builder::VulkanValidationFeature second) {
        using T = std::underlying_type<VulkanInstance::Builder::VulkanValidationFeature>::type;
        return static_cast<VulkanInstance::Builder::VulkanValidationFeature>(static_cast<T>(first) & static_cast<T>(second));
    }
    
} // namespace vks