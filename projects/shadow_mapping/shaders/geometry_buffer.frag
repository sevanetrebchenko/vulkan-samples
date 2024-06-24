
#version 450 core

layout (location = 0) in vec3 view_position;
layout (location = 1) in vec3 view_normal;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec3 camera_position; // Unused
    int debug_view; // Unused
} global;

layout (set = 1, binding = 1) uniform PhongUniforms {
    vec3 ambient;
    float specular_exponent;
    vec3 diffuse;
    int flat_shaded;
    vec3 specular;
} shading;

layout (location = 0) out vec3 out_position;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec3 out_ambient;
layout (location = 3) out vec3 out_diffuse;
layout (location = 4) out vec4 out_specular; // (r, g, b, exponent)

vec3 calculate_face_normal(vec3 position) {
    vec3 dx = dFdx(position);
    vec3 dy = dFdy(position);
    return normalize(cross(-dx, dy));
}

void main() {
    out_position = view_position.xyz;

    if (shading.flat_shaded == 1) {
        out_normal = calculate_face_normal(view_position);
    }
    else {
        out_normal = normalize(view_normal);
    }
    out_ambient = shading.ambient;
    out_diffuse = shading.diffuse;
    out_specular = vec4(shading.specular, shading.specular_exponent);
}