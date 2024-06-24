
#version 450

layout (location = 0) in vec3 vertex_position;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec3 camera_position; // Unused
    int debug_view; // Unused
} global;

layout (set = 1, binding = 0) uniform ObjectTransforms {
    mat4 model;
    mat4 normal;
} object;

// Output vertex information to world space
layout (location = 0) out vec3 world_position;

void main() {
    // Lighting calculations are done in view space
    vec4 wp = object.model * vec4(vertex_position, 1.0f);
    world_position = wp.xyz;

    gl_Position = global.projection * global.view * wp;
}
