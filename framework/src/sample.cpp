
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
        
        m_render_context.initialize(m_window, {
        });
    }
    
    void Sample::run() {
    
    }
    
    void Sample::shutdown() {
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