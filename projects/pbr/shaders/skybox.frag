
#version 450

layout (set = 0, binding = 1) uniform samplerCube skybox;

layout (location = 0) in vec3 world_position;
layout (location = 0) out vec4 out_color;

void main() {
    vec3 color = texture(skybox, normalize(world_position)).xyz;

    // Tone mapping
    color = color / (color + vec3(1.0f));
    color = pow(color, vec3(1.0f / 2.2f));

    out_color = vec4(color, 1.0f);
}