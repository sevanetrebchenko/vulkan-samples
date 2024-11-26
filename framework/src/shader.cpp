//
//#include "shader.hpp"
//#include <iostream>
//#include <fstream>
//
//namespace vks {
//
//    std::string read(const std::filesystem::path& filepath) {
//        std::ifstream file(filepath, std::ios::in);
//        if (!file.is_open()) {
//            throw std::runtime_error("");
//        }
//
//        // Get the length of the file
//        file.seekg(0, std::ifstream::end);
//        std::streamsize length = file.tellg();
//        file.seekg(0, std::ifstream::beg);
//
//        // Reading the file in line by line is slower, but avoids <bad token> errors later with preprocessing
//        std::string result;
//        result.reserve(length);
//
//        std::string line;
//        while (std::getline(file, line)) {
//            result += line;
//            result += '\n';
//        }
//
//        return std::move(result);
//    }
//
//    struct ShaderIncluder final : public shaderc::CompileOptions::IncluderInterface {
//        // Retrieves the contents of 'file'
//        shaderc_include_result* GetInclude(const char* file, shaderc_include_type type, const char* source, std::size_t depth) override {
//            // Determine the global filepath of the included file
//            // TODO: <standard> includes for default / shared shaders?
//            std::filesystem::path filepath(file);
////            if (type == shaderc_include_type_relative) {
////                // Reconstruct global filepath from a relative include
////                filepath = std::filesystem::path(source).parent_path() / filepath;
////            }
//
//            shaderc_include_result& result = include_results.emplace_back();
//
//            if (std::filesystem::exists(filepath)) {
//                // Ensure that a shader is only included once
//                bool found = false;
//                for (const Include& include : includes) {
//                    if (include.filepath == filepath) {
//                        // File has already been included (note: this is not an error)
//                        result.source_name = include.filepath.c_str();
//                        result.source_name_length = include.filepath.length();
//
//                        // Do not include this file multiple times
//                        result.content = nullptr;
//                        result.content_length = 0u;
//
//                        found = true;
//                    }
//                }
//
//                if (!found) {
//                    // Register new include
//                    Include& include = includes.emplace_back();
//                    include.filepath = filepath.string();
//                    include.contents = read(filepath);
//
//                    result.source_name = include.filepath.c_str();
//                    result.source_name_length = include.filepath.length();
//                    result.content = include.contents.c_str();
//                    result.content_length = include.contents.length();
//                }
//            }
//            else {
//                // Provided filepath does not exist
//                // For a failed inclusion, source_name should be empty
//                result.source_name = nullptr;
//                result.source_name_length = 0u;
//
//                // For a failed inclusion, content contains the error message
//                result.content = "";
//                result.content_length = strlen(result.content);
//            }
//
//            return &result;
//        }
//
//        void ReleaseInclude(shaderc_include_result* data) override {
//            // Nothing to do here
//        }
//
//        struct Include {
//            std::string filepath; // Stored as std::string and not std::filesystem::path to be able to directly reference filepath.c_str() in the return of GetInclude
//            std::string contents;
//        };
//
//        // includes references only the unique includes
//        // Contents of included files must remain valid from when GetInclude is invoked to retrieve the include contents to when ReleaseInclude is invoked to release them
//        std::vector<Include> includes;
//        std::vector<shaderc_include_result> include_results;
//    };
//
//
//    ShaderModule::ShaderModule(std::filesystem::path path, ShaderStage stage) : filepath(std::move(path)),
//                                                                                last_modified_time(std::filesystem::last_write_time(filepath)),
//                                                                                stage(stage),
//                                                                                handle(nullptr) {
//        if (!std::filesystem::exists(filepath)) {
//            throw std::runtime_error("");
//        }
//    }
//
//    ShaderModule::ShaderModule(std::filesystem::path path) : filepath(std::move(path)),
//                                                             last_modified_time(std::filesystem::last_write_time(filepath)),
//                                                             handle(nullptr) {
//        if (!std::filesystem::exists(filepath)) {
//            throw std::runtime_error("");
//        }
//
//        // Determine stage from shader extension
//        std::filesystem::path extension = filepath.extension();
//        if (extension == ".vert") {
//            stage = ShaderStage::Vertex;
//        }
//        else if (extension == ".frag") {
//            stage = ShaderStage::Fragment;
//        }
//        else if (extension == ".comp") {
//            stage = ShaderStage::Compute;
//        }
//        else if (extension == ".geom") {
//            stage = ShaderStage::Geometry;
//        }
//        else {
//            // Unknown shader extension
//            throw std::runtime_error("");
//        }
//    }
//
//    ShaderModule::~ShaderModule() {
//        // TODO: destroy shader module
//    }
//
//    ShaderCompiler::ShaderCompiler(std::filesystem::path filepath) : m_module(std::move(filepath)) {
//        configure_compile_options();
//    }
//
//    ShaderCompiler::ShaderCompiler(std::filesystem::path filepath, ShaderStage stage) : m_module(std::move(filepath), stage) {
//        configure_compile_options();
//    }
//
//    ShaderCompiler::~ShaderCompiler() = default;
//
//    void ShaderCompiler::define_macro(const char* name, const char* value) {
//        m_options.AddMacroDefinition(name, strlen(name), value, strlen(value));
//    }
//
//    ShaderModule ShaderCompiler::compile() {
//        shaderc::Compiler compiler { };
//
//        // Vulkan GLSL
//        shaderc_shader_kind type;
//        switch (m_module.stage) {
//            case ShaderStage::Vertex:
//                type = shaderc_glsl_vertex_shader;
//                break;
//            case ShaderStage::Fragment:
//                type = shaderc_glsl_fragment_shader;
//                break;
//            case ShaderStage::Compute:
//                type = shaderc_glsl_compute_shader;
//                break;
//            case ShaderStage::Geometry:
//                type = shaderc_glsl_geometry_shader;
//                break;
//        }
//
//        std::string source = read(m_module.filepath);
//
//        shaderc::PreprocessedSourceCompilationResult preprocess_result = compiler.PreprocessGlsl(source.c_str(), source.size(), type, m_module.filepath.string().c_str(), m_options);
//        if (preprocess_result.GetCompilationStatus() != shaderc_compilation_status_success) {
//            throw std::runtime_error("");
//        }
//
//        source = std::string(preprocess_result.begin(), preprocess_result.end());
//
//        // Compile to SPIR-V bytecode
//        // Note: function assumes shader entry point is 'main'
//        shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, type, m_module.filepath.string().c_str(), m_options);
//        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
//            throw std::runtime_error("");
//        }
//
//        std::vector<unsigned> spirv { result.cbegin(), result.cend() };
//        std::size_t size = spirv.size() * sizeof(unsigned); // Size in bytes
//
//        // Generate shader
//        VkShaderModuleCreateInfo shader_module_create_info { };
//        shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//        shader_module_create_info.codeSize = size;
//        shader_module_create_info.pCode = spirv.data();
//
////        if (vkCreateShaderModule(device, &shader_module_create_info, nullptr, &m_module.handle) != VK_SUCCESS) {
////            throw std::runtime_error("");
////        }
//
//        // Reflect SPIR-V bytecode
//        spvReflectCreateShaderModule(size, spirv.data(), &m_module.reflection_data);
//
//        return m_module;
//    }
//
//    void ShaderCompiler::configure_compile_options() {
//        #ifdef NDEBUG
//            // Enable shader performance optimizations for Release builds
//            m_compile_options.SetOptimizationLevel(shaderc_optimization_level_performance);
//        #else
//            m_options.SetWarningsAsErrors();
//        #endif
//
//        m_options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
//        m_options.SetIncluder(std::make_unique<ShaderIncluder>());
//    }
//
//}