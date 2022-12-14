
#pragma once

#include "core/defines.hpp"

namespace vks {
    
    struct Version {
        // Convert this to a single version number.
        operator std::uint32_t() const;
        
        std::uint32_t major;
        std::uint32_t minor;
        std::uint32_t patch;
    };
    
}