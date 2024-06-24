
#version 450

layout (location = 0) in vec3 vertex_position;

layout (set = 1, binding = 0) uniform ObjectTransforms {
    mat4 model;
    mat4 normal;
} object;

void main() {
    // Output vertices in world space
    gl_Position = object.model * vec4(vertex_position, 1.0f);
}
