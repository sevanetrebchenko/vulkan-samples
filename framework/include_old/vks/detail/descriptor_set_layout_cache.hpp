
#ifndef DESCRIPTOR_SET_LAYOUT_CACHE_HPP
#define DESCRIPTOR_SET_LAYOUT_CACHE_HPP

#include "vks/vulkan/descriptor_set.hpp"

#include <unordered_map> // std::unordered_map
#include <memory> // std::shared_ptr

namespace vks {
    
    struct DescriptorSetLayoutCache {
        static DescriptorSetLayoutCache& instance();
        
        std::shared_ptr<DescriptorSet> get_descriptor_set(const DescriptorSetLayout& layout);
        
        std::unordered_map<std::size_t, std::shared_ptr<DescriptorSetLayout>> layouts;
    };
    
}


#endif // DESCRIPTOR_SET_LAYOUT_CACHE_HPP
