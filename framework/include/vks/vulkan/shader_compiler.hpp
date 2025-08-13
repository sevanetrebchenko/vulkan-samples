
#ifndef SHADER_COMPILER_HPP
#define SHADER_COMPILER_HPP

#include <utils/result.hpp>
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>
#include <string> // std::string
#include <cstdint> // std::uint32_t
#include <vector> // std::vector

namespace vks {
    
    struct UniformDescriptor {
        enum class ResourceType : std::uint8_t {
            Scalar, // Determined by component_type
            Vec2, Vec3, Vec4,
            Mat2, Mat3, Mat4,
            Struct
        };
        enum class ComponentType : std::uint8_t {
            Float, Integer, Unsigned, Bool
        };
        
        std::string name;
        ResourceType type;
        ComponentType component_type;
        VkShaderStageFlags stage;
        std::uint32_t size;
        std::uint32_t offset;
        std::uint32_t count = 1;
        std::vector<UniformDescriptor> members;
    };
    
    struct BufferDescriptor {
        enum class ResourceType : std::uint8_t {
            UniformBuffer, StorageBuffer
        };
        
        std::string name;
        ResourceType type;
        VkShaderStageFlags stage;
        std::uint32_t set;
        std::uint32_t binding;
        std::uint32_t size;
        std::vector<UniformDescriptor> members;
    };
    
    struct SamplerDescriptor {
        std::string name;
        VkShaderStageFlags stage;
        std::uint32_t set;
        std::uint32_t binding;
        std::uint32_t count = 1;
    };
    
    struct SampledImageDescriptor {
        enum class ResourceType : std::uint8_t {
            Texture2D, Texture3D, TextureCube, Texture2DArray,
        };
        enum class ComponentType : std::uint8_t {
            Float, Integer, Unsigned, Bool
        };
        
        std::string name;
        ResourceType type;
        ComponentType component_type;
        VkShaderStageFlags stage;
        std::uint32_t set;
        std::uint32_t binding;
        std::uint32_t count = 1;
    };
    
    struct CombinedImageSamplerDescriptor {
        enum class ResourceType : std::uint8_t {
            Sampler2D, Sampler3D, SamplerCube, Sampler2DArray,
        };
        enum class ComponentType : std::uint8_t {
            Float, Integer, Unsigned, Bool
        };
        
        std::string name;
        ResourceType type;
        ComponentType component_type;
        VkShaderStageFlags stage;
        std::uint32_t set;
        std::uint32_t binding;
        std::uint32_t count = 1;
    };
    
    struct StorageImageDescriptor {
        enum class ResourceType : std::uint8_t {
            Image2D, Image3D, ImageCube, Image2DArray,
        };
        enum class ComponentType : std::uint8_t {
            Float, Integer, Unsigned, Bool
        };
        std::string name;
        ResourceType type;
        ComponentType component_type;
        VkShaderStageFlags stage;
        std::uint32_t set;
        std::uint32_t binding;
        std::uint32_t count = 1;
    };
    
    struct InputAttachmentDescriptor {
        // TODO:
    };

    struct ShaderReflectionData {
        std::vector<BufferDescriptor> buffers;
        
        std::vector<UniformDescriptor> push_constants;
        
        // Image/sampler resources
        std::vector<SamplerDescriptor> samplers;
        std::vector<SampledImageDescriptor> sampled_images;
        std::vector<CombinedImageSamplerDescriptor> combined_image_samplers;
        std::vector<StorageImageDescriptor> storage_images;
        
        // Subpass resources
        std::vector<InputAttachmentDescriptor> input_attachments;
    };
    
    struct ShaderModule {
        VkShaderModule module;
        ShaderReflectionData reflection_data;
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
        std::uint8_t size;

        std::string name;
    };
    
    struct ShaderStageDescription {
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
        
        // Preprocessor definitions
        template <typename ...Ts>
        ShaderStageDescription& define_macros(const std::pair<std::string, std::string>& macro, const Ts&...);
        ShaderStageDescription& define_macro(const std::string& name, const std::string& value);
        
        VkShaderStageFlags stage;
        std::filesystem::path path;
        std::vector<ShaderConstant> constants;
        std::unordered_map<std::string, std::string> preprocessor_definitions;
    };
    
    class ShaderCompiler {
        public:
            // TODO: incorporate result for error message
            ShaderModule compile(const ShaderStageDescription& description) const;
            
        private:
            VkShaderModule compile(const std::vector<std::uint32_t>& spv);
            ShaderReflectionData generate_reflection_data(const std::vector<std::uint32_t>& spv);
            
            shaderc::Compiler m_compiler;
    };
    
}

#endif // SHADER_COMPILER_HPP
