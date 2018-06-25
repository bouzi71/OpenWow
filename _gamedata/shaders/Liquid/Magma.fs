#version 330
#include "../lib/fragDeferredWrite.glsl"

struct VSOutput
{
	vec3 WorldSpacePos;
	vec3 TexCoord;
	vec3 Normal;
};

// Input
in VSOutput VSout;

// Uniforms
uniform sampler2D gColorMap;
uniform sampler2D gSpecularMap;

uniform float gShallowAlpha;
uniform float gDeepAlpha;
uniform vec3 gColorLight;
uniform vec3 gColorDark;


void main(void)
{
	float alpha = texture(gColorMap, VSout.TexCoord.xy).a;
	
	setMatID(1.0);
	setPos(VSout.WorldSpacePos);
	setNormal(normalize(VSout.Normal));
	setAlbedo4(texture(gColorMap, VSout.TexCoord.xy).rgba);
	setSpecParams(vec3(0.0f, 0.0f, 0.0f), 1.0);
};