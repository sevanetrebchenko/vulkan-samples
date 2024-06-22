
#version 450

layout (location = 0) in vec2 vertex_uv;

// Shader constants
layout (constant_id = 0) const int KERNEL_SIZE = 64;
layout (constant_id = 1) const float SAMPLE_RADIUS = 0.5f; // Sample radius of the effect

// Uniforms
layout (set = 0, binding = 0) uniform sampler2D positions;
layout (set = 0, binding = 1) uniform sampler2D normals;
layout (set = 0, binding = 2) uniform sampler2D depth;
layout (set = 0, binding = 3) uniform sampler2D noise;

layout (set = 0, binding = 4) uniform GlobalUniforms {
    mat4 projection; // Camera projection matrix
    vec4 samples[KERNEL_SIZE]; // Kernel
} globals;

float linearize_depth(float d) {
    float camera_near = 0.01f;
    float camera_far = 100.0f;
    return (2.0 * camera_near) / (camera_far + camera_near - d * (camera_far - camera_near));
}

// Output image has only one dimension to store the occlusion factor
layout (location = 0) out vec4 out_occlusion;

void main() {
    // https://john-chapman-graphics.blogspot.com/2013/01/ssao-tutorial.html

    vec3 position = texture(positions, vertex_uv).xyz;
    vec3 normal = texture(normals, vertex_uv).xyz;

    // Retrieve a random vector from the noise texture
    vec2 framebuffer_size = vec2(textureSize(positions, 0)); // Use base image (lowest LOD)
    vec2 noise_texture_size = vec2(textureSize(noise, 0));
    vec2 noise_uv = (framebuffer_size / noise_texture_size) * vertex_uv;
    vec3 random_direction = normalize(texture(noise, noise_uv).xyz);

    // Create change-of-basis matrix to reorient the random vector around the surface normal
    // https://math.hmc.edu/calculus/hmc-mathematics-calculus-online-tutorials/linear-algebra/gram-schmidt-method/

    // Project the random vector onto the normal
    // Subtract the projection from the random vector to get a vector that lies in the tangent plane perpendicular to the normal
    vec3 tangent = normalize(random_direction - normal * dot(random_direction, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    float occlusion_factor = 0.0f;

    vec4 ndc = globals.projection * vec4(position, 1.0f);
    ndc.xy /= ndc.w;
    float actual_depth = texture(depth, ndc.xy * 0.5f + 0.5f).r;

    for (int i = 0; i < KERNEL_SIZE; ++i) {
        // Get the view space position of the sample
        vec3 sample_position = tbn * globals.samples[i].xyz;
        sample_position = sample_position * SAMPLE_RADIUS + position;

        // Project sample into screen space to get texture coordinates at that position
        vec4 offset = vec4(sample_position, 1.0f);
        offset = globals.projection * offset; // Transform from view space to clip space
        offset.xy /= offset.w; // Perspective divide to NDC coordinates

        // Use the modified sample to test the depth of the fragment at that sample from the perspective of the user
        // This operations retrieves the depth of the first non-occluded (closest) fragment that is rendered to the geometry buffer
        float sample_depth = texture(depth, offset.xy * 0.5f + 0.5f).r;
        float range_check = smoothstep(0.0f, 1.0f, SAMPLE_RADIUS / abs(actual_depth - sample_depth));
        occlusion_factor += (sample_depth >= actual_depth ? 1.0f : 0.0f) * range_check;
    }

    float occlusion = occlusion_factor / float(KERNEL_SIZE);
    out_occlusion = vec4(vec3(occlusion), 1.0f);
}
