
#version 450

layout (location = 0) in vec2 vertex_uv;

// Shader constants
layout (constant_id = 0) const int KERNEL_SIZE = 64;
layout (constant_id = 1) const float SAMPLE_RADIUS = 0.5f; // Sample radius of the effect

// Uniforms
layout (set = 0, binding = 0) uniform sampler2D positions;
layout (set = 0, binding = 1) uniform sampler2D normals;
layout (set = 0, binding = 2) uniform sampler2D noise;

layout (set = 0, binding = 3) uniform GlobalUniforms {
    mat4 projection; // Camera projection matrix
    vec4 samples[KERNEL_SIZE]; // Kernel
} globals;

// Output image has only one dimension to store the occlusion factor
layout (location = 0) out float out_occlusion;

void main() {
    out_occlusion = 0.0f;
}
