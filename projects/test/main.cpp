
#include "shader_compiler.hpp"

int main() {
    using namespace vks;
    ShaderCompiler compiler = ShaderCompiler("shaders/sample.vert");
    compiler.compile();
    return 0;
}