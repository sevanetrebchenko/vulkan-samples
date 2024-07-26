
#version 450 core

layout (location = 0) in vec3 world_position;
layout (location = 1) in vec3 world_normal;
layout (location = 2) in vec2 uv;

layout (set = 0, binding = 1) uniform sampler2D albedo;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(texture(albedo, uv).xyz, 1.0f);
}