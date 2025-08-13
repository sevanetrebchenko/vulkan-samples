
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
        
        void add_shader_stage(VkShaderStageFlags stage);
        
        std::string name;
        ResourceType type;
        ComponentType component_type;
        VkShaderStageFlags stages;
        std::uint32_t size;
        std::uint32_t offset;
        std::uint32_t count = 1;
        std::vector<UniformDescriptor> members;
    };
    
    struct BufferDescriptor {
        enum class ResourceType : std::uint8_t {
            UniformBuffer, StorageBuffer
        };
        
        // Add a shader stage in which this resource is referenced
        // Applies recursively to all member descriptors (if applicable)
        void add_shader_stage(VkShaderStageFlags stage);
        
        std::string name;
        ResourceType resource;
        VkShaderStageFlags stages;
        std::uint32_t set;
        std::uint32_t binding;
        std::uint32_t size;
        std::vector<UniformDescriptor> members;
    };
    
    struct SamplerDescriptor {
        std::string name;
        VkShaderStageFlags stages;
        std::uint32_t set;
        std::uint32_t binding;
        std::uint32_t count = 1;
    };
    
    // For images / samplers, the sampled type represents the underlying type of the sampler
    // For example, this is an uint for usampler2D, or a float for sampler2D
    enum SampledType : std::uint8_t {
        Float, Integer, Unsigned
    };
    
    struct SampledImageDescriptor {
        enum class ResourceType : std::uint8_t {
            Texture2D, Texture3D, TextureCube, Texture2DArray,
        };
        
        std::string name;
        ResourceType resource;
        SampledType type;
        VkShaderStageFlags stages;
        std::uint32_t set;
        std::uint32_t binding;
        std::uint32_t count = 1;
    };
    
    struct CombinedImageSamplerDescriptor {
        enum class ResourceType : std::uint8_t {
            Sampler2D, Sampler3D, SamplerCube, Sampler2DArray,
        };
        
        std::string name;
        ResourceType resource;
        SampledType type;
        VkShaderStageFlags stages;
        std::uint32_t set;
        std::uint32_t binding;
        std::uint32_t count = 1;
    };
    
    struct StorageImageDescriptor {
        enum class ResourceType : std::uint8_t {
            Image2D, Image3D, ImageCube, Image2DArray,
        };

        std::string name;
        ResourceType resource;
        SampledType type;
        VkShaderStageFlags stages;
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
            ShaderCompiler(std::shared_ptr<Device> device);
            ~ShaderCompiler();
            
            utils::Result<ShaderModule> compile(const ShaderStageDescription& description) const;
            
        private:
            shaderc_shader_kind to_shaderc_type(VkShaderStageFlags stage) const;
            
            // Reflection API
            [[nodiscard]] ShaderReflectionData reflect(const SpvReflectShaderModule& module) const;
            [[nodiscard]] std::vector<UniformDescriptor> reflect_block_members(const SpvReflectBlockVariable& block) const;
            
            [[nodiscard]] UniformDescriptor::ResourceType reflect_uniform_type(const SpvReflectTypeDescription* type) const;
            [[nodiscard]] UniformDescriptor::ComponentType reflect_uniform_component_type(const SpvReflectTypeDescription* type) const;
            
            [[nodiscard]] CombinedImageSamplerDescriptor::ResourceType reflect_image_resource_type(const SpvReflectTypeDescription* type) const;
            [[nodiscard]] SampledImageDescriptor::ResourceType reflect_sampled_image_resource_type(const SpvReflectTypeDescription* type) const;
            [[nodiscard]] StorageImageDescriptor::ResourceType reflect_storage_image_resource_type(const SpvReflectTypeDescription* type) const;
            [[nodiscard]] SampledType reflect_sampled_type(const SpvReflectTypeDescription* type) const;
            
            std::shared_ptr<Device> m_device;
    };
    
    class ShaderCache {
        public:
        
        
        private:
    };
    
}

#endif // SHADER_HPP
