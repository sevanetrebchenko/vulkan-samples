
#version 450 core

layout (location = 0) in vec4 view_position;
layout (location = 1) in vec4 view_normal;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec3 camera_position;
} globals;

// This is technically updated once per model, but can be shared across different instances of the same material
// Objects using the same material can save on a call to vkCmdBindDescriptorSets and reuse the same data
layout (set = 1, binding = 1) uniform BlinnPhongUniforms {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float specular_exponent;
} shading;

layout (location = 0) out vec4 out_color;

void main() {
    vec3 color = vec3(0.0f);
    vec4 N = view_normal;
    vec4 view_direction = (globals.view * vec4(globals.camera_position, 1.0f)) - view_position;
    vec4 V = normalize(view_direction);

    // Hardcode one light for the time being
    vec4 light_position = vec4(0.0f, 5.0f, 0.0f, 1.0f);
    vec3 light_color = vec3(1.0f, 1.0f, 0.0f); // red

    vec4 L = normalize((globals.view * light_position) - view_position);

    // Ambient
    color += shading.ambient;

    // Diffuse
    float lambert = max(dot(N, L), 0.0f);
    color += light_color * shading.diffuse * lambert;

    // Specular
    if (lambert > 0.0f) {
        // Specular highlights only happen with visible faces
        vec4 R = normalize(2 * lambert * N - L); // 2 N.L * N - L
        if (max(dot(R, V), 0.0f) > 0.0f) {
            color += shading.specular * pow(max(dot(R, V), 0.0f), shading.specular_exponent);
        }
    }

    out_color = vec4(color, 1.0f);
}
