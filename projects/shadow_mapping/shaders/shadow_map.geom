
#version 450

layout (constant_id = 0) const int LIGHT_COUNT = 32;

// Geometry shader inputs are in world space
layout (location = 0) in vec3 vertex_positions[];

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

struct Light {
    mat4 transform; // Stores projection * view
    vec3 position;

    float outer;
    vec3 direction;
    float inner;
    int type; // 0 - point, 1 - directional, 2 - spot
};

layout (set = 0, binding = 1) uniform LightingData {
    Light lights[LIGHT_COUNT];
} lighting;

void main() {
	for (int layer = 0; layer < LIGHT_COUNT; ++layer) {
		// The shadow framebuffer has one color attachment with many layers (one layer for each light)
        Light light = lighting.lights[layer];
        for (int i = 0; i < 3; ++i) {
            gl_Position = light.transform * vec4(vertex_positions[i], 1.0f);
            gl_Layer = layer; // Render the shadow map to this layer

            EmitVertex();
        }
        EndPrimitive();
    }
}
