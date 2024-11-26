//
//#ifndef RENDERPASS_HPP
//#define RENDERPASS_HPP
//
//#include "types.hpp"
//#include "shader.hpp"
//#include <vulkan/vulkan.h>
//#include <filesystem> // std::filesystem
//#include <memory> // std::shared_ptr
//#include <functional>
//
//namespace vks {
//
//    class RenderPass {
//        public:
//            class Builder;
//
//        private:
//
//    };
//
//    class RenderPass::Builder {
//        public:
//            Builder();
//            ~Builder();
//
//            Builder& set_extent(unsigned x, unsigned y, unsigned width, unsigned height);
//
//            // set_shader_stage attempts to determine shader stage from the file extension (if not explicitly specified)
//            Builder& add_shader_stage(std::filesystem::path filepath);
//            Builder& add_shader_stage(std::filesystem::path filepath, std::function<void(ShaderCompiler&)>&& fn);
//
//            Builder& add_shader_stage(std::filesystem::path filepath, ShaderStage stage);
//            Builder& add_shader_stage(std::filesystem::path filepath, ShaderStage stage, std::function<void(ShaderCompiler&)>&& fn);
//
//            std::shared_ptr<RenderPass> build();
//
//        private:
//            std::shared_ptr<RenderPass> m_handle;
//    };
//
//}
//
//#endif // RENDERPASS_HPP
