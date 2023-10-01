
#include "../Globals/GlobalsVS.hlsli"


struct VSInput
{
    float3 Pos : POSITION;
    float2 Uvs : TEX;
};

struct VSToPS
{
    float4 WorldPos : POS;
    float2 TexCoord : TEX;
};



VSToPS main(VSInput vin)
{
    VSToPS vs_out;

    vs_out.WorldPos = mul(float4(vin.Pos, 1.0), model);
    
    vs_out.WorldPos /= vs_out.WorldPos.w;
    
    vs_out.TexCoord = vin.Uvs;

    return vs_out;
}