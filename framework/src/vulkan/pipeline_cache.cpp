
#include "vks/vulkan/pipeline_cache.hpp"

namespace vks {
    
    VertexAttribute::VertexAttribute() : name(),
                                         offset(),
                                         format(),
                                         location() {
    }
    
    VertexBinding::VertexBinding() : binding(),
                                     attributes(),
                                     stride(),
                                     rate() {
    }
    
    VertexInputState::VertexInputState() : bindings() {
    }
    
    VertexInputState& VertexInputState::add_binding(std::uint32_t binding, VkVertexInputRate rate) {
    }
    
    VertexInputState& VertexInputState::add_attribute(const std::string& name) {
    }
    
    ColorBlendState::ColorBlendState() : enabled(false),
                                         src_color_blend_factor(VK_BLEND_FACTOR_ONE),
                                         dst_color_blend_factor(VK_BLEND_FACTOR_ZERO),
                                         color_blend_op(VK_BLEND_OP_ADD),
                                         src_alpha_blend_factor(VK_BLEND_FACTOR_ONE),
                                         dst_alpha_blend_factor(VK_BLEND_FACTOR_ZERO),
                                         alpha_blend_op(VK_BLEND_OP_ADD),
                                         color_write_mask(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT) {
    }
    
    RasterizationState::RasterizationState() : rendering_mode(VK_POLYGON_MODE_FILL),
                                               cull_mode(VK_CULL_MODE_BACK_BIT),
                                               winding_order(VK_FRONT_FACE_COUNTER_CLOCKWISE),
                                               line_width(1.0f),
                                               depth_bias_enable(false),
                                               depth_bias_constant(0.0f),
                                               depth_bias_clamp(0.0f),
                                               depth_bias_slope(0.0f) {
    }
    
    DepthStencilState::DepthStencilState() : depth_test_enable(true),
                                             depth_write_enable(true),
                                             depth_compare_op(VK_COMPARE_OP_LESS_OR_EQUAL),
                                             depth_bounds_test_enable(false),
                                             min_depth_bounds(1.0f),
                                             max_depth_bounds(1.0f),
                                             stencil_test_enable(false),
                                             front(),
                                             back() {
    }
    
    MultisampleState::MultisampleState() : sample_count(VK_SAMPLE_COUNT_1_BIT) {
    }
    
}