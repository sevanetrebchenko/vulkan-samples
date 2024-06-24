
#version 450

#define LIGHT_COUNT 32

layout (triangles) in;
layout (triangle_strip, max_vertices = 3 * LIGHT_COUNT) out;

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

// The geometry shader receives all vertices of a primitive as input

void main() {
	for (int layer = 0; layer < LIGHT_COUNT; ++layer) {
		// The shadow framebuffer has one color attachment with many layers (one layer for each light)
        Light light = lighting.lights[layer];
        for (int i = 0; i < gl_in.length(); ++i) { // Input length = 3
            gl_Layer = layer; // Render the shadow map to this layer
            gl_Position = light.transform * gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}
