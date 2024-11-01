
#include "shader.hpp"
#include "renderpass.hpp"
#include "context.hpp"

int main() {
    using namespace vks;
    
    std::shared_ptr<Context> context = Context::Builder().set_application_name("Test")
                                                         .set_extent(1920, 1080)
                                                         .enable_extension("")
                                                         .enable_features({
                                                             .geometryShader = true,
                                                         })
                                                         .build();
    
    
    
    
    ShaderCompiler compiler("shaders/sample.vert");
    compiler.compile();

//    builder.set_extent(0, 0, width, height)
//           .add_shader_stage("shaders/sample.vert", [](ShaderCompiler& compiler) {
//           })
//           .add_shader_stage("asdf")
//           .build();
    
    return 0;
}