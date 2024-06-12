
#include "sample.hpp"

class Deferred final : public Sample {
    public:
        Deferred() : Sample("Deferred Rendering") {
        }
        
        ~Deferred() override {
        }
        
    private:
        void record_command_buffers() override {
        }
};

DEFINE_SAMPLE_MAIN(Deferred);
