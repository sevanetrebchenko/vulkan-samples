
#ifndef SHADER_HPP
#define SHADER_HPP

#include <vulkan/vulkan.h>
#include <spirv_reflect.h>
#include <shaderc/shaderc.hpp>

#include <cstdint> // std::uint8_t
#include <unordered_map> // std::unordered_map
#include <filesystem> // std::filesystem::path

namespace vks {

    enum class ShaderStage : std::uint8_t {
        Vertex = VK_SHADER_STAGE_VERTEX_BIT,
        Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
        Geometry = VK_SHADER_STAGE_GEOMETRY_BIT,
        Compute = VK_SHADER_STAGE_COMPUTE_BIT,
    };
    
    struct ShaderModule {
        struct SpecializationConstant {
            // Vulkan GLSL specialization constants must be one of: bool, int, uint, float, double
            union {
                bool b;
                int i;
                unsigned u;
                float f;
                double d;
            } value;
            std::uint8_t size;
            
            const char* name;
        };
        
        // TODO: should have a way to access Vulkan instance / device
        ShaderModule(std::filesystem::path filepath);
        ShaderModule(std::filesystem::path filepath, ShaderStage stage);
        ~ShaderModule();
        
        ShaderStage stage;
        std::filesystem::path filepath;
        std::filesystem::file_time_type last_modified_time;
        
        std::vector<SpecializationConstant> constants;
        std::unordered_map<std::string, std::string> preprocessor_definitions;
        
        VkShaderModule handle;
        SpvReflectShaderModule reflection_data;
    };
    
    class ShaderCache {
        public:
            void add(std::filesystem::path filepath, ShaderStage stage);
            
            const ShaderModule& get(const char* name);
            
        private:
            // Mapping between name and shader module
            std::unordered_map<const char*, ShaderModule> m_modules;
    };
    
    class ShaderCompiler {
        public:
            ShaderCompiler(std::filesystem::path filepath); // Stage is automatically deduced from the shader extension
            ShaderCompiler(std::filesystem::path filepath, ShaderStage stage);
            ~ShaderCompiler();
            
            // Compile-time (macro) definition
            void define_macro(const char* name, const char* value);
            
            // Vulkan GLSL specialization constants
            // Can be one of: bool, integer, unsigned integer, float, double
            template <typename T>
            void define_constant(const char* name, T value);
            
            ShaderModule compile();
            
        private:
            // Configures shared compiler options
            void configure_compile_options();
            
            ShaderModule m_module;
            shaderc::CompileOptions m_options;
    };
    
}

// Template definitions
#include "shader.tpp"

#endif // SHADER_HPP
