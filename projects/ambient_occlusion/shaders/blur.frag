
layout (location = 0) in vec2 vertex_uv;

// Shader uniforms
layout (set = 0, binding = 0) uniform sampler2D ambient_occlusion;

// Output image has only one dimension to store the blurred occlusion factor
layout (location = 0) out float out_occlusion;

void main() {
    out_occlusion = 0.0f;
}