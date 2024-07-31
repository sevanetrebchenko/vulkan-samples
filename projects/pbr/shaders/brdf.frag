
#version 450 core

layout (location = 0) in vec3 world_position;
layout (location = 1) in vec3 world_normal;
layout (location = 2) in vec2 uv;

// TBN matrix converts from tangent space to object space
layout (location = 3) in mat3 tbn;

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
layout (set = 0, binding = 7) uniform samplerCube prefiltered_environment_map;
layout (set = 0, binding = 8) uniform sampler2D brdf_lut;

layout (location = 0) out vec4 out_color;

const float pi = 3.141592f;

// Fresnel-Schlick
vec3 F(vec3 N, vec3 V, vec3 F0, float roughness) {
    float cos_theta = max(dot(N, V), 0.0f);
    return F0 + (max(vec3(1.0f - roughness), F0) - F0) * pow(clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f);
}

float geometry_shlick_ggx(float NdotV, float roughness) {
    float a = roughness;

    // This k has a different value when used for direct lighting from analytical lights
    // This computation is specifically for IBL
    float k = (a * a) / 2.0f;

    return NdotV / (NdotV * (1.0f - k) + k);
}

// The geometry function G approximates the surface area that is obstructed or overshadowed by neighboring microfacets, causing light rays to be occluded
// Higher levels of roughness results in a higher value of G, logically corresponding to a higher probability that surfaces shadow one another
float G(vec3 N, vec3 V, vec3 L, float roughness) {
    // Use Smith's method to account for both geometry obstruction (view direction, NdotV) and geometry shadowing (light direction, NdotL)
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    return geometry_shlick_ggx(NdotV, roughness) * geometry_shlick_ggx(NdotL, roughness);
}

// Trowbridge-Reitz GGX normal distribution function (D)
float D(vec3 N, vec3 H, float roughness) {
    // Uses Disney's reparametrization of alpha = roughness ^ 2
	float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NdotH = max(dot(N, H), 0.0f);
    float denominator = (NdotH * NdotH) * (alpha2 - 1.0f) + 1.0f;

    return alpha2 / (pi * denominator * denominator);
}


//// Get normal, tangent and bitangent vectors.
//vec3 get_normal() {
//    vec2 uv_dx = dFdx(-uv);
//    vec2 uv_dy = dFdy(-uv);
//
//    if (length(uv_dx) <= 1e-2) {
//      uv_dx = vec2(1.0, 0.0);
//    }
//
//    if (length(uv_dy) <= 1e-2) {
//      uv_dy = vec2(0.0, 1.0);
//    }
//
//    vec3 t_ = (uv_dy.t * dFdx(world_position) - uv_dx.t * dFdy(world_position)) / (uv_dx.s * uv_dy.t - uv_dy.s * uv_dx.t);
//    vec3 n, t, b, ng;
//
//    ng = normalize(cross(dFdx(world_position), dFdy(world_position)));
//    t = normalize(t_ - dot(ng, t_) * ng);
//    b = cross(ng, t);
//    return normalize(mat3(t, b, ng) * normalize(texture(normal_map, uv).rgb * 2.0 - vec3(1.0)));
//}

vec3 NormalTBN(vec3 textureNormal, vec3 worldPos, vec3 normal, vec2 texCoord)
{
	vec3 Q1 = dFdx(worldPos);
	vec3 Q2 = dFdy(worldPos);
	vec2 st1 = dFdx(texCoord);
	vec2 st2 = dFdy(texCoord);

	vec3 N = normalize(normal);
	vec3 T = normalize(Q1 * st2.y - Q2 * st1.y);
	vec3 B = normalize(cross(N, T));

	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * textureNormal);
}

vec3 get_normal() {
    vec3 normal = texture(normal_map, uv).rgb * 2.0f - 1.0f; // Remap from [0.0, 1.0] range in normal map to [-1.0, 1.0]
    vec3 N = NormalTBN(normal, world_position, world_normal, uv);
    return normalize(N);
}

void main() {
    if (global.debug_view == 1) {
        // Normals in the normal map are defined in the tangent space of the surface
        vec3 N = get_normal();
        vec3 V = normalize(global.camera_position - world_position);

        vec3 R = reflect(-V, N);

        // Model uses a combined metallic / roughness map
        // Metallic is sampled from the B channel, roughness is sampled from the G channel
        float metallic = texture(metallic_roughness_map, uv).b;
        float roughness = texture(metallic_roughness_map, uv).g;
        vec3 albedo = texture(albedo_map, uv).rgb;
        float ao = texture(ao_map, uv).r;

        vec3 F0 = mix(vec3(0.04), albedo, metallic);

        vec3 fresnel = F(N, V, F0, roughness);

        vec3 kS = fresnel;
        vec3 kD = 1.0f - kS;
        kD *= 1.0 - metallic;

        vec3 irradiance = texture(irradiance_map, N).rgb;
        vec3 diffuse = irradiance * albedo;

        int mipmap_levels = textureQueryLevels(prefiltered_environment_map);
        vec2 brdf = texture(brdf_lut, vec2(max(dot(N, V), 0.0f), roughness)).rg;

        vec3 specular_brdf = (F0 * brdf.x + brdf.y) * textureLod(prefiltered_environment_map, R, roughness * mipmap_levels).rgb;
        vec3 diffuse_brdf = kD * albedo * irradiance;

        vec3 color = diffuse_brdf + specular_brdf;

        // Tone mapping
        color = color / (color + vec3(1.0f));
        color = pow(color, vec3(1.0f / 2.2f));

        out_color = vec4(color, 1.0f);
    }
    else if (global.debug_view == 2) {
        // PBR output
        vec3 N = normalize(world_normal);
        vec3 V = normalize(global.camera_position - world_position);

        // Model uses a combined metallic / roughness map
        // Metallic is sampled from the B channel, roughness is sampled from the G channel
        float metallic = texture(metallic_roughness_map, uv).b;
        float roughness = texture(metallic_roughness_map, uv).g;
        vec3 albedo = texture(albedo_map, uv).rgb;
        float ao = texture(ao_map, uv).r;

        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        vec3 ks = F(N, V, F0, roughness);
        vec3 kd = 1.0f - ks;

        vec3 irradiance = texture(irradiance_map, N).rgb;
        vec3 diffuse = irradiance; // * albedo;

        vec3 color = diffuse; // (kd * diffuse) * ao;

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
        vec3 N = get_normal();
        out_color = vec4(N, 1.0f);

        // TODO: cycle between surface normals, normal map texture, and per-fragment normals

        // out_color = vec4(world_normal, 1.0f);
    }
}