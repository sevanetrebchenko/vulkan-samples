
#version 450 core

layout (constant_id = 0) const int LIGHT_COUNT = 32;

layout (set = 1, binding = 2) uniform sampler2D albedo_map;
layout (set = 1, binding = 3) uniform sampler2D ao_map;
layout (set = 1, binding = 4) uniform sampler2D metallic_roughness_map;
layout (set = 1, binding = 5) uniform sampler2D normal_map;

// Per object uniforms
layout (set = 1, binding = 0) uniform ObjectUniforms {
    mat4 model;
    mat4 normal;
} object;

layout (set = 1, binding = 1) uniform MaterialUniforms {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} material;

layout(push_constant, std430) uniform PushConstants {
    vec3 asdf;
    float d;
} pcs;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(1.0f);
//    gl_FragDepth = 1.0f;
}