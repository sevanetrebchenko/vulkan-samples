
#include "hash.hpp"
#include "vks/vulkan/utility.hpp"
#include "vks/vulkan/shader.hpp"

namespace std {
    
    std::size_t hash<vks::ShaderStageDescription>::operator()(const vks::ShaderStageDescription& stage) const {
        std::size_t seed = 0;
        
        // Shader stage is not hashed because it is determined from the shader file extension
        vks::hash_combine(seed, stage.path);
        
        // Include names of all preprocessor definitions in the hash
        for (const auto& [name, value] : stage.preprocessor_definitions) {
            vks::hash_combine(seed, name);
            vks::hash_combine(seed, value);
        }
        
        // Hash shader specialization constants
        for (const vks::ShaderConstant& constant : stage.constants) {
            vks::hash_combine(seed, constant);
        }

        return seed;
    }
    
    std::size_t hash<vks::ShaderConstant>::operator()(const vks::ShaderConstant& constant) const {
        std::size_t seed = 0;
        
        // Explicitly hash std::string_view since hashing of const char* hashes the pointer address and not the string contents
        vks::hash_combine(seed, constant.name);
        
        // This only compares the 'active' union bytes and avoids issues with padding and/or differences in endianness
        vks::hash_combine(seed, reinterpret_cast<const char*>(&constant.value), constant.size);
        
        return seed;
    }
    
}
