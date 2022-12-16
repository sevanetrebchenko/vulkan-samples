
#include "graphics/context.hpp"

namespace vks {
    
    Context::~Context() {
    }
    
    Window::Builder Context::create_window() {
        return Window::Builder();
    }
    
    Device::Builder Context::create_device() {
        return Device::Builder();
    }
    
    Context::Context() {
    }
    
}