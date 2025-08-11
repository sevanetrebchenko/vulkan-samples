
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
            void collect_requirements(const SampleRequirements& requirements);
            
            void create_vulkan_instance();
            
            [[nodiscard]] bool validate_layer_support() const;
            [[nodiscard]] bool validate_extension_support() const;
            
            void create_surface(const Window& window);
            
            VkInstance m_instance;
            VkDebugUtilsMessengerEXT m_debug_messenger;
            
            VkSurfaceKHR m_surface;
            
            std::vector<const char*> m_layers;
            std::vector<const char*> m_extensions;
            
            DeviceHandle m_device;
    };

}

#endif // RENDER_CONTEXT_HPP
