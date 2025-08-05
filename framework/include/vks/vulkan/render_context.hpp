
#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include "vks/scene.hpp"
#include "vks/window.hpp"
#include "vks/vulkan/pipeline_cache.hpp"
#include "vks/vulkan/render_graph.hpp"

namespace vks {
    
    // Forward declarations
    struct SampleRequirements;
    enum class FeatureFlags : std::uint32_t;

    class RenderContext {
        public:
            void initialize(const Window& window, const SampleRequirements& requirements);
            
            void begin_frame();
            void end_frame();
            
            PipelineCache pipeline_cache;
            RenderGraph render_graph;
            
        private:
            struct QueueFamilies {
                struct Family {
                    std::uint32_t index;
                    VkQueueFlags flags;
                };
                
                [[nodiscard]] int compute_score() const;
                
                Family graphics;
                Family compute;
                Family transfer;
            };
            
            void collect_instance_requirements(const SampleRequirements& requirements);
            
            void create_vulkan_instance();
            
            [[nodiscard]] bool validate_layer_support() const;
            [[nodiscard]] bool validate_instance_extension_support() const;
            
            void create_surface(const Window& window);
            
            void collect_device_requirements(const SampleRequirements& requirements);
            
            void select_physical_device();
            
            bool validate_device_extension_support(VkPhysicalDevice gpu) const;
            bool validate_feature_support(VkPhysicalDevice gpu) const;
            QueueFamilies select_queue_families(VkPhysicalDevice gpu) const;
            int compute_device_score(VkPhysicalDevice gpu) const;
            
            void create_device();
            
            void retrieve_queue_handles();
            
            VkInstance m_instance;
            VkDebugUtilsMessengerEXT m_debug_messenger;
            
            VkSurfaceKHR m_surface;
            
            std::vector<const char*> m_layers;
            std::vector<const char*> m_instance_extensions;
            
            VkPhysicalDevice m_gpu;
            FeatureFlags m_enabled_features;
            
            std::vector<const char*> m_device_extensions;
            QueueFamilies m_queue_families;
            
            VkDevice m_device;
    };

}

#endif // RENDER_CONTEXT_HPP
