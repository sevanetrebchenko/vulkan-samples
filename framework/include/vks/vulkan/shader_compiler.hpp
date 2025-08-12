
#ifndef SHADER_COMPILER_HPP
#define SHADER_COMPILER_HPP

#include <string> // std::string
#include <cstdint> // std::uint32_t
#include <vector> // std::vector
#include <vulkan/vulkan.h>

namespace vks {
    
    struct ShaderModule {
    
    };
    
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
    
    class ShaderCompiler {
        public:
        
        private:
        
        
    };
    
}

#endif // SHADER_COMPILER_HPP
