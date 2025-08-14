
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
        
        // Reflect bindings
        std::uint32_t binding_count;
        spvReflectEnumerateDescriptorBindings(&module, &binding_count, nullptr);
        
        std::vector<SpvReflectDescriptorBinding*> bindings(binding_count);
        spvReflectEnumerateDescriptorBindings(&module, &binding_count, bindings.data());
        
        for (const SpvReflectDescriptorBinding* binding : bindings) {
            switch (binding->descriptor_type) {
                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                    reflection_data.samplers.emplace_back(SamplerDescriptor::reflect(*binding));
                    break;
                
                case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    reflection_data.sampled_textures.emplace_back(SampledTextureDescriptor::reflect(*binding));
                    break;
                    
                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    reflection_data.textures.emplace_back(TextureDescriptor::reflect(*binding));
                    break;
                    
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    reflection_data.storage_images.emplace_back(StorageImageDescriptor::reflect(*binding));
                    break;
                    
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    reflection_data.buffers.emplace_back(BufferDescriptor::reflect(*binding));
                    break;
                
                // TODO: currently unhandled
                case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                    break;
                
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                    utils::logging::warning("Texture buffers are not (currently) supported");
                    break;
            }
        }
        
        // Reflect push constants
        std::uint32_t push_constant_count;
        spvReflectEnumeratePushConstantBlocks(&module, &push_constant_count, nullptr);
        if (push_constant_count > 0) {
            std::vector<SpvReflectBlockVariable*> push_constant_blocks(push_constant_count);
            spvReflectEnumeratePushConstantBlocks(&module, &push_constant_count, push_constant_blocks.data());

            // Should only be one push constant block
            const SpvReflectBlockVariable* block = push_constant_blocks[0];
            reflection_data.push_constants.emplace_back(UniformDescriptor::reflect(*block));
        }
        
        // Reflect input variables
        std::uint32_t input_count;
        spvReflectEnumerateInputVariables(&module, &input_count, nullptr);
        
        std::vector<SpvReflectInterfaceVariable*> inputs(input_count);
        spvReflectEnumerateInputVariables(&module, &input_count, inputs.data());
        
        for (const SpvReflectInterfaceVariable* input : inputs) {
            if (input->built_in != -1) {
                // Skip built-in variables
                continue;
            }
            
            reflection_data.inputs.emplace_back(InterfaceVariableDescriptor::reflect(*input));
        }
        
        // Reflect output variables
        std::uint32_t output_count;
        spvReflectEnumerateOutputVariables(&module, &output_count, nullptr);
        
        std::vector<SpvReflectInterfaceVariable*> outputs(output_count);
        spvReflectEnumerateOutputVariables(&module, &output_count, outputs.data());
        
        for (const SpvReflectInterfaceVariable* output : outputs) {
            if (output->built_in == -1) {
                // Skip built-in variables
                continue;
            }
            
            reflection_data.outputs.push_back(InterfaceVariableDescriptor::reflect(*output));
        }
        
        // Reflect compute workgroup dimensions (comes from entry point)
        const SpvReflectEntryPoint* entry_point = spvReflectGetEntryPoint(&module, module.entry_point_name);
        if (module.shader_stage == SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT) {
            // Only valid for compute shaders
            reflection_data.workgroup_size.x = entry_point->local_size.x;
            reflection_data.workgroup_size.y = entry_point->local_size.y;
            reflection_data.workgroup_size.z = entry_point->local_size.z;
        }
        
        // Reflect shader metadata
        reflection_data.entry_point = module.entry_point_name;
        reflection_data.stage = (VkShaderStageFlagBits) module.shader_stage;
        
        return reflection_data;
    }
    
    std::vector<UniformDescriptor> reflect_block_members(const SpvReflectBlockVariable& block) {
        std::vector<UniformDescriptor> members;
        
        for (uint32_t i = 0; i < block.member_count; ++i) {
            members.emplace_back(UniformDescriptor::reflect(block.members[i]));
        }
        
        return std::move(members);
    }
    
    template <typename T, typename U>
    inline T reflect_as(const U& var) {
        T descriptor { };
        descriptor.name = var.name ? var.name : "";
        
        const SpvReflectTypeDescription* type = var.type_description;
        
        // Determine size and alignment
        if constexpr (requires { var.size; var.padded_size; }) {
            // SpvReflectBlockVariable struct contains size + alignment directly
            descriptor.size = var.size;
            descriptor.alignment = var.padded_size;
            
            // Provided values automatically account for arrays
            if (var.array.dims_count > 0) {
                descriptor.dimensions.reserve(var.array.dims_count);
                for (uint32_t i = 0; i < var.array.dims_count; ++i) {
                    descriptor.dimensions.push_back(var.array.dims[i]);
                }
            }
        }
        else {
            // Size and alignment must be calculated from the SpvReflectInterfaceVariable type
            std::uint32_t size = 0;
            std::uint32_t alignment = 0;
            
            if (type->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL) {
                // bool is 4 bytes in shaders
                size = 4;
                alignment = 4;
            }
            else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
                std::uint32_t width = type->traits.numeric.scalar.width / 8; // scalar.width contains width in bits (32bit)
                size = width;
                alignment = width;
            }
            else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
                std::uint32_t width = type->traits.numeric.scalar.width / 8;  // Handles both float (32bit) and double (64bit) values
                size = width;
                alignment = width;
            }
            else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
                std::uint32_t component_size = type->traits.numeric.scalar.width / 8;
                std::uint32_t count = type->traits.numeric.vector.component_count;
                
                size = component_size * count;
                alignment = (count == 3) ? component_size * 4 : size;  // vec3 aligns like vec4
            }
            else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
                std::uint32_t component_size = type->traits.numeric.scalar.width / 8;
                std::uint32_t columns = type->traits.numeric.vector.component_count;
                std::uint32_t rows = type->traits.numeric.matrix.row_count;
                
                size = component_size * columns * rows;
                alignment = component_size * ((rows == 3) ? 4 : rows);  // Columns are aligned like vectors (above)
            }
            
            descriptor.alignment = alignment;  // Array alignment (below) is also element alignment
            
            // Size and alignment must also be applied to arrays
            if (var.array.dims_count > 0) {
                std::uint32_t element_count = 1;
                for (uint32_t i = 0; i < var.array.dims_count; ++i) {
                    element_count *= var.array.dims[i];
                }
                descriptor.size = size * element_count;
            }
            else {
                descriptor.size = size;
            }
        }

        // Handle arrays
        if (var.array.dims_count > 0) {
            descriptor.dimensions.resize(var.array.dims_count);
            for (uint32_t i = 0; i < var.array.dims_count; ++i) {
                descriptor.dimensions[i] = var.array.dims[i];
            }
        }
        
        // Determine resource type
        if (type->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
            switch (type->traits.numeric.vector.component_count) {
                case 2:
                    descriptor.resource_type = ShaderVariable::ResourceType::Vec2;
                    break;

                case 3:
                    descriptor.resource_type = ShaderVariable::ResourceType::Vec3;
                    break;
                    
                case 4:
                    descriptor.resource_type = ShaderVariable::ResourceType::Vec4;
                    break;
                    
                default:
                    descriptor.resource_type = ShaderVariable::ResourceType::Scalar;
                    break;
            }
        }
        else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
            switch (type->traits.numeric.matrix.column_count) {
                case 2:
                    descriptor.resource_type = ShaderVariable::ResourceType::Mat2;
                    break;
                    
                case 3:
                    descriptor.resource_type = ShaderVariable::ResourceType::Mat3;
                    break;

                // TODO: handle non-uniformly sized matrices
                case 4:
                default:
                    descriptor.resource_type = ShaderVariable::ResourceType::Mat4;
                    break;
            }
        }
        else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT) {
            descriptor.resource_type = ShaderVariable::ResourceType::Struct;
        }
        else {
            descriptor.resource_type = ShaderVariable::ResourceType::Scalar;
        }
        
        // Determine component type
        if (type->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL) {
            descriptor.component_type = ShaderVariable::ComponentType::Bool;
        }
        else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
            if (type->traits.numeric.scalar.signedness) {
                descriptor.component_type = ShaderVariable::ComponentType::Integer;
            }
            else {
                descriptor.component_type = ShaderVariable::ComponentType::Unsigned;
            }
        }
        else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
            if (type->traits.numeric.scalar.width == 64) {
                descriptor.component_type = ShaderVariable::ComponentType::Double;
            }
            else {
                descriptor.component_type = ShaderVariable::ComponentType::Float;
            }
        }
        
        return std::move(descriptor);
    }
    
    UniformDescriptor UniformDescriptor::reflect(const SpvReflectBlockVariable& var) {
        UniformDescriptor descriptor = reflect_as<UniformDescriptor>(var);
        
        if (var.member_count > 0) {
            // Recursively process nested members
            descriptor.resource_type = ResourceType::Struct;
            descriptor.members = reflect_block_members(var);
        }
        
        return std::move(descriptor);
    }
    
    BufferDescriptor BufferDescriptor::reflect(const SpvReflectDescriptorBinding& binding) {
        BufferDescriptor descriptor { };
        
        descriptor.name = binding.name ? binding.name : "";
        descriptor.resource = binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ? BufferDescriptor::ResourceType::UniformBuffer : BufferDescriptor::ResourceType::StorageBuffer;
        descriptor.set = binding.set;
        descriptor.binding = binding.binding;
        descriptor.size = binding.block.size;
        descriptor.alignment = binding.block.padded_size;
        descriptor.members = reflect_block_members(binding.block);
        
        return std::move(descriptor);
    }
    
    SamplerDescriptor SamplerDescriptor::reflect(const SpvReflectDescriptorBinding& binding) {
        SamplerDescriptor descriptor { };
        
        descriptor.name = binding.name ? binding.name : "";
        descriptor.set = binding.set;
        descriptor.binding = binding.binding;
        
        descriptor.dimensions.reserve(binding.array.dims_count);
        for (uint32_t i = 0; i < binding.array.dims_count; ++i) {
            descriptor.dimensions.push_back(binding.array.dims[i]);
        }
        
        return std::move(descriptor);
    }

    // Textures / images share most reflection logic
    template <typename T>
    inline T reflect_as(const SpvReflectDescriptorBinding& binding) {
        T descriptor { };
        
        descriptor.name = binding.name ? binding.name : "";
        descriptor.set = binding.set;
        descriptor.binding = binding.binding;
        
        descriptor.dimensions.reserve(binding.array.dims_count);
        for (uint32_t i = 0; i < binding.array.dims_count; ++i) {
            descriptor.dimensions.push_back(binding.array.dims[i]);
        }
        
        const SpvReflectTypeDescription* type = binding.type_description;
        
        // Determine component type
        if (type->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
            descriptor.sample_type = T::SampleType::Float;
        }
        else { // } if (type->type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
            descriptor.sample_type = type->traits.numeric.scalar.signedness ? T::SampleType::Integer : T::SampleType::Unsigned;
        }
        
        return std::move(descriptor);
    }
    
    TextureDescriptor TextureDescriptor::reflect(const SpvReflectDescriptorBinding& binding) {
        TextureDescriptor descriptor = reflect_as<TextureDescriptor>(binding);
        
        // Extract resource type from image traits
        switch (binding.image.dim) {
            case SpvDim2D:
                descriptor.resource_type = binding.image.arrayed ? ResourceType::Texture2DArray : ResourceType::Texture2D;
                break;
                
            case SpvDim3D:
                descriptor.resource_type = ResourceType::Texture3D;
                break;
                
            case SpvDimCube:
                descriptor.resource_type = ResourceType::TextureCube;
                break;
        }
        
        return std::move(descriptor);
    }
    
    SampledTextureDescriptor SampledTextureDescriptor::reflect(const SpvReflectDescriptorBinding& binding) {
        SampledTextureDescriptor descriptor = reflect_as<SampledTextureDescriptor>(binding);
        
        // Extract resource type from image traits
        switch (binding.image.dim) {
            case SpvDim2D:
                descriptor.resource_type = binding.image.arrayed ? ResourceType::Sampler2DArray : ResourceType::Sampler2D;
                break;
                
            case SpvDim3D:
                descriptor.resource_type = ResourceType::Sampler3D;
                break;
                
            case SpvDimCube:
                descriptor.resource_type = ResourceType::SamplerCube;
                break;
        }
        
        return std::move(descriptor);
    }
    
    StorageImageDescriptor StorageImageDescriptor::reflect(const SpvReflectDescriptorBinding& binding) {
        StorageImageDescriptor descriptor = reflect_as<StorageImageDescriptor>(binding);
        
        // Extract resource type from image traits
        switch (binding.image.dim) {
            case SpvDim2D:
                descriptor.resource_type = binding.image.arrayed ? ResourceType::Image2DArray : ResourceType::Image2D;
                break;
                
            case SpvDim3D:
                descriptor.resource_type = ResourceType::Image3D;
                break;
                
            case SpvDimCube:
                descriptor.resource_type = ResourceType::ImageCube;
                break;
        }
        
        return std::move(descriptor);
    }
    
    InterfaceBlockMember InterfaceBlockMember::reflect(const SpvReflectInterfaceVariable& var) {
        return reflect_as<InterfaceBlockMember>(var);
    }
    
    InterfaceVariableDescriptor InterfaceVariableDescriptor::reflect(const SpvReflectInterfaceVariable& var) {
        InterfaceVariableDescriptor descriptor = reflect_as<InterfaceVariableDescriptor>(var);
        
        descriptor.location = var.location;
        descriptor.format = (VkFormat) var.format;
        
        // Interface variables may be a flat struct (no nested struct members)
        if (var.member_count) {
            descriptor.resource_type = ShaderVariable::ResourceType::Struct;
            descriptor.members.resize(var.member_count);
            
            for (std::uint32_t i = 0; i < var.member_count; ++i) {
                descriptor.members[i] = InterfaceBlockMember::reflect(var.members[i]);
            }
        }
        
        return std::move(descriptor);
    }
    
    ShaderStageDescription::ShaderStageDescription(std::filesystem::path filepath) : path(filepath) {
        stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    
}