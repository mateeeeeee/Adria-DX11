#include "../Globals/GlobalsVS.hlsli"

struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 Uvs : TEX;
    float3 Normal : NORMAL;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEX;
    float3 Normal : NORMAL;
};



VS_OUTPUT main(VS_INPUT vin)
{
    VS_OUTPUT vout = (VS_OUTPUT) 0;
    return vout;
}