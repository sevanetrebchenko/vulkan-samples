
#version 450

layout (location = 0) in vec3 vertex_position;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec3 camera_position; // Unused
    int debug_view; // Unused
} global;

layout (location = 0) out vec3 world_position;

void main() {
    vec4 wp = vec4(vertex_position, 1.0f); // Skybox is a unit cube rendered without depth information
    mat4 view = mat4(mat3(global.view)); // Take away camera position transformation to render skybox from the origin of the world

    world_position = wp.xyz;
    world_position.xz *= -1.0f;
    gl_Position = global.projection * view * wp;
}