
#version 450

layout (location = 0) in vec2 vertex_uv;

layout (binding = 0) uniform sampler2D positions;
layout (binding = 1) uniform sampler2D normals;
layout (binding = 2) uniform sampler2D ambient;
layout (binding = 3) uniform sampler2D diffuse;
layout (binding = 4) uniform sampler2D specular;
layout (binding = 5) uniform sampler2D depth;

struct Light {
    vec3 position;
    float radius;
};

layout (set = 0, binding = 6) uniform LightingUniforms {
    mat4 view;
    vec3 camera_position;
    // TODO: lights array
} lighting;

// For viewing individual attachments
layout (set = 0, binding = 7) uniform RenderSettings {
    int view;
} settings;

layout (location = 0) out vec4 out_color;

void main() {
    if (settings.view == 0) {
        // Output positions
        out_color = vec4(texture(positions, vertex_uv).xyz, 1.0f);
    }
    else if (settings.view == 1) {
        // Output normals
        out_color = vec4(texture(normals, vertex_uv).xyz, 1.0f);
    }
    else if (settings.view == 2) {
        // Output ambient
        out_color = vec4(texture(ambient, vertex_uv).xyz, 1.0f);
    }
    else if (settings.view == 3) {
        // Output diffuse
        out_color = vec4(texture(diffuse, vertex_uv).xyz, 1.0f);
    }
    else if (settings.view == 4) {
        // Output specular (rgb) + specular exponent (a)
        out_color = texture(specular, vertex_uv);
    }
    else if (settings.view == 5) {
        // Output depth
        // Depth information is stored in the r channel (depth buffer is a 1-channel image)
        float z = texture(depth, vertex_uv).r;

        // Linearlize depth
        float camera_near = 0.01f;
        float camera_far = 10.0f;
        float depth = (2.0 * camera_near) / (camera_far + camera_near - z * (camera_far - camera_near));

        out_color = vec4(vec3(depth), 1.0f);
    }
    else {
        // Output lighting
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
}
