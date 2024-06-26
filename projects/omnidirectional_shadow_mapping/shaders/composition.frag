
#version 450

layout (location = 0) in vec2 vertex_uv;

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec3 camera_position; // Unused
    int debug_view; // Unused
    float camera_far_plane;
} global;

layout (set = 0, binding = 1) uniform LightUniforms {
    mat4 transform[6];
    vec3 position;
    float outer;
    vec3 direction;
    float inner;
    vec3 color;
} light;

layout (set = 0, binding = 2) uniform sampler2D positions; // world space
layout (set = 0, binding = 3) uniform sampler2D normals; // world space
layout (set = 0, binding = 4) uniform sampler2D ambient;
layout (set = 0, binding = 5) uniform sampler2D diffuse;
layout (set = 0, binding = 6) uniform sampler2D specular;
layout (set = 0, binding = 7) uniform samplerCube shadow;

layout (location = 0) out vec4 out_color;

float linearize(float d) {
    float camera_near = 0.01f;
    float camera_far = 100.0f;
    return (2.0 * camera_near) / (camera_far + camera_near - d * (camera_far - camera_near));
}

float shadowing(vec3 position) {
    vec3 fragment_to_light = position - light.position;

    float closest = texture(shadow, fragment_to_light).r * global.camera_far_plane;
    float current = length(fragment_to_light);

    // Shadowing is determined by comparing the distance from the light to the point to the (linearized) depth at the point when rendered from the lights POV
    // If the distance is greater than the depth, there is geometry occluding the point and it should be rendered in shadow
    float bias = 0.01f;
    return current - bias > closest ? 1.0f : 0.0f;
}

void main() {
    vec3 position = texture(positions, vertex_uv).xyz;
    vec3 N = texture(normals, vertex_uv).xyz;
    vec3 V = normalize(global.camera_position - position);

    vec3 L = normalize(light.position - position);

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
    out_color = vec4(ambient_component + (1.0f - shadowing(position)) * (diffuse_component + specular_component), 1.0f);
}
