
#version 450

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 camera; // Holds camera projection * view
    vec3 camera_position;
} global;

layout (set = 1, binding = 0) uniform ObjectUniforms {
    mat4 model;
    mat4 normal;
} object;

layout (location = 0) out vec3 world_position;
layout (location = 1) out vec3 world_normal;

void main() {
    vec4 wp = object.model * vec4(vertex_position, 1.0f);

    world_position = wp.xyz;
    world_normal = normalize(object.normal * vec4(vertex_normal, 0.0f)).xyz;

    // M * V * P
    // Output fragment position in NDC space
    gl_Position = global.camera * wp;
}