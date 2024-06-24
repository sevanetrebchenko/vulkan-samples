
#version 450

layout (constant_id = 0) const int LIGHT_COUNT = 32;

layout (location = 0) in vec2 vertex_uv;

struct Light {
    mat4 transform; // Stores projection * view
    vec3 position;

    float outer;
    vec3 direction;
    float inner;
    vec3 color;
    int type; // 0 - point, 1 - directional, 2 - spot
};

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec3 camera_position;
    int debug_view;
} global;

layout (set = 0, binding = 1) uniform LightingUniforms {
    Light lights[LIGHT_COUNT];
} lighting;

layout (set = 0, binding = 2) uniform sampler2D positions;
layout (set = 0, binding = 3) uniform sampler2D normals;
layout (set = 0, binding = 4) uniform sampler2D ambient;
layout (set = 0, binding = 5) uniform sampler2D diffuse;
layout (set = 0, binding = 6) uniform sampler2D specular;
layout (set = 0, binding = 7) uniform sampler2D depth;
layout (set = 0, binding = 8) uniform sampler2DArray shadow;

layout (location = 0) out vec4 out_color;

void main() {
    vec3 color = vec3(0.0f);

    float d = texture(depth, vertex_uv).r;
    float s = texture(shadow, vec3(vertex_uv, 0)).r;

    vec4 view_position = texture(positions, vertex_uv);
    vec4 N = texture(normals, vertex_uv);
    vec4 V = normalize((global.view * vec4(global.camera_position, 1.0f)) - view_position);

    // Hardcode one light for the time being
    vec4 light_position = vec4(0.0f, 3.f, 1.0f, 1.0f);
    vec3 light_color = vec3(1.0f, 1.0f, 1.0f);

    vec4 L = normalize((global.view * light_position) - view_position);

    // Ambient
    color += texture(ambient, vertex_uv).xyz;

    // Diffuse
    float lambert = max(dot(N, L), 0.0f);
    color += light_color * texture(diffuse, vertex_uv).rgb * lambert;

    // Specular
    if (lambert > 0.0f) {
        vec4 specular = texture(specular, vertex_uv);

        // Specular highlights only happen with visible faces
        vec4 R = normalize(2 * lambert * N - L); // 2 N.L * N - L
        if (max(dot(R, V), 0.0f) > 0.0f) {
            color += specular.rgb * pow(max(dot(R, V), 0.0f), specular.a);
        }
    }

    out_color = vec4(color, 1.0f);
}
