
#include "sample.hpp"

class AmbientOcclusion : public Sample {
    public:
        AmbientOcclusion() : Sample("Ambient Occlusion") {
        }
        
        ~AmbientOcclusion() override {
        }
    
    private:
        void initialize_resources() override {
        }
        
        void update(double dt) override {
        }
        
        void record_command_buffers(unsigned image_index) override {
        }
        
        void destroy_resources() override {
        }
        
};

DEFINE_SAMPLE_MAIN(AmbientOcclusion);