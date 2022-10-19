
#include "core/sample.hpp"
#include <vulkan/vulkan.h>

#include <windows.h>



#include <iostream>

namespace vks {

    Sample::Sample() : instance_(VulkanInstance::Builder().build()) {
        
        auto handle = LoadLibrary(TEXT("vulkan-1.dll"));
        if (handle) {
            std::cout << "asdf" << std::endl;
        }
        
        instance_ = VulkanInstance::Builder()
                .with_target_api_version(1, 3)
                .with_enabled_extension("")
                .with_application_name("")
                .build();
        
//        VulkanDevice device = instance_.create_device()
//        .using_selector([](const VulkanDevice::Properties& properties) -> int {
//            int score = 0;
//
//            switch (properties.type) {
//                case VulkanDevice::Type::INTEGRATED_GPU:
//                    score += 100;
//                    break;
//                case VulkanDevice::Type::DISCRETE_GPU:
//                    score += 1000;
//                    break;
//                case VulkanDevice::Type::VIRTUAL_GPU:
//                    score += 10;
//                    break;
//                case VulkanDevice::Type::CPU:
//                default:
//                    break;
//            }
//
//            return score;
//        })
//        .with_debug_name("My Vulkan Device")
//        .with_extension("ADSF")
//        .with_features({
//            .
//        })
//        .build();
    }

    Sample::~Sample() {
    }

} // namespace vks