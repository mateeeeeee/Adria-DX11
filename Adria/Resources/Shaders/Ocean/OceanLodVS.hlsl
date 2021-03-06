
#include "../Globals/GlobalsVS.hlsli"


struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 Uvs : TEX;
};

struct VS_OUTPUT
{
    float4 WorldPos : POS;
    float2 TexCoord : TEX;
};



VS_OUTPUT main(VS_INPUT vin)
{
    VS_OUTPUT vs_out;

    vs_out.WorldPos = mul(float4(vin.Pos, 1.0), model);
    
    vs_out.WorldPos /= vs_out.WorldPos.w;
    
    vs_out.TexCoord = vin.Uvs;

    return vs_out;
}