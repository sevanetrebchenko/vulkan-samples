
#version 450 core

layout (location = 0) in vec3 world_position;
layout (location = 1) in vec3 world_normal;
layout (location = 2) in vec2 uv;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec3 camera_position;
    int debug_view;
} global;

layout (set = 0, binding = 1) uniform sampler2D albedo;
layout (set = 0, binding = 2) uniform sampler2D ao;
layout (set = 0, binding = 3) uniform sampler2D emissive;
layout (set = 0, binding = 4) uniform sampler2D roughness;
layout (set = 0, binding = 5) uniform sampler2D normal;

layout (location = 0) out vec4 out_color;

void main() {
    if (global.debug_view == 1) {
        // PBR + IBL output
        out_color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else if (global.debug_view == 2) {
        // PBR output
        out_color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else if (global.debug_view == 3) {
        // Albedo (debug)
        out_color = vec4(texture(albedo, uv).xyz, 1.0f);
    }
    else if (global.debug_view == 4) {
        // Ambient occlusion (debug)
        out_color = vec4(texture(ao, uv).xyz, 1.0f);
    }
    else if (global.debug_view == 5) {
        // Emissive (debug)
        out_color = vec4(texture(emissive, uv).xyz, 1.0f);
    }
    else if (global.debug_view == 6) {
        // Roughness (debug)
        out_color = vec4(texture(roughness, uv).xyz, 1.0f);
    }
    else if (global.debug_view == 7) {
        // Normals (debug)
        out_color = vec4(texture(normal, uv).xyz, 1.0f);
    }
}