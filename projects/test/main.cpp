
#include "shader.hpp"
#include "renderpass.hpp"

#include "vks/vulkan/context.hpp"

int main() {
    using namespace vks;

    ShaderStageDescription vertex_shader_description { "shaders/sample.vert" };
    vertex_shader_description.define_macro("TEST", "1");
    
    PipelineDescription pipeline_description { };
    pipeline_description.add_shader_stage(vertex_shader_description);
    
    Context context { };
    context.create_pipeline(pipeline_description);
    
//    std::shared_ptr<Context> context = Context::Builder().set_application_name("My Vulkan Application")
//                                                         .set_extent(1920, 1080)
//                                                         .build();
    
//    builder.set_extent(0, 0, width, height)
//           .add_shader_stage("shaders/sample.vert", [](ShaderCompiler& compiler) {
//           })
//           .add_shader_stage("asdf")
//           .build();
    
    return 0;
}