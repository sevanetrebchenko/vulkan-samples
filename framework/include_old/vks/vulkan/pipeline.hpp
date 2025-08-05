
#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "vks/vulkan/shader.hpp"
#include "vks/vulkan/descriptor_set.hpp"
#include "vks/types.hpp"

#include <filesystem> // std::filesystem::path
#include <functional> // std::function

namespace vks {

/**
Automatically reflected from shaders:
 - Descriptor set layouts (bindings, types, stages)
 - Pipeline layout (descriptor sets + push constants)
 - Vertex input attributes (locations, formats)
 - Shader stages and entry points
 - Specialization constants
 - Push constant ranges

Must be specified (cannot be reflected):
 - Render pass / render targets
 - Rasterization state (fill mode, culling, etc.)
 - Viewport/scissor (usually dynamic anyway)
 - Multisampling settings
 - Depth/stencil test configuration
 - Color blending settings
 - Vertex buffer bindings (stride, rate)

Reasonable default values:
 - Input assembly (triangle list is common)
 - Rasterization (fill, no culling, front face CCW)
 - Depth testing (less-equal, write enabled)
 - Color blending (no blending)
**/
    
    struct VertexAttribute {
        const char* name;  // Must match shader input variable name
        
    };


    
    struct BlendState {
    };

    struct ViewportState {
        ViewportState() {}
        ~ViewportState() {}
        
        ViewportState& set_offset(unsigned x, unsigned y);
        ViewportState& set_extent(unsigned width, unsigned height);
        ViewportState& set_depth_range(float min, float max);
        
        VkViewport viewport;
    };
    
    struct RasterizationState {
        VkCullModeFlags cull_mode;
        VkFrontFace winding;
        VkPolygonMode mode;
    };
    
    struct VertexBinding {
        // binding - binding point of the buffer used to read data from
        unsigned binding;
        
        // stride - number of bytes between consecutive elements in the buffer
        unsigned stride;
        
        std::vector<VertexAttribute> attributes;
        
        // inputRate - specifies whether this data is updated per vertex or per instance (for instanced rendering)
        //   - VK_VERTEX_INPUT_RATE_VERTEX - updated per vertex
        //   - VK_VERTEX_INPUT_RATE_INSTANCE - updated per instance
        VkVertexInputRate rate;
    };
    
    // TODO: query against maxVertexInputBindings, retrieved from the device
    struct VertexInputDescription {
        VertexInputDescription& add_attribute(unsigned binding, const char* name);
        VertexInputDescription& set_binding_stride(unsigned binding, unsigned stride);
        VertexInputDescription& set_binding_input_rate(unsigned binding, VkVertexInputRate rate);
        
        std::vector<VertexBinding> bindings;
    };

    struct GraphicsPipelineDescription {
        // input assembly state
        
        // Up to 5 shader stages
        GraphicsPipelineDescription& add_shader_stage(ShaderStageDescription stage_description);
        
        GraphicsPipelineDescription& set_primitive_topology(VkPrimitiveTopology primitive_topology);
        GraphicsPipelineDescription& set_rasterization_state(RasterizationState rasterization_state);
        GraphicsPipelineDescription& set_vertex_input(VertexInputDescription vertex_input_description);
        GraphicsPipelineDescription& set_viewport_state(ViewportState viewport_state);
        
        // Supported shader stages: vertex, tesselation control, tesselation evaluation, geometry, fragment
        std::vector<ShaderStageDescription> shader_stages;
        VkPrimitiveTopology primitive_topology;
        
        VertexInputDescription vertex_input_description;
        
        RasterizationState rasterization_state;
        ViewportState vs;
    };
    
    struct PushConstant {
        const char* name;
        unsigned size;
        unsigned offset;
        unsigned padding;
        VkShaderStageFlags stages;
    };
    
    struct GraphicsPipeline {
        using Buffer = void*;
        
        void bind(Buffer buffer, unsigned binding);
        
        [[nodiscard]] std::shared_ptr<DescriptorSet> get_descriptor_set(unsigned int set) const;
        
        GraphicsPipelineDescription description;
        std::vector<PushConstant> push_constants;
        
        std::vector<DescriptorSet> descriptor_sets;
    };

    struct ComputePipeline {
    
    };
    
}

#endif // PIPELINE_HPP
