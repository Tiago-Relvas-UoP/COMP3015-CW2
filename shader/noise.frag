#version 460

#define PI 3.14159265

in vec2 TexCoord;
layout (location = 0) out vec4 FragColor;

uniform vec4 Color;
uniform sampler2D NoiseTex;

uniform vec4 SkyColor = vec4(0.3, 0.3, 0.9, 1.0);
uniform vec4 CloudColor = vec4(1.0, 1.0, 1.0, 1.0);

// Alpha (Transparency of noise)
uniform float GlobalAlpha = 1.0;

void main() 
{
	vec4 noise = texture(NoiseTex, TexCoord);
	float t = (cos( noise.a * PI ) + 1.0) / 2.0;
	vec4 color = mix (SkyColor, CloudColor, t);

	FragColor = vec4( color.rgb, GlobalAlpha);
}
