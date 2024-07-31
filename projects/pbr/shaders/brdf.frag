
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

vec3 calculate_normal() {
    // This calculation follows the calculation of vertex tangents, except using derivatives of the interpolated world position / texture coordinates
    // Derivation: https://web.archive.org/web/20110708081637/http://www.terathon.com/code/tangent.html

    // Assuming points p0, p1, and p2 with respective texture coordinates uv0, uv1, and uv2, the calculation is as follows:
    // vec3 e1 = p1 - p0;
    // vec3 e2 = p2 - p0;

    vec3 e1 = dFdx(world_position);
    vec3 e2 = dFdy(world_position);

    // vec2 st1 = uv1 - uv0;
    // vec2 st2 = uv2 - uv0;

    vec2 uv_dx = dFdx(uv);
	vec2 uv_dy = dFdy(uv);

    vec3 tangent = normalize((uv_dy.t * e1 - uv_dx.t * e2) / (uv_dx.s * uv_dy.t - uv_dy.s * uv_dx.t));
    // vec3 bitangent = f * (st2.s * e1 - st1.s * e2);

    // The result of this can be used to offset the geometry normal by the tangent space normal texture (normal mapping) by creating a TBN matrix

	vec3 N = normalize(world_normal); // Geometry normal
	vec3 T = normalize(tangent - dot(N, tangent) * N); // Re-orthogonalize
	vec3 B = cross(N, T);

    // Texture normal (tangent space) remapped from [0.0, 1.0] to [-1.0, 1.0]
    vec3 texture_normal = normalize(texture(normal_map, uv).rgb * 2.0 - vec3(1.0));
    return normalize(mat3(T, B, N) * texture_normal);
}

void main() {
    if (global.debug_view == 1) {
        // Normals in the normal map are defined in the tangent space of the surface
        vec3 N = calculate_normal(); // normalize(world_normal);
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
        vec3 N = calculate_normal();
        out_color = vec4(N, 1.0f);

        // TODO: cycle between surface normals, normal map texture, and per-fragment normals

        // out_color = vec4(world_normal, 1.0f);
    }
}