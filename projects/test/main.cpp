
#include "vks/vulkan/pipeline_cache.hpp"
#include "vks/vulkan/render_graph.hpp"
#include "vks/sample.hpp"

using namespace vks;

class Test final : public Sample {
    public:
        Test() : Sample("test") {
        }
        
        ~Test() override {
        }
        
        SampleRequirements get_requirements() override {
            SampleRequirements requirements { };
            return requirements;
        }
        
        void load() override {
        }
        
        void update(float dt) override {
        
        }
        
        void render_frame(RenderContext& context) override {
        }
        
        void unload() override {
        }
        
    private:
    
};


int main() {

    using namespace vks;

    Test test { };
    test.initialize();
    
//    PipelineCache pc { };
//
//    pc.register_pipeline_template("gbuffer")
//      .configure_shader_stage("shaders/sample.vert", [](ShaderStageDescription& description) {
//          description.define_constant("LIGHT_COUNT", 64);
//      })
//      .configure_vertex_input([](VertexInput& input) {
//          input.add_binding(0)
//                   .add_attribute("position")
//                   .add_attribute("normal")
//                   .add_attribute("uv");
//      })
//   ;
    
//    RenderGraph rg { };
    
    
    return 0;
}