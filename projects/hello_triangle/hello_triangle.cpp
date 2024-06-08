
#include "sample.hpp"

#include <iostream> // std::cout, std::endl

int main() {
    Sample sample("Hello Triangle");
    sample.initialize();
    
    while (sample.active()) {
    }
    
    return 0;
}