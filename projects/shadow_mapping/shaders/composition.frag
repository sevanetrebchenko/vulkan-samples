
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

layout (set = 0, binding = 2) uniform sampler2D positions; // world space
layout (set = 0, binding = 3) uniform sampler2D normals; // world space
layout (set = 0, binding = 4) uniform sampler2D ambient;
layout (set = 0, binding = 5) uniform sampler2D diffuse;
layout (set = 0, binding = 6) uniform sampler2D specular;
layout (set = 0, binding = 7) uniform sampler2DArray shadow;

layout (location = 0) out vec4 out_color;

float linearize(float d) {
    float camera_near = 0.01f;
    float camera_far = 100.0f;
    return (2.0 * camera_near) / (camera_far + camera_near - d * (camera_far - camera_near));
}

float shadowing(vec3 position, vec3 normal, int i) {
    Light light = lighting.lights[i];

    vec4 shadow_position = light.transform * vec4(position, 1.0f);
    shadow_position /= shadow_position.w; // Perspective divide
    shadow_position.xy = shadow_position.xy * 0.5f + 0.5f; // [0.0, 1.0]

    if (shadow_position.z < 0.0f || shadow_position.z > 1.0f) {
        // Point is behind the light view frustum
        // Return this point as fully illuminated
        return 0.0f;
    }

    // Depth value from the perspective of the light
    float shadow_map_depth = texture(shadow, vec3(shadow_position.xy, i)).r;

    // When transformed, the fragment has a greater depth than when rendered from the light, meaning it is obstructed and cannot be seen by the light
    // It is in shadow
    float bias = 0.0001f; // max(0.005f * (1.0f - dot(normal, light.direction)), 0.0005f);
    return shadow_position.z - bias >= shadow_map_depth ? 1.0f : 0.0f;
}

void main() {
    vec3 color = vec3(0.0f);

    vec3 position = texture(positions, vertex_uv).xyz;
    vec3 N = texture(normals, vertex_uv).xyz;
    vec3 V = normalize(global.camera_position - position);

    for (int i = 0; i < LIGHT_COUNT; ++i) {
        Light light = lighting.lights[i];

    //    vec3 L = normalize(light_position - world_position);
        vec3 L = -normalize(light.direction); // Directional light

        // Ambient
        vec3 ambient_component = texture(ambient, vertex_uv).xyz;

        // Diffuse
        float lambert = max(dot(N, L), 0.0f);
        vec3 diffuse_component = light.color * texture(diffuse, vertex_uv).rgb * lambert;

        // Specular
        vec3 specular_component = vec3(0.0f);
        if (lambert > 0.0f) {
            vec4 s = texture(specular, vertex_uv); // specular color.rgb, specular exponent

            // Specular highlights only happen with visible faces
            vec3 R = normalize(2 * lambert * N - L); // 2 N.L * N - L
            if (max(dot(R, V), 0.0f) > 0.0f) {
                specular_component = s.rgb * pow(max(dot(R, V), 0.0f), s.a);
            }
        }

        // Shadowing affects diffuse + specular components, but leaves the ambient component untouched
        color += ambient_component + (1.0f - shadowing(position, N, i)) * (diffuse_component + specular_component);
    }

    out_color = vec4(color, 1.0f);
}
