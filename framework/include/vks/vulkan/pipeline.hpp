
#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "vks/vulkan/shader.hpp"
#include "vks/types.hpp"

#include <filesystem> // std::filesystem::path
#include <functional> // std::function

namespace vks {
    
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
    
    struct VertexAttribute {
        const char* name;
        unsigned location;
        VkFormat format;
    };
    
    struct VertexBinding {
        unsigned binding;
        unsigned stride;
        std::vector<VertexAttribute> attributes;
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
        
        std::vector<PushConstant> push_constants;
    };

    struct ComputePipeline {
    
    };
    
}

#endif // PIPELINE_HPP
