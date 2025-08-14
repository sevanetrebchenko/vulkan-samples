
#ifndef SHADER_HPP
#define SHADER_HPP

#include "vks/vulkan/device.hpp"
#include <utils/result.hpp>
#include <vulkan/vulkan.h>
#include <spirv_reflect.h>
#include <shaderc/shaderc.hpp>
#include <string> // std::string
#include <cstdint> // std::uint32_t
#include <vector> // std::vector

namespace vks {
    
    struct ShaderVariable {
        enum class ResourceType : std::uint8_t {
            Scalar, // Determined by component_type
            Vec2, Vec3, Vec4,
            Mat2, Mat3, Mat4,
            Struct
        };
        enum class ComponentType : std::uint8_t {
            Float, Double, Integer, Unsigned, Bool
        };
        
        std::string name;
        ResourceType resource_type;
        ComponentType component_type;
        VkShaderStageFlags stages;
        std::uint32_t size;
        std::uint32_t alignment; // Size + padding
        std::vector<std::uint32_t> dimensions;
    };
    
    struct UniformDescriptor : public ShaderVariable {
        static UniformDescriptor reflect(const SpvReflectBlockVariable& var);
        
        std::uint32_t offset;
        std::vector<UniformDescriptor> members;
    };
    
    struct BufferDescriptor {
        static BufferDescriptor reflect(const SpvReflectDescriptorBinding& binding);
        
        enum class ResourceType : std::uint8_t {
            UniformBuffer, StorageBuffer
        };
        
        std::string name;
        ResourceType resource;
        VkShaderStageFlags stages;
        std::uint32_t set;
        std::uint32_t binding;
        std::uint32_t size;
        std::uint32_t alignment;
        std::vector<UniformDescriptor> members;
    };
    
    struct SamplerDescriptor {
        static SamplerDescriptor reflect(const SpvReflectDescriptorBinding& binding);
        
        std::string name;
        VkShaderStageFlags stages;
        std::uint32_t set;
        std::uint32_t binding;
        std::vector<std::uint32_t> dimensions;
    };
    
    struct TextureDescriptor {
        // Extracts relevant descriptor information
        static TextureDescriptor reflect(const SpvReflectDescriptorBinding& binding);
        
        enum class ResourceType : std::uint8_t {
            Texture2D, Texture3D, TextureCube, Texture2DArray,
        };
        
        // Represents the underlying type of the sampler
        enum class SampleType : std::uint8_t {
            Float, Integer, Unsigned
        };
        
        std::string name;
        ResourceType resource_type;
        SampleType sample_type;
        VkShaderStageFlags stages;
        std::uint32_t set;
        std::uint32_t binding;
        std::vector<std::uint32_t> dimensions;
    };
    
    struct SampledTextureDescriptor {
        static SampledTextureDescriptor reflect(const SpvReflectDescriptorBinding& binding);
        
        enum class ResourceType : std::uint8_t {
            Sampler2D, Sampler3D, SamplerCube, Sampler2DArray,
        };
        // Represents the underlying type of the sampler
        enum class SampleType : std::uint8_t {
            Float, Integer, Unsigned
        };
        
        std::string name;
        ResourceType resource_type;
        SampleType sample_type;
        VkShaderStageFlags stages;
        std::uint32_t set;
        std::uint32_t binding;
        std::vector<std::uint32_t> dimensions;
    };
    
    struct StorageImageDescriptor {
        static StorageImageDescriptor reflect(const SpvReflectDescriptorBinding& binding);
        
        enum class ResourceType : std::uint8_t {
            Image2D, Image3D, ImageCube, Image2DArray,
        };

        // Represents the underlying type of the image
        enum class SampleType : std::uint8_t {
            Float, Integer, Unsigned
        };
        
        std::string name;
        ResourceType resource_type;
        SampleType sample_type;
        VkShaderStageFlags stages;
        std::uint32_t set;
        std::uint32_t binding;
        std::vector<std::uint32_t> dimensions;
    };
    
    struct InputAttachmentDescriptor {
        // TODO:
    };

    // Shader interface variables represent shader inputs / outputs
    // These have a very similar layout to shader uniforms, but must be flat (no nested structs)
    
    struct InterfaceBlockMember : public ShaderVariable {
        static InterfaceBlockMember reflect(const SpvReflectInterfaceVariable& var);
    };
    
    struct InterfaceVariableDescriptor : public ShaderVariable {
        static InterfaceVariableDescriptor reflect(const SpvReflectInterfaceVariable& var);
        
        std::uint32_t location;
        VkFormat format;
        std::vector<InterfaceBlockMember> members; // Only valid for interface blocks
    };
    
    struct ShaderReflectionData {
        std::vector<BufferDescriptor> buffers;
        
        std::vector<UniformDescriptor> push_constants;
        
        // Image / sampler resources
        std::vector<SamplerDescriptor> samplers;
        std::vector<TextureDescriptor> textures;
        std::vector<SampledTextureDescriptor> sampled_textures;
        std::vector<StorageImageDescriptor> storage_images;
        
        // Subpass resources
        std::vector<InputAttachmentDescriptor> input_attachments;
        
        std::vector<InterfaceVariableDescriptor> inputs;
        std::vector<InterfaceVariableDescriptor> outputs;
        
        // Miscellaneous metadata
        std::string entry_point;
        VkShaderStageFlagBits stage;
        
        // Compute (local) workgroup dimensions
        struct {
            std::uint32_t x;
            std::uint32_t y;
            std::uint32_t z;
        } workgroup_size;
    };
    
    struct ShaderModule {
        VkShaderModule module;
        ShaderReflectionData reflection_data;
    };
    

    
    struct ShaderStageDescription {
        // TODO: custom entry point + combined shader source files
        ShaderStageDescription(std::filesystem::path filepath);
        
        [[nodiscard]] bool operator==(const ShaderStageDescription& other) const;
        
        // Vulkan GLSL specialization constants
        // Must be one of: bool, integer, unsigned integer, float, double
        template <typename T, typename ...Ts>
        ShaderStageDescription& define_constants(const std::pair<std::string, T>& constant, const Ts&...);
        ShaderStageDescription& define_constant(const std::string& name, bool value);
        ShaderStageDescription& define_constant(const std::string& name, int value);
        ShaderStageDescription& define_constant(const std::string& name, unsigned value);
        ShaderStageDescription& define_constant(const std::string& name, float value);
        ShaderStageDescription& define_constant(const std::string& name, double value);
        
        VkShaderStageFlags stage;
        std::filesystem::path path;
        std::unordered_map<std::string, std::string> preprocessor_definitions;
    };
    
    class ShaderCompiler {
        public:
            ShaderCompiler(std::shared_ptr<Device> device);
            ~ShaderCompiler();
            
            [[nodiscard]] utils::Result<ShaderModule> compile(const ShaderStageDescription& description) const;
            
        private:
            [[nodiscard]] shaderc_shader_kind to_shaderc_type(VkShaderStageFlags stage) const;
            [[nodiscard]] ShaderReflectionData reflect(const SpvReflectShaderModule& module) const;
            
            std::shared_ptr<Device> m_device;
    };
    
    class ShaderCache {
        public:
        
        
        private:
    };
    
}

#endif // SHADER_HPP
