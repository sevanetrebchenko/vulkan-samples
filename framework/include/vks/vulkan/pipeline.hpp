
#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "vks/vulkan/shader.hpp"
#include "vks/types.hpp"

#include <filesystem> // std::filesystem::path
#include <functional> // std::function

namespace vks {
    
    struct PipelineDescription {
        // vertex attributes
        // uniform bindings + descriptor sets
        
        // blending, rasterization, depth testing
        
        PipelineDescription& add_shader_stage(const ShaderStageDescription& stage);
        
        // Up to 5 shader stages can be attached to a Vulkan pipeline at once:
        //   Graphics: vertex, tesselation control, tesselation evaluation, geometry, fragment (5)
        //   Compute: compute (1)
        //   Mesh: pipelines: task, mesh, fragment (3)
        ShaderStageDescription shader_stages[5];
        u8 num_active_stages;
    };

}

#endif // PIPELINE_HPP
