
#include "vulkan/instance.hpp"
#include "vulkan/device.hpp"
#include "core/assert.hpp"
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <sstream>

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
    
//    VulkanDevice::Builder VulkanInstance::create_device() const {
//        Device::Builder builder { };
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
//        Device::Builder().with_selector([](const Device::Properties& device_properties) -> int {
//            int score = 0;
//
//
//
//            return score;
//        }).with_debug_name()
//    }
    
    VulkanInstance::Builder::Builder() : application_name_(""),
                                         application_version_({
                                             .major = 1u,
                                             .minor = 0u,
                                             .patch = 0u,
                                             .variant = 0u
                                         }),
                                         engine_name_("vulkan-samples"),
                                         engine_version_({
                                             .major = 1u,
                                             .minor = 0u,
                                             .patch = 0u,
                                             .variant = 0u
                                         }),
                                         required_api_version_({
                                             .major = 0u,
                                             .minor = 0u,
                                             .patch = 0u,
                                             .variant = 0u
                                         }),
                                         // Use Vulkan 1.0 API version by default.
                                         minimum_api_version_({
                                             .major = 1u,
                                             .minor = 0u,
                                             .patch = 0u,
                                             .variant = 0u
                                         }),
                                         headless_mode_(false),
                                         validation_features_(),
                                         requested_extensions_(),
                                         requested_layers_()
                                         {
        
        // Enable default presentation extensions.
        with_headless_mode(false);
        
        // Enable / disable default Vulkan validation features.
        with_enabled_validation_feature(GPU_VALIDATION | CORE_VALIDATION | THREAD_SAFETY | OBJECT_LIFETIMES | UNIQUE_HANDLES);
    }
    
    VulkanInstance::Builder::~Builder() {
    }

    VulkanInstance VulkanInstance::Builder::build() {
        VulkanInstance instance { };
        
        // Validate instance extensions.
        std::uint32_t available_extension_count = 0u;
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr);
    
        std::vector<VkExtensionProperties> available_extensions(available_extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, available_extensions.data());
    
        std::vector<const char*> unsupported_extensions { };
        unsupported_extensions.reserve(requested_extensions_.size()); // Maximum number of unsupported extensions.
    
        // Verify all requested extensions are available.
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
    
        if (!unsupported_extensions.empty()) {
            std::stringstream builder { };
            builder << "ERROR: Failed to create Vulkan instance - found " << unsupported_extensions.size() << " unsupported extension(s):\n";
            for (const char* extension : unsupported_extensions) {
                builder << "\t- " << extension << '\n';
            }
            throw std::runtime_error(builder.str());
        }
    
        instance.extensions = requested_extensions_;
        
        // Validate instance validation layers.
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
    
        if (!unsupported_layers.empty()) {
            std::stringstream builder { };
            builder << "ERROR: Failed to create Vulkan instance - found " << unsupported_layers.size() << " unsupported validation layer(s):\n";
            for (const char* layer : unsupported_layers) {
                builder << "\t- " << layer << '\n';
            }
            throw std::runtime_error(builder.str());
        }
        
        instance.layers = requested_layers_;
        
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
        
        // CORE_VALIDATION (enabled by default).
        if (!has_validation_feature(VulkanValidationFeature::CORE_VALIDATION)) {
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_CORE_CHECKS_EXT);
        }
    
        // GPU_VALIDATION controls a combination of shader and GPU validation features.
        if (has_validation_feature(VulkanValidationFeature::GPU_VALIDATION)) {
            enabled_validation_features.emplace_back(VkValidationFeatureEnableEXT::VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
            enabled_validation_features.emplace_back(VkValidationFeatureEnableEXT::VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT);
    
            // VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT and VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT are enabled by default.
        }
        else {
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT);
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT);
    
            // VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT and VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT are disabled by default.
        }
        
        // THREAD_SAFETY (enabled by default).
        if (!has_validation_feature(VulkanValidationFeature::THREAD_SAFETY)) {
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_THREAD_SAFETY_EXT);
        }
        
        // STATELESS_VALIDATION (enabled by default).
        if (!has_validation_feature(VulkanValidationFeature::STATELESS_VALIDATION)) {
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_API_PARAMETERS_EXT);
        }
    
        // OBJECT_LIFETIMES (enabled by default).
        if (!has_validation_feature(VulkanValidationFeature::OBJECT_LIFETIMES)) {
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_OBJECT_LIFETIMES_EXT);
        }
    
        // UNIQUE_HANDLES (enabled by default).
        if (!has_validation_feature(VulkanValidationFeature::UNIQUE_HANDLES)) {
            disabled_validation_features.emplace_back(VkValidationFeatureDisableEXT::VK_VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES_EXT);
        }
    
        // SYNCHRONIZATION (disabled by default).
        if (has_validation_feature(VulkanValidationFeature::SYNCHRONIZATION)) {
            enabled_validation_features.emplace_back(VkValidationFeatureEnableEXT::VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
        }
    
        // BEST_PRACTICES (disabled by default).
        if (has_validation_feature(VulkanValidationFeature::BEST_PRACTICES)) {
            enabled_validation_features.emplace_back(VkValidationFeatureEnableEXT::VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
        }
    
        // DEBUG_PRINTING (disabled by default).
        if (has_validation_feature(VulkanValidationFeature::DEBUG_PRINTING)) {
            enabled_validation_features.emplace_back(VkValidationFeatureEnableEXT::VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
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
        Version api_version { };
    
        std::uint32_t available_instance_version;
        vkEnumerateInstanceVersion(&available_instance_version);
        
        if (required_api_version_ > 0u) {
            // Framework prioritizes meeting the required API version.
            // Ensure required API version is supported.
            if (required_api_version_ > available_instance_version) {
                std::stringstream builder { };
                builder << "ERROR: Failed to create Vulkan instance - Vulkan API version ("
                        << required_api_version_.major << '.'
                        << required_api_version_.minor << '.'
                        << required_api_version_.patch << '.'
                        << required_api_version_.variant
                        << ") requested by 'with_required_api_version(...)' is unavailable.\n";
                throw std::runtime_error(builder.str());
            }
            
            api_version = required_api_version_;
        }
        else if (minimum_api_version_ > 0u) {
            // Minimum API version was set.
            // Ensure minimum API version is supported.
            if (minimum_api_version_ > available_instance_version) {
                std::stringstream builder { };
                builder << "ERROR: Failed to create Vulkan instance - Vulkan API version ("
                        << required_api_version_.major << '.'
                        << required_api_version_.minor << '.'
                        << required_api_version_.patch << '.'
                        << required_api_version_.variant
                        << ") requested by 'with_minimum_api_version(...)' is unavailable.\n";
                throw std::runtime_error(builder.str());
            }
            
            api_version = minimum_api_version_;
        }
        else {
            // Use supported instance version.
            api_version = {
                .major = VK_API_VERSION_MAJOR(available_instance_version),
                .minor = VK_API_VERSION_MINOR(available_instance_version),
                .patch = VK_API_VERSION_PATCH(available_instance_version),
                .variant = VK_API_VERSION_VARIANT(available_instance_version),
            };
            
            // TODO: Log message.
        }
        
        // Create Vulkan instance.
        VkApplicationInfo application_info {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = &validation_features,
            .pApplicationName = application_name_,
            .applicationVersion = application_version_,
            .pEngineName = engine_name_,
            .engineVersion = engine_version_,
            .apiVersion = api_version
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
            .patch = patch,
            .variant = 0u
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
            .patch = patch,
            .variant = 0u
        };
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_required_api_version(std::uint32_t version) {
        required_api_version_ = {
            .major = VK_API_VERSION_MAJOR(version),
            .minor = VK_API_VERSION_MINOR(version),
            .patch = VK_API_VERSION_PATCH(version),
            .variant = VK_API_VERSION_VARIANT(version),
        };
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_required_api_version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch, std::uint32_t variant) {
        required_api_version_ = {
            .major = major,
            .minor = minor,
            .patch = patch,
            .variant = variant
        };
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_minimum_api_version(std::uint32_t version) {
        minimum_api_version_ = {
            .major = VK_API_VERSION_MAJOR(version),
            .minor = VK_API_VERSION_MINOR(version),
            .patch = VK_API_VERSION_PATCH(version),
            .variant = VK_API_VERSION_VARIANT(version),
        };
    
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_minimum_api_version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch, std::uint32_t variant) {
        minimum_api_version_ = {
            .major = major,
            .minor = minor,
            .patch = patch,
            .variant = variant
        };
    
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_enabled_extension(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }

        // Ensure requested extension does not already exist.
        bool found = false;

        for (const char* extension : requested_extensions_) {
            if (strcmp(requested, extension) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            // Only add the first instance of the extension.
            requested_extensions_.emplace_back(requested);
        }

        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_disabled_extension(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
    
        for (std::size_t i = 0u; i < requested_extensions_.size(); ++i) {
            const char* extension = requested_extensions_[i];
            
            if (strcmp(requested, extension) == 0) {
                // Found extension.
                std::swap(requested_extensions_[i], requested_extensions_[requested_extensions_.size() - 1u]); // Swap idiom.
                requested_extensions_.pop_back();
                
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
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_disabled_validation_layer(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
    
        for (std::size_t i = 0u; i < requested_layers_.size(); ++i) {
            const char* layer = requested_layers_[i];
        
            if (strcmp(requested, layer) == 0) {
                // Found validation layer.
                std::swap(requested_layers_[i], requested_layers_[requested_layers_.size() - 1u]); // Swap idiom.
                requested_layers_.pop_back();
            
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
        
        // Enable extension for supporting validation features if not enabled already.
        // Includes all functionality provided in the (deprecated) 'VK_EXT_validation_flags' extension.
        with_enabled_extension(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    
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
        
        headless_mode_ = headless;
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