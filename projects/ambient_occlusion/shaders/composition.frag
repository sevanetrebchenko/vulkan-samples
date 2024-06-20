
#version 450

layout (location = 0) in vec2 vertex_uv;

layout (binding = 0) uniform sampler2D positions;
layout (binding = 1) uniform sampler2D normals;
layout (binding = 2) uniform sampler2D ambient;
layout (binding = 3) uniform sampler2D diffuse;
layout (binding = 4) uniform sampler2D specular;

struct Light {
    vec3 position;
    float radius;
};

layout (set = 0, binding = 5) uniform LightingUniforms {
    mat4 view;
    vec3 camera_position;
    // TODO: lights array
} lighting;

layout (location = 0) out vec4 out_color;

void main() {
    vec3 color = vec3(0.0f);

    vec4 view_position = texture(positions, vertex_uv);
    vec4 N = texture(normals, vertex_uv);
    vec4 V = normalize((lighting.view * vec4(lighting.camera_position, 1.0f)) - view_position);

    // Hardcode one light for the time being
    vec4 light_position = vec4(0.0f, 3.f, 1.0f, 1.0f);
    vec3 light_color = vec3(1.0f, 1.0f, 1.0f);

    vec4 L = normalize((lighting.view * light_position) - view_position);

    // Ambient
    color += texture(ambient, vertex_uv).xyz;

    // Diffuse
    float lambert = max(dot(N, L), 0.0f);
    color += light_color * texture(diffuse, vertex_uv).xyz * lambert;

    // Specular
    if (lambert > 0.0f) {
        vec4 specular = texture(specular, vertex_uv);

        // Specular highlights only happen with visible faces
        vec4 R = normalize(2 * lambert * N - L); // 2 N.L * N - L
        if (max(dot(R, V), 0.0f) > 0.0f) {
            color += specular.xyz * pow(max(dot(R, V), 0.0f), specular.a);
        }
    }

    out_color = vec4(color, 1.0f);
}
