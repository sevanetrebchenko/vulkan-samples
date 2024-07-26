
#version 450

layout (set = 0, binding = 1) uniform samplerCube skybox;

layout (location = 0) in vec3 world_position;
layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(texture(skybox, normalize(world_position)).xyz, 1.0f);
}