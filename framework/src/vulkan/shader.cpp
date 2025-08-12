
#include "vks/vulkan/shader.hpp"
#include <utils/logging.hpp>
#include <shaderc/shaderc.hpp>

namespace vks {

    const ShaderModule& ShaderCache::get_shader_module(const ShaderStageDescription& description) {
        // Shared (read) lock
        // Multiple get_shader_module calls can run in parallel, but recompilation gets exclusive access
        std::shared_lock read_lock(m_cache_mutex);
        
        std::size_t index = m_description_to_index[description];
        if (index >= m_modules.size()) {
            // Shader does not exist
        }
        else {
            CachedShaderModule& module = m_modules[index];
            if (module.needs_recompilation.load()) {
                // Shaders are compiled when they are requested
                read_lock.unlock();
                std::unique_lock write_lock(m_cache_mutex);
                if (module.needs_recompilation.load()) {
                    compile_shader(description);
                }
            }
        }
        
        
//        auto& entry = m_modules[idx];
//
//        if (entry.needs_recompile.load()) {
//            // Shaders are compiled only when they are requested
//            lock.unlock();
//            std::unique_lock write_lock(m_cache_mutex);
//            // Double-check after acquiring write lock
//            if (entry.needs_recompile.load()) {
//                recompile_shader(entry);
//                entry.needs_recompile.store(false);
//            }
//        }
//
//        return entry.module;

        return { };
    }
    
    shaderc_shader_kind to_shaderc_stage(VkShaderStageFlags stage) {
        if (stage == VK_SHADER_STAGE_VERTEX_BIT) {
            return shaderc_vertex_shader;
        }
        else if (stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
            return shaderc_fragment_shader;
        }
        else if (stage == VK_SHADER_STAGE_GEOMETRY_BIT) {
            return shaderc_geometry_shader;
        }
        else if (stage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
            return shaderc_tess_control_shader;
        }
        else if (stage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) {
            return shaderc_tess_evaluation_shader;
        }
        else if (stage == VK_SHADER_STAGE_COMPUTE_BIT) {
            return shaderc_compute_shader;
        }
        else if (stage == VK_SHADER_STAGE_TASK_BIT_EXT) {
            return shaderc_task_shader;
        }
        else { // if (stage == VK_SHADER_STAGE_MESH_BIT_EXT) {
            return shaderc_mesh_shader;
        }
    }
    
    std::string read(const std::filesystem::path& filepath) {
        std::ifstream file(filepath, std::ios::in);
        if (!file.is_open()) {
            utils::logging::fatal("Unable to open shader '{}' for read", filepath);
        }

        // Get the length of the file
        file.seekg(0, std::ifstream::end);
        std::streamsize length = file.tellg();
        file.seekg(0, std::ifstream::beg);

        // Reading the file in line by line is slower, but avoids <bad token> errors later with preprocessing
        std::string result;
        result.reserve(length);

        std::string line;
        while (std::getline(file, line)) {
            result += line + '\n';
        }

        return std::move(result);
    }
    
    void ShaderCache::invalidate_shader_variants(const std::filesystem::path& filepath) {
        std::shared_lock lock(m_cache_mutex);
        if (auto it = m_filepath_to_index.find(filepath); it != m_filepath_to_index.end()) {
            for (std::size_t index : it->second) {
                m_modules[index].needs_recompilation.store(true);
            }
        }
    }
    
    void ShaderCache::compile_shader(const ShaderStageDescription& description) {
        auto iter = m_description_to_index.find(description);
        if (iter != m_description_to_index.end()) {
            // Shader already exists
            // TODO: recompile
        }
        
        // Recompile shader
        shaderc::Compiler compiler { };
        shaderc_shader_kind type = to_shaderc_stage(description.stage);
        std::string source = read(description.path);
        
        // Preprocess shader
        shaderc::CompileOptions options { };
        
        #ifndef NDEBUG
            options.SetWarningsAsErrors();
        #else
            // Enable shader performance optimizations for Release builds
            options.SetOptimizationLevel(shaderc_optimization_level_performance);
        #endif

        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        options.SetIncluder(std::make_unique<ShaderIncluder>());
        
        for (const auto& [name, value] : description.preprocessor_definitions) {
            options.AddMacroDefinition(name, value);
        }
        
        const std::string& path = description.path.string();
        
        shaderc::PreprocessedSourceCompilationResult preprocessed = compiler.PreprocessGlsl(source.c_str(), source.size(), type, path.c_str(), options);
        if (preprocessed.GetCompilationStatus() != shaderc_compilation_status_success) {
            utils::logging::fatal("Shader preprocessing failed with error code: {}");
        }
        source = std::string(preprocessed.begin(), preprocessed.end());
        
        // Compile to SPIR-V
        // TODO: function assumes shader entry point is main
        shaderc::SpvCompilationResult compiled = compiler.CompileGlslToSpv(source, type, path.c_str(), options);
        if (compiled.GetCompilationStatus() != shaderc_compilation_status_success) {
            utils::logging::fatal("Shader compilation failed with error code: {}");
        }
        
        std::vector<std::uint32_t> spirv = { compiled.cbegin(), compiled.cend() };
        std::size_t size = spirv.size() * sizeof(std::uint32_t); // Size in bytes
        
        // Generate shader
        VkShaderModuleCreateInfo shader_module_create_info { };
        shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_create_info.codeSize = size;
        shader_module_create_info.pCode = spirv.data();
        
        VkShaderModule module;
        CHECK_CALL(vkCreateShaderModule, *m_device, &shader_module_create_info, nullptr, &module);

        // Reflect SPIR-V bytecode
        SpvReflectShaderModule reflection_data;
        SpvReflectResult reflected = spvReflectCreateShaderModule(size, spirv.data(), &reflection_data);
        if (reflected != SPV_REFLECT_RESULT_SUCCESS) {
            utils::logging::fatal("Shader reflection failed with error code {}", reflected);
        }
    }

}