#version 460

in vec3 Vec;

layout (binding = 4) uniform samplerCube skyBoxTex; 
layout (location = 0) out vec4 FragColor;

void main() 
{
	// Sample cubemap using eye-space direction
	vec3 texColor=texture(skyBoxTex, normalize(-Vec)).rgb;
	texColor=pow(texColor, vec3(1.0/2.2)); // Gamma Correction

	// Output 
	FragColor = vec4(texColor, 1.0);
}
