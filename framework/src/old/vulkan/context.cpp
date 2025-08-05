
#include "vks/vulkan/context.hpp"
#include "vks/types.hpp"
#include "utils/logging.hpp"
#include "utils/exceptions.hpp"
#include "utils/platform.hpp"

#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>

#include <fstream> // std::ifstream

namespace vks {

    namespace detail {
        
        
        
        std::size_t get_shader_stage_index(const PipelineDescription& pipeline_description, ShaderStage stage) {
            std::size_t num_stages = pipeline_description.shader_stages.size();
            
            for (std::size_t i = 0; i < num_stages; ++i) {
                if (pipeline_description.shader_stages[i].stage == stage) {
                    return i;
                }
            }
            
            return num_stages;
        }
        
    }
    


}