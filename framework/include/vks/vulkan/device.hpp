
#ifndef DEVICE_HPP
#define DEVICE_HPP

#include "vks/core.hpp"
#include <vulkan/vulkan.h>
#include <vector> // std::vector

namespace vks {
    
    // Forward declarations
    struct SampleRequirements;
    enum class FeatureFlags : std::uint32_t;
    
    class Device final : public ManagedObject<Device> {
        public:
            Device(VkInstance instance, VkSurfaceKHR surface, const SampleRequirements& requirements);
            ~Device() override;
            
            VkSurfaceFormatKHR get_surface_format(VkSurfaceKHR surface) const;
            
            VkQueue get_graphics_queue() const;
            VkQueue get_compute_queue() const;
            VkQueue get_transfer_queue() const;
            
        private:
            struct DeviceQueues {
                [[nodiscard]] std::uint32_t calculate_score() const;
                [[nodiscard]] std::vector<VkDeviceQueueCreateInfo> to_create_infos() const;
                void retrieve_queue_handles(VkDevice device);
                
                std::uint32_t graphics_family_index;
                std::uint32_t compute_family_index;
                std::uint32_t transfer_family_index;
                
                VkQueueFlags graphics_family_flags;
                VkQueueFlags compute_family_flags;
                VkQueueFlags transfer_family_flags;
                
                VkQueue graphics;
                VkQueue compute;
                VkQueue transfer;
            };
            
            void collect_requirements(const SampleRequirements& requirements);
            
            void select_physical_device(VkInstance instance, VkSurfaceKHR surface);
            
            bool validate_extension_support(VkPhysicalDevice gpu) const;
            bool validate_feature_support(VkPhysicalDevice gpu) const;
            DeviceQueues select_queue_families(VkSurfaceKHR surface, VkPhysicalDevice gpu) const;
            std::uint32_t calculate_device_score(VkPhysicalDevice gpu, const DeviceQueues& queues) const;
            
            void create_device();

            VkPhysicalDevice m_gpu;
            std::vector<const char*> m_extensions;
            FeatureFlags m_enabled_features;
            DeviceQueues m_queues;
            
            VkDevice m_device;
    };
    
}

#endif // DEVICE_HPP
