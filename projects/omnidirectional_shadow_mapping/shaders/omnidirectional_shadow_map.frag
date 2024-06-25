
#version 450 core

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec3 camera_position; // Unused
    int debug_view; // Unused
    float camera_far_plane;
} global;

layout (set = 0, binding = 1) uniform LightUniforms {
    mat4 transform[6];
    vec3 position;
    float outer;
    vec3 direction;
    float inner;
    vec3 color;
} light;

layout (location = 0) in vec3 world_position;

void main() {
    // Store distance to light
    gl_FragDepth = length(world_position - light.position) / global.camera_far_plane;
}