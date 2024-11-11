
#ifndef SHADER_COMPILER_HPP
#define SHADER_COMPILER_HPP

#include "vks/vulkan/descriptor_set.hpp"
#include "vks/vulkan/pipeline.hpp"

#include <vulkan/vulkan.h>
#include <spirv_reflect.h>

// #include <filesystem> // std::filesystem::file_time_type

namespace vks {
    
    // TODO: automatic pipeline recompilation
    
    struct ShaderModule {
        VkShaderModule vk_module;
        SpvReflectShaderModule spv_module;
    };
    
    ShaderModule compile_shader(VkDevice device, const ShaderStageDescription& stage_description);
    
}

#endif // SHADER_COMPILER_HPP
