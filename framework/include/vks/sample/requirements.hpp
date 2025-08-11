
#ifndef REQUIREMENTS_HPP
#define REQUIREMENTS_HPP

#include "vks/sample/features.hpp"
#include <cstdint> // std::uint32_t

namespace vks {
    
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
    
}

#endif // REQUIREMENTS_HPP
