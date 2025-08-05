
#include "vks/sample.hpp"

namespace vks {

    Sample::Sample(std::string name) {
    
    }
    
    Sample::~Sample() {
    
    }
    
    void Sample::initialize() {
        SampleRequirements requirements = get_requirements();
        
        if (requirements.display_mode != DisplayMode::None) {
            m_window.initialize(requirements.width, requirements.height, m_name.c_str());
        }
        
        m_render_context.initialize(m_window, requirements);
    }
    
    void Sample::run() {
    
    }
    
    void Sample::shutdown() {
        m_window.shutdown();
    }
    
    SampleRequirements::SampleRequirements() : width(640),
                                               height(480),
                                               display_mode(DisplayMode::Windowed),
                                               enabled_features(FeatureFlags::None) {
    }
}