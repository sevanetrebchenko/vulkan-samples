
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "vks/vulkan/device.hpp"

namespace vks {
    
    struct Renderer {
        std::shared_ptr<Device> device;
    };
    
}

#endif // RENDERER_HPP
