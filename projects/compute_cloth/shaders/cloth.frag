
#version 450

layout (location = 0) in vec3 world_position;
layout (location = 1) in vec3 world_normal;
layout (location = 2) in vec2 uv;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 camera; // Holds camera projection * view
    vec3 camera_position;
} global;

layout (set = 0, binding = 1) uniform LightUniforms {
    vec3 position; // Point light
    float radius;
} light;

layout (set = 1, binding = 1) uniform ObjectUniforms {
    vec3 diffuse;
    int flat_shaded;
    vec3 specular;
    float specular_exponent;
} object;

layout (location = 0) out vec4 out_color;

vec3 calculate_face_normal(vec3 position) {
    vec3 dx = dFdx(position);
    vec3 dy = dFdy(position);
    return normalize(cross(-dx, dy));
}

void main() {
    vec3 color = vec3(0.0f);

    vec3 V = normalize(global.camera_position - world_position);
    vec3 N = object.flat_shaded == 1 ? calculate_face_normal(world_position) : normalize(world_normal);
    vec3 L = normalize(light.position - world_position);

    // Ambient
    color += object.diffuse * 0.2f;

    // Diffuse
    float lambert = max(dot(N, L), 0.0f);
    vec3 light_color = vec3(1.0f);
    color += light_color * object.diffuse * lambert;

    // Specular
    vec3 specular_component = vec3(0.0f);
    if (lambert > 0.0f) {
        // Specular highlights only happen with visible faces
        vec3 R = normalize(2 * lambert * N - L); // 2 N.L * N - L
        if (max(dot(R, V), 0.0f) > 0.0f) {
            color += object.specular * pow(max(dot(R, V), 0.0f), object.specular_exponent);
        }
    }
    out_color = vec4(color, 1.0f);
}