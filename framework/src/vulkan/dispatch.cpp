
#include "vulkan/dispatch.hpp"
#include "core/debug.hpp"
#include <vulkan/vulkan.h>
#include <stdexcept>

#if defined(VKS_PLATFORM_WINDOWS)
    #include <windows.h>
// #elif defined(VKS_PLATFORM_LINUX) || defined(VKS_PLATFORM_APPLE)
#else
    #include <dlfcn.h>
#endif

namespace vks {
    
    struct VulkanLoader {
        VulkanLoader();
        ~VulkanLoader();
    
        #if defined VKS_PLATFORM_WINDOWS
            HMODULE library;
        #else
            void* library;
        #endif
    
        void (VKAPI_PTR *fp_vk_get_instance_proc_addr)(VkInstance instance, const char* name);
    };
    
    VulkanLoader::VulkanLoader() {
        #if defined VKS_PLATFORM_WINDOWS
        
            const char* name = "vulkan-1.dll";
            library = LoadLibraryA(name);
            
            if (!library) {
                // TODO: FormatMessage error.
                throw VulkanException("ERROR: Unable to load Vulkan - '%s' not found.", name);
            }
            
            fp_vk_get_instance_proc_addr = reinterpret_cast<void(*)(VkInstance, const char*)>(GetProcAddress(library, "vkGetInstanceProcAddr"));
            
        #elif defined(VKS_PLATFORM_LINUX)
            
            const char* name = "libvulkan.so.1";
            library = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
            
            if (!library) {
                throw VulkanException("ERROR: Unable to load Vulkan - '%s' not found.", name);
            }
            
            fp_vk_get_instance_proc_addr = reinterpret_cast<void(*)(VkInstance, const char*)>(dlsym(library, "vkGetInstanceProcAddr"));
            
        #elif defined(VKS_PLATFORM_APPLE)
            
            const char* name = "libvulkan.dylib";
            library = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
            
            if (!library) {
                throw VulkanException("ERROR: Unable to load Vulkan - '%s' not found.", name);
            }
            
            fp_vk_get_instance_proc_addr = reinterpret_cast<void(*)(VkInstance, const char*)>(dlsym(library, "vkGetInstanceProcAddr"));
    
        #endif
    }
    
    VulkanLoader::~VulkanLoader() {
        #if defined(VKS_PLATFORM_WINDOWS)
            FreeLibrary(library);
        #else
            dlclose(library);
        #endif
    }
    
    void* load_command(const char* name) {
        static VulkanLoader loader;
        return loader.fp_vk_get_instance_proc_addr;
    }
    
}