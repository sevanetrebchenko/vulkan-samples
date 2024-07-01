
#version 450

layout (location = 0) in vec3 world_position;
layout (location = 1) in vec3 world_normal;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 camera; // Holds camera projection * view
    vec3 camera_position;
} global;

// Object material properties
layout (set = 1, binding = 1) uniform ObjectUniforms {
    vec3 diffuse;
    int flat_shaded;
    vec3 specular;
    float specular_exponent;
} object;

layout (location = 0) out vec4 out_color;

void main() {
    vec3 color = vec3(1.0f);

    out_color = vec4(color, 1.0f);
}