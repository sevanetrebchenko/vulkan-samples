
#include "vks/detail/shader_compiler.hpp"
#include "vks/vulkan/utility.hpp"
#include "vks/renderer.hpp"
#include "hash.hpp"

#include "utils/filesystem.hpp"
#include "utils/platform.hpp"
#include "utils/logging.hpp"
#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>

#include <fstream> // std::ifstream

namespace vks {

    struct Include {
        std::filesystem::path filepath;
        std::string contents;
    };
    
    struct ShaderPreprocessor final : public shaderc::CompileOptions::IncluderInterface {
        ShaderPreprocessor();
        ~ShaderPreprocessor() override;

        shaderc_include_result* GetInclude(const char* file, shaderc_include_type type, const char* source, std::size_t depth) override;
        void ReleaseInclude(shaderc_include_result* data) override;

        // Contents of included files must remain valid from when GetInclude is invoked to retrieve the include contents to when ReleaseInclude is invoked to release them
        std::vector<Include> includes;
        std::vector<shaderc_include_result> include_results;
    };
    
    ShaderPreprocessor::ShaderPreprocessor() {
    }
    
    ShaderPreprocessor::~ShaderPreprocessor() = default;

    shaderc_include_result* ShaderPreprocessor::GetInclude(const char* file, shaderc_include_type type, const char* source, std::size_t depth) {
        // Determine the global filepath of the included file
        std::filesystem::path filepath(file);
        shaderc_include_result& result = include_results.emplace_back();

        if (std::filesystem::exists(filepath)) {
            // Ensure that a shader is included only once
            bool found = false;

            for (const Include& include : includes) {
                if (include.filepath == filepath) {
                    found = true;
                    break;
                }
            }

            std::size_t length = strlen(file);

            if (found) {
                // File has already been included and should not be duplicated
                result.source_name = file;
                result.source_name_length = length;
                result.content = nullptr;
                result.content_length = 0;
            }
            else {
                // Register new include
                Include& include = includes.emplace_back();
                include.filepath = filepath;
                include.contents = utils::load(filepath);

                result.source_name = file;
                result.source_name_length = length;
                result.content = include.contents.c_str();
                result.content_length = include.contents.length();
            }
        }
        else {
            // For a failed inclusion, source_name should be empty
            result.source_name = nullptr;
            result.source_name_length = 0;

            // For a failed inclusion, content contains the error message
            result.content = "";
            result.content_length = strlen(result.content);
        }

        return &result;
    }

    void ShaderPreprocessor::ReleaseInclude(shaderc_include_result* data) {
        // Nothing to do here
    }
    
    struct ShaderCompiler {
        ShaderCompiler();
        ~ShaderCompiler();

        

        std::unordered_map<ShaderStageDescription, ShaderModule> modules;
    };
    
    ShaderModule compile_shader(VkDevice device, const ShaderStageDescription& stage_description) {
        // Configure compile options
        shaderc::CompileOptions options { };

        #ifndef NDEBUG
            // Optimizing for performance interferes with shader reflection
            // options.SetOptimizationLevel(shaderc_optimization_level_performance);
        #else
            options.SetWarningsAsErrors();
        #endif

        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        options.SetIncluder(std::make_unique<ShaderPreprocessor>());

        // Add preprocessor definitions
        for (const auto& [name, value] : stage_description.preprocessor_definitions) {
            options.AddMacroDefinition(name, value);
        }

        // Using Vulkan GLSL
        shaderc_shader_kind type;
        switch (stage_description.stage) {
            case VK_SHADER_STAGE_VERTEX_BIT:
                type = shaderc_glsl_vertex_shader;
                break;
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                type = shaderc_glsl_tess_control_shader;
                break;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                type = shaderc_glsl_tess_evaluation_shader;
                break;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                type = shaderc_glsl_geometry_shader;
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                type = shaderc_glsl_fragment_shader;
                break;
            case VK_SHADER_STAGE_COMPUTE_BIT:
                type = shaderc_glsl_compute_shader;
                break;
//            case ShaderStage::Mesh:
//                type = shaderc_glsl_mesh_shader;
//                break;
//            case ShaderStage::Task:
//                type = shaderc_glsl_task_shader;
//                break;
        }

//        // Shader modules should only be compiled again if they do not yet exist or if the shader source has been modified since being compiled the first time
//        bool recompile = true;
//
//        std::filesystem::file_time_type last_modified_time = std::filesystem::last_write_time(stage_description.path);
//        auto iter = shader_cache.find(stage_description.path);
//
//        if (iter != shader_cache.end()) {
//            const ShaderModule& cached = iter->second;
//
//            if (cached.last_modified_time == last_modified_time) {
//                // Shader source has not been modified since the original time it was cached and is still valid
//                recompile = false;
//            }
//        }
//
//        if (!recompile) {
//            // Shader module is guaranteed to be current
//            return iter->second;
//        }

        shaderc::Compiler compiler { };
        ShaderModule module { };
        std::string source = utils::load(stage_description.path);

        // Replace preprocessor definitions and resolve includes
        #if defined(PLATFORM_WINDOWS)
            // Convert std::filesystem::path::c_str() to a const char* (returns const wchar_t* on Windows)
            std::string path = stage_description.path.string();
            shaderc::PreprocessedSourceCompilationResult preprocess_result = compiler.PreprocessGlsl(source.c_str(), source.size(), type, path.c_str(), options);
        #else
            const char* path = stage_description.path.c_str();
            shaderc::PreprocessedSourceCompilationResult preprocess_result = compiler.PreprocessGlsl(source.c_str(), source.size(), type, path, options);
        #endif

        shaderc_compilation_status preprocess_status = preprocess_result.GetCompilationStatus();
        if (preprocess_status != shaderc_compilation_status_success) {
            std::string error = utils::format("Shader compilation failed with error code {} - {}", std::underlying_type<shaderc_compilation_status>::type(preprocess_status), preprocess_result.GetErrorMessage());
            utils::logging::error(error);
            throw std::runtime_error(error);
        }

        source = std::string(preprocess_result.begin(), preprocess_result.end());

        // Compile to SPIR-V bytecode

        // Assume shader entry point is 'main'
        #if defined(PLATFORM_WINDOWS)
            shaderc::SpvCompilationResult compile_result = compiler.CompileGlslToSpv(source, type, path.c_str(), options);
        #else
            shaderc::SpvCompilationResult compile_result = compiler.CompileGlslToSpv(source, type, path, options);
        #endif

        shaderc_compilation_status compile_status = compile_result.GetCompilationStatus();
        if (compile_status != shaderc_compilation_status_success) {
            std::string error = utils::format("Shader compilation failed with error code {} - {}", std::underlying_type<shaderc_compilation_status>::type(compile_status), compile_result.GetErrorMessage());
            utils::logging::error(error);
            throw std::runtime_error(error);
        }

        std::vector<unsigned> spirv { compile_result.cbegin(), compile_result.cend() };
        std::size_t size = spirv.size() * sizeof(unsigned); // Size in bytes

        // Create shader module
        VkShaderModuleCreateInfo shader_module_create_info { };
        shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_create_info.codeSize = size;
        shader_module_create_info.pCode = spirv.data();

        VkResult result = vkCreateShaderModule(device, &shader_module_create_info, nullptr, &module.vk_module);
        if (result != VK_SUCCESS) {
            std::string error = utils::format("vkCreateShaderModule failed with error code {}", result);
            utils::logging::error(error);
            throw std::runtime_error(error);
        }

        // Generate reflection data using SPIR-V bytecode
        spvReflectCreateShaderModule(size, spirv.data(), &module.spv_module);
        return module;
    }

}
