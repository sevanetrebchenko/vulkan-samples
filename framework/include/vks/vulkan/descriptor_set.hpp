
#ifndef DESCRIPTOR_SET_HPP
#define DESCRIPTOR_SET_HPP

#include "vks/vulkan/shader.hpp"

#include "utils/enum.hpp"
#include <vulkan/vulkan.h>

namespace vks {
    
    struct Image {
        // The number of dimensions in the image
        // 1 - sampler1D
        // 2 - sampler2D
        // 3 - sampler3D
        // 4 - samplerCube
        std::size_t dimensionality;
        
        [[nodiscard]] bool is_depth() const;
        [[nodiscard]] bool is_array() const;
        [[nodiscard]] bool is_multisampled() const;
        
        std::size_t flags;
    };
    
    struct Member {
        const char* name;
        
        std::size_t size;
        
        // The alignment of a struct member may differ from its size due to alignment requirements
        std::size_t alignment;
        
        // Offset is specified relative to the parent
        std::size_t offset;
        
        // Only used for array types
        // dimensions.size() is the number of dimensions
        // dimensions[n] is the size of the nth dimension
        std::vector<std::size_t> dimensions;
        std::size_t stride;
        
        // Only used for structs
        std::vector<Member> children;
        
        // Only used for vec/mac types
        std::uint8_t rows;
        std::uint8_t columns;
        bool is_column_major;
    };
    
    struct Resource {
        const char* name;
        
        // Only used for array types
        // dimensions.size() is the number of dimensions
        // dimensions[n] is the size of the nth dimension
        std::vector<std::size_t> dimensions;
        
        // Only used for structs
        std::vector<Resource> members;
        
        VkFormat format;
        
        std::size_t size;
        std::size_t alignment;
        std::size_t offset;
        
        std::size_t stride;
        
        std::size_t flags;
    };
    
    // Represents a single shader resource (uniform buffer, sampler, etc.)
    struct Descriptor {
        unsigned binding;
        VkDescriptorType type;
        Resource resource;
        VkShaderStageFlags stages; // Stages in which this resource is used
    };
    
    // Describes the layout of the descriptor set
    struct DescriptorSetDescription {
        DescriptorSetDescription(unsigned set);
        
        DescriptorSetDescription& add_descriptor(unsigned binding, VkDescriptorType, unsigned count, VkShaderStageFlags stage);
        DescriptorSetDescription& enable_dynamic_descriptors(bool enable);
        
        unsigned set;
        std::vector<Descriptor> descriptors;
        
        bool dynamic;
    };
    
    DescriptorSetDescription& DescriptorSetDescription::add_descriptor(unsigned int binding, VkDescriptorType type, unsigned int count, VkShaderStageFlags stage) {
        // Check to see if the descriptor set layout already contains an entry for this binding
        bool found = false;
        
        for (Descriptor& descriptor : descriptors) {
            if (descriptor.binding == binding) {
                // Bindings of the same index must reference the same resource for a given descriptor set
                // Ensure that this resource can be used across multiple shader stages, as it was first encountered in a shader stage that is different from the one currently being parsed
                descriptor.stages |= stage;
                found = true;
                break;
            }
        }
        
        if (!found) {
            // Descriptor with this binding index has not been encountered yet
            descriptors.push_back({
                .binding = binding,
                .type = (VkDescriptorType) type,
                .count = count,
                .stages = stage
            });
        }
        
        return *this;
    }
    
    struct DescriptorSet {
        
        template <typename T>
        void set_uniform(const char* name, const T& data);
        
        void set_uniform(std::size_t binding, const char* name, void* data);
        
        void set_dynamic(bool dynamic);
        
        
        DescriptorSetDescription description;
        
        VkDescriptorSet set;
        VkDescriptorSetLayout layout;
        
        void* buffer;
        
        // TODO: texture samplers
    };
    
}

#endif // DESCRIPTOR_SET_HPP
