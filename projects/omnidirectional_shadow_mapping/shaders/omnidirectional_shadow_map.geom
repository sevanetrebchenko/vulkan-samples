
#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out; // 3 vertices * 6 faces

layout (location = 0) out vec3 world_position;

layout (set = 0, binding = 1) uniform LightUniforms {
    mat4 transform[6];
    vec3 position;
    float outer;
    vec3 direction;
    float inner;
    vec3 color;
} light;

void main() {
    for (int face = 0; face < 6; ++face) {
        // Bound framebuffer has 6 attachments, one per cubemap face
        // One pass of this shader renders 6 cube map attachments for one light (light index is passed by push constant)
        mat4 transform = light.transform[face];

        for (int i = 0; i < gl_in.length(); ++i) { // Input length = 3
            gl_Layer = face; // Render the shadow map to this layer
            gl_Position = transform * gl_in[i].gl_Position;

            world_position = gl_in[i].gl_Position.xyz;

            EmitVertex();
        }

        EndPrimitive();
    }
}
