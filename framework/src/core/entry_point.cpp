
#include <iostream>
#include "core/entry_point.hpp"

int main(int argc, char** argv) {
    if (!create_application()) {
        std::cerr << "Failed to create application." << std::endl;
        return 1;
    }

    return 0;
}