
#version 450

layout (location = 0) in vec2 vertex_uv;

layout (binding = 0) uniform sampler2D positions;
layout (binding = 1) uniform sampler2D normals;
layout (binding = 2) uniform sampler2D ambient;
layout (binding = 3) uniform sampler2D diffuse;
layout (binding = 4) uniform sampler2D specular;

layout (set = 0, binding = 5) uniform LightingUniforms {
    vec3 position;
} lighting;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(texture(normals, vertex_uv).xyz, 1.0f);
}
