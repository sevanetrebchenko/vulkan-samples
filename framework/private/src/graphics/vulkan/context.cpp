
#include "graphics/vulkan/context.hpp"

namespace vks {
    
    // VulkanContext function definitions.
    VulkanContext::VulkanContext() : Context() {
    }
    
    VulkanContext::~VulkanContext() {
    }
    
    // Public-facing function definitions.
    
    Context::Builder::Builder() : m_context(std::make_shared<VulkanContext>()) {
    }
    
    Context::Builder::~Builder() {
    }
    
    std::shared_ptr <Context> Context::Builder::build() {
        return nullptr;
    }
    
    Context::Builder& Context::Builder::with_application_name(const char* name) {
    }
    
    Context::Builder& Context::Builder::with_application_version(u32 major, u32 minor, u32 patch) {
    }
    
    Context::Builder& Context::Builder::with_engine_name(const char* name) {
    }
    
    Context::Builder& Context::Builder::with_engine_version(u32 major, u32 minor, u32 patch) {
    }
    
    Context::Builder& Context::Builder::with_target_runtime_version(u32 major, u32 minor, u32 patch) {
    }
    
    Context::Builder& Context::Builder::with_enabled_extension(const char* extension) {
    }
    
    Context::Builder& Context::Builder::with_disabled_extension(const char* extension) {
    }
    
    Context::Builder& Context::Builder::with_enabled_validation_layer(const char* layer) {
    }
    
    Context::Builder& Context::Builder::with_disabled_validation_layer(const char* layer) {
    }
    
    Context::Builder& Context::Builder::with_headless_mode(bool headless) {
    }
    
}