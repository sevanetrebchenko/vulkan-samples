
#ifndef RENDERPASS_HPP
#define RENDERPASS_HPP

#include "types.hpp"
#include "shader.hpp"
#include <vulkan/vulkan.h>
#include <filesystem> // std::filesystem
#include <memory> // std::shared_ptr
#include <functional>

namespace vks {
    
    class RenderPass {
        public:
            class Builder;
            
        private:
        
    };
    
    class RenderPass::Builder {
        public:
            Builder();
            Builder(Builder&& other) noexcept;
            
            ~Builder();
            
            Builder& set_extent(unsigned x, unsigned y, unsigned width, unsigned height);

            // set_shader_stage attempts to determine shader stage from the file extension (if not explicitly specified)
            Builder& add_shader_stage(std::filesystem::path filepath, ShaderStage stage);
            Builder& add_shader_stage(std::filesystem::path filepath, std::function<void(ShaderCompiler&)>&& fn);
            
            void build();
            
        private:
            std::shared_ptr<RenderPass> m_handle;
    };

//    class RenderPass::Builder::ShaderCompiler final : vks::ShaderCompiler {
//        public:
//            // Defines the same public interface as the standalone ShaderCompiler
//            // The main difference lies in the return value, as this is intended to allow for chaining calls together when building render passes
//            // Note: member function shadowing is intentional (implementation forwards the call to the base vks::ShaderCompiler anyway)
//
//            ~ShaderCompiler();
//
//            // Compile-time (macro) definition
//            ShaderCompiler& define_macro(const char* name, const char* value);
//
//            // Vulkan GLSL specialization constants
//            // Can be one of: bool, integer, unsigned integer, float, double
//            template <typename T>
//            ShaderCompiler& define_constant(const char* name, T value);
//
//            RenderPass::Builder& compile();
//
//        private:
//            // This ShaderCompiler should only be instantiated by the RenderPass::Builder
//            friend class RenderPass::Builder;
//            ShaderCompiler(RenderPass::Builder& builder, std::filesystem::path&& filepath, ShaderStage stage);
//    };
    


    
}

#endif // RENDERPASS_HPP
