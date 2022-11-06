
#include "vulkan/loader.hpp"
#include "core/debug.hpp"
#include <vulkan/vulkan.h>

#if defined(VKS_PLATFORM_WINDOWS)
    #include <windows.h>
// #elif defined(VKS_PLATFORM_LINUX) || defined(VKS_PLATFORM_APPLE)
#else
    #include <dlfcn.h>
#endif

namespace vks {
    namespace detail {
    
        static struct VulkanLoader {
            VulkanLoader();
            ~VulkanLoader();
        
            #if defined VKS_PLATFORM_WINDOWS
                HMODULE library;
            #else
                void* library;
            #endif
        
            void (VKAPI_PTR *(VKAPI_PTR *fp_vk_get_instance_proc_addr)(VkInstance, const char*))(void);
        } loader;
        
        VulkanLoader::VulkanLoader() {
            #if defined VKS_PLATFORM_WINDOWS
            
                const char* name = "vulkan-1.dll";
                library = LoadLibraryA(name);
                
                if (!library) {
                    // TODO: FormatMessage error.
                    throw VulkanException("ERROR: Unable to initialize Vulkan loader - '%s' not found.", name);
                }
    
                fp_vk_get_instance_proc_addr = reinterpret_cast<typeof(fp_vk_get_instance_proc_addr)>(GetProcAddress(library, "vkGetInstanceProcAddr"));
            
            #elif defined(VKS_PLATFORM_LINUX)
            
                const char* name = "libvulkan.so.1";
                library = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
                
                if (!library) {
                    throw VulkanException("ERROR: Unable to initialize Vulkan loader - '%s' not found.", name);
                }
                
                fp_vk_get_instance_proc_addr = reinterpret_cast<void(*)(VkInstance, const char*)>(dlsym(library, "vkGetInstanceProcAddr"));
                
            #elif defined(VKS_PLATFORM_APPLE)
                
                const char* name = "libvulkan.dylib";
                library = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
                
                if (!library) {
                    throw VulkanException("ERROR: Unable to initialize Vulkan loader - '%s' not found.", name);
                }
                
                fp_vk_get_instance_proc_addr = reinterpret_cast<void(*)(VkInstance, const char*)>(dlsym(library, "vkGetInstanceProcAddr"));
            
            #endif
            
            ASSERT(fp_vk_get_instance_proc_addr != nullptr, "ERROR: Failed to initialize Vulkan function loader.");
        }
        
        VulkanLoader::~VulkanLoader() {
            #if defined(VKS_PLATFORM_WINDOWS)
                FreeLibrary(library);
            #else
                dlclose(library);
            #endif
        }
    
        NODISCARD void_fp vk_get_instance_proc_addr(VkInstance instance, const char* name) {
            return loader.fp_vk_get_instance_proc_addr(instance, name);
        }
        
    }
}