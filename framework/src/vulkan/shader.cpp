
#include "vks/vulkan/shader.hpp"
#include <utils/logging.hpp>
#include <shaderc/shaderc.hpp>

namespace vks {
    
    std::string load_shader(const std::filesystem::path& filepath) {
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
    
    struct ShaderIncluder final : public shaderc::CompileOptions::IncluderInterface {
        // Returns the contents of 'file'
        shaderc_include_result* GetInclude(const char* file, shaderc_include_type type, const char* source, std::size_t depth) override {
            // TODO: <standard> includes for default / shared shaders?
            std::filesystem::path filepath(file);
//            if (type == shaderc_include_type_relative) {
//                // Reconstruct global filepath from a relative include
//                filepath = std::filesystem::path(source).parent_path() / filepath;
//            }

            shaderc_include_result& result = include_results.emplace_back();

            if (std::filesystem::exists(filepath)) {
                // Ensure that a shader is only included once
                bool found = false;
                for (const Include& include : includes) {
                    if (include.filepath == filepath.string()) {
                        // File has already been included (note: this is not an error)
                        result.source_name = include.filepath.c_str();
                        result.source_name_length = include.filepath.length();

                        // Do not include this file multiple times
                        result.content = nullptr;
                        result.content_length = 0u;

                        found = true;
                    }
                }

                if (!found) {
                    // Register new include
                    Include& include = includes.emplace_back();
                    include.filepath = filepath.string();
                    include.contents = load_shader(filepath);

                    result.source_name = include.filepath.c_str();
                    result.source_name_length = include.filepath.length();
                    result.content = include.contents.c_str();
                    result.content_length = include.contents.length();
                }
            }
            else {
                // Provided filepath does not exist
                // For a failed inclusion, source_name should be empty
                result.source_name = nullptr;
                result.source_name_length = 0u;

                // For a failed inclusion, content contains the error message
                result.content = "";
                result.content_length = strlen(result.content);
            }

            return &result;
        }

        void ReleaseInclude(shaderc_include_result* data) override {
            // Nothing to do here
        }

        struct Include {
            std::string filepath; // Stored as std::string and not std::filesystem::path to be able to directly reference filepath.c_str() in the return of GetInclude
            std::string contents;
        };

        // 'includes' contains only unique includes
        // Contents of included files must remain valid from when GetInclude is invoked to retrieve the include contents to when ReleaseInclude is invoked to release them
        std::vector<Include> includes;
        std::vector<shaderc_include_result> include_results;
    };
    
    ShaderCompiler::ShaderCompiler(std::shared_ptr<Device> device) : m_device(std::move(device)) {
    }
    
    ShaderCompiler::~ShaderCompiler() = default;
    
    utils::Result<ShaderModule> ShaderCompiler::compile(const ShaderStageDescription& description) const {
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
        
        shaderc::Compiler compiler { };
        shaderc_shader_kind type = to_shaderc_type(description.stage);
        std::string source = load_shader(description.path);
        
        const std::string& path = description.path.string();
        shaderc_compilation_status status;
        
        shaderc::PreprocessedSourceCompilationResult preprocessed = compiler.PreprocessGlsl(source.c_str(), source.size(), type, path.c_str(), options);
        status = preprocessed.GetCompilationStatus();
        if (status != shaderc_compilation_status_success) {
            return utils::Result<ShaderModule>::NOT_OK("");
        }
        
        source = std::string(preprocessed.begin(), preprocessed.end());
        
        // Compile to SPIR-V
        // TODO: function assumes shader entry point is main
        shaderc::SpvCompilationResult compiled = compiler.CompileGlslToSpv(source, type, path.c_str(), options);
        status = compiled.GetCompilationStatus();
        if (status != shaderc_compilation_status_success) {
            return utils::Result<ShaderModule>::NOT_OK("");
        }
        
        std::vector<std::uint32_t> spv = { compiled.cbegin(), compiled.cend() };
        std::size_t size = spv.size() * sizeof(std::uint32_t); // Size in bytes
        
        // Generate shader
        VkShaderModuleCreateInfo shader_module_create_info { };
        shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_create_info.codeSize = size;
        shader_module_create_info.pCode = spv.data();
        
        VkShaderModule shader { };
        CHECK_CALL(vkCreateShaderModule, *m_device, &shader_module_create_info, nullptr, &shader);

        // Reflect SPIR-V bytecode
        SpvReflectShaderModule spv_module;
        SpvReflectResult reflected = spvReflectCreateShaderModule(size, spv.data(), &spv_module);
        if (reflected != SPV_REFLECT_RESULT_SUCCESS) {
            vkDestroyShaderModule(*m_device, shader, nullptr);
            return utils::Result<ShaderModule>::NOT_OK("");
        }

        return utils::Result<ShaderModule>::OK(shader, reflect(spv_module));
    }
    
    shaderc_shader_kind ShaderCompiler::to_shaderc_type(VkShaderStageFlags stage) const {
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
    
    ShaderReflectionData ShaderCompiler::reflect(const SpvReflectShaderModule& module) const {
        ShaderReflectionData reflection_data { };
        std::string entry_point = module.entry_point_name ? module.entry_point_name : "main";
        
        std::uint32_t binding_count;
        spvReflectEnumerateDescriptorBindings(&module, &binding_count, nullptr);
        
        std::vector<SpvReflectDescriptorBinding*> bindings(binding_count);
        spvReflectEnumerateDescriptorBindings(&module, &binding_count, bindings.data());
        
        for (SpvReflectDescriptorBinding* binding : bindings) {
            VkShaderStageFlags stage = (VkShaderStageFlags) module.shader_stage;
            
            switch (binding->descriptor_type) {
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
                    BufferDescriptor& buffer = reflection_data.buffers.emplace_back();
                    buffer.name = binding->name;
                    buffer.resource = binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ? BufferDescriptor::ResourceType::UniformBuffer : BufferDescriptor::ResourceType::StorageBuffer;
                    buffer.set = binding->set;
                    buffer.binding = binding->binding;
                    buffer.size = binding->block.size;
                    buffer.members = reflect_block_members(binding->block);
                    
                    buffer.add_shader_stage(stage);
                    break;
                }
                
                case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
                    CombinedImageSamplerDescriptor& sampler = reflection_data.combined_image_samplers.emplace_back();
                    sampler.name = binding->name;
                    sampler.set = binding->set;
                    sampler.binding = binding->binding;
                    sampler.stages = stage;
                    sampler.count = binding->count;
                    
                    const SpvReflectTypeDescription* type = binding->type_description;
                    sampler.resource = reflect_image_resource_type(type);
                    sampler.type = reflect_sampled_type(type);
                    
                    break;
                }
                
//                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
//                    SampledImageDescriptor image;
//                    image.name = binding->name;
//                    image.set = binding->set;
//                    image.binding = binding->binding;
//                    image.stage = stage;
//                    image.count = binding->count;
//
//                    image.type = reflect_sampled_image_dimension(binding->image);
//                    image.component_type = reflect_component_type(binding->image);
//
//                    reflection.sampled_images.push_back(std::move(image));
//                    break;
//                }
//
//                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: {
//                    SamplerDescriptor sampler;
//                    sampler.name = binding->name;
//                    sampler.set = binding->set;
//                    sampler.binding = binding->binding;
//                    sampler.stage = stage;
//                    sampler.count = binding->count;
//                    reflection.samplers.push_back(std::move(sampler));
//                    break;
//                }
//
//                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
//                    StorageImageDescriptor image;
//                    image.name = binding->name;
//                    image.set = binding->set;
//                    image.binding = binding->binding;
//                    image.stage = stage;
//                    image.count = binding->count;
//
//                    image.type = reflect_storage_image_dimension(binding->image);
//                    image.component_type = reflect_component_type(binding->image);
//
//                    reflection.storage_images.push_back(std::move(image));
//                    break;
//                }
                
                case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
                    break;
                }
                
                case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: {
                    break;
                }
                
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                    utils::logging::warning("Texture buffers are not (currently) supported");
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    utils::logging::warning("Dynamic uniform / storage buffers are not (currently) supported");
            }
        }
    
        // Process push constants
//        uint32_t push_count = 0;
//        spvReflectEnumeratePushConstantBlocks(&module, &push_count, nullptr);
//        if (push_count > 0) {
//            std::vector<SpvReflectBlockVariable*> push_blocks(push_count);
//            spvReflectEnumeratePushConstantBlocks(&module, &push_count, push_blocks.data());
//
//            // Should only be one push constant block
//            auto* block = push_blocks[0];
//            reflection.push_constants = reflect_block_members(*block, stage);
//        }
        
        return reflection_data;
    }
    
    std::vector<UniformDescriptor> ShaderCompiler::reflect_block_members(const SpvReflectBlockVariable& block) const {
        std::vector<UniformDescriptor> members;
        
        for (uint32_t i = 0; i < block.member_count; ++i) {
            const SpvReflectBlockVariable& member = block.members[i];
            
            UniformDescriptor& uniform = members.emplace_back();
            uniform.name = member.name ? member.name : "";
            uniform.size = member.size;
            uniform.offset = member.offset;
            uniform.count = 1;
            
            if (member.array.dims_count > 0) {
                uniform.count = member.array.dims[0];
            }
            
            // Arbitrary, no ComponentType for struct types
            uniform.component_type = reflect_component_type(member.type_description);
            
            if (member.member_count > 0) {
                // Recursively process nested struct members
                uniform.type = UniformDescriptor::ResourceType::Struct;
                uniform.members = reflect_block_members(member);
            }
            else {
                uniform.type = reflect_type(member.type_description);
            }
        }
        
        return std::move(members);
    }
    
    UniformDescriptor::ComponentType ShaderCompiler::reflect_uniform_component_type(const SpvReflectTypeDescription* type) const {
        if (type->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL) {
            return UniformDescriptor::ComponentType::Bool;
        }
        else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
            return type->traits.numeric.scalar.signedness ? UniformDescriptor::ComponentType::Integer : UniformDescriptor::ComponentType::Unsigned;
        }
        return UniformDescriptor::ComponentType::Float;
    }
    
    UniformDescriptor::ResourceType ShaderCompiler::reflect_uniform_type(const SpvReflectTypeDescription* type) const {
        switch (type->type_flags) {
            case SPV_REFLECT_TYPE_FLAG_BOOL:
            case SPV_REFLECT_TYPE_FLAG_INT:
            case SPV_REFLECT_TYPE_FLAG_FLOAT:
                return UniformDescriptor::ResourceType::Scalar;
                
            case SPV_REFLECT_TYPE_FLAG_VECTOR:
                switch (type->traits.numeric.vector.component_count) {
                    case 2:
                        return UniformDescriptor::ResourceType::Vec2;
                    case 3:
                        return UniformDescriptor::ResourceType::Vec3;
                    case 4:
                        return UniformDescriptor::ResourceType::Vec4;
                    default:
                        return UniformDescriptor::ResourceType::Scalar;
                }
                
            case SPV_REFLECT_TYPE_FLAG_MATRIX:
                if (type->traits.numeric.matrix.column_count == 2) {
                    return UniformDescriptor::ResourceType::Mat2;
                }
                else if (type->traits.numeric.matrix.column_count == 3) {
                    return UniformDescriptor::ResourceType::Mat3;
                }
                return UniformDescriptor::ResourceType::Mat4;
                
            case SPV_REFLECT_TYPE_FLAG_STRUCT:
                return UniformDescriptor::ResourceType::Struct;
                
            default:
                utils::logging::warning("Potentially unhandled resource type");
                return UniformDescriptor::ResourceType::Scalar;
        }
    }
    
    CombinedImageSamplerDescriptor::ResourceType ShaderCompiler::reflect_image_resource_type(const SpvReflectTypeDescription* type) const {
    
    }
    
    SampledImageDescriptor::ResourceType ShaderCompiler::reflect_sampled_image_resource_type(const SpvReflectTypeDescription* type) const {
    
    }
    
    StorageImageDescriptor::ResourceType ShaderCompiler::reflect_storage_image_resource_type(const SpvReflectTypeDescription* type) const {
    
    }
    
    SampledType ShaderCompiler::reflect_sampled_type(const SpvReflectTypeDescription* type) const {
        // Combined image samplers have the following type flags set:
        //   - SPV_REFLECT_TYPE_FLAG_EXTERNAL_IMAGE
        //   - SPV_REFLECT_TYPE_FLAG_REF
        //   - SPV_REFLECT_TYPE_FLAG_FLOAT or SPV_REFLECT_TYPE_FLAG_INT (depending on the declaration)
        // This represents a sampler that references an external image
        
        // In order to determine the underlying sampled type (ComponentType), we'll use a mix of these type flags and the sampler's numeric type traits,
        // which contains information about sampler signedness (for distinguishing between Integer and samplers)
        SpvReflectTypeFlags flags = type->type_flags;
        
        if (flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
            // Float sampler (sampler2D, samplerCube, etc.)
            return SampledType::Float;
        }
        else { // if (flags & SPV_REFLECT_TYPE_FLAG_INT) {
            // Integer sampler (isampler2D, usamplerCube, etc.)
            return type->traits.numeric.scalar.signedness ? SampledType::Integer : SampledType::Unsigned;
        }
    }
    
    void BufferDescriptor::add_shader_stage(VkShaderStageFlags stage) {
        stages |= stage;
        for (UniformDescriptor& member : members) {
            member.add_shader_stage(stage);
        }
    }
    
    void UniformDescriptor::add_shader_stage(VkShaderStageFlags stage) {
        stages |= stage;
        for (UniformDescriptor& member : members) {
            member.add_shader_stage(stage);
        }
    }
    
    ShaderStageDescription::ShaderStageDescription(std::filesystem::path filepath) : path(filepath) {
        stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
}