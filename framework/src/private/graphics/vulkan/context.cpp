
#include "private/graphics/vulkan/context.hpp"

namespace vks {
    
    // VulkanContext function definitions.
    VulkanContext::VulkanContext() : Context(),
                                     m_fp_vk_destroy_instance(nullptr), // Instance-level function.
                                     m_handle(nullptr),
                                     m_application_name("Vulkan Application"),
                                     m_application_version({
                                         .major = 1u,
                                         .minor = 0u,
                                         .patch = 0u
                                     }),
                                     m_engine_name("vulkan-samples"),
                                     m_engine_version({
                                         .major = 1u,
                                         .minor = 0u,
                                         .patch = 0u
                                     }),
                                     // Vulkan 1.0 by default...
                                     m_runtime_version({
                                         .major = 1u,
                                         .minor = 0u,
                                         .patch = 0u
                                     }),
                                     m_headless(false)
                                     {
    }
    
    VulkanContext::~VulkanContext() {
    }
    
    VulkanContext::operator VkInstance() const {
        return m_handle;
    }
    
    // Public-facing function definitions.
    
    Context::Builder::Builder() : m_context(std::make_shared<VulkanContext>()) {
    }
    
    Context::Builder::~Builder() = default;
    
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