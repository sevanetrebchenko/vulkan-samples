
#ifndef HASH_HPP
#define HASH_HPP

#include "vks/vulkan/shader.hpp"
#include <functional> // std::hash

namespace std {

    template <>
    struct hash<vks::ShaderStageDescription> {
        [[nodiscard]] std::size_t operator()(const vks::ShaderStageDescription& stage) const;
    };
    
    template <>
    struct hash<vks::ShaderConstant> {
        [[nodiscard]] std::size_t operator()(const vks::ShaderConstant& constant) const;
    };
 
}

#endif // HASH_HPP
