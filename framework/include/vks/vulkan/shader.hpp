
#ifndef SHADER_HPP
#define SHADER_HPP

#include "vks/types.hpp"

#include <vulkan/vulkan.h>
#include <filesystem> // std::filesystem::path
#include <vector> // std::vector
#include <unordered_map> // std::unordered_map

namespace vks {
    
    enum class ShaderStage : u8 {
        Vertex = VK_SHADER_STAGE_VERTEX_BIT,
        Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
        Compute = VK_SHADER_STAGE_COMPUTE_BIT,
    };
    
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
            ShaderStageDescription(std::filesystem::path path);
            ShaderStageDescription(std::filesystem::path path, ShaderStage stage);
            ~ShaderStageDescription();
            
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
