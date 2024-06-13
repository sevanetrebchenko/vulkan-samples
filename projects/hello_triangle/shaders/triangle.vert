
#version 450

layout (location = 0) in vec2 vertex_position;
layout (location = 1) in vec3 vertex_color;

layout (location = 0) out vec4 frag_color;

void main() {
    frag_color = vec4(vertex_color, 1.0);
    gl_Position = vec4(vertex_position, 0.0, 1.0);
}
