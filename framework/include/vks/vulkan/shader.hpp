
#ifndef SHADER_COMPILER_HPP
#define SHADER_COMPILER_HPP

#include "vks/vulkan/device.hpp"
#include <vulkan/vulkan.h>
#include <spirv_reflect.h>
#include <cstdint> // std::uint8_t
#include <filesystem> // std::filesystem::path, std::filesystem::file_time_type
#include <unordered_map> // std::unordered_map
#include <utility> // std::hash
#include <deque> // std::deque
#include <shared_mutex> // std::shared_mutex

namespace vks {
    
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
    
    struct ShaderResource {
        enum class Type {
            // Primitive types

            
            // Aggregate types
            Struct,
            Array,
            

            
            // Storage images
            Image2D, Image3D, Image2DArray,
            IImage2D, IImage3D, IImage2DArray,
            UImage2D, UImage3D, UImage2DArray,
            
            // Buffer types
            UniformBuffer,
            StorageBuffer,
            PushConstantBlock
        } type;
        
        std::string name;
        std::uint32_t binding;
        std::uint32_t set;
        
        std::uint32_t size;
        std::uint32_t offset; // Global offset into buffer
        
        // For array descriptors
        std::uint32_t count;
        std::uint32_t stride;
        
        // For nested members
        std::vector<ShaderResource> members;
    };
    
    struct ShaderModule {
        VkShaderModule module;
        std::vector<ShaderResource> resources;
    };
    
    class ShaderCache {
        public:
            ShaderCache(std::shared_ptr<Device> device);
            
            // Shader cache does lazy shader compilation - shaders are only compiled when they are needed
            const ShaderModule& get_shader_module(const ShaderStageDescription& description);
            void invalidate_shader_variants(const std::filesystem::path& filepath);
        
        private:
            struct CachedShaderModule : ShaderModule {
                std::atomic<bool> needs_recompilation;
                std::filesystem::file_time_type last_modified_time;
            };
            
            struct ShaderStageDescriptionHash {
                [[nodiscard]] std::size_t operator()(const ShaderStageDescription& description) const;
            };
            
            void compile_shader(const ShaderStageDescription& description);
            
            std::shared_ptr<Device> m_device;
            
            std::deque<CachedShaderModule> m_modules; // Shader modules are stored as a deque to avoid invalidating references on reallocation
            std::shared_mutex m_cache_mutex; // For threadsafe shader recompilation
            
            std::unordered_map<ShaderStageDescription, std::size_t, ShaderStageDescriptionHash> m_description_to_index;
            std::unordered_map<std::filesystem::path, std::vector<std::size_t>> m_filepath_to_index;
    };
    
}

#endif // SHADER_COMPILER_HPP
