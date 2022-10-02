
#include "core/sample.hpp"
#include <vulkan/vulkan.h>
#include <iostream>

namespace vks {

    Sample::Sample() : instance_(VulkanInstance::Builder().build()) {
        
        instance_ = VulkanInstance::Builder()
                .with_enabled_extension("asdf")
                .with_enabled_extension("")
                .with_application_name("")
                .with_minimum_api_version(1, 0, 0)
                .with_headless_mode()
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