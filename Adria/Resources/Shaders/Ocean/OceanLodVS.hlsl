
#include "../Globals/GlobalsVS.hlsli"


struct VSInput
{
    float3 Pos : POSITION;
    float2 Uvs : TEX;
};

struct VSOutput
{
    float4 WorldPos : POS;
    float2 TexCoord : TEX;
};



VSOutput main(VSInput vin)
{
    VSOutput vs_out;

    vs_out.WorldPos = mul(float4(vin.Pos, 1.0), model);
    
    vs_out.WorldPos /= vs_out.WorldPos.w;
    
    vs_out.TexCoord = vin.Uvs;

    return vs_out;
}