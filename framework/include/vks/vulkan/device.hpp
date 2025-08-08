
#ifndef DEVICE_HPP
#define DEVICE_HPP

#include "vks/core.hpp"
#include "vks/vulkan/queue.hpp"
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
            
            VkPhysicalDevice get_physical_device() const;
            VkDevice get_device() const;
            
            // Returns a non-owning reference to the underlying queue
            Queue get_graphics_queue() const;
            Queue get_compute_queue() const;
            Queue get_transfer_queue() const;
            
        private:
            struct QueueFamilySelection {
                std::uint32_t graphics_family_index;
                std::uint32_t compute_family_index;
                std::uint32_t transfer_family_index;
                
                VkQueueFlags graphics_family_flags;
                VkQueueFlags compute_family_flags;
                VkQueueFlags transfer_family_flags;
            };
            
            struct DeviceSelection {
                VkPhysicalDevice gpu;
                QueueFamilySelection queue_families;
            };
            
            void get_device_requirements(const SampleRequirements& requirements);
            
            // Returns the optimal queue family selection for the selected device
            DeviceSelection select_physical_device(VkInstance instance, VkSurfaceKHR surface);
            bool validate_extension_support(VkPhysicalDevice gpu) const;
            bool validate_feature_support(VkPhysicalDevice gpu) const;
            QueueFamilySelection select_queue_families(VkSurfaceKHR surface, VkPhysicalDevice gpu) const;
            
            std::uint32_t calculate_device_score(VkPhysicalDevice gpu) const;
            std::uint32_t calculate_queue_family_score(const QueueFamilySelection& queue_families) const;
            
            void create_device(const QueueFamilySelection& queue_families);
            void retrieve_device_queues(const QueueFamilySelection& queue_families);
            
            VkPhysicalDevice m_gpu;
            VkDevice m_device;
            
            std::vector<const char*> m_extensions;
            FeatureFlags m_enabled_features;

            Queue m_graphics_queue;
            Queue m_compute_queue;
            Queue m_transfer_queue;
    };
    
}

#endif // DEVICE_HPP
