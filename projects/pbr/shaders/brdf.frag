
#version 450 core

layout (location = 0) in vec3 world_position;
layout (location = 1) in vec3 world_normal;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(normalize(world_normal), 1.0f);
}