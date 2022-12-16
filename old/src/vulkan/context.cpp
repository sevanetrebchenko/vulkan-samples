
#include "graphics/context.hpp"
#include "graphics/vulkan/context.hpp"

namespace vks {
    
    vks::Context::Builder::Builder() {
    
    }
    
    vks::Context::Builder::~Builder() {
    
    }
    
    std::shared_ptr <Context> vks::Context::Builder::build() {
        return nullptr;
    }
    
    Builder& vks::Context::Builder::with_application_name(const char* name) {
        return <#initializer#>;
    }
    
    Builder& vks::Context::Builder::with_application_version(u32 major, u32 minor, u32 patch) {
        return <#initializer#>;
    }
    
    Builder& vks::Context::Builder::with_engine_name(const char* name) {
        return <#initializer#>;
    }
    
    Builder& vks::Context::Builder::with_engine_version(u32 major, u32 minor, u32 patch) {
        return <#initializer#>;
    }
    
    Builder& vks::Context::Builder::with_target_runtime_version(u32 major, u32 minor, u32 patch) {
        return <#initializer#>;
    }
    
    Builder& vks::Context::Builder::with_enabled_extension(const char* extension) {
        return <#initializer#>;
    }
    
    Builder& vks::Context::Builder::with_disabled_extension(const char* extension) {
        return <#initializer#>;
    }
    
    Builder& vks::Context::Builder::with_enabled_validation_layer(const char* layer) {
        return <#initializer#>;
    }
    
    Builder& vks::Context::Builder::with_disabled_validation_layer(const char* layer) {
        return <#initializer#>;
    }
    
    Builder& vks::Context::Builder::with_headless_mode(bool headless) {
        return <#initializer#>;
    }
    

}