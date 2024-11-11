
#ifndef DESCRIPTOR_SET_HPP
#define DESCRIPTOR_SET_HPP

#include "vks/vulkan/shader.hpp"

#include "utils/enum.hpp"
#include <vulkan/vulkan.h>

namespace vks {

    enum class ResourceType {
        UniformBuffer = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        StorageBuffer = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    };
    
    // Represents a single shader resource (uniform buffer, sampler, etc.)
    struct Resource {
        unsigned binding;
        ResourceType type;
        unsigned count;
        ShaderStage stages; // Stages in which this resource is used
    };
    
    // Complete layout of a descriptor set
    // Separate from descriptor set instances (multiple descriptor sets can use the same layout)
    struct DescriptorSetLayout {
        unsigned index;
        std::vector<Resource> resources;
    };
    
    enum class UniformType {
        Bool,
        Integer,
        Float,
        Double,
        
        Vector,
        Matrix,
        
        Struct,
        Array
    };
    
    struct Uniform {
        UniformType type; // Underlying uniform type
        
        const char* name;
        unsigned offset; // Global offset into the uniform buffer, accounting for alignment
        unsigned size;
        
        // Used only for structs
        std::vector<Uniform> members;
        
        // Used only for arrays
        std::vector<unsigned> capacity; // Number of elements in the array
        unsigned dimensions; // For multidimensional arrays, eg. float[2][3] has 2 dimensions
        unsigned stride;
    };
    
    struct DescriptorSetDescription {
    
    };
    
    struct DescriptorSet {
    };
    
}

#endif // DESCRIPTOR_SET_HPP
