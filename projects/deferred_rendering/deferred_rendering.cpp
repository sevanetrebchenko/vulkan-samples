
#include "sample.hpp"
#include "helpers.hpp"

class DeferredRendering final : public Sample {
    public:
        DeferredRendering() : Sample("Deferred Rendering") {
        }
        
        ~DeferredRendering() override {
        }
        
    private:
        void render() override {
        }
        
        void record_command_buffers(unsigned image_index) override {
        }
        
        void initialize_pipelines() override {
        }
        
        void destroy_pipelines() override {
        }
        
        void initialize_render_passes() override {
        }
        
        void destroy_render_passes() override {
        }
        
        void initialize_framebuffers() override {
        }
        
        void initialize_resources() override {
            Model bunny = load_model("assets/models/bunny_high_poly.obj");
        }
        
        void destroy_resources() override {
        }
        
};

DEFINE_SAMPLE_MAIN(DeferredRendering);
