
#include "sample.hpp"

#include <iostream> // std::cout, std::endl

class HelloTriangle final : public Sample {
    public:
        HelloTriangle() : Sample("Hello Triangle") {
        }
        
        ~HelloTriangle() override {
        }
        
    private:
        
};


int main() {
    HelloTriangle sample { };
    sample.initialize();
    
    while (sample.active()) {
        sample.run();
    }
    
    return 0;
}