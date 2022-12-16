
#pragma once

#include <memory>

#include "core/defines.hpp"
#include "graphics/device.hpp"
#include "graphics/window.hpp"

namespace vks {

    class Context : public ManagedObject<Context> {
        public:
            class Builder;
            ~Context() override;
            
            NODISCARD Window::Builder create_window();
            NODISCARD Device::Builder create_device();
            
        protected:
            // Graphics context can only be created using Context::Builder.
            Context();
    };
    
    class Context::Builder {
        public:
            Builder();
            ~Builder();
        
            // Returns nullptr on failure.
            NODISCARD std::shared_ptr<Context> build();
        
            Builder& with_application_name(const char* name);
            Builder& with_application_version(u32 major, u32 minor, u32 patch = 0u);
        
            Builder& with_engine_name(const char* name);
            Builder& with_engine_version(u32 major, u32 minor, u32 patch = 0u);
        
            // Specify the graphics API version that the application source code expects to target.
            Builder& with_target_runtime_version(u32 major, u32 minor, u32 patch = 0u);
        
            Builder& with_enabled_extension(const char* extension);
            Builder& with_disabled_extension(const char* extension);
        
            Builder& with_enabled_validation_layer(const char* layer);
            Builder& with_disabled_validation_layer(const char* layer);
        
            // Headless mode does not load extensions required for presentation.
            Builder& with_headless_mode(bool headless = true);
            
        private:
            std::shared_ptr<Context> m_context;
    };

}
