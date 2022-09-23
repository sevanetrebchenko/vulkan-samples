
#include "core/sample.hpp"
#include <vulkan/vulkan.h>
#include <iostream>

namespace vks {

    Sample::Sample() : instance_(VulkanInstance::Builder().with_extension(VK_KHR_SURFACE_EXTENSION_NAME)
                                                      #if defined VKS_PLATFORM_WINDOWS
                                                          .with_extension("VK_KHR_win32_surface")
                                                      #elif defined VKS_PLATFORM_LINUX
                                                          .with_extension("VK_KHR_xcb_surface");
                                                      #elif defined VKS_PLATFORM_APPLE
                                                          .with_extension("VK_EXT_metal_surface");
                                                      #endif
                                                      #if !defined NDEBUG
                                                          .with_layer("VK_LAYER_KHRONOS_validation")
                                                      #endif
                                                          .build())
                                                          {
        // TODO: sample configuration?
    }

    Sample::~Sample() {
    }

} // namespace vks