
#include "shader_compiler.hpp"
#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>
#include <fstream> // std::ifstream
#include <utility>
#include <iostream>

namespace vks {
    
    ShaderCompiler::ShaderCompiler(std::filesystem::path filepath) : m_filepath(std::move(filepath)) {
        if (!std::filesystem::exists(m_filepath)) {
//            throw utils::FormattedError("unable to open shader '{}'", m_filepath);
        }
        
        #ifdef NDEBUG
            // Enable shader performance optimizations for Release builds
            m_compile_options.SetOptimizationLevel(shaderc_optimization_level_performance);
        #else
            m_options.SetWarningsAsErrors();
        #endif
        
        std::filesystem::path extension = m_filepath.extension();
        if (extension == ".vert") {
            m_stage = ShaderModule::Stage::Vertex;
        }
        else if (extension == ".frag") {
            m_stage = ShaderModule::Stage::Fragment;
        }
        else if (extension == ".comp") {
            m_stage = ShaderModule::Stage::Compute;
        }
        else if (extension == ".geom") {
            m_stage = ShaderModule::Stage::Geometry;
        }
    }
    
    ShaderCompiler::~ShaderCompiler() {
    }
    
    void ShaderCompiler::define_macro(const char* name, const char* value) {
        m_options.AddMacroDefinition(name, strlen(name), value, strlen(value));
    }
    
    void ShaderCompiler::compile() {
        std::string source = read(m_filepath);
        
        std::cout << source << std::endl;
        
        shaderc::Compiler compiler { };
        
        // Vulkan GLSL
        shaderc_shader_kind type;
        switch (m_stage) {
            case ShaderModule::Stage::Vertex:
                type = shaderc_glsl_vertex_shader;
                break;
            case ShaderModule::Stage::Fragment:
                type = shaderc_glsl_fragment_shader;
                break;
            case ShaderModule::Stage::Compute:
                type = shaderc_glsl_compute_shader;
                break;
            case ShaderModule::Stage::Geometry:
                type = shaderc_glsl_geometry_shader;
                break;
        }
        
        // Compile to SPIR-V bytecode
        // Assumes shader entry point is 'main'
        shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, type, m_filepath.string().c_str());
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        }
        
        generate_reflection_data({ result.cbegin(), result.cend() });
    }
    
    template <typename T>
    void ShaderCompiler::define_constant(const char* name, T value) {
        using Type = typename std::decay<T>::type;
        static_assert(std::is_same<Type, bool>::value || std::is_same<Type, int>::value || std::is_same<Type, unsigned int>::value || std::is_same<Type, float>::value || std::is_same<Type, double>::value, "shader specialization constant must be one of: bool, int, uint, float, double");
        
        SpecializationConstant& constant = m_constants.emplace_back();
        constant.name = name;
        constant.value = value;
        constant.size = sizeof(value);
    }
    
    void ShaderCompiler::generate_reflection_data(const std::vector<unsigned>& spirv) {
        spv_reflect::ShaderModule module = spv_reflect::ShaderModule(spirv);
        SpvReflectResult result;
        
        unsigned input_variable_count;
        result = module.EnumerateInputVariables(&input_variable_count, nullptr);
        
        std::vector<SpvReflectInterfaceVariable*> input_variables(input_variable_count);
        result = module.EnumerateInputVariables(&input_variable_count, input_variables.data());
    }
    
    std::string ShaderCompiler::read(const std::filesystem::path& path) {
        std::ifstream in(path, std::ios::in);
        if (!in.is_open()) {
            // throw utils::FormattedError("unable to open shader '{}'", m_filepath);
        }
        
        std::stringstream source;
        std::string line;
        
        // #include "filename" must be on its own line to be considered a valid include
        std::regex include_pattern("^\\s*#\\s*include\\s+\"([\\w\\d/.]+)\"\\s*$");
        std::smatch match;
        
        while (std::getline(in, line)) {
            if (std::regex_match(line, match, include_pattern)) {
                // match[0] is the whole string, match[1] is the name of the included file (group 1)
                std::string include = match[1].str();

                if (m_included_files.find(include) != m_included_files.end()) {
                    // File has already been included
                    continue;
                }
                
                m_included_files.emplace(include);
                source << read(include) << '\n';
            }
            else {
                source << line << '\n';
            }
        }
        
        return source.str();
    }

//
//    ShaderModule::ShaderModule(std::filesystem::path filepath) : filepath(std::move(filepath)),
//                                                                 last_modified_time(std::filesystem::last_write_time(this->filepath)),
//                                                                 stage(0),
//                                                                 module(nullptr) {
//        #ifdef NDEBUG
//            // Enable shader performance optimizations and warnings for Release builds
//            m_compile_options.SetOptimizationLevel(shaderc_optimization_level_performance);
//            m_compile_options.SetWarningsAsErrors();
//        #endif
//
//        // Shader type is determined by the file extension
//        std::filesystem::path extension = this->filepath.extension();
//        if (extension == ".vert") {
//            stage = VK_SHADER_STAGE_VERTEX_BIT;
//        }
//        else if (extension == ".frag") {
//            stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//        }
//        else if (extension == ".geom") {
//            stage = VK_SHADER_STAGE_GEOMETRY_BIT;
//        }
//        else if (extension == ".comp") {
//            stage = VK_SHADER_STAGE_COMPUTE_BIT;
//        }
//        else {
//            throw std::runtime_error(utils::format("unknown shader extension '{}'", extension.string()));
//        }
//    }
//
//    void ShaderModule::define_macro(const char* name, const char* value) {
//        m_compile_options.AddMacroDefinition(name, strlen(name), value, strlen(value));
//    }
//
//    bool ShaderModule::should_recompile() {
//        return std::filesystem::last_write_time(filepath) > last_modified_time;
//    }
//
//    utils::Result<VkPipelineShaderStageCreateInfo, ShaderModule::CompilationError> ShaderModule::compile() {
//
//    }
//
//
//
//    std::shared_ptr<ShaderModule> ShaderModule::compile(std::filesystem::path path) {
//        // Compile shader to SPIR-V bytecode
//
//        std::ifstream file(path, std::ios::ate | std::ios::binary);
//        if (!file.is_open()) {
//            throw std::runtime_error("failed to open shader: " + path.string());
//        }
//
//        std::streamsize file_size = file.tellg();
//        std::string source;
//        source.resize(file_size);
//
//        // Read data
//        file.seekg(0);
//        file.read(source.data(), file_size);
//        file.close();
//
//        shaderc::CompileOptions options { };
//        for (const auto&[directive, value] : preprocessor_definitions) {
//            options.AddMacroDefinition(directive, value);
//        }
//
//        // TODO: support multiple shader languages
//        shaderc_shader_kind type;
//        std::filesystem::path path = std::filesystem::path(filepath);
//        std::filesystem::path extension = path.extension();
//        if (extension == ".vert") {
//            type = shaderc_glsl_vertex_shader;
//        }
//        else if (extension == ".frag") {
//            type = shaderc_glsl_fragment_shader;
//        }
//        else if (extension == ".geom") {
//            type = shaderc_glsl_geometry_shader;
//        }
//        else if (extension == ".comp") {
//            type = shaderc_glsl_compute_shader;
//        }
//        else {
//            throw std::runtime_error("unknown shader type!");
//        }
//
//        shaderc::Compiler compiler { };
//        std::string filename = path.stem().u8string(); // Convert from wchar_t
//
//        // Function assumes entry point is 'main'
//        shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, type, filename.c_str());
//
//        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
//            throw std::runtime_error("failed to compile " + std::string(filepath) + ": " + result.GetErrorMessage());
//        }
//
//        std::vector<unsigned> spirv = { result.cbegin(), result.cend() };
//
//
//        // Reflect SPIR-V bytecode
//
//        return m_module;
//    }
//
//
//    ShaderModule::operator VkPipelineShaderStageCreateInfo() const {
//        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineShaderStageCreateInfo.html
//        VkPipelineShaderStageCreateInfo create_info { };
//
//        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//        create_info.stage = stage;
//        create_info.module = module;
//        create_info.pName = entry;
//        create_info.pSpecializationInfo = specialization_info;
//
//        return create_info;
//    }
    
}