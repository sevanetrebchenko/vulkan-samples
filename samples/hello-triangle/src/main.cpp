
#include <iostream>
#include "framework.hpp"
#include <cassert>
#include "core/assert.hpp"

bool create_application() {
    // vks::Sample sample { };
    
    for (int i = 0; i < 10; ++i) {
        std::cout << (1 << i) << std::endl;
    }
    
    std::uint32_t feature = 5;
    ASSERT(feature && !(feature & (feature - 1u)), "hello");
    
    std::cout << "Hello from hello-triangle sample." << std::endl;
    return false;
}