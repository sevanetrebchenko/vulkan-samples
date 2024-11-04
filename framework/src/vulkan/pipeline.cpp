
#include "vks/vulkan/pipeline.hpp"

namespace vks {
    
    PipelineDescription& PipelineDescription::add_shader_stage(const ShaderStageDescription& stage) {
        for (ShaderStageDescription& current : shader_stages) {
            if (current.stage == stage.stage) {
                current = stage;
                return *this;
            }
        }
        
        shader_stages.emplace_back(stage);
        return *this;
    }
    
}
