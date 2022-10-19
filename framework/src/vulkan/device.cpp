//
//#include "vulkan/device.hpp"
//#include "vulkan/instance.hpp"
//#include <cstring>
//#include <stdexcept>
//
//namespace vks {
//
//    VulkanDevice VulkanDevice::Builder::build() const {
//        // Enumerate all available physical devices.
//        std::uint32_t physical_device_count = 0u;
//        vkEnumeratePhysicalDevices(instance_->handle, &physical_device_count, nullptr);
//
//        if (physical_device_count == 0u) {
//            // Detected no physical devices with Vulkan support.
//            throw std::runtime_error("ERROR: Failed to create Vulkan device - detected no physical devices with Vulkan support.");
//        }
//
//        std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
//        vkEnumeratePhysicalDevices(instance_->handle, &physical_device_count, physical_devices.data());
//
//        // Determine physical device suitability.
//        for (VkPhysicalDevice handle : physical_devices) {
//            VkPhysicalDeviceProperties properties { };
//            vkGetPhysicalDeviceProperties(handle, &properties);
//
////            properties.
////
////            VkPhysicalDeviceFeatures features { };
////            vkGetPhysicalDeviceFeatures(handle, &features);
//        }
//    }
//
//    std::vector<const char*> VulkanDevice::Builder::validate_extensions(VkPhysicalDevice handle) const {
//        // Enumerate all available extensions.
//        std::uint32_t available_extension_count = 0u;
//        vkEnumerateDeviceExtensionProperties(handle, nullptr, &available_extension_count, nullptr);
//
//        std::vector<VkExtensionProperties> available_extensions(available_extension_count);
//        vkEnumerateDeviceExtensionProperties(handle, nullptr, &available_extension_count, available_extensions.data());
//
//        std::vector<const char*> unsupported_extensions { };
//        unsupported_extensions.reserve(extensions_.size()); // Maximum number of unsupported extensions.
//
//        for (const char* requested : extensions_) {
//            bool found = false;
//
//            for (std::uint32_t i = 0u; i < available_extension_count; ++i) {
//                const char* available = available_extensions[i].extensionName;
//
//                if (strcmp(requested, available) == 0) {
//                    found = true;
//                    break;
//                }
//            }
//
//            if (!found) {
//                unsupported_extensions.emplace_back(requested);
//            }
//        }
//
//        return unsupported_extensions;
//    }
//
//}