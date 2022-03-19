#include "../Globals/GlobalsPS.hlsli"


struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEX;
    float3 Normal : NORMAL;
};

float4 main() : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}