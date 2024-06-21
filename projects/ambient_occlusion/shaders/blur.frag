
#version 450

layout (location = 0) in vec2 vertex_uv;

// Shader uniforms
layout (set = 0, binding = 0) uniform sampler2D ambient_occlusion;

// Output image has only one dimension to store the blurred occlusion factor
layout (location = 0) out float out_occlusion;

void main() {
    vec2 size = 1.0f / vec2(textureSize(ambient_occlusion, 0));
    float result = 0.0;

    int dimension = 2;

    // Blur in a dimension x dimension kernel
    for (int x = -dimension; x < dimension; ++x) {
        for (int y = -dimension; y < dimension; ++y) {
            vec2 offset = vec2(float(x), float(y)) * size;
            result += texture(ambient_occlusion, vertex_uv + offset).r;
        }
    }

    out_occlusion = result / (dimension * dimension * 4);
}