
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

    enum class CullMode {
        None = VK_CULL_MODE_NONE,
        Front = VK_CULL_MODE_FRONT_BIT,
        Back = VK_CULL_MODE_BACK_BIT,
        Both = VK_CULL_MODE_FRONT_AND_BACK
    };
    
    enum class WindingOrder {
        Clockwise = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        CounterClockwise = VK_FRONT_FACE_CLOCKWISE
    };
    
    enum class PolygonMode {
        Point = VK_POLYGON_MODE_POINT,
        Line = VK_POLYGON_MODE_LINE,
        Fill = VK_POLYGON_MODE_FILL
    };
    
    struct RasterizationState {
        CullMode cull_mode;
        WindingOrder winding;
        PolygonMode mode;
    };
    
    enum class VertexInputRate {
        Vertex = VK_VERTEX_INPUT_RATE_VERTEX,
        Instance = VK_VERTEX_INPUT_RATE_INSTANCE
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
        VertexInputRate rate;
    };
    
    // TODO: query against maxVertexInputBindings, retrieved from the device
    struct VertexInputDescription {
        VertexInputDescription& add_attribute(unsigned binding, const char* name);
        VertexInputDescription& set_binding_stride(unsigned binding, unsigned stride);
        VertexInputDescription& set_binding_input_rate(unsigned binding, VertexInputRate rate);
        
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
        
        // Shader stages, in order: vertex, tesselation control, tesselation evaluation, geometry, fragment
        ShaderStageDescription shader_stages[5];
        VkPrimitiveTopology primitive_topology;
        
        VertexInputDescription vertex_input_description;
        
        RasterizationState rasterization_state;
        ViewportState vs;
    };
    
    struct GraphicsPipeline {
    
        using Buffer = void*;
        
        void bind(Buffer buffer, unsigned binding);
    };

    struct ComputePipeline {
    
    };
    
}

#endif // PIPELINE_HPP
