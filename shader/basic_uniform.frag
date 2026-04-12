#version 460

// Inputs
in vec3 LightDir[3];
in vec3 ViewDir;
in vec2 TexCoord;
in vec3 Position;

// Toilet Mesh Textures 
layout (binding = 0) uniform sampler2D BaseTex;
layout (binding = 1) uniform sampler2D NormalMap;

// HDR Pipleline Textures
layout (binding = 2) uniform sampler2D HdrTex;
layout (binding = 3) uniform sampler2D BlurTex1;
layout (binding = 4) uniform sampler2D BlurTex2;

// Output
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 HdrColor;

// Pass Index/Num
uniform int Pass;

// Bloom/Blur
uniform float LumThresh; // Luminance threshold
uniform float PixOffset[10] = float[](0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
uniform float Weight[10];

// RGB/XYZ conversion matrix
uniform mat3 rgb2xyz = mat3
(
	0.4124564, 0.2126729, 0.0193339,
	0.3575761, 0.7151522, 0.1191920,
	0.1804375, 0.0721750, 0.9503041
);

// XYZ/RGB conversion matrices
uniform mat3 xyz2rgb = mat3
(
	3.2404542, -0.9692660, 0.0556434,
	-1.5371385, 1.8760108, -0.2040259,
	-0.4985314, 0.0415560, 1.0572252
);

// Parameters
uniform float Exposure = 0.35;
uniform float White = 0.928;
uniform float AveLum;
float Gamma = 1/2.2f;

// Light Info
uniform struct LightInfo
{
	vec4 Position; // Light position in eye coords.
	vec3 La; // Ambient light intensity
	vec3 L; // Diffuse and specular light intensity
} Light[3];

// Material Info
uniform struct MaterialInfo
{
	vec3 Ks; // Specular Reflectivity
	float Shininess; // Specular Shininess factor
} Material;

uniform struct FogInfo
{
	float MaxDist;
	float MinDist;
	vec3 Color;
}Fog;

// Luminance method
float luminance(vec3 color)
{
	return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}

// Blinn-Phong Shading
vec3 blinnPhong(int light, vec3 n)
{
	// Texture
	vec3 texColor = texture(BaseTex, TexCoord).rgb;

	vec3 diffuse = vec3(0), spec = vec3(0); 

	vec3 ambient = Light[light].La * texColor;  // Ambient

	vec3 s = normalize(LightDir[light]); // Light Direction

	float sDotN = max(dot(s, n), 0.0);
	diffuse = texColor * sDotN ; // Diffuse

	if (sDotN > 0.0)
	{
		vec3 v = normalize(ViewDir); // View Vector in Tangent-space
		vec3 h = normalize(v+s); // Blinn Half-vector
		spec = Material.Ks * pow(max(dot(h, n), 0.0), Material.Shininess); // Specular
	}

	return ambient + (diffuse + spec) * Light[light].L; // Final mix: Combines Ambient, Diffuse and Specular
}

// Render Pass
vec4 pass1()
{
	// Normal Mapping
	vec3 norm = texture(NormalMap, TexCoord).xyz;
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

	// Final Mix
	vec3 color = mix(Fog.Color, shadeColor, fogFactor);

	// Final output
    return vec4(color, 1.0);
}

// Bright-pass filter (write to BlurTex1)
vec4 pass2()
{
	vec4 val = texture(HdrTex, TexCoord);

	if ( luminance(val.rgb) > LumThresh)
		return val;
	else
		return vec4(0.0);
}

// First blur pass (read from BlurTex1, write to BlurTex2)
vec4 pass3() 
{
	float dy = 1.0 / (textureSize(BlurTex1, 0)).y;
	vec4 sum = texture(BlurTex1, TexCoord) * Weight[0];

	for(int i = 1; i < 10; i++)
	{
		sum += texture(BlurTex1, TexCoord + vec2(0.0, PixOffset[i]) * dy) * Weight[i];
		sum += texture(BlurTex1, TexCoord - vec2(0.0, PixOffset[i]) * dy) * Weight[i];
	}
	return sum;

}

// Second blur pass (read from BlurTex2, write to BlurTex1)
vec4 pass4() 
{
	float dx = 1.0 / (textureSize(BlurTex2, 0)).x;
	vec4 sum = texture(BlurTex2, TexCoord) * Weight[0];

	for(int i = 1; i < 10; i++)
	{
		sum += texture(BlurTex2, TexCoord + vec2(PixOffset[i], 0.0) * dx) * Weight[i];
		sum += texture(BlurTex2, TexCoord - vec2(PixOffset[i], 0.0) * dx) * Weight[i];
	}

	return sum;
}

// (Read from BlurTex1 and HdrTex, write to default buffer).
vec4 pass5() 
{
	/////////////// Tone mapping ///////////////
	// Retrieve high-res color from texture
	vec4 color = texture( HdrTex, TexCoord );

	// Convert to XYZ
	vec3 xyzCol = rgb2xyz * vec3(color);

	// Convert to xyY
	float xyzSum = xyzCol.x + xyzCol.y + xyzCol.z;
	vec3 xyYCol = vec3( xyzCol.x / xyzSum, xyzCol.y / xyzSum, xyzCol.y);

	// Apply the tone mapping operation to the luminance (xyYCol.z or xyzCol.y)
	float L = (Exposure * xyYCol.z) / AveLum;
	L = (L * ( 1 + L / (White * White) )) / ( 1 + L );

	// Using the new luminance, convert back to XYZ
	xyzCol.x = (L * xyYCol.x) / (xyYCol.y);
	xyzCol.y = L;
	xyzCol.z = (L * (1 - xyYCol.x - xyYCol.y))/xyYCol.y;

	// Convert back to RGB
	vec4 toneMapColor = vec4( xyz2rgb * xyzCol, 1.0);

	///////////// Combine with blurred texture /////////////
	// We want linear filtering on this texture access so that we get additional blurring.
	vec4 blurTex = texture(BlurTex1, TexCoord);
	return toneMapColor + blurTex;
}

// Main function: Calls a pass method depending on current index
void main() 
{
	if (Pass == 1) 
		FragColor = pass1();

	else if (Pass == 2) 
		FragColor = pass2();

	else if (Pass == 3) 
		FragColor = pass3();

	else if (Pass == 4) 
		FragColor = pass4();

	else if (Pass == 5) 
		FragColor = vec4(pow(vec3(pass5()), vec3(1.0/Gamma)), 1.0); // Gamma Correction
}


// OLD MAIN 
/*void main() 
{
	// Normal Mapping
	vec3 norm = texture(NormalMap, TexCoord).xyz;
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

	// Final Mix
	vec3 color = mix(Fog.Color, shadeColor, fogFactor);

	// Final output
    FragColor = vec4(color, 1.0);
}*/
