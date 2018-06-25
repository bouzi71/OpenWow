#version 330
#include "../lib/fragDeferredWrite.glsl"

// In put
in struct
{
	vec3 WorldSpacePos;
	vec3 Normal;
	vec2 TexCoordDetail;
	vec2 TexCoordAlpha;
} VSInput;

// Diffuses
uniform sampler2D gColorMap[4];

// Speculars textures
uniform sampler2D gSpecularMap[4];

// Alpha Textures + Shadow Texture
uniform sampler2D gBlend;

// Shadow Params
uniform bool gShadowMapExists;
uniform vec3 gShadowColor;

void main()
{
	vec3 resultColor = texture(gColorMap[0], VSInput.TexCoordDetail).rgb;
	vec4 resultSpecular = texture(gSpecularMap[0], VSInput.TexCoordDetail);
	
	// Layer 1
	float alpha0 = texture(gBlend, VSInput.TexCoordAlpha)[0];
	resultColor = mix(resultColor, texture(gColorMap[1], VSInput.TexCoordDetail).rgb, alpha0);
	resultSpecular = mix(resultSpecular, texture(gSpecularMap[1], VSInput.TexCoordDetail), alpha0);
	
	if (gShadowMapExists)
	{
		float alphaShadow = texture(gBlend, VSInput.TexCoordAlpha)[3];
		resultColor = mix(resultColor,  gShadowColor, alphaShadow);
	}

	//
	setMatID(1.0);
	setPos(VSInput.WorldSpacePos);
	setNormal(VSInput.Normal);
	setAlbedo(resultColor.rgb);
	setSpecParams(resultSpecular.rgb, 1.0);
};
