
#include "shader.hpp"
#include "renderpass.hpp"

#include "vks/vulkan/device.hpp"

int main() {
    using namespace vks;

    DeviceDescription device_description { };
    device_description.set_application_name("My Vulkan Application")
                      .set_extent(1920, 1080);

    std::shared_ptr<Device> device = Device::instance();
    device->initialize(device_description);
    
    ShaderStageDescription vertex_shader { };
    vertex_shader.set_filepath("shaders/sample.vert");
    vertex_shader.define_constant("ASDF", 64);
    
    ShaderStageDescription fragment_shader { };
    fragment_shader.set_filepath("shaders/sample.frag");
    fragment_shader.define_constant("LIGHT_COUNT", 16);
    
    VertexInputDescription vertex_input { };
    vertex_input.add_attribute(0, "vertex_position");
    vertex_input.add_attribute(0, "vertex_normal");
    vertex_input.add_attribute(0, "vertex_uv");
    vertex_input.set_binding_stride(0, 64);
    vertex_input.set_binding_input_rate(0, VK_VERTEX_INPUT_RATE_VERTEX);

    GraphicsPipelineDescription graphics_pipeline { };
    graphics_pipeline.add_shader_stage(vertex_shader);
    graphics_pipeline.add_shader_stage(fragment_shader);
    graphics_pipeline.set_vertex_input(vertex_input);
    
    device->create_graphics_pipeline(graphics_pipeline);
    
    return 0;
}