
#include "vulkan/instance.hpp"
#include "vulkan/device.hpp"
#include "core/debug.hpp"
#include <cstring>
#include <cstdint>

#include <iostream>

namespace vks {
    
    struct VulkanInstance::InternalData final : public VulkanInstance {
        InternalData();
        ~InternalData() override;
        
        // Attempts to initialize VulkanInstance internals.
        NODISCARD bool build();
        
        // Verifies that all requested extensions are supported by the Vulkan implementation.
        NODISCARD bool verify_extension_support() const;
        
        // Verifies that all requested validation layers are supported by the Vulkan implementation.
        NODISCARD bool verify_validation_layer_support() const;
        
        // Verifies that the target Vulkan runtime version is supported on the system.
        NODISCARD bool verify_target_runtime_version() const;
        
        // vkDestroyInstance
        VkInstance m_handle;
        void (VKAPI_PTR* m_fp_vk_destroy_instance)(VkInstance, const VkAllocationCallbacks*);
        
        const char* m_application_name;
        Version m_application_version;
        
        const char* m_engine_name;
        Version m_engine_version;
        
        Version m_runtime_version;
        
        std::vector<const char*> m_extensions;
        std::vector<const char*> m_validation_layers;
    };
    
    VulkanInstance::InternalData::InternalData() : VulkanInstance(),
                                                   m_fp_vk_destroy_instance(nullptr),
                                                   m_handle(nullptr),
                                                   m_application_name("Vulkan Application"),
                                                   m_application_version({
                                                       .major = 1u,
                                                       .minor = 0u,
                                                       .patch = 0u
                                                   }),
                                                   m_engine_name("vulkan-samples"),
                                                   m_engine_version({
                                                       .major = 1u,
                                                       .minor = 0u,
                                                       .patch = 0u
                                                   }),
                                                   // Vulkan 1.0 by default...
                                                   m_runtime_version({
                                                       .major = 1u,
                                                       .minor = 0u,
                                                       .patch = 0u
                                                   }) {
    }
    
    VulkanInstance::InternalData::~InternalData() {
        if (m_fp_vk_destroy_instance) {
            m_fp_vk_destroy_instance(m_handle, nullptr);
        }
        m_handle = nullptr;
    }
    
    bool VulkanInstance::InternalData::verify_extension_support() const {
        static VkResult (VKAPI_PTR* fp_vk_enumerate_instance_extension_properties)(const char*, u32*, VkExtensionProperties*) = reinterpret_cast<typeof(fp_vk_enumerate_instance_extension_properties)>(detail::vk_get_instance_proc_addr(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties"));
        ASSERT(fp_vk_enumerate_instance_extension_properties != nullptr, "");
        
        // Determine total number of supported instance extensions.
        u32 available_extension_count = 0u;
        fp_vk_enumerate_instance_extension_properties(nullptr, &available_extension_count, nullptr);
        
        std::vector<VkExtensionProperties> available_extensions(available_extension_count);
        fp_vk_enumerate_instance_extension_properties(nullptr, &available_extension_count, available_extensions.data());
        
        // Include extensions provided by enabled validation layers.
        for (const char* layer : m_validation_layers) {
            u32 layer_extension_count = 0u;
            fp_vk_enumerate_instance_extension_properties(layer, &layer_extension_count, nullptr);
            
            std::vector<VkExtensionProperties> layer_extensions(layer_extension_count);
            fp_vk_enumerate_instance_extension_properties(layer, &layer_extension_count, layer_extensions.data());
            
            available_extension_count += layer_extension_count;
            available_extensions.insert(available_extensions.end(), layer_extensions.begin(), layer_extensions.end());
        }
        
        std::vector<const char*> unsupported_extensions;
        unsupported_extensions.reserve(m_extensions.size()); // Maximum number of unsupported extensions.
        
        // Verify all requested extensions are available.
        for (const char* requested : m_extensions) {
            bool found = false;
            
            for (u32 i = 0u; i < available_extension_count; ++i) {
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
        
        return unsupported_extensions.empty();
    }
    
    bool VulkanInstance::InternalData::verify_validation_layer_support() const {
        static VkResult (VKAPI_PTR* fp_vk_enumerate_instance_layer_properties)(u32*, VkLayerProperties*) = reinterpret_cast<typeof(fp_vk_enumerate_instance_layer_properties)>(detail::vk_get_instance_proc_addr(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties"));
        ASSERT(fp_vk_enumerate_instance_layer_properties != nullptr, "");
        
        u32 available_layer_count = 0u;
        fp_vk_enumerate_instance_layer_properties(&available_layer_count, nullptr);
        
        std::vector<VkLayerProperties> available_layers(available_layer_count);
        fp_vk_enumerate_instance_layer_properties(&available_layer_count, available_layers.data());
        
        std::vector<const char*> unsupported_layers;
        unsupported_layers.reserve(m_validation_layers.size()); // Maximum number of unsupported layers.
        
        // Verify that all requested validation layers are available.
        for (const char* requested : m_validation_layers) {
            bool found = false;
            
            for (u32 i = 0u; i < available_layer_count; ++i) {
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
        
        return unsupported_layers.empty();
    }
    
    bool VulkanInstance::InternalData::verify_target_runtime_version() const {
        static VkResult (VKAPI_PTR* fp_vk_enumerate_instance_version)(u32*) = reinterpret_cast<typeof(fp_vk_enumerate_instance_version)>(detail::vk_get_instance_proc_addr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
        
        // vkEnumerateInstanceVersion is provided by Vulkan 1.1 and higher - if it doesn't exist, targeted Vulkan version must be 1.0.
        u32 available_instance_version;
        if (fp_vk_enumerate_instance_version) {
            fp_vk_enumerate_instance_version(&available_instance_version);
        }
        else {
            available_instance_version = VK_API_VERSION_1_0;
        }
        
        return m_runtime_version <= available_instance_version;
    }
    
    bool VulkanInstance::InternalData::build() {
        if (!verify_validation_layer_support()) {
            return false;
        }
    
        if (!verify_extension_support()) {
            return false;
        }
    
        if (!verify_target_runtime_version()) {
            return false;
        }
    
        // Create Vulkan instance.
        VkApplicationInfo application_info {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = m_application_name,
            .applicationVersion = m_application_version,
            .pEngineName = m_engine_name,
            .engineVersion = m_engine_version,
            .apiVersion = m_runtime_version
        };
    
        VkInstanceCreateInfo instance_create_info {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .pApplicationInfo = &application_info,
            .enabledLayerCount = static_cast<u32>(m_validation_layers.size()),
            .ppEnabledLayerNames = m_validation_layers.data(),
            .enabledExtensionCount = static_cast<u32>(m_extensions.size()),
            .ppEnabledExtensionNames = m_extensions.data()
        };
    
        static VkResult (VKAPI_PTR* fp_vk_create_instance)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*) = reinterpret_cast<typeof(fp_vk_create_instance)>(detail::vk_get_instance_proc_addr(VK_NULL_HANDLE, "vkCreateInstance"));
        ASSERT(fp_vk_create_instance != nullptr, "");
    
        VkResult result = fp_vk_create_instance(&instance_create_info, nullptr, &m_handle);
        if (result != VK_SUCCESS) {
            return false;
        }
    
        // Initialize any straggler members not initialized in the constructor.
        m_fp_vk_destroy_instance = reinterpret_cast<typeof(m_fp_vk_destroy_instance)>(detail::vk_get_instance_proc_addr(m_handle, "vkDestroyInstance"));
        return true;
    }
    
    VulkanInstance::VulkanInstance() {
    }
    
    VulkanInstance::~VulkanInstance() {
    }
    
    VulkanInstance::operator VkInstance() const {
        std::shared_ptr<const VulkanInstance::InternalData> internal = std::reinterpret_pointer_cast<const VulkanInstance::InternalData>(shared_from_this());
        return internal->m_handle;
    }
    
    VulkanDevice::Builder VulkanInstance::create_device() {
        VulkanDevice::Builder builder(shared_from_this());
        return builder;
    }
    
    VulkanWindow::Builder VulkanInstance::create_window() {
        VulkanWindow::Builder builder(shared_from_this());
        return builder;
    }
    
    VulkanInstance::Builder::Builder() : m_instance(std::make_shared<VulkanInstance::InternalData>()) {
        with_headless_mode(false);
        with_enabled_validation_layer("VK_LAYER_KHRONOS_validation");
    }
    
    VulkanInstance::Builder::~Builder() = default;
    
    std::shared_ptr<VulkanInstance> VulkanInstance::Builder::build() const {
        std::shared_ptr<VulkanInstance::InternalData> internal = std::reinterpret_pointer_cast<VulkanInstance::InternalData>(m_instance);
    
        if (!internal->build()) {
            return nullptr;
        }
        
        return m_instance;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_application_name(const char* name) {
        if (strlen(name) > 0u) {
            std::shared_ptr<VulkanInstance::InternalData> internal = std::reinterpret_pointer_cast<VulkanInstance::InternalData>(m_instance);
            internal->m_application_name = name;
        }
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_application_version(u32 major, u32 minor, u32 patch) {
        std::shared_ptr<VulkanInstance::InternalData> internal = std::reinterpret_pointer_cast<VulkanInstance::InternalData>(m_instance);
    
        internal->m_application_version = {
            .major = major,
            .minor = minor,
            .patch = patch
        };
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_engine_name(const char* name) {
        if (strlen(name) > 0u) {
            std::shared_ptr<VulkanInstance::InternalData> internal = std::reinterpret_pointer_cast<VulkanInstance::InternalData>(m_instance);
            internal->m_engine_name = name;
        }
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_engine_version(u32 major, u32 minor, u32 patch) {
        std::shared_ptr<VulkanInstance::InternalData> internal = std::reinterpret_pointer_cast<VulkanInstance::InternalData>(m_instance);
    
        internal->m_engine_version = {
            .major = major,
            .minor = minor,
            .patch = patch
        };
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_target_runtime_version(u32 major, u32 minor, u32 patch) {
        std::shared_ptr<VulkanInstance::InternalData> internal = std::reinterpret_pointer_cast<VulkanInstance::InternalData>(m_instance);
    
        internal->m_runtime_version = {
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
    
        std::shared_ptr<VulkanInstance::InternalData> internal = std::reinterpret_pointer_cast<VulkanInstance::InternalData>(m_instance);
    
        // Ensure requested extension does not already exist.
        bool found = false;
        
        for (const char* extension : internal->m_extensions) {
            if (strcmp(requested, extension) == 0) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            // Only add the first instance of the extension.
            internal->m_extensions.emplace_back(requested);
        }
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_disabled_extension(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
    
        std::shared_ptr<VulkanInstance::InternalData> internal = std::reinterpret_pointer_cast<VulkanInstance::InternalData>(m_instance);
    
        for (std::size_t i = 0u; i < internal->m_extensions.size(); ++i) {
            const char* extension = internal->m_extensions[i];
            
            if (strcmp(requested, extension) == 0) {
                // Found extension.
                std::swap(internal->m_extensions[i], internal->m_extensions[internal->m_extensions.size() - 1u]); // Swap idiom.
                internal->m_extensions.pop_back();
                
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
    
        std::shared_ptr<VulkanInstance::InternalData> internal = std::reinterpret_pointer_cast<VulkanInstance::InternalData>(m_instance);
    
        // Ensure requested validation layer does not already exist.
        bool found = false;
        
        for (const char* layer : internal->m_validation_layers) {
            if (strcmp(requested, layer) == 0) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            internal->m_validation_layers.emplace_back(requested);
        }
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_disabled_validation_layer(const char* requested) {
        if (strlen(requested) == 0u) {
            return *this;
        }
        
        std::shared_ptr<VulkanInstance::InternalData> internal = std::reinterpret_pointer_cast<VulkanInstance::InternalData>(m_instance);
        
        for (std::size_t i = 0u; i < internal->m_validation_layers.size(); ++i) {
            const char* layer = internal->m_validation_layers[i];
            
            if (strcmp(requested, layer) == 0) {
                // Found validation layer.
                std::swap(internal->m_validation_layers[i], internal->m_validation_layers[internal->m_validation_layers.size() - 1u]); // Swap idiom.
                internal->m_validation_layers.pop_back();
                
                // Validation layer is guaranteed to be present in the list only once.
                break;
            }
        }
        
        return *this;
    }
    
    VulkanInstance::Builder& VulkanInstance::Builder::with_headless_mode(bool headless) {
        if (headless) {
            #if defined VKS_PLATFORM_WINDOWS
                with_disabled_extension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
            #elif defined VKS_PLATFORM_LINUX
                // with_disabled_extension("VK_KHR_xcb_surface");
                // with_disabled_extension("VK_KHR_xlib_surface");
                // with_disabled_extension("VK_KHR_wayland_surface");
            #elif defined VKS_PLATFORM_ANDROID
                // with_disabled_extension("VK_KHR_android_surface");
            #elif defined VKS_PLATFORM_APPLE
                // with_disabled_extension("VK_EXT_metal_surface");
            #endif
        }
        else {
            #if defined VKS_PLATFORM_WINDOWS
                with_enabled_extension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
            #elif defined VKS_PLATFORM_LINUX
                // with_enabled_extension("VK_KHR_xcb_surface");
                // with_enabled_extension("VK_KHR_xlib_surface");
                // with_enabled_extension("VK_KHR_wayland_surface");
            #elif defined VKS_PLATFORM_ANDROID
                // with_enabled_extension("VK_KHR_android_surface");
            #elif defined VKS_PLATFORM_APPLE
                // with_enabled_extension("VK_EXT_metal_surface");
            #endif
        }
        
        with_enabled_extension(VK_KHR_SURFACE_EXTENSION_NAME);
        return *this;
    }
    
} // namespace vks