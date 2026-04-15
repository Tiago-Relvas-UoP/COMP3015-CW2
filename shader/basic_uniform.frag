#version 460

in vec3 LightDir[3];
in vec3 ViewDir;
in vec2 TexCoord;
in vec3 Position;

layout (binding = 0) uniform sampler2D baseTex;
layout (binding = 1) uniform sampler2D normalMap;
layout (binding = 2) uniform sampler2D Texture0;
layout (binding = 4) uniform samplerCube skyBoxTex; 

layout (location = 0) out vec4 FragColor;

uniform float EdgeThreshold;
uniform int Pass;
uniform float Weight[5];

uniform struct LightInfo
{
	vec4 Position; // Light position in eye coords.
	vec3 La; // Ambient light intensity
	vec3 L; // Diffuse and specular light intensity
} Light[3];

uniform struct MaterialInfo
{
	vec3 Ks; // Specular Reflectivity
	float Shininess; // Specular Shininess factor
} Material;

uniform struct FogInfo
{
	float MaxDist;
	float MinDist;
}Fog;

// Toon Shading Levels/ScaleFactor
const int levels = 3;
const float scaleFactor = 1.0/levels;

// Blinn-Phong Light Model
vec3 blinnPhong(int light, vec3 n)
{
	// Texture
	vec3 texColor = texture(baseTex, TexCoord).rgb;

	vec3 diffuse = vec3(0), spec = vec3(0); 

	vec3 ambient = Light[light].La * texColor;  // Ambient

	vec3 s = normalize(LightDir[light]); // Light Direction

	float sDotN = max(dot(s, n), 0.0);
	diffuse = texColor * floor(sDotN * levels) * scaleFactor ; // Diffuse with Toon Shading.
	// diffuse = texColor * sDotN ; // Diffuse without Toon Shading

	if (sDotN > 0.0)
	{
		vec3 v = normalize(ViewDir); // View Vector in Tangent-space
		vec3 h = normalize(v+s); // Blinn Half-vector
		spec = Material.Ks * pow(max(dot(h, n), 0.0), Material.Shininess); // Specular
	}

	return ambient + (diffuse + spec) * Light[light].L; // Final mix: Combines Ambient, Diffuse and Specular
}

// Pass 1: Main Scene using Normal Mapping, Fog Factor, and Blinn-Phong.
vec4 pass1()
{
	// Normal Mapping
	vec3 norm = texture(normalMap, TexCoord).xyz;
	norm.xy = 2.0 * norm.xy - 1.0;

	// Fog calculation
	float dist = abs(Position.z);
	float fogFactor=(Fog.MaxDist - dist)/(Fog.MaxDist - Fog.MinDist);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	
	// Shading for each light source
	vec3 shadeColor = vec3(0.0);
	for (int i = 0; i < 3; i++)
	{
		shadeColor += blinnPhong(i, normalize(norm));
	}

	// Final output
	return vec4(shadeColor, fogFactor);
}

// Pass 2: Vertical Gaussian Blur 
vec4 pass2()
{
	ivec2 pix = ivec2(gl_FragCoord.xy); // We grab a pixel to check if Edge

	vec4 sum = texelFetch(Texture0, pix, 0) * Weight[0];

	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, 1)) * Weight[1];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, -1)) * Weight[1];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, 2)) * Weight[2];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, -2)) * Weight[2];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, 3)) * Weight[3];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, -3)) * Weight[3];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, 4)) * Weight[4];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, -4)) * Weight[4];

	return sum;
}

// Pass 3: Horizontal Gaussian Blur
vec4 pass3()
{
	ivec2 pix = ivec2(gl_FragCoord.xy); // We grab a pixel to check if Edge

	vec4 sum = texelFetch(Texture0, pix, 0) * Weight[0];

	sum += texelFetchOffset(Texture0, pix, 0, ivec2(1, 0)) * Weight[1];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(-1, 0)) * Weight[1];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(2, 0)) * Weight[2];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(-2, 0)) * Weight[2];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(3, 0)) * Weight[3];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(-3, 0)) * Weight[3];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(4, 0)) * Weight[4];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(-4, 0)) * Weight[4];

	return sum;
}

// Main Method. Depending on current pass uniform value, the corresponding method function is called.
void main() 
{
	if (Pass == 1) FragColor = pass1();
	else if (Pass == 2) FragColor = pass2();
	else if (Pass == 3) FragColor = pass3();
}
