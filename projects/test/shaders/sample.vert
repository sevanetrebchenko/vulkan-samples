
#version 450 core

#include "shaders/globals.glsl"
#include "shaders/globals.glsl"

layout (constant_id = 1) const float ASDF = 32;

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;

struct A {
    int a;
};

struct B {
    A a;
    int b;
};

layout(push_constant, std430) uniform PushConstants {
    vec3 asdf;
    float opacity;
} pcs;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec3 camera_position; // Unused
    int debug_view; // Unused
    B b;
    vec4 arr[4][6];
} globals;

// Per object uniforms
layout (set = 1, binding = 0) uniform ObjectUniforms {
    mat4 model;
    mat4 normal;
} object;

layout (location = 0) out vec3 view_position;
layout (location = 1) out vec3 view_normal;

void main() {
    // Output position + normal in camera space for doing lighting calculations
    view_normal = vec3(normalize(globals.view * object.normal * vec4(vertex_normal, 0.0)));

    vec4 vp = globals.view * object.model * vec4(vertex_position, 1.0);

    view_position = vp.xyz;

    // M * V * P
    // Output fragment position in NDC space
    gl_Position = globals.projection * vp;
}