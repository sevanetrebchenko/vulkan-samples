
#include "renderpass.hpp"

namespace vks {
    
    RenderPass::Builder::Builder() {
    }
    
    RenderPass::Builder::~Builder() {
    }
    
    RenderPass::Builder::Builder(RenderPass::Builder&& other) noexcept : m_handle(std::move(other.m_handle)) {
    }
    
}