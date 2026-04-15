#version 460

layout (location = 0) in vec3 VertexPosition;
	
out vec3 Vec;
uniform mat4 MVP;
uniform mat4 ModelViewMatrix;

void main()
{
	// Compute eye-space vector for cubemap sampling
	Vec = (ModelViewMatrix * vec4(VertexPosition, 0.0)).xyz;

	// Output
	gl_Position = MVP * vec4(VertexPosition, 1.0);
}