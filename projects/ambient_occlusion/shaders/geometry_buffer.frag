
#version 450 core

layout (location = 0) in vec3 view_position;
layout (location = 1) in vec3 view_normal;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec3 camera_position;
} globals;

layout (set = 1, binding = 1) uniform PhongUniforms {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float specular_exponent;
} shading;

layout (location = 0) out vec3 out_position;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec3 out_ambient;
layout (location = 3) out vec3 out_diffuse;
layout (location = 4) out vec4 out_specular; // (r, g, b, exponent)

float normal_blend = 1.0f;
vec3 calculate_face_normal(vec3 position) {
    vec3 dx = dFdx(position);
    vec3 dy = dFdy(position);
    return normalize(cross(dx, dy));
}

void main() {
    out_position = view_position.xyz;
    out_normal = mix(calculate_face_normal(view_position), normalize(view_normal), normal_blend); // Normal color
    out_ambient = shading.ambient;
    out_diffuse = shading.diffuse;
    out_specular = vec4(shading.specular, shading.specular_exponent);
}
