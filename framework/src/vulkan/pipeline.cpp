
#include "vks/vulkan/pipeline.hpp"

namespace vks {
    
    PipelineDescription& PipelineDescription::add_shader_stage(const ShaderStageDescription& stage) {
        shader_stages[num_active_stages++] = stage;
        return *this;
    }
    
}
