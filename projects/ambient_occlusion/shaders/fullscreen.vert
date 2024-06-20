
#version 450

layout (location = 0) out vec2 vertex_uv;

// Draw a triangle from (-1, -1) (top left) to (3, -1) (top right) to (-1, 3) (bottom left)
// This needs to be flipped to account for the camera flipping vertex winding order
vec2 uvs[3] = {
	vec2(3, -1),
	vec2(-1, -1),
	vec2(-1, 3)
};

// Draw a full screen triangle (should be called by vkCmdDraw(command_buffer, 3, 1, 0, 0))
// Vertex shader is called with 3 vertices
void main() {
	// NDC space is [-1, 1], where the center of the screen is the origin
	// Draw a triangle so that it covers the entire viewport (keeping in mind the Vulkan coordinate system has the +y axis going down)

	//                                (1, -1)
	// (-1, -1)  o-----------------------o-----------------------o  (3, -1)
	//           |                       |                   o
	//           |         (0, 0)        |               o
	//           |           o           |           o
	//           |                       |       o
	//           |                       |   o
	//  (-1, 1)  o-----------------------o
	//           |                   o     (1, 1)
	//           |               o
	//           |           o
	//           |       o
	//           |   o
	//           o
	//        (-1, 3)

	// UV coordinates for the entire triangle are on the range of [0, 2] because the triangle is twice as big as the viewport
	// The only UV coordinates that will be used will be in the range [0, 1]
	vertex_uv = uvs[gl_VertexIndex];
	gl_Position = vec4(vertex_uv * 2.0f - 1.0f, 0.0f, 1.0f);
}
