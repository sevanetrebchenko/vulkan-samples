
#version 450

layout (location = 0) in vec3 vertex_position; // Input directly from SSBO, in world space
layout (location = 1) in vec3 vertex_normal; // Input directly from SSBO, in world_space
layout (location = 2) in vec2 vertex_uv;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 camera; // Holds camera projection * view
    vec3 camera_position;
} global;

layout (location = 0) out vec3 world_position;
layout (location = 1) out vec3 world_normal;
layout (location = 2) out vec2 uv;

void main() {
    uv = vertex_uv;
    world_position = vertex_position;
    world_normal = normalize(vertex_normal);

    gl_Position = global.camera * vec4(vertex_position, 1.0f);
}