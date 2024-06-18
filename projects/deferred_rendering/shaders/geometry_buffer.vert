
#version 450 core

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;

// Global uniforms (uploaded once per frame for all objects)
layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec3 camera_position;
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