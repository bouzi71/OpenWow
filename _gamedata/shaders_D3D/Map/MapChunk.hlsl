#include "..\\_gamedata\\shaders_D3D\\CommonTypes.h"

#ifndef _IS_NORTREND
#pragma message( "_IS_NORTREND undefined. Default to 0.")
#define _IS_NORTREND 0 // should be defined by the application.
#endif

struct MapChunk_Material
{
    float4 DiffuseColor;
	uint   LayersCnt;
    //-------------------------- ( 16 bytes )
};

struct VertexShaderInput
{
	float3 position       : POSITION;
	float3 normal         : NORMAL0;
	float4 mccvColor      : COLOR0;
	float2 texCoordDetail : TEXCOORD0;
	float2 texCoordAlpha  : TEXCOORD1;
};

struct VertexShaderOutput
{
	float4 positionVS     : SV_POSITION;
	float4 positionWS     : POSITION;
	float3 normal         : NORMAL0;
	float4 mccvColor      : COLOR0;
	float2 texCoordDetail : TEXCOORD0;
	float2 texCoordAlpha  : TEXCOORD1;
};


// Uniforms
cbuffer PerObject : register(b0)
{
	float4x4 ModelViewProjection;
}
cbuffer Material : register(b2)
{
    MapChunk_Material Material;
};

// Textures and samples
Texture2D ColorMap0       : register(t0);
Texture2D ColorMap1       : register(t1);
Texture2D ColorMap2       : register(t2);
Texture2D ColorMap3       : register(t3);
Texture2D AlphaMap        : register(t4);
sampler   ColorMapSampler : register(s0);
sampler   AlphaMapSampler : register(s1);

VertexShaderOutput VS_main(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.positionVS = mul(ModelViewProjection, float4(IN.position, 1.0f));
	OUT.positionWS = float4(IN.position, 1.0f);
	OUT.normal = IN.normal;
	OUT.mccvColor = IN.mccvColor;
	OUT.texCoordDetail = IN.texCoordDetail;
	OUT.texCoordAlpha = IN.texCoordAlpha;
	
	return OUT;
}

PixelShaderOutput PS_main(VertexShaderOutput IN) : SV_TARGET
{
	float3 layersColor = float3(0,0,0);
	float3 resultColor = float3(0,0,0);
	
#if _IS_NORTREND == 1

	/* NORTREND */
	float alphaSumma = 0.0;
	if (Material.LayersCnt >= 2)
	{
		float alphaCurrent = AlphaMap.Sample(AlphaMapSampler, IN.texCoordAlpha).r;
		alphaSumma += alphaCurrent;
		layersColor += ColorMap1.Sample(ColorMapSampler, IN.texCoordDetail).rgb * alphaCurrent;
	}
	if (Material.LayersCnt >= 3)
	{
		float alphaCurrent = AlphaMap.Sample(AlphaMapSampler, IN.texCoordAlpha).g;
		alphaSumma += alphaCurrent;
		layersColor += ColorMap2.Sample(ColorMapSampler, IN.texCoordDetail).rgb * alphaCurrent;
	}
	if (Material.LayersCnt >= 4)
	{
		float alphaCurrent = AlphaMap.Sample(AlphaMapSampler, IN.texCoordAlpha).b;
		alphaSumma += alphaCurrent;
		layersColor += ColorMap3.Sample(ColorMapSampler, IN.texCoordDetail).rgb * alphaCurrent;
	}
	resultColor = ColorMap0.Sample(ColorMapSampler, IN.texCoordDetail).rgb * (1.0 - alphaSumma) + layersColor;
	
#else

	/* NOT NORTREND */
	layersColor = ColorMap0.Sample(ColorMapSampler, IN.texCoordDetail).rgb;
	if (Material.LayersCnt >= 2)
	{
		float alphaCurrent = AlphaMap.Sample(AlphaMapSampler, IN.texCoordAlpha).r;
		layersColor = lerp(layersColor, ColorMap1.Sample(ColorMapSampler, IN.texCoordDetail).rgb, alphaCurrent);
	}
	if (Material.LayersCnt >= 3)
	{
		float alphaCurrent = AlphaMap.Sample(AlphaMapSampler, IN.texCoordAlpha).g;
		layersColor = lerp(layersColor, ColorMap2.Sample(ColorMapSampler, IN.texCoordDetail).rgb, alphaCurrent);
	}
	if (Material.LayersCnt >= 4)
	{
		float alphaCurrent = AlphaMap.Sample(AlphaMapSampler, IN.texCoordAlpha).b;
		layersColor = lerp(layersColor, ColorMap3.Sample(ColorMapSampler, IN.texCoordDetail).rgb, alphaCurrent);
	}
	resultColor = layersColor;
	
#endif
	
	//resultColor *= (IN.mccvColor * 2.0f);

	PixelShaderOutput OUT;
	OUT.PositionWS = IN.positionWS;
	OUT.Diffuse = float4(resultColor, 1.0f);
	OUT.Specular = float4(1.0f, 1.0f, 1.0f, 1.0f);
	OUT.NormalWS = float4(IN.normal, 1.0f);
	return OUT;
}