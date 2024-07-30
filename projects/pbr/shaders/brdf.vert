
#version 450 core

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec4 vertex_tangent;
layout (location = 3) in vec2 vertex_uv;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec3 camera_position; // Unused
    int debug_view; // Unused
} global;

// Per object uniforms
layout (set = 1, binding = 0) uniform ObjectUniforms {
    mat4 model;
    mat4 normal; // World from object
} object;

layout (location = 0) out vec3 world_position;
layout (location = 1) out vec3 world_normal;
layout (location = 2) out vec2 uv;
layout (location = 3) out vec3 tangent;
layout (location = 4) out vec3 bitangent;

void main() {
    vec4 wp = object.model * vec4(vertex_position, 1.0f);
    world_position = wp.xyz;

    world_normal = normalize(object.normal * vec4(vertex_normal, 0.0f)).xyz;

    // Re-orthogonalize using the Gram-Schmidt algorithm
    tangent = normalize(object.normal * vertex_tangent).xyz;
    tangent = normalize(tangent - dot(tangent, world_normal) * world_normal);

    bitangent = cross(world_normal, tangent);

    uv = vertex_uv;

    // M * V * P
    // Output fragment position in NDC space
    gl_Position = global.projection * global.view * wp;
}