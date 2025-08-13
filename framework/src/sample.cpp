
#include "vks/sample.hpp"
#include "vks/vulkan/device.hpp"

namespace vks {

    Sample::Sample(std::string name) : m_name(std::move(name)) {
    }
    
    Sample::~Sample() {
    }
    
    void Sample::initialize() {
        SampleRequirements requirements = get_requirements();
        
        if (requirements.display_mode != DisplayMode::None) {
            m_window.initialize(requirements.width, requirements.height, m_name.c_str());
        }
        
        m_render_context.initialize(m_window, requirements);
        
        // Load sample
        load(m_render_context);
    }
    
    void Sample::run() {
        float dt;
        
        while (m_window.active()) {
            m_window.poll();
            
            m_render_context.begin_frame();
                update(dt);
                render_frame(m_render_context);
            m_render_context.end_frame();
        }
    }
    
    void Sample::shutdown() {
        unload();
        m_render_context.shutdown();
        m_window.shutdown();
    }
    
    SampleRequirements Sample::get_requirements() {
        return { };
    }
    
    SampleRequirements::SampleRequirements() : width(640),
                                               height(480),
                                               display_mode(DisplayMode::Windowed),
                                               enabled_features(FeatureFlags::None) {
    }
    
}