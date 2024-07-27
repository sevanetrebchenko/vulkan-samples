
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

layout (set = 0, binding = 1) uniform sampler2D albedo_map;
layout (set = 0, binding = 2) uniform sampler2D ao_map;
layout (set = 0, binding = 3) uniform sampler2D emissive_map;
layout (set = 0, binding = 4) uniform sampler2D metallic_roughness_map;
layout (set = 0, binding = 5) uniform sampler2D normal_map;
layout (set = 0, binding = 6) uniform samplerCube irradiance_map;

layout (location = 0) out vec4 out_color;

vec3 fresnel_schlick(vec3 N, vec3 V, vec3 F0, float roughness) {
    float cos_theta = max(dot(N, V), 0.0f);
    return F0 + (max(vec3(1.0f - roughness), F0) - F0) * pow(clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f);
}

void main() {
    if (global.debug_view == 1) {
        // PBR + IBL output
        out_color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else if (global.debug_view == 2) {
        // PBR output
        vec3 N = normalize(world_normal);
        vec3 V = normalize(world_position - global.camera_position);

        // Model uses a combined metallic / roughness map
        // Metallic is sampled from the B channel, roughness is sampled from the G channel
        float metallic = texture(metallic_roughness_map, uv).b;
        float roughness = texture(metallic_roughness_map, uv).g;
        vec3 albedo = texture(albedo_map, uv).rgb;
        float ao = texture(ao_map, uv).r;

        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        vec3 ks = fresnel_schlick(N, V, F0, roughness);
        vec3 kd = 1.0f - ks;

        vec3 irradiance = texture(irradiance_map, N).rgb;
        vec3 diffuse = irradiance * albedo;

        vec3 color = (kd * diffuse) * ao;

        // Tone mapping
        color = color / (color + vec3(1.0f));
        color = pow(color, vec3(1.0f / 2.2f));

        out_color = vec4(color, 1.0f);
    }
    else if (global.debug_view == 3) {
        // Albedo (debug)
        out_color = vec4(texture(albedo_map, uv).xyz, 1.0f);
    }
    else if (global.debug_view == 4) {
        // Ambient occlusion (debug)
        out_color = vec4(texture(ao_map, uv).xyz, 1.0f);
    }
    else if (global.debug_view == 5) {
        // Emissive (debug)
        out_color = vec4(texture(emissive_map, uv).xyz, 1.0f);
    }
    else if (global.debug_view == 6) {
        // Metallic / roughness (debug)
        out_color = vec4(texture(metallic_roughness_map, uv).xyz, 1.0f);
    }
    else if (global.debug_view == 7) {
        // Normals (debug)
        out_color = vec4(texture(normal_map, uv).xyz, 1.0f);
    }
}