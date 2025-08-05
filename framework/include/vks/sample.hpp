
#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#include "vks/window.hpp"
#include "vks/vulkan/render_context.hpp"
#include <utils/enum.hpp>
#include <string> // std::string

namespace vks {
    
    enum class FeatureFlags : std::uint32_t {
        None = 0,
        Raytracing = 1 << 0,
        MeshShaders = 1 << 1,
        VariableRateShading = 1 << 2,
        GeometryShaders = 1 << 3,
        TesselationShaders = 1 << 4,
    };
    
    DEFINE_ENUM_BITFIELD_OPERATIONS(FeatureFlags);
    
    enum class DisplayMode {
        None = 0,  // Headless
        Windowed = 1 << 0,
        Fullscreen = 1 << 1,
    };
    
    struct SampleRequirements {
        SampleRequirements();
        
        SampleRequirements& set_width(std::uint32_t width);
        SampleRequirements& set_height(std::uint32_t height);
        SampleRequirements& set_extent(std::uint32_t width, std::uint32_t height);
        
        SampleRequirements& set_display_mode(DisplayMode mode);
        
        SampleRequirements& enable_feature(FeatureFlags flags);
        SampleRequirements& enable_features(FeatureFlags flags);
        
        std::uint32_t width;
        std::uint32_t height;
        DisplayMode display_mode;
        FeatureFlags enabled_features;
    };
    
    class Sample {
        public:
            Sample(std::string name);
            virtual ~Sample();
            
            void initialize();
            void run();
            void shutdown();
            
        protected:
        
        
        private:
            virtual SampleRequirements get_requirements() = 0;
            
            virtual void load() = 0;
            virtual void update(float dt) = 0;
            virtual void render_frame(RenderContext& context) = 0;
            virtual void unload() = 0;
            
            std::string m_name;
            Window m_window;
            RenderContext m_render_context;
    };
    
}

#endif // SAMPLE_HPP
