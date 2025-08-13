
#include "vks/vulkan/pipeline_cache.hpp"
#include "vks/vulkan/shader.hpp"
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
        
        void load(RenderContext& context) override {
//            PipelineCache& pipeline_cache = context.pipeline_cache;
//
//            pipeline_cache.register_pipeline_template("gbuffer")
//                .add_shader_stage("shaders/sample.vert")
//                .add_shader_stage("shaders/sample.frag")
//                .configure_vertex_input([](VertexInput& input) {
//                    input.add_binding(0)
//                         .add_attribute("vertex_position")
//                         .add_attribute("vertex_normal");
//                })
//                .use_triangles()
//                .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT);
        }
        
        void update(float dt) override {
        }
        
        void render_frame(RenderContext& context) override {
        }
        
        void unload() override {
        }
        
    private:
    
};

DEFINE_SAMPLE_MAIN(Test);