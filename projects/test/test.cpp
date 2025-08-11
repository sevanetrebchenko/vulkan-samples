
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

DEFINE_SAMPLE_MAIN(Test);