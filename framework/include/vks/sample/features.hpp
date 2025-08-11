
#ifndef FEATURES_HPP
#define FEATURES_HPP

#include <utils/enum.hpp>
#include <cstdint> // std::uint32_t

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
    
}

#endif // FEATURES_HPP
