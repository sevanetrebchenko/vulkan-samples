
#ifndef SHADER_COMPILER_HPP
#define SHADER_COMPILER_HPP

#include <utils/result.hpp>
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>
#include <filesystem> // std::filesystem::path, std::filesystem::file_time_type
#include <memory> // std::shared_ptr
#include <vector> // std::vector
#include <unordered_map> // std::unordered_map

namespace vks {
    
    class ShaderModule {
        public:
            enum class Stage {
                Vertex,
                Fragment,
                Compute,
                Geometry
            };
            
            struct CompilationError {
                CompilationError();
                
                std::string message;
                std::size_t num_errors;
            };
            
            ShaderModule(std::filesystem::path filepath);
            ~ShaderModule();
            
            // Compile-time (macro) definition
            void define_macro(const char* name, const char* value);
            
            // Vulkan GLSL specialization constants
            // Can be one of: bool, integer, unsigned integer, float, double
            template <typename T>
            void define_constant(const char* name, T value);
            
            bool should_recompile();
            utils::Result<VkPipelineShaderStageCreateInfo, CompilationError> compile();
            
        private:
            void preprocess();
            VkPipelineShaderStageCreateInfo to_pipeline_stage() const;
            
            std::filesystem::path filepath;
            std::filesystem::file_time_type last_modified_time;
            
            shaderc::CompileOptions m_compile_options;
            
            std::vector<VkSpecializationMapEntry> specialization_constants;
            
            VkShaderStageFlags stage;
            VkShaderModule module;
    };
    
    
    class ShaderCompiler {
        public:
            ShaderCompiler(std::filesystem::path filepath);
            ~ShaderCompiler();
            
            // Compile-time (macro) definition
            void define_macro(const char* name, const char* value);
            
            // Vulkan GLSL specialization constants
            // Can be one of: bool, integer, unsigned integer, float, double
            template <typename T>
            void define_constant(const char* name, T value);
            
            // Compiles shader and generates reflection data
            void compile();
            
            // Section: reflection data
            
        private:
            struct SpecializationConstant {
                // GLSL specialization constants must be one of: bool, int, uint, float, double
                union {
                    bool b;
                    int i;
                    unsigned u;
                    float f;
                    double d;
                } value;
                
                const char* name;
                std::uint8_t size;
            };

            // Reads in shader source from disk, performs parsing of includes
            std::string read(const std::filesystem::path& path);
            
            void generate_reflection_data(const std::vector<unsigned>& spirv);
            
            std::filesystem::path m_filepath;
            ShaderModule::Stage m_stage;
            
            shaderc::CompileOptions m_options;
            std::vector<SpecializationConstant> m_constants;
            
            std::unordered_set<std::string> m_included_files;
    };
    

    
}

#endif // SHADER_COMPILER_HPP
