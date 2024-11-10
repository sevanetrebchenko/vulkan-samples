
#ifndef SHADER_HPP
#define SHADER_HPP

#include "vks/types.hpp"

#include <vulkan/vulkan.h>
#include <filesystem> // std::filesystem::path
#include <vector> // std::vector
#include <unordered_map> // std::unordered_map

namespace vks {
    
    enum class ShaderStage {
        // Graphics pipeline shader stages
        Vertex = VK_SHADER_STAGE_VERTEX_BIT,
        TesselationControl = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
        TesselationEvaluation = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
        Geometry = VK_SHADER_STAGE_GEOMETRY_BIT,
        Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
        
        // Compute pipelines shader stages
        Compute = VK_SHADER_STAGE_COMPUTE_BIT,
        
        // Mesh pipeline shader stages
        Mesh = VK_SHADER_STAGE_MESH_BIT_EXT,
        Task = VK_SHADER_STAGE_TASK_BIT_EXT,
        
        // Raytracing pipeline shader stages
        // ...
        
        None = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM,
    };
    
    std::size_t to_pipeline_index(ShaderStage stage);
    
    struct ShaderConstant {
        // Vulkan GLSL specialization constants must be one of: bool, int, uint, float, double
        union {
            bool b;
            int i;
            unsigned u;
            float f;
            double d;
        } value;
        u8 size;

        const char* name;
    };

    class ShaderStageDescription {
        public:
            ShaderStageDescription();
            ~ShaderStageDescription();
            
            ShaderStageDescription& set_filepath(std::filesystem::path path);
            ShaderStageDescription& set_filepath(std::filesystem::path path, ShaderStage stage);
            
            ShaderStageDescription& define_macro(const char* name, const char* value);
    
            ShaderStageDescription& define_constant(const char* name, bool value);
            ShaderStageDescription& define_constant(const char* name, int value);
            ShaderStageDescription& define_constant(const char* name, unsigned value);
            ShaderStageDescription& define_constant(const char* name, float value);
            ShaderStageDescription& define_constant(const char* name, double value);
            
            std::filesystem::path path;
            ShaderStage stage;
            
            std::vector<ShaderConstant> constants;
            std::unordered_map<const char*, const char*> preprocessor_definitions;
        
        private:
            [[nodiscard]] ShaderConstant& get_constant(const char* name);
    };
    
}

#endif // SHADER_HPP
