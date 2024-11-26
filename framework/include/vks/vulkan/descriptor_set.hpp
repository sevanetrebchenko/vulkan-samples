
#ifndef DESCRIPTOR_SET_HPP
#define DESCRIPTOR_SET_HPP

#include "vks/vulkan/shader.hpp"

#include "utils/enum.hpp"
#include <vulkan/vulkan.h>

namespace vks {
    
    // Represents a single shader resource (uniform buffer, sampler, etc.)
    struct Descriptor {
        unsigned binding;
        VkDescriptorType type;
        unsigned count;
        VkShaderStageFlags stages; // Stages in which this resource is used
    };
    
    // Complete layout of a descriptor set
    // Separate from descriptor set instances (multiple descriptor sets can use the same layout)
    struct DescriptorSetLayout {
        unsigned index;
        std::vector<Descriptor> descriptors;
    };
    
    struct Uniform {
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
        DescriptorSetDescription& set_layout(DescriptorSetLayout layout);
        DescriptorSetDescription& enable_dynamic_descriptors(bool enable);
        
    };
    
    struct DescriptorSet {
        
        template <typename T>
        void set_uniform(const char* name, const T& data);
        
        std::shared_ptr<DescriptorSetLayout> layout;
        std::vector<Uniform> uniforms;
        
        // TODO: texture samplers
    };
    
}

#endif // DESCRIPTOR_SET_HPP
