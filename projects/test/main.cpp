
#include "shader.hpp"
#include "renderpass.hpp"

int main() {
    using namespace vks;
    
    unsigned width = 1920, height = 1080;
    RenderPass::Builder builder { };
    
    ShaderCompiler compiler("shaders/sample.vert");
    compiler.compile();

//    builder.set_extent(0, 0, width, height)
//           .add_shader_stage("shaders/sample.vert", [](ShaderCompiler& compiler) {
//           })
//           .add_shader_stage("asdf")
//           .build();
    
    return 0;
}