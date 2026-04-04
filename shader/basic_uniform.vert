#version 460

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec4 VertexTangent;

uniform struct LightInfo
{
	vec4 Position; // Light position in eye coords.
	vec3 La; // Ambient light intensity
	vec3 L; // Diffuse and specular light intensity
} Light[3];

out vec3 LightDir[3];
out vec3 ViewDir;
out vec2 TexCoord;
out vec3 Position;
	
uniform mat4 ModelViewMatrix;
uniform mat4 MVP;
uniform mat3 NormalMatrix;
uniform mat4 ProjectionMatrix;

void main()
{
	// Transform normal and tangent to eye space
	vec3 norm = normalize(NormalMatrix * VertexNormal);
	vec3 tang = normalize(NormalMatrix * vec3(VertexTangent));

	// Compute the binormal
	vec3 binormal = normalize(cross(norm, tang)) * VertexTangent.w;

	// Matrix for transformation to tangent space
	mat3 toObjectLocal = mat3(
		tang.x, binormal.x, norm.x,
		tang.y, binormal.y, norm.y,
		tang.z, binormal.z, norm.z
	);

	// Vertex position in Eye Space rather than tangent
	vec3 pos = vec3(ModelViewMatrix * vec4(VertexPosition, 1.0)).xyz;
	Position = pos; // For Fog

	// Conversion of each light position from eye to tangent space
	for (int i = 0; i < 3; i++)
	{
		LightDir[i] = toObjectLocal * (Light[i].Position.xyz - pos);
	}

	// Convered to tangent using Matrix
	ViewDir = toObjectLocal * normalize(-pos);
	TexCoord=VertexTexCoord;

	// Final output
	gl_Position = MVP * vec4(VertexPosition, 1.0);
}