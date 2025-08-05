
#ifndef PIPELINE_CACHE_HPP
#define PIPELINE_CACHE_HPP

#include "vks/vulkan/shader.hpp"
#include <vulkan/vulkan.h>
#include <string> // std::string
#include <filesystem> // std::filesystem::path
#include <functional> // std::function

namespace vks {
    
    struct VertexAttribute {
        VertexAttribute();
        
        std::string name;
        std::uint32_t offset;
        VkFormat format;
        std::uint32_t location;
    };
    
    struct VertexBinding {
        VertexBinding();
        
        std::uint32_t binding;
        std::vector<VertexAttribute> attributes;
        std::uint32_t stride;
        VkVertexInputRate rate;
    };
    
    // Format and location of shader input variables are reflected directly from the shader source, eliminating the need for manual specification
    // Attributes are matched by name
    struct VertexInput {
        VertexInput();
        
        VertexInput& add_binding(std::uint32_t binding, VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX);
        VertexInput& add_attribute(const std::string& name);
        
        std::vector<VertexBinding> bindings;
    };
    
    struct ColorBlendState {
        ColorBlendState();
        
        bool enabled;
        
        VkBlendFactor src_color_blend_factor;
        VkBlendFactor dst_color_blend_factor;
        VkBlendOp color_blend_op;
        
        VkBlendFactor src_alpha_blend_factor;
        VkBlendFactor dst_alpha_blend_factor;
        VkBlendOp alpha_blend_op;
        
        VkColorComponentFlags color_write_mask;
    };
    
    struct RasterizationState {
        RasterizationState();
        
        VkPolygonMode rendering_mode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace winding_order = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        float line_width;
        bool depth_bias_enable;
        float depth_bias_constant;
        float depth_bias_clamp;
        float depth_bias_slope;
    };
    
    struct DepthStencilState {
        DepthStencilState();
        
        bool depth_test_enable;
        bool depth_write_enable;
        VkCompareOp depth_compare_op;

        bool depth_bounds_test_enable;
        float min_depth_bounds;
        float max_depth_bounds;
        
        bool stencil_test_enable;
        VkStencilOpState front;
        VkStencilOpState back;
    };
    
    struct MultisampleState {
        MultisampleState();
        
        VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
    };
    

    
    struct PipelinePresets {
        static inline RasterizationState wireframe();
        static inline RasterizationState shadow_map();
        
        static inline ColorBlendState no_blending();
        static inline ColorBlendState alpha_blending();
        static inline ColorBlendState additive_blending();
        
        static inline VertexInput position3_normal3();
        static inline VertexInput position3_uv2();
        static inline VertexInput position3_normal3_uv2();
        static inline VertexInput position3_normal3_tangent3_uv2();
        
        static inline VertexInput skinned_mesh();
        
        static inline MultisampleState msaa_2x();
        static inline MultisampleState msaa_4x();
    };
    
    struct PipelineTemplate {
        std::string name;
        
        std::vector<ShaderStageDescription> shader_stages;
        
        VertexInput vertex_input_state;
        RasterizationState rasterization_state;
        DepthStencilState depth_stencil_state;
        MultisampleState multisample_state;
        
        std::vector<VkDynamicState> dynamic_state;
    };
    
    class PipelineBuilder {
        public:
            // Shader type is determined from the file extension:
            //   .vert - vertex
            //   .frag - fragment
            //   .geom - geometry
            //   .tesc - tesselation (control)
            //   .tese - tesselation (evaluation)
            //   .comp - compute
            //   .task - task
            //   .mesh - mesh
            template <typename Fn>
            PipelineBuilder& configure_shader_stage(std::filesystem::path filepath, Fn&& fn);
            PipelineBuilder& add_shader_stage(std::filesystem::path filepath);
            
            // Vertex input
            template <typename Fn>
            PipelineBuilder& configure_vertex_input(Fn&& fn);
            PipelineBuilder& set_vertex_input(const VertexInput& state);
            
            // Rasterization state
            template <typename Fn>
            PipelineBuilder& configure_rasterization_state(Fn&& fn);
            PipelineBuilder& set_rasterization_state(const RasterizationState& state);
            
            PipelineBuilder& enable_culling(VkCullModeFlags mode);
            PipelineBuilder& disable_culling();
            
            PipelineBuilder& set_winding_order(VkFrontFace face);
            
            PipelineBuilder& set_rendering_mode(VkPolygonMode mode);
            PipelineBuilder& enable_wireframe();
            
            PipelineBuilder& set_rendering_primitive(VkPrimitiveTopology topology);
            PipelineBuilder& use_triangles();
            PipelineBuilder& use_triangle_strip();
            PipelineBuilder& use_lines();
            PipelineBuilder& use_points();
            
            // Depth stencil state
            template <typename Fn>
            PipelineBuilder& configure_depth_stencil_state(Fn&& fn);
            PipelineBuilder& set_depth_stencil_state(const DepthStencilState& state);
            
            PipelineBuilder& enable_depth_test(VkCompareOp compare_op);
            PipelineBuilder& disable_depth_test();
            PipelineBuilder& enable_depth_write();
            PipelineBuilder& disable_depth_write();
            
            PipelineBuilder& set_depth_compare_op(VkCompareOp op);
            
            PipelineBuilder& enable_depth_bias(float constant, float slope);
            PipelineBuilder& disable_depth_bias();

            // Multisampling state
            template <typename Fn>
            PipelineBuilder& configure_multisample_state(Fn&& fn);
            PipelineBuilder& set_multisample_state(const MultisampleState& state);
            
            PipelineBuilder& set_sample_count(VkSampleCountFlagBits samples);
            
            // Dynamic state
            template <typename ...Ts>
            PipelineBuilder& add_dynamic_states(VkDynamicState state, const Ts&...);
            PipelineBuilder& add_dynamic_state(VkDynamicState state);
            
        private:
            PipelineTemplate& m_pipeline;
    };
    
    class PipelineCache {
        public:
            PipelineBuilder register_pipeline_template(const std::string& name);
            
            // For rebuilding the pipeline at runtime when the underlying shader is modified
            void invalidate_pipeline_template(const std::string& name);
            
            [[nodiscard]] void instantiate_pipeline();
            
        private:
    };
    
}

#endif // PIPELINE_CACHE_HPP
