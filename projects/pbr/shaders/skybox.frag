
#version 450

layout (set = 0, binding = 1) uniform samplerCube skybox;

layout (location = 0) in vec3 world_position;
layout (location = 0) out vec4 out_color;

// For visualizing different mip levels, if applicable
layout (push_constant) uniform PushConstants {
    uint mip_level;
} push_constants;

void main() {
    vec3 color = textureLod(skybox, normalize(world_position), push_constants.mip_level).xyz;

    // Tone mapping
    color = color / (color + vec3(1.0f));
    color = pow(color, vec3(1.0f / 2.2f));

    out_color = vec4(color, 1.0f);
}